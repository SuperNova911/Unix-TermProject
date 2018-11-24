// #include "database.h"
#include "dataPack.h"
#include "interface.h"

#include <errno.h>
#include <fcntl.h>
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

enum ClientStatus
{
    Default, Login, LectureBrowser, Lobby, Chat
} CurrentClientStatus;

struct LoginInfo_t
{
    char studentID[16];
    char password[32];
} LoginInfo;

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
void updateCommandAndInput();

void userRequest(enum Command command);
void lectureRequest(enum Command command);
void attendanceRequest(enum Command command);
void chatRequest(enum Command command);

void drawLayoutByStatus();

// 인터페이스
extern WINDOW *StatusWindow;
extern WINDOW *NoticeWindow, *NoticeWindowBorder;
extern WINDOW *EventWindow, *EventWindowBorder;
extern WINDOW *MessageWindow, *MessageWindowBorder;
extern WINDOW *CommandWindow, *CommandWindowBorder;
extern WINDOW *InputWindow, *InputWindowBorder;

UserInfo CurrentUserInfo;

int ServerSocket;
int Fdmax;
fd_set Master, Reader;

char UserInputGuide[256];
char UserInputBuffer[256];

enum Command CurrentRequest;
char Arguments[4][128];

int main()
{
    initializeGlobalVariable();

    atexit(onClose);
    initiateInterface();

    CurrentClientStatus = Default;
    drawLayoutByStatus();

    printMessage(MessageWindow, "Try connect to server...\n");
    while (true)
    {
        if (connectServer())
            break;

        printMessage(MessageWindow, "Cannot connect to server\n");
        printMessage(InputWindow, "Press any key to reconnect to server");
        getchar();
        printMessage(MessageWindow, "Try connect to server...\n");
        sleep(3);
    }

    sleep(3);
    CurrentClientStatus = Login;
    drawLayoutByStatus();


    while (true)
    {
        if (/*loginUser(LoginInfo.studentID, LoginInfo.password) || */true)
            break;

        printMessage(MessageWindow, "Login failed, please check your StudentID or Password\n");
        getchar();
        sleep(3);
    }

    sleep(3);
    CurrentClientStatus = LectureBrowser;
    drawLayoutByStatus();

    // lecture brawser

    sleep(3);
    CurrentClientStatus = Lobby;
    drawLayoutByStatus();

    async();

    return 0;
}

// 전역 변수 초기화
void initializeGlobalVariable()
{
    CurrentClientStatus = Default;
    CurrentRequest = NONE;

    memset(&CurrentUserInfo, 0, sizeof(UserInfo));
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
            if (errno != EINTR)
            {
                printMessage(MessageWindow, "select: %s\n", strerror(errno));
                sleep(3);
                return;
            }
            continue;
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

    updateInput(UserInputGuide, UserInputBuffer);
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
            else if (strlen(Arguments[1]) == 0)
            {
                setInputGuide("비밀번호를 입력하세요");

            }


    }
}

// CommandWindow의 명령어 목록을 ClientStatus에 따라 업데이트
void updateCommandList()
{
    switch (CurrentClientStatus)
    {
        case Login:
            break;

        case LectureBrowser:
            updateCommand("1: 강의 목록 보기\n2: 강의 입장\n\n\n\n\n\n\n9: 로그아웃\n0: 프로그램 종료");
        
        case Lobby:
            updateCommand("1. 공지사항 보기\n2. 출석 체크\n3. 미니 퀴즈\n4. 강의 대화 입장\n5. 강의실 나가기\n\n\n\n9: 로그아웃\n0: 프로그램 종료");
            break;
            
        case Chat:
            updateCommand("!quit: 대화 나가기\n!user: 현재 사용자");
            break;

        default:
            break;
    }
}

void updateCommandAndInput()
{
    if (CurrentRequest == None)
    {
        setInputGuide("명령어를 입력하세요");
        updateCommandList();
    }
    else
    {
        receiveArgument();
    }
}

void userRequest(enum Command command)
{
    DataPack dataPack;
    memset(&dataPack, 0, sizeof(DataPack));

    dataPack.command = command;
    memcpy(&dataPack.userInfo, &CurrentUserInfo, sizeof(UserInfo));

    switch (command)
    {
        case USER_LOGIN_REQUEST:
            buildDataPack(&dataPack, LoginInfo.studentID, LoginInfo.password, NULL, NULL, NULL);
            memset(&LoginInfo, 0, sizeof(struct LoginInfo_t));
            break;
        
        case USER_LOGOUT_REQUEST:
            memcpy(&dataPack.userInfo, &CurrentUserInfo, sizeof(UserInfo));
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
    memcpy(&dataPack.userInfo, &CurrentUserInfo, sizeof(UserInfo));

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
    memcpy(&dataPack.userInfo, &CurrentUserInfo, sizeof(UserInfo));

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
    memcpy(&dataPack.userInfo, &CurrentUserInfo, sizeof(UserInfo));

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

void drawLayoutByStatus()
{
    switch (CurrentClientStatus)
    {
        case Login:
            drawLoginLayout();
            break;
        case LectureBrowser:
            drawDefaultLayout();
            break;
        case Lobby:
            drawLectureLayout();
            break;
        case Chat:
            drawLectureLayout();
            break;
        default:
            drawDefaultLayout();
            break;
    }
}