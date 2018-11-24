#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SERVER_PORT 10743
#define SERVER_ADDRESS "192.168.98.128"
#define MAX_CLIENT 64

enum Command
{
    NONE,

    USER_LOGIN_REQUEST, USER_LOGOUT_REQUEST,
    LECTURE_LIST_REQUEST, LECTURE_CREATE_REQUEST, LECTURE_REMOVE_REQUEST,
    LECTURE_ENTER_REQUEST, LECTURE_LEAVE_REQUEST, LECTURE_REGISTER_REQUEST, LECTURE_DEREGISTER_REQUEST,
    ATTENDANCE_START_REQUEST, ATTNEDANCE_STOP_REQUEST, ATTENDANCE_RESULT_REQUEST, ATTENDANCE_CHECK_REQUEST,
    CHAT_ENTER_REQUEST, CHAT_LEAVE_REQUEST, CHAT_USER_LIST_REQUEST, CHAT_SEND_MESSAGE_REQUEST,
    
    LOGIN_RESPONSE, LOGOUT_RESPONSE,
    LECTURE_LIST_RESPONSE, LECTURE_CREATE_RESPONSE, LECTURE_REMOVE_RESPONSE,
    LECTURE_ENTER_RESPONSE, LECTURE_LEAVE_RESPONSE, LECTURE_REGISTER_RESPONSE, LECTURE_DEREGISTER_RESPONSE,
    ATTENDANCE_START_RESPONSE, ATTNEDANCE_STOP_RESPONSE, ATTENDANCE_RESULT_RESPONSE, ATTENDANCE_CHECK_RESPONSE,
    CHAT_ENTER_RESPONSE, CHAT_LEAVE_RESPONSE, CHAT_USER_LIST_RESPONSE, CHAT_SEND_MESSAGE_RESPONSE,
};

enum UserRole
{
    None, Admin, Student, Professor
};

typedef struct UserInfo_t
{
    enum UserRole role;
    char userName[16];
    char studentID[16];
} UserInfo;

typedef struct DataPack_t
{
    enum Command command;
    bool result;
    char data1[128];
    char data2[128];
    char data3[128];
    char data4[128];
    char message[508 - sizeof(UserInfo)];
    UserInfo userInfo;
} DataPack;

// 서버
bool initiateServer();
void async();
void acceptClient();
void receiveData(int socket);
bool sendDataPack(int receiver, DataPack *dataPack);

// 통신
bool decomposeDataPack(int sender, DataPack *dataPack);

// 인터페이스
void initiateInterface();
void drawDefaultLayout();
void drawBorder(WINDOW *window, char *windowName);
void printMessage(WINDOW *window, const char *format, ...);
void onClose();

int ServerSocket;
time_t ServerUpTime;

int Fdmax;
fd_set Master, Reader;

WINDOW *MessageWindow, *MessageWindowBorder;
WINDOW *CommandWindow, *CommandWindowBorder;
WINDOW *InputWindow, *InputWindowBorder;

int main()
{
    atexit(onClose);
    initiateInterface();
    drawDefaultLayout();

    initiateServer();
    async();
    
    return 0;
}

// TCP/IP 소켓 서버 시작
// [반환] true: 성공, false: 실패
bool initiateServer()
{
    printMessage(MessageWindow, "Initiate server\n");

    // TCP/IP 소켓
    ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocket == -1)
    {
        printMessage(MessageWindow, "socket: %s\n", strerror(errno));
        return false;
    }

    int socketOption = 1;
    setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, &socketOption, sizeof(socketOption));

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(struct sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

    if (bind(ServerSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)) == -1)
    {
        printMessage(MessageWindow, "bind: %s\n", strerror(errno));
        return false;
    }

    if (listen(ServerSocket, MAX_CLIENT) == -1)
    {
        printMessage(MessageWindow, "listen: %s\n", strerror(errno));
        return false;
    }

    // 서버 시작 시간 저장
    time(&ServerUpTime);

    return true;
}

// 비 동기 입력 대기
void async()
{
    FD_ZERO(&Master);
    FD_ZERO(&Reader);
    FD_SET(ServerSocket, &Master);
    Fdmax = ServerSocket;

    while (true)
    {
        Reader = Master;
        if (select(Fdmax + 1, &Reader, NULL, NULL, NULL) == -1)
        {
            printMessage(MessageWindow, "select: %s\n", strerror(errno));
            return;
        }

        for (int fd = 0; fd <= Fdmax; fd++)
        {
            if (FD_ISSET(fd, &Reader) == 0)
                continue;

            if (fd == ServerSocket)
            {
                acceptClient();
            }
            else if (fd == 0)   // stdin
            {

            }
            else
            {
                receiveData(fd);
            }
        }
    }
}

// 클라이언트 연결 요청 수락
void acceptClient()
{
    int newSocket;
    struct sockaddr_in clientAddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    newSocket = accept(ServerSocket, (struct sockaddr *)&clientAddr, &addrlen);
    if (newSocket == -1)
    {
        printMessage(MessageWindow, "accept: %s\n", strerror(errno));
    }
    else
    {
        FD_SET(newSocket, &Master);
        if (newSocket > Fdmax)
        {
            Fdmax = newSocket;
        }
        printMessage(MessageWindow, "Accept new client, socket: '%d'\n", newSocket);
    }
}

// 클라이언트로부터 받은 데이터를 분석
// [매개변수] socket: 데이터를 받아올 클라이언트의 소켓
void receiveData(int socket)
{
    int readBytes;
    char buffer[sizeof(DataPack)] = { 0, };

    DataPack receivedDataPack;
    DataPack *receivedDataPackPtr = &receivedDataPack;
    memset(receivedDataPackPtr, 0, sizeof(DataPack));

    readBytes = recv(socket, buffer, sizeof(buffer), 0);
    switch (readBytes)
    {
        // 예외 처리
        case -1:
            close(socket);
            FD_CLR(socket, &Master);
            printMessage(MessageWindow, "recv: %s, socket: %d\n", strerror(errno), socket);
            break;

        // 클라이언트와 연결 종료
        case 0:
            close(socket);
            FD_CLR(socket, &Master);
            printMessage(MessageWindow, "Client disconnected, socket: '%d'\n", socket);
            break;

        // 받은 데이터 유효성 검사
        default:
            if (readBytes == sizeof(DataPack))
            {
                receivedDataPackPtr = (DataPack *)buffer;
                decomposeDataPack(socket, receivedDataPackPtr);
            }
            else
            {
                printMessage(MessageWindow, "Invalid data received, socket: '%d', readBytes: '%d'\n", socket, readBytes);
            }
            break;
    }
}

// 클라이언트에게 전송 받은 DataPack의 Command에 따라 지정된 작업을 수행
// [매개변수] sender: DataPack을 전송한 클라이언트 소켓, dataPack: 전송 받은 DataPack
// [반환] true: , false: 
bool decomposeDataPack(int sender, DataPack *dataPack)
{
    printMessage(MessageWindow, "sender: '%d', command: '%d', result: '%d'\ndata1: '%s', data2: '%s'\ndata3: '%s', data4: '%s', message: '%s'\nrole: '%d', userName: '%s', studentID: '%s'", sender, dataPack->command, dataPack->result, dataPack->data1, dataPack->data2, dataPack->data3, dataPack->data4, dataPack->message, dataPack->userInfo.role, dataPack->userInfo.userName, dataPack->userInfo.studentID);

    switch(dataPack->command)
    {
        case LECTURE_ENTER_REQUEST:
            break;

        default:
            break;
    }
}

// 클라이언트에게 DataPack을 전송
// [매개변수] receiver: 전송 대상 클라이언트 소켓, dataPack: 전송 할 DataPack
// [반환] true: 전송 성공, false: 전송 실패
bool sendDataPack(int receiver, DataPack *dataPack)
{
    return false;
}

// ncurses라이브러리를 이용한 사용자 인터페이스 초기화
void initiateInterface()
{
    setlocale(LC_CTYPE, "ko_KR.utf-8");
    initscr();
    noecho();
    curs_set(FALSE);
}

// 기본 레이아웃으로 윈도우 그리기
void drawDefaultLayout()
{
    int parentX, parentY;
    getmaxyx(stdscr, parentY, parentX);

    int commandWindowBorderWidth = 20;
    int userInputWindowBorderHeight = 4;

    MessageWindow = newwin(parentY - userInputWindowBorderHeight - 2, parentX - commandWindowBorderWidth - 2, 1, 1);
    MessageWindowBorder = newwin(parentY - userInputWindowBorderHeight, parentX - commandWindowBorderWidth, 0, 0);

    CommandWindow = newwin(parentY - userInputWindowBorderHeight - 2, commandWindowBorderWidth - 2, 1, parentX - commandWindowBorderWidth + 1);
    CommandWindowBorder = newwin(parentY - userInputWindowBorderHeight, commandWindowBorderWidth, 0, parentX - commandWindowBorderWidth);

    InputWindow = newwin(userInputWindowBorderHeight - 2, parentX - 2, parentY - userInputWindowBorderHeight + 1, 1);
    InputWindowBorder = newwin(userInputWindowBorderHeight, parentX, parentY - userInputWindowBorderHeight, 0);

    scrollok(MessageWindow, TRUE);
    scrollok(CommandWindow, TRUE);

    drawBorder(MessageWindowBorder, "MESSAGE");
    drawBorder(CommandWindowBorder, "COMMAND");
    drawBorder(InputWindowBorder, "USER INPUT");
    
    wrefresh(MessageWindow);
    wrefresh(CommandWindow);
    wrefresh(InputWindow);
}

// 윈도우 테두리 그리기
// [매개변수] window: 테두리를 그릴 윈도우, windowName: 상단에 보여줄 윈도우 이름
void drawBorder(WINDOW *window, char *windowName)
{
    int x, y;
    getmaxyx(window, y, x);

    // 테두리 그리기
    mvwprintw(window, 0, 0, "+"); 
    mvwprintw(window, y - 1, 0, "+"); 
    mvwprintw(window, 0, x - 1, "+"); 
    mvwprintw(window, y - 1, x - 1, "+"); 
    for (int i = 1; i < (y - 1); i++)
    {
        mvwprintw(window, i, 0, "|"); 
        mvwprintw(window, i, x - 1, "|"); 
    }
    for (int i = 1; i < (x - 1); i++)
    {
        mvwprintw(window, 0, i, "-"); 
        mvwprintw(window, y - 1, i, "-"); 
    }

    // 윈도우 이름 출력
    mvwprintw(window, 0, 4, windowName); 

    wrefresh(window);
}

// 매개변수로 전달한 윈도우에 문자열 출력
// [매개변수] window: 문자열을 출력할 윈도우, format: 문자열 출력 포맷
void printMessage(WINDOW *window, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vwprintw(window, format, args);
    va_end(args);
    wrefresh(window);
}

// 프로그램 종료시 수행
void onClose()
{
    // ncurses윈도우 종료
    delwin(MessageWindow);
    delwin(MessageWindowBorder);
    delwin(CommandWindow);
    delwin(CommandWindowBorder);
    delwin(InputWindow);
    delwin(InputWindowBorder);
    endwin();
}