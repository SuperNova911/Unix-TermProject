#include "database.h"
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
#define MAX_CLIENT 64
#define MAX_LECTURE 16

typedef struct LectureInfo_t
{
    int onlineUserCount;
    Lecture lecture;
    UserInfo onlineUser[LECTURE_MAX_MEMBER];
} LectureInfo;

typedef struct OnlineUser_t
{
    int socket;
    int currentLectureID;
    User user;
} OnlineUser;

void initializeGlobalVariable();

// 서버
bool initiateServer();
void async();
void acceptClient();
void receiveData(int socket);
bool sendDataPack(int receiver, DataPack *dataPack);

// 통신
bool decomposeDataPack(int sender, DataPack *dataPack);

void userLogin(int sender, char *studentID, char *password);
void userLogout(int sender, UserInfo *userInfo);
void lectureList(int sender);
void lectureCreate(int sender, char *lectureName, UserInfo *userInfo);
void lectureRemove(int sender, char *lectureName, UserInfo *userInfo);
void lectureEnter(int sender, char *lectureName, UserInfo *userInfo);
void lectureLeave(int sender, char *lectureName, UserInfo *userInfo);
void lectureRegister(int sender, char *lectureName, UserInfo *userInfo);
void lectureDeregister(int sender, char *lectureName, UserInfo *userInfo);
void attendanceStart(int sender, int duration, char *answer, char *quiz, UserInfo *userInfo);
void attendanceStop(int sender, UserInfo *userInfo);
void attendanceExtend(int sender, int duration, UserInfo *userInfo);
void attendanceResult(int sender, UserInfo *userInfo);
void attendanceCheck(int sender, char *HWID, char *answer, UserInfo *userInfo);
void chatEnter(int sender, UserInfo *userInfo);
void chatLeave(int sender, UserInfo *userInfo);
void chatUserList(int sender);
void chatSendMessage(int sender, char *message, UserInfo *userInfo);

bool addUserToList(User *user);
bool removeUserFromList(char *studentID);
bool getUserFromList(User *user, char *studentID);


int userCount;
int lectureCount;
OnlineUser OnlineUserList[MAX_CLIENT];
LectureInfo LectureInfoList[MAX_LECTURE];

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
    userCount = 0;
    lectureCount = 0;

    for (int index = 0; index < MAX_CLIENT; index++)
        memset(&OnlineUserList[index], 0, sizeof(OnlineUser));
    for (int index = 0; index < MAX_LECTURE; index++)
        memset(&LectureInfoList[index], 0, sizeof(LectureInfo));
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
            userLogin(sender, dataPack->data1, dataPack->data2);
            break;
        case USER_LOGOUT_REQUEST:
            userLogout(sender, &dataPack->userInfo);
            break;

        case LECTURE_LIST_REQUEST:
            lectureList(sender);
            break;
        case LECTURE_CREATE_REQUEST:
            lectureCreate(sender, dataPack->data1, &dataPack->userInfo);
            break;
        case LECTURE_REMOVE_REQUEST:
            lectureRemove(sender, dataPack->data1, &dataPack->userInfo);
            break;
        case LECTURE_ENTER_REQUEST:
            lectureEnter(sender, dataPack->data1, &dataPack->userInfo);
            break;
        case LECTURE_LEAVE_REQUEST:
            lectureLeave(sender, dataPack->data1, &dataPack->userInfo);
            break;
        case LECTURE_REGISTER_REQUEST:
            lectureRegister(sender, dataPack->data1, &dataPack->userInfo);
            break;
        case LECTURE_DEREGISTER_REQUEST:
            lectureDeregister(sender, dataPack->data1, &dataPack->userInfo);
            break;

        case ATTENDANCE_START_REQUEST:
            attendanceStart(sender, atoi(dataPack->data1), dataPack->data2, dataPack->message, &dataPack->userInfo);
            break;
        case ATTNEDANCE_STOP_REQUEST:
            attendanceStop(sender, &dataPack->userInfo);
            break;
        case ATTNEDANCE_EXTEND_REQUEST:
            attendanceExtend(sender, atoi(dataPack->data1), &dataPack->userInfo);
            break;
        case ATTENDANCE_RESULT_REQUEST:
            attendanceResult(sender, &dataPack->userInfo);
            break;
        case ATTENDANCE_CHECK_REQUEST:
            attendanceCheck(sender, dataPack->data1, dataPack->message, &dataPack->userInfo);
            break;

        case CHAT_ENTER_REQUEST:
            chatEnter(sender, &dataPack->userInfo);
            break;
        case CHAT_LEAVE_REQUEST:
            chatEnter(sender, &dataPack->userInfo);
            break;
        case CHAT_USER_LIST_REQUEST:
            chatEnter(sender);
            break;
        case CHAT_SEND_MESSAGE_REQUEST:
            chatEnter(sender, dataPack->message, &dataPack->userInfo);
            break;

        default:
            printMessage(MessageWindow, "Invaild command: command: '%d'\n", dataPack->command);
            break;
    }
}

void userLogin(int sender, char *studentID, char *password)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = USER_LOGIN_RESPONSE;

    if (loginUser(studentID, password))
    {
        User user;
        loadUserByID(&user, studentID);
        addUserToList(&user);

        dataPack.result = true;
        dataPack.userInfo.role = user.role;
        strncpy(dataPack.userInfo.userName, user.userName, sizeof(dataPack.userInfo.userName));
        strncpy(dataPack.userInfo.studentID, user.studentID, sizeof(dataPack.userInfo.studentID));
    }
    else
    {
        dataPack.result = false;
        strncpy(dataPack.message, "학번 또는 비밀번호가 틀립니다.", sizeof(dataPack.message));
    }

    sendDataPack(sender, &dataPack);
}

void userLogout(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = USER_LOGOUT_RESPONSE;

    if (removeUserFromList(userInfo->studentID))
    {
        dataPack.result = true;
    }
    else
    {
        dataPack.result = false;
        strncpy(dataPack.message, "이미 로그아웃 하였습니다.", sizeof(dataPack.message));
    }

    sendDataPack(sender, &dataPack);
}

void lectureList(int sender)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_LIST_RESPONSE;

    for (int index = 0; index < lectureCount; index++)
    {

    }

    sendDataPack(sender, &dataPack);
}

void lectureCreate(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_CREATE_RESPONSE;

    if (userInfo->role != Professor && userInfo->role != Admin)
    {
        dataPack.result = false;
        strncpy(dataPack.message, "강의를 개설 할 권한이 없습니다.", sizeof(dataPack.message));
        sendDataPack(sender, &dataPack);
        return;
    }

    if ((lectureCount + 1) == MAX_LECTURE)
    {
        dataPack.result = false;
        strncpy(dataPack.message, "더 이상 강의를 개설할 수 없습니다.", sizeof(dataPack.message));
        sendDataPack(sender, &dataPack);
        return;
    }
    
    Lecture newLecture;
    memset(&newLecture, 0, sizeof(Lecture));
    strncpy(newLecture.professorID, userInfo->studentID, sizeof(newLecture.professorID));
    strncpy(newLecture.lectureName, lectureName, sizeof(newLecture.lectureName));
    time(&newLecture.createDate);

    createLecture(&newLecture);

    LectureInfoList[lectureCount].lecture = newLecture;
    lectureCount++;

    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureRemove(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_REMOVE_RESPONSE;

    if (userInfo->role != Professor && userInfo->role != Admin)
    {
        dataPack.result = false;
        strncpy(dataPack.message, "강의를 삭제 할 권한이 없습니다.", sizeof(dataPack.message));
        sendDataPack(sender, &dataPack);
        return;
    }

    for (int index = 0; index < lectureCount; index++)
    {
        if (strcmp(LectureInfoList[index].lecture.lectureName, lectureName) == 0)
        {
            LectureInfoList[index] = LectureInfoList[lectureCount];
            memset(&LectureInfoList[lectureCount], 0, sizeof(LectureInfo));
            lectureCount--;

            removeLecture(LectureInfoList[index].lecture.lectureID);

            dataPack.result = true;
            sendDataPack(sender, &dataPack);
            return;
        }
    }

    dataPack.result = false;
    strncpy(dataPack.message, "해당 이름이 일치하는 강의가 없습니다.", sizeof(dataPack.message));
    sendDataPack(sender, &dataPack);
}

void lectureEnter(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_ENTER_RESPONSE;

    for (int index = 0; index < lectureCount; index++)
    {
        if (strcmp(LectureInfoList[index].lecture.lectureName, lectureName) == 0)
        {
            if ((LectureInfoList[index].onlineUserCount + 1) < LECTURE_MAX_MEMBER)
            {
                memcpy(&LectureInfoList[index].onlineUser[LectureInfoList[index].onlineUserCount], userInfo, sizeof(UserInfo));
                LectureInfoList[index].onlineUserCount++;

                for (int userIndex = 0; userIndex < userCount; userIndex++)
                {
                    if (strcmp(OnlineUserList[userIndex].user.studentID, userInfo->studentID) == 0)
                    {
                        OnlineUserList[userIndex].currentLectureID = LectureInfoList[index].lecture.lectureID;
                        break;
                    }
                }

                dataPack.result = true;
                sendDataPack(sender, &dataPack);
                return;
            }
            else
            {
                dataPack.result = false;
                strncpy(dataPack.message, "강의실이 가득 차 들어갈 수 없습니다.", sizeof(dataPack.message));
                sendDataPack(sender, &dataPack);
                return;
            }
        }
    }

    dataPack.result = false;
    strncpy(dataPack.message, "해당 이름이 일치하는 강의가 없습니다.", sizeof(dataPack.message));
    sendDataPack(sender, &dataPack);
}

void lectureLeave(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_LEAVE_RESPONSE;

    for (int index = 0; index < lectureCount; index++)
    {
        if (strcmp(LectureInfoList[index].lecture.lectureName, lectureName) == 0)
        {
            for (int userIndex; userIndex < LectureInfoList[index].onlineUserCount; userIndex++)
            {
                if (strcmp(LectureInfoList[index].onlineUser[userIndex].studentID, userInfo->studentID) == 0)
                {
                    LectureInfoList[index].onlineUser[userIndex] = LectureInfoList[index].onlineUser[LectureInfoList[index].onlineUserCount];
                    memset(&LectureInfoList[index].onlineUser[LectureInfoList[index].onlineUserCount], 0, sizeof(UserInfo));
                    LectureInfoList[index].onlineUserCount--;

                    for (int onlineIndex = 0; onlineIndex < userCount; onlineIndex++)
                    {
                        if (strcmp(OnlineUserList[onlineIndex].user.studentID, userInfo->studentID) == 0)
                        {
                            OnlineUserList[onlineIndex].currentLectureID = 0;
                            break;
                        }
                    }

                    dataPack.result = true;
                    sendDataPack(sender, &dataPack);
                    return;
                }
            }

            dataPack.result = false;
            strncpy(dataPack.message, "이미 강의에서 나갔습니다.", sizeof(dataPack.message));
            sendDataPack(sender, &dataPack);
            return;
        }
    }

    dataPack.result = false;
    strncpy(dataPack.message, "해당 강의가 존재하지 않습니다.", sizeof(dataPack.message));
    sendDataPack(sender, &dataPack);
}

void lectureRegister(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_REGISTER_RESPONSE;

    for (int index = 0; index < lectureCount; index++)
    {
        if (strcmp(LectureInfoList[index].lecture.lectureName, lectureName) == 0)
        {
            if (lecture_registerUser(LectureInfoList[index].lecture.lectureID, userInfo->studentID))
            {
                dataPack.result = true;
                sendDataPack(sender, &dataPack);
                return;
            }
            else
            {
                dataPack.result = false;
                strncpy(dataPack.message, "DB에 쓰는 도중 문제가 발생하였습니다.", sizeof(dataPack.message));
                sendDataPack(sender, &dataPack);
                return;
            }
        }
    }

    dataPack.result = false;
    strncpy(dataPack.message, "해당 강의가 존재하지 않습니다.", sizeof(dataPack.message));
    sendDataPack(sender, &dataPack);
}

void lectureDeregister(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_DEREGISTER_RESPONSE;

    for (int index = 0; index < lectureCount; index++)
    {
        if (strcmp(LectureInfoList[index].lecture.lectureName, lectureName) == 0)
        {
            if (lecture_deregisterUser(LectureInfoList[index].lecture.lectureID, userInfo->studentID))
            {
                dataPack.result = true;
                sendDataPack(sender, &dataPack);
                return;
            }
            else
            {
                dataPack.result = false;
                strncpy(dataPack.message, "DB에 쓰는 도중 문제가 발생하였습니다.", sizeof(dataPack.message));
                sendDataPack(sender, &dataPack);
                return;
            }
        }
    }

    dataPack.result = false;
    strncpy(dataPack.message, "해당 강의가 존재하지 않습니다.", sizeof(dataPack.message));
    sendDataPack(sender, &dataPack);
}

void attendanceStart(int sender, int duration, char *answer, char *quiz, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_REQUEST;

    for (int lectureIndex = 0; lectureIndex < lectureCount; lectureIndex++)
    {
        for (int userIndex = 0; userIndex < LectureInfoList[lectureIndex].onlineUserCount; userIndex++)
        {

        }
    }

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void attendanceStop(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_REQUEST;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void attendanceExtend(int sender, int duration, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_REQUEST;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void attendanceResult(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_REQUEST;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void attendanceCheck(int sender, char *HWID, char *answer, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_REQUEST;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}

void chatEnter(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_REQUEST;

    for (int userIndex = 0; userIndex < userCount)
    {
    }

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void chatLeave(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_REQUEST;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void chatUserList(int sender)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_REQUEST;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void chatSendMessage(int sender, char *message, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_REQUEST;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}

// 클라이언트에게 DataPack을 전송
// [매개변수] receiver: 전송 대상 클라이언트 소켓, dataPack: 전송 할 DataPack
// [반환] true: 전송 성공, false: 전송 실패
bool sendDataPack(int receiver, DataPack *dataPack)
{
    int sendBytes;

    sendBytes = send(receiver, (char *)dataPack, sizeof(DataPack), 0);
    switch (sendBytes)
    {
        case -1:
            printMessage(MessageWindow, "데이터를 전송하는 중 문제가 발생하였습니다. send: '%s'\n", strerror(errno));
            break;

        case 0:
            printMessage(MessageWindow, "클라이언트와 연결 할 수 없습니다. socket: '%d'\n", receiver);
            break;

        default:
            if (sendBytes == sizeof(DataPack))
                return true;
            
            printMessage(MessageWindow, "데이터를 완전히 전송하는데에 실패하였습니다. socket: '%d', sendBytes: '%d'\n", receiver, sendBytes);
            break;
    }

    return false;
}

// OnlineUserList에 새로운 User를 추가
// [매개변수] user: 리스트에 추가 할 User구조체
// [반환] true: 리스트에 추가 성공, false: 리스트가 가득 참
bool addUserToList(User *user)
{
    if (userCount + 1 < MAX_CLIENT)
    {
        memcpy(&OnlineUserList[userCount].user, user, sizeof(OnlineUser));
        userCount++;
        return true;
    }

    return false;
}

// OnlineUserList에서 studentID가 일치 하는 User 삭제
// [매개변수] studentID: 삭제 할 User의 studentID
// [반환] true: 리스트에서 삭제 됨, false: 리스트가 비어 있거나 studentID가 일치하는 User가 없음
bool removeUserFromList(char *studentID)
{
    if (userCount < 1)
        return false;

    for (int index = 0; index < userCount - 1; index++)
    {
        if (strcmp(OnlineUserList[index].user.studentID, studentID) == 0)
        {
            OnlineUserList[index] = OnlineUserList[userCount];
            memset(&OnlineUserList[userCount], 0, sizeof(OnlineUser));
            userCount--;
            return true;
        }
    }

    return false;
}

// OnlineUserList에서 studentID가 일치하는 User를 가져옴
// [매개변수] user: 검색 결과를 받을 User포인터, studentID: 검색 할 User의 studentID
// [반환] true: studentID가 일치하는 User가 있음, false: studentID가 일치하는 User가 없음
bool getUserFromList(User *user, char *studentID)
{
    for (int index = 0; index < userCount; index++)
    {
        if (strcmp(OnlineUserList[index].user.studentID, studentID) == 0)
        {
            user = &OnlineUserList[index].user;
            return true;
        }
    }

    return false;
}

