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

enum Command
{
    NONE,

    USER_LOGIN_REQUEST, USER_LOGOUT_REQUEST,
    LECTURE_LIST_REQUEST, LECTURE_CREATE_REQUEST, LECTURE_REMOVE_REQUEST,
    LECTURE_ENTER_REQUEST, LECTURE_LEAVE_REQUEST, LECTURE_REGISTER_REQUEST, LECTURE_DEREGISTER_REQUEST,
    ATTENDANCE_START_REQUEST, ATTNEDANCE_STOP_REQUEST, ATTENDANCE_RESULT_REQUEST, ATTENDANCE_CHECK_REQUEST,
    CHAT_ENTER_REQUEST, CHAT_LEAVE_REQUEST, CHAT_USER_LIST_REQUEST, CHAT_SEND_MESSAGE_REQUEST,
    
    USER_LOGIN_RESPONSE, USER_LOGOUT_RESPONSE,
    LECTURE_LIST_RESPONSE, LECTURE_CREATE_RESPONSE, LECTURE_REMOVE_RESPONSE,
    LECTURE_ENTER_RESPONSE, LECTURE_LEAVE_RESPONSE, LECTURE_REGISTER_RESPONSE, LECTURE_DEREGISTER_RESPONSE,
    ATTENDANCE_START_RESPONSE, ATTNEDANCE_STOP_RESPONSE, ATTENDANCE_RESULT_RESPONSE, ATTENDANCE_CHECK_RESPONSE,
    CHAT_ENTER_RESPONSE, CHAT_LEAVE_RESPONSE, CHAT_USER_LIST_RESPONSE, CHAT_SEND_MESSAGE_RESPONSE,
};

enum UserRole
{
    None, Admin, Student, Professor
};

enum ClientStatus
{
    Login, LectureBrowser, Lobby, Chat
} CurrentClientStatus;

struct UserInfo_t
{
    enum UserRole role;
    char userName[16];
    char studentID[16];
} UserInfo;

struct LoginInfo_t
{
    char studentID[16];
    char password[32];
} LoginInfo;

typedef struct DataPack_t
{
    enum Command command;
    bool result;
    char data1[128];
    char data2[128];
    char data3[128];
    char data4[128];
    char message[508 - sizeof(struct UserInfo_t)];
    struct UserInfo_t userInfo;
} DataPack;

void initializeGlobalVariable();

// 서버
bool connectServer();
void async();
void receiveData();
bool sendDataPack(DataPack *dataPack);

bool decomposeDataPack(DataPack *dataPack);
void sendSampleDataPack();

void getUserInput();
void setInputGuide(char *inputGuide);
void receiveUserCommand();
void receiveArgument();
void updateCommandList();
void updateUserInput();
void updateCommandAndInput();

void buildDataPack(DataPack *targetDataPack, char *data1, char *data2, char *data3, char *data4, char *message);
void userRequest(enum Command command);
void lectureRequest(enum Command command);
void attendanceRequest(enum Command command);
void chatRequest(enum Command command);

// 인터페이스
void initiateInterface();
void drawDefaultLayout();
void drawBorder(WINDOW *window, char *windowName);
void printMessage(WINDOW *window, const char *format, ...);
void onClose();

int ServerSocket;
int Fdmax;
fd_set Master, Reader;

char UserInputGuide[256];
char UserInputBuffer[256];

enum Command CurrentRequest;
char Arguments[4][128];

WINDOW *MessageWindow, *MessageWindowBorder;
WINDOW *CommandWindow, *CommandWindowBorder;
WINDOW *UserInputWindow, *UserInputWindowBorder;

int main()
{
    initializeGlobalVariable();

    atexit(onClose);
    initiateInterface();
    drawDefaultLayout();

    if (connectServer())
    {
        CurrentClientStatus = Lobby;
        strncpy(UserInputGuide, "Your Command", sizeof(UserInputGuide));
        updateCommandList();
        updateUserInput();
    }
    else
    {

    }

    strncpy(LoginInfo.studentID, "201510743", sizeof(LoginInfo.studentID));
    strncpy(LoginInfo.password, "suwhan77", sizeof(LoginInfo.password));
    userRequest(USER_LOGIN_REQUEST);
    async();

    return 0;
}

// 전역 변수 초기화
void initializeGlobalVariable()
{
    CurrentClientStatus = None;
    CurrentRequest = NONE;

    memset(&UserInfo, 0, sizeof(struct UserInfo_t));
    memset(&LoginInfo, 0, sizeof(struct LoginInfo_t));
    memset(UserInputGuide, 0, sizeof(UserInputGuide));
    memset(UserInputBuffer, 0, sizeof(UserInputBuffer));
    for (int index = 0; index < 4; index++)
        memset(Arguments[index], 0, sizeof(char) * 128);

}

// TCP/IP 소켓 서버와 연결
// [반환] true: 성공, false: 실패
bool connectServer()
{
    printMessage(MessageWindow, "Try connect to server...\n");

    // TCP/IP 소켓
    ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocket == -1)
    {
        printMessage(MessageWindow, "socket: %s\n", strerror(errno));
        return false;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(struct sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

    // 서버에 연결 요청
    if (connect(ServerSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)) == -1)
    {
        printMessage(MessageWindow, "connect: %s\n", strerror(errno));
        return false;
    }

    printMessage(MessageWindow, "Connected to server\n");
    return true;
}

// 비 동기 입력 대기
void async()
{
    FD_ZERO(&Master);
    FD_ZERO(&Reader);
    FD_SET(0, &Master);
    FD_SET(ServerSocket, &Master);
    Fdmax = ServerSocket;

    while (true)
    {
        updateCommandAndInput();

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
                receiveData();
            }
            else if (fd == 0)   // stdin
            {
                getUserInput();
            }
            else
            {
                // Unknown source
            }
        }
    }
}

// 서버로부터 받은 데이터를 분석
void receiveData()
{
    int readBytes;
    char buffer[sizeof(DataPack)] = { 0, };

    DataPack receivedDataPack;
    DataPack *receivedDataPackPtr = &receivedDataPack;
    memset(&receivedDataPack, 0, sizeof(DataPack));

    readBytes = recv(ServerSocket, buffer, sizeof(buffer), 0);
    switch (readBytes)
    {
        // 예외 처리
        case -1:
            close(ServerSocket);
            FD_CLR(ServerSocket, &Master);
            printMessage(MessageWindow, "recv: %s\n", strerror(errno));
            break;
        
        // 서버와 연결 종료
        case 0:
            close(ServerSocket);
            FD_CLR(ServerSocket, &Master);
            printMessage(MessageWindow, "Disconnected from server\n");
            break;

        // 받은 데이터 유효성 검사
        default:
            if (readBytes == sizeof(DataPack))
            {
                receivedDataPackPtr = (DataPack *)buffer;
                decomposeDataPack(&receivedDataPack);
            }
            else
            {
                printMessage(MessageWindow, "Invalid data received, readBytes: '%d'\n", readBytes);
            }
            break;
    }
}

// 서버에게 전송 받은 DataPack의 Command에 따라 지정된 작업을 수행
// [매개변수] dataPack: 전송 받은 DataPack
// [반환] true: , false: 
bool decomposeDataPack(DataPack *dataPack)
{
    printMessage(MessageWindow, "command: '%d', message: '%s'\n", dataPack->command, dataPack->message);

    switch(dataPack->command)
    {
        case LECTURE_LIST_REQUEST:
            break;

        case LECTURE_CREATE_REQUEST:
            break;
        
        default:
            break;
    }

    return false;
}

// 서버에게 DataPack을 전송
// [매개변수] dataPack: 전송 할 DataPack
// [반환] true: 전송 성공, false: 전송 실패
bool sendDataPack(DataPack *dataPack)
{
    // printMessage(MessageWindow, "command: '%d', message: '%s'\n", dataPack->command, dataPack->message);

    int sendBytes;

    sendBytes = send(ServerSocket, (char *)dataPack, sizeof(DataPack), 0);
    switch (sendBytes)
    {
        case -1:
            return false;
        
        case 0:
            return false;
        
        default:
            return true;
    }
}

// DataPack 전송 테스트
void sendSampleDataPack()
{
    DataPack sampleDataPack;
    memset(&sampleDataPack, 0, sizeof(DataPack));

    sampleDataPack.command = LECTURE_REGISTER_REQUEST;
    strncpy(sampleDataPack.message, "Definitely not Dopa", sizeof(sampleDataPack.message));

    sendDataPack(&sampleDataPack);

    printMessage(MessageWindow, "Sample DataPack has been sent to the server\n");
}

// 사용자의 키 입력을 읽어옴
void getUserInput()
{
    int userInput;

    userInput = getchar();
    switch (userInput)
    {
        // 엔터
        case 13:
            sendSampleDataPack();
            memset(UserInputBuffer, 0, sizeof(UserInputBuffer));
            break;

        // 백스페이스
        case 127:
            if (strlen(UserInputBuffer) > 0)
            {
                UserInputBuffer[strlen(UserInputBuffer) - 1] = 0;
            }
            break;

        default:
            if (32 <= userInput && userInput <= 126)
            {
                if (strlen(UserInputBuffer) < sizeof(UserInputBuffer) + 1)
                {
                    UserInputBuffer[strlen(UserInputBuffer)] = (char)userInput;
                }
            }
    }

    updateUserInput();
}

void setInputGuide(char *inputGuide)
{
    strncpy(UserInputGuide, inputGuide, sizeof(UserInputGuide));
}

// 사용자가 입력한 Command를 처리
void receiveUserCommand()
{
    if (strlen(UserInputBuffer) < 1)
        return;

    switch (CurrentClientStatus)
    {
        case Login:
            break;

//printMessage(CommandWindow, "1: 강의 목록 보기\n2: 강의 입장\n\n9: 로그아웃\n0: 프로그램 종료");
        case LectureBrowser:
            switch (atoi(UserInputBuffer))
            {
                case 1:
                    break;
                case 2:
                    break;
                case 9:
                    break;
                case 0:
                    break;
                default:
                    break;
            }
            break;
        
        case Lobby:
            switch (atoi(UserInputBuffer))
            {
                case 1:
                    break;
                case 2:
                    break;
                case 3:
                    break;
                default:
                    break;
            }
            break;

        case Chat:
            break;

        default:
            break;
    }
}

void receiveArgument()
{
    switch (CurrentRequest)
    {
        case USER_LOGIN_REQUEST:
            if (strlen(Arguments[0]) == 0)
            {
                setInputGuide("학번을 입력하세요");

            }
            else if (strlen(Arguments[1] == 0))
            {
                setInputGuide("비밀번호를 입력하세요");

            }


    }
}

// CommandWindow의 명령어 목록을 ClientStatus에 따라 업데이트
void updateCommandList()
{
    wclear(CommandWindow);

    switch (CurrentClientStatus)
    {
        case Login:
            break;

        case LectureBrowser:
            printMessage(CommandWindow, "1: 강의 목록 보기\n2: 강의 입장\n\n9: 로그아웃\n0: 프로그램 종료");
        
        case Lobby:
            printMessage(CommandWindow, "1. 공지사항 확인\n2. 출석체크\n3. 강의 멤버 보기\n4. 강의실 나가기\n\n9: 로그아웃\n0: 프로그램 종료");
            break;
            
        case Chat:
            printMessage(CommandWindow, "!quit: 채팅방 나가기\n!user: 채팅방 사용자 보기");
            break;

        default:
            break;
    }
}

// UserInputWindow의 사용자 입력을 업데이트
void updateUserInput()
{
    wclear(UserInputWindow);
    printMessage(UserInputWindow, "%s: %s", UserInputGuide, UserInputBuffer);
}

void updateCommandAndInput()
{
    if (CurrentRequest == None)
    {
        setInputGuide("명령어를 입력하세요");
        updateCommandList();
        updateUserInput();
    }
    else
    {
        receiveArgument();

    }
}

void buildDataPack(DataPack *targetDataPack, char *data1, char *data2, char *data3, char *data4, char *message)
{
    if (data1 != NULL)
        strncpy(targetDataPack->data1, data1, sizeof(targetDataPack->data1));
    if (data2 != NULL)
        strncpy(targetDataPack->data2, data2, sizeof(targetDataPack->data2));
    if (data3 != NULL)
        strncpy(targetDataPack->data3, data3, sizeof(targetDataPack->data3));
    if (data4 != NULL)
        strncpy(targetDataPack->data4, data4, sizeof(targetDataPack->data4));
    if (message != NULL)
        strncpy(targetDataPack->message, message, sizeof(targetDataPack->message));
}

void userRequest(enum Command command)
{
    DataPack dataPack;
    memset(&dataPack, 0, sizeof(DataPack));

    dataPack.command = command;
    memcpy(&dataPack.userInfo, &UserInfo, sizeof(struct UserInfo_t));

    switch (command)
    {
        case USER_LOGIN_REQUEST:
            buildDataPack(&dataPack, LoginInfo.studentID, LoginInfo.password, NULL, NULL, NULL);
            memset(&LoginInfo, 0, sizeof(struct LoginInfo_t));
            break;
        
        case USER_LOGOUT_REQUEST:
            memcpy(&dataPack.userInfo, &UserInfo, sizeof(struct UserInfo_t));
            break;
        
        default:
            return;
    }

    sendDataPack(&dataPack);
}

void lectureRequest(enum Command command)
{
    DataPack dataPack;
    memset(&dataPack, 0, sizeof(DataPack));

    dataPack.command = command;
    memcpy(&dataPack.userInfo, &UserInfo, sizeof(struct UserInfo_t));

    switch (command)
    {
        case LECTURE_LIST_REQUEST:
            break;
        
        case LECTURE_CREATE_REQUEST:
            break;
        
        case LECTURE_REMOVE_REQUEST:
            break;
        
        case LECTURE_ENTER_REQUEST:
            break;
        
        case LECTURE_LEAVE_REQUEST:
            break;
        
        case LECTURE_REGISTER_REQUEST:
            break;
        
        case LECTURE_DEREGISTER_REQUEST:
            break;

        default:
            return;
    }

    sendDataPack(&dataPack);
}

void attendanceRequest(enum Command command)
{
    DataPack dataPack;
    memset(&dataPack, 0, sizeof(DataPack));

    dataPack.command = command;
    memcpy(&dataPack.userInfo, &UserInfo, sizeof(struct UserInfo_t));

    switch (command)
    {
        case ATTENDANCE_START_REQUEST:
            break;
        
        case ATTNEDANCE_STOP_REQUEST:
            break;
        
        case ATTENDANCE_RESULT_REQUEST:
            break;
        
        case ATTENDANCE_CHECK_REQUEST:
            break;

        default:
            return;
    }

    sendDataPack(&dataPack);
}

void chatRequest(enum Command command)
{
    DataPack dataPack;
    memset(&dataPack, 0, sizeof(DataPack));

    dataPack.command = command;
    memcpy(&dataPack.userInfo, &UserInfo, sizeof(struct UserInfo_t));

    switch (command)
    {
        case CHAT_ENTER_REQUEST:
            break;
        
        case CHAT_LEAVE_REQUEST:
            break;
        
        case CHAT_USER_LIST_REQUEST:
            break;
        
        case CHAT_SEND_MESSAGE_REQUEST:
            break;

        default:
            return;
    }

    sendDataPack(&dataPack);
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

    int commandWindowBorderWidth = 22;
    int userInputWindowBorderHeight = 4;

    MessageWindow = newwin(parentY - userInputWindowBorderHeight - 2, parentX - commandWindowBorderWidth - 2, 1, 1);
    MessageWindowBorder = newwin(parentY - userInputWindowBorderHeight, parentX - commandWindowBorderWidth, 0, 0);

    CommandWindow = newwin(parentY - userInputWindowBorderHeight - 2, commandWindowBorderWidth - 2, 1, parentX - commandWindowBorderWidth + 1);
    CommandWindowBorder = newwin(parentY - userInputWindowBorderHeight, commandWindowBorderWidth, 0, parentX - commandWindowBorderWidth);

    UserInputWindow = newwin(userInputWindowBorderHeight - 2, parentX - 2, parentY - userInputWindowBorderHeight + 1, 1);
    UserInputWindowBorder = newwin(userInputWindowBorderHeight, parentX, parentY - userInputWindowBorderHeight, 0);

    scrollok(MessageWindow, TRUE);
    scrollok(CommandWindow, TRUE);

    drawBorder(MessageWindowBorder, "MESSAGE");
    drawBorder(CommandWindowBorder, "COMMAND");
    drawBorder(UserInputWindowBorder, "USER INPUT");

    wrefresh(MessageWindow);
    wrefresh(CommandWindow);
    wrefresh(UserInputWindow);
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
    delwin(UserInputWindow);
    delwin(UserInputWindowBorder);
    endwin();
}