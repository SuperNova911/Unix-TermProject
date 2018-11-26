#include "dataPack.h"
#include "interface.h"

#include <errno.h>
#include <fcntl.h>
#include <locale.h>
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
#define MAX_LECTURE 16

void initializeGlobalVariable();

// 서버
bool initiateServer();
void async();
void acceptClient();
void receiveData(int socket);
bool sendDataPack(int receiver, DataPack *dataPack);

// 통신
bool decomposeDataPack(int sender, DataPack *dataPack);

void userLogin(int receiver, char *studentID, char *password);
void userLogout(int receiver, UserInfo *userInfo);

void lectureList(int receiver);
void lectureCreate(int receiver, char *lectureName);
void lectureRemove(int receiver, char *lectureName);
void lectureEnter(int receiver, char *lectureName);
void lectureLeave(int receiver, char *lectureName);
void lectureRegister(int receiver, char *lectureName);
void lectureDeregister(int receiver, char *lectureName);

User userList[MAX_CLIENT];
Lecture lectureList[MAX_LECTURE];

int ServerSocket;
time_t ServerUpTime;

int Fdmax;
fd_set Master, Reader;

extern WINDOW *MessageWindow, *MessageWindowBorder;
extern WINDOW *CommandWindow, *CommandWindowBorder;
extern WINDOW *InputWindow, *InputWindowBorder;

int main()
{
    initializeGlobalVariable();
    
    atexit(onClose);
    initiateInterface();

    drawDefaultLayout();

    initiateServer();

    async();
    
    return 0;
}

// 전역 변수 초기화
void initializeGlobalVariable()
{
    for (int index = 0; index < MAX_CLIENT; index++)
        memset(&userList[index], 0, sizeof(User));
    for (int index = 0; index < MAX_LECTURE; index++)
        memset(&lectureList[index], 0, sizeof(Lecture));
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
    printMessage(MessageWindow, "sender: '%d', command: '%d', data1: '%s', data2: '%s', message: '%s'\n",
        sender, dataPack->command, dataPack->data1, dataPack->data2, dataPack->message);

    switch(dataPack->command)
    {
        case USER_LOGIN_REQUEST:
            break;
        case USER_LOGOUT_REQUEST:
            break;
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
        case ATTENDANCE_START_REQUEST:
            break;
        case ATTNEDANCE_STOP_REQUEST:
            break;
        case ATTNEDANCE_EXTEND_REQUEST:
            break;
        case ATTENDANCE_RESULT_REQUEST:
            break;
        case ATTENDANCE_CHECK_REQUEST:
            break;
        case CHAT_ENTER_REQUEST:
            break;
        case CHAT_LEAVE_REQUEST:
            break;
        case CHAT_USER_LIST_REQUEST:
            break;
        case CHAT_SEND_MESSAGE_REQUEST:
            break;

        default:
            break;
    }
}

void userLogin(int receiver, char *studentID, char *password)
{
    DataPack dataPack;
    resetDataPack(&dataPack);

    // if (loginUser(studentID, password))
    // {
    //     User user = loadUserByID(studentID);

    //     dataPack.result = true;
    //     dataPack.userInfo.role = user.role;
    //     strncpy(dataPack.userInfo.userName, user.userName, sizeof(dataPack.userInfo.userName));
    //     strncpy(dataPack.userInfo.studentID, user.studentID, sizeof(dataPack.userInfo.studentID));
    //     return true;
    // }
    // else
    // {
    //     dataPack.result = false;
    //     strncpy(dataPack.message, "학번 또는 비밀번호가 틀립니다.");
    //     return false;
    // }

    sendDataPack(receiver, &dataPack);
}

void userLogout(int receiver, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);

    sendDataPack(receiver, &dataPack);
}

void lectureList(int receiver)
{
    DataPack dataPack;
    resetDataPack(&dataPack);

    sendDataPack(receiver, &dataPack);
}

void lectureCreate(int receiver, char *lectureName)
{
    DataPack dataPack;
    resetDataPack(&dataPack);

    sendDataPack(receiver, &dataPack);
}

void lectureRemove(int receiver, char *lectureName)
{
    DataPack dataPack;
    resetDataPack(&dataPack);

    sendDataPack(receiver, &dataPack);
}

void lectureEnter(int receiver, char *lectureName)
{
    DataPack dataPack;
    resetDataPack(&dataPack);

    sendDataPack(receiver, &dataPack);
}

void lectureLeave(int receiver, char *lectureName)
{
    DataPack dataPack;
    resetDataPack(&dataPack);

    sendDataPack(receiver, &dataPack);
}

void lectureRegister(int receiver, char *lectureName)
{
    DataPack dataPack;
    resetDataPack(&dataPack);

    sendDataPack(receiver, &dataPack);
}

void lectureDeregister(int receiver, char *lectureName)
{
    DataPack dataPack;
    resetDataPack(&dataPack);

    sendDataPack(receiver, &dataPack);
}

// 클라이언트에게 DataPack을 전송
// [매개변수] receiver: 전송 대상 클라이언트 소켓, dataPack: 전송 할 DataPack
// [반환] true: 전송 성공, false: 전송 실패
bool sendDataPack(int receiver, DataPack *dataPack)
{
    return false;
}