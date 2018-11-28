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


typedef struct OnlineUser_t
{
    int socket;
    int currentLectureID;
    bool onChat;
    User user;
} OnlineUser;

typedef struct LectureInfo_t
{
    int onlineUserCount;
    Lecture lecture;
    OnlineUser *onlineUser[MAX_LECTURE_MEMBER];
} LectureInfo;

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
void attendanceStart(int sender, char *duration, char *answer, char *quiz, UserInfo *userInfo);
void attendanceStop(int sender, UserInfo *userInfo);
void attendanceExtend(int sender, int duration, UserInfo *userInfo);
void attendanceResult(int sender, UserInfo *userInfo);
void attendanceCheck(int sender, char *HWID, char *answer, UserInfo *userInfo);
void chatEnter(int sender, UserInfo *userInfo);
void chatLeave(int sender, UserInfo *userInfo);
void chatUserList(int sender, UserInfo *userInfo);
void chatSendMessage(int sender, char *message, UserInfo *userInfo);

bool addOnlineUser(User *user, int socket);
bool removeOnlineUserBySocket(int socket);
bool removeOnlineUserByID(char *studentID);
bool addLecture(Lecture *lecture);
bool removeLectureByName(char *lectureName);
bool userEnterLecture(char *studentID, char *lectureName);
bool userLeaveLecture(char *studentID, char *lectureName);
bool isSameUserID(User *user, char *studentID);
bool isSameLectureName(LectureInfo *lectureInfo, char *lectureName);
OnlineUser *findOnlineUserByID(char *studentID, bool *result);
LectureInfo *findLectureInfoByName(char *lectureName, bool *result);
LectureInfo *findLectureInfoByID(int lectureID, bool *result);
void setErrorMessage(DataPack *dataPack, char *message);
bool hasPermission(UserInfo *userInfo);

int UserCount;
int LectureCount;
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

    LectureInfoList[0].lecture.lectureID = 1;
    LectureInfoList[1].lecture.memberCount = 2;
    strncpy(LectureInfoList[0].lecture.professorID, "201510743", sizeof(LectureInfoList[0].lecture.professorID));
    strncpy(LectureInfoList[0].lecture.lectureName, "MyCustomClass", sizeof(LectureInfoList[0].lecture.lectureName));
    strncpy(LectureInfoList[0].lecture.memberList[0], "201500000", sizeof(LectureInfoList[0].lecture.professorID));
    strncpy(LectureInfoList[0].lecture.memberList[1], "201510743", sizeof(LectureInfoList[0].lecture.professorID));
    time(&LectureInfoList[0].lecture.createDate);

    LectureInfoList[1].lecture.lectureID = 2;
    LectureInfoList[1].lecture.memberCount = 2;
    strncpy(LectureInfoList[1].lecture.professorID, "12345", sizeof(LectureInfoList[1].lecture.professorID));
    strncpy(LectureInfoList[1].lecture.lectureName, "UnixClass", sizeof(LectureInfoList[1].lecture.lectureName));
    strncpy(LectureInfoList[1].lecture.memberList[1], "201500000", sizeof(LectureInfoList[1].lecture.professorID));
    strncpy(LectureInfoList[1].lecture.memberList[1], "201510743", sizeof(LectureInfoList[1].lecture.professorID));
    time(&LectureInfoList[1].lecture.createDate);
    
    LectureInfoList[2].lecture.lectureID = 3;
    strncpy(LectureInfoList[2].lecture.professorID, "54321", sizeof(LectureInfoList[2].lecture.professorID));
    strncpy(LectureInfoList[2].lecture.lectureName, "PrivateClass", sizeof(LectureInfoList[2].lecture.lectureName));
    time(&LectureInfoList[2].lecture.createDate);

    LectureCount = 3;

    initiateServer();

    async();
    
    return 0;
}

// 전역 변수 초기화
void initializeGlobalVariable()
{
    UserCount = 0;
    LectureCount = 0;

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
    printMessage(MessageWindow, "role: '%d', userName: '%s', studentID: '%s'\n", dataPack->userInfo.role, dataPack->userInfo.userName, dataPack->userInfo.studentID);

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
            attendanceStart(sender, dataPack->data1, dataPack->data2, dataPack->message, &dataPack->userInfo);
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
            chatLeave(sender, &dataPack->userInfo);
            break;
        case CHAT_USER_LIST_REQUEST:
            chatUserList(sender, &dataPack->userInfo);
            break;
        case CHAT_SEND_MESSAGE_REQUEST:
            chatSendMessage(sender, dataPack->message, &dataPack->userInfo);
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

    if (loginUser(studentID, password) == false)
    {
        setErrorMessage(&dataPack, "학번 또는 비밀번호가 틀립니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    User user;
    if (loadUserByID(&user, studentID) == false)
    {
        setErrorMessage(&dataPack, "사용자 정보를 불러올 수 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    printMessage(MessageWindow, "id: '%s', name: '%s'\n", user.studentID, user.userName);

    if (addOnlineUser(&user, sender) == false)
    {
        setErrorMessage(&dataPack, "서버가 가득 찼습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    dataPack.result = true;
    dataPack.userInfo.role = user.role;
    strncpy(dataPack.userInfo.userName, user.userName, sizeof(dataPack.userInfo.userName));
    strncpy(dataPack.userInfo.studentID, user.studentID, sizeof(dataPack.userInfo.studentID));
    sendDataPack(sender, &dataPack);
}

void userLogout(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = USER_LOGOUT_RESPONSE;

    bool findResult;
    OnlineUser *onlineUser = findOnlineUserByID(userInfo->studentID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "이미 로그아웃 하였습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (onlineUser->currentLectureID != 0)
    {
        if (onlineUser->onChat)
        {
            onlineUser->onChat = false;
            chatSendMessage(sender, "강의 대화를 나갔습니다.", userInfo);
        }

        LectureInfo *lectureInfo = findLectureInfoByID(onlineUser->currentLectureID, &findResult);
        userLeaveLecture(userInfo->studentID, lectureInfo->lecture.lectureName);
    }

    if (removeOnlineUserByID(userInfo->studentID) == false)
    {
        setErrorMessage(&dataPack, "이미 로그아웃 하였습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureList(int sender)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_LIST_RESPONSE;

    if (LectureCount == 0)
    {
        setErrorMessage(&dataPack, "개설된 강의가 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    for (int index = 0; index < LectureCount; index++)
    {
        sprintf(dataPack.message, "%s%s[%d/%d]\n", dataPack.message, LectureInfoList[index].lecture.lectureName, LectureInfoList[index].onlineUserCount, MAX_LECTURE_MEMBER);
    }

    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureCreate(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_CREATE_RESPONSE;

    if (hasPermission(userInfo) == false)
    {
        setErrorMessage(&dataPack, "강의를 개설할 권한이 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    Lecture newLecture;
    memset(&newLecture, 0, sizeof(Lecture));
    strncpy(newLecture.professorID, userInfo->studentID, sizeof(newLecture.professorID));
    strncpy(newLecture.lectureName, lectureName, sizeof(newLecture.lectureName));
    time(&newLecture.createDate);

    if (addLecture(&newLecture) == false)
    {
        setErrorMessage(&dataPack, "더 이상 강의를 개설할 수 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (createLecture(&newLecture) == false)
    {
        removeLectureByName(newLecture.lectureName);
        setErrorMessage(&dataPack, "강의를 개설하던 중 문제가 발생하였습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureRemove(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_REMOVE_RESPONSE;

    if (hasPermission(userInfo) == false)
    {
        setErrorMessage(&dataPack, "강의를 삭제할 권한이 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (removeLectureByName(lectureName) == false)
    {
        setErrorMessage(&dataPack, "해당 이름이 일치하는 강의가 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureEnter(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_ENTER_RESPONSE;

    if (userEnterLecture(userInfo->studentID, lectureName) == false)
    {
        setErrorMessage(&dataPack, "해당 강의가 없거나 가득 찼습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    buildDataPack(&dataPack, lectureName, NULL, NULL, NULL, NULL);
    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureLeave(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_LEAVE_RESPONSE;

    bool findResult;
    OnlineUser *onlineUser = findOnlineUserByID(userInfo->studentID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "사용자 정보를 찾을 수 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (onlineUser->onChat)
    {
        onlineUser->onChat = false;
        chatSendMessage(sender, "강의 대화를 나갔습니다.", userInfo);
    }

    if (userLeaveLecture(userInfo->studentID, lectureName) == false)
    {
        setErrorMessage(&dataPack, "해당 강의 정보가 잘못됐거나 이미 나갔습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureRegister(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_REGISTER_RESPONSE;

    bool findResult;
    LectureInfo *lectureInfo = findLectureInfoByName(lectureName, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "해당 이름이 일치하는 강의가 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    for (int index = 0; index < lectureInfo->lecture.memberCount; index++)
    {
        if (strcmp(lectureInfo->lecture.memberList[index], userInfo->studentID) == 0)
        {
            setErrorMessage(&dataPack, "이미 가입한 강의 입니다");
            sendDataPack(sender, &dataPack);
            return;
        }
    }

    if ((lectureInfo->onlineUserCount + 1) == MAX_LECTURE_MEMBER)
    {
        setErrorMessage(&dataPack, "강의가 가득찼습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (lecture_registerUser(lectureInfo->lecture.lectureID, userInfo->studentID) == false)
    {
        setErrorMessage(&dataPack, "가입 요청 중 문제가 발생하였습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    OnlineUser *onlineUser = findOnlineUserByID(userInfo->studentID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "잘못된 사용자 정보입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    lectureInfo->onlineUser[lectureInfo->onlineUserCount] = onlineUser;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}

void lectureDeregister(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_DEREGISTER_RESPONSE;

    bool findResult;
    LectureInfo *lectureInfo = findLectureInfoByName(lectureName, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "해당 이름이 일치하는 강의가 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    // Do something

    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void attendanceStart(int sender, char *duration, char *answer, char *quiz, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_RESPONSE;

    if (hasPermission(userInfo) == false)
    {
        setErrorMessage(&dataPack, "권한이 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    bool findResult;
    OnlineUser *onlineUser = findOnlineUserByID(userInfo->studentID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "잘못된 사용자 정보입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    LectureInfo *lectureInfo = findLectureInfoByID(onlineUser->currentLectureID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "입장중인 강의가 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    struct tm *timeData;
    time_t currentTime;
    currentTime = time(NULL);
    timeData = localtime(&currentTime);
    timeData->tm_min += atoi(duration);


    for (int index = 0; index < lectureInfo->onlineUserCount; index++)
    {
    }

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void attendanceStop(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTNEDANCE_STOP_RESPONSE;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void attendanceExtend(int sender, int duration, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTNEDANCE_EXTEND_RESPONSE;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void attendanceResult(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_RESULT_RESPONSE;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}
void attendanceCheck(int sender, char *HWID, char *answer, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_CHECK_RESPONSE;

    dataPack.result = false;
    sendDataPack(sender, &dataPack);
}

void chatEnter(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = CHAT_ENTER_RESPONSE;

    bool findResult;
    OnlineUser *onlineUser = findOnlineUserByID(userInfo->studentID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "잘못된 사용자 정보입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (onlineUser->currentLectureID == 0)
    {
        setErrorMessage(&dataPack, "강의실에 먼저 입장해야 합니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (onlineUser->onChat)
    {
        setErrorMessage(&dataPack, "이미 강의 대화에 있습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    onlineUser->onChat = true;
    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}
void chatLeave(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = CHAT_LEAVE_RESPONSE;

    bool findResult;
    OnlineUser *onlineUser = findOnlineUserByID(userInfo->studentID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "잘못된 사용자 정보입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (onlineUser->currentLectureID == 0)
    {
        setErrorMessage(&dataPack, "이미 강의실에서 나갔습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (onlineUser->onChat == false)
    {
        setErrorMessage(&dataPack, "이미 강의 대화에서 나갔습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    onlineUser->onChat = false;
    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}
void chatUserList(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = CHAT_USER_LIST_RESPONSE;

    bool findResult;
    OnlineUser *onlineUser = findOnlineUserByID(userInfo->studentID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "잘못된 사용자 정보입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    LectureInfo *lectureInfo = findLectureInfoByID(onlineUser->currentLectureID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "강의 정보를 찾을 수 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    for (int index = 0; index < lectureInfo->onlineUserCount; index++)
    {
        if (lectureInfo->onlineUser[index]->onChat)
        {
            sprintf(dataPack.message, "%s%s, ", dataPack.message, lectureInfo->onlineUser[index]->user.userName);
        }
    }

    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}
void chatSendMessage(int sender, char *message, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = CHAT_SEND_MESSAGE_RESPONSE;

    bool findResult;
    OnlineUser *onlineUser = findOnlineUserByID(userInfo->studentID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "잘못된 사용자 정보입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    LectureInfo *lectureInfo = findLectureInfoByID(onlineUser->currentLectureID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "강의 정보를 찾을 수 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    struct tm *timeData;
    time_t currentTime;
    time(&currentTime);
    timeData = localtime(&currentTime);

    char timeString[6];
    sprintf(timeString, "%02d:%02d", timeData->tm_hour, timeData->tm_min);

    buildDataPack(&dataPack, userInfo->userName, timeString, NULL, NULL, message);

    for (int index = 0; index < lectureInfo->onlineUserCount; index++)
    {
        if (lectureInfo->onlineUser[index]->onChat)
        {
            dataPack.result = true;
            sendDataPack(lectureInfo->onlineUser[index]->socket, &dataPack);
        }
    }
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



bool addOnlineUser(User *user, int socket)
{
    if ((UserCount + 1) == MAX_CLIENT)
        return false;

    memcpy(&OnlineUserList[UserCount].user, user, sizeof(User));
    OnlineUserList[UserCount].socket = socket;
    UserCount++;

    return true;
}

bool removeOnlineUserBySocket(int socket)
{
    for (int index = 0; index < UserCount; index++)
    {
        if (OnlineUserList[index].socket == socket)
        {
            memcpy(&OnlineUserList[index], &OnlineUserList[UserCount], sizeof(OnlineUser));
            memset(&OnlineUserList[UserCount], 0, sizeof(OnlineUser));
            UserCount--;
            return true;
        }
    }

    return false;
}

bool removeOnlineUserByID(char *studentID)
{
    for (int index = 0; index < UserCount; index++)
    {
        if (isSameUserID(&OnlineUserList[index].user, studentID))
        {
            memcpy(&OnlineUserList[index], &OnlineUserList[UserCount], sizeof(OnlineUser));
            memset(&OnlineUserList[UserCount], 0, sizeof(OnlineUser));
            UserCount--;
            return true;
        }
    }

    return false;
}

bool addLecture(Lecture *lecture)
{
    if ((LectureCount + 1) < MAX_LECTURE)
        return false;
    
    memcpy(&LectureInfoList[LectureCount].lecture, lecture, sizeof(Lecture));
    LectureCount++;
    return true;
}

bool removeLectureByName(char *lectureName)
{
    for (int index = 0; index < LectureCount; index++)
    {
        if (isSameLectureName(&LectureInfoList[index], lectureName))
        {
            memcpy(&LectureInfoList[index], &LectureInfoList[LectureCount], sizeof(LectureInfo));
            memset(&LectureInfoList[LectureCount], 0, sizeof(LectureInfo));
            LectureCount--;
            return true;
        }
    }

    return false;
}

bool userEnterLecture(char *studentID, char *lectureName)
{
    bool findResult;
    LectureInfo *lectureInfo = findLectureInfoByName(lectureName, &findResult);
    if (findResult == false)
        return false;

    if ((lectureInfo->onlineUserCount + 1) == MAX_LECTURE_MEMBER)
        return false;

    OnlineUser *onlineUser = findOnlineUserByID(studentID, &findResult);
    if (findResult == false)
        return false;

    lectureInfo->onlineUser[lectureInfo->onlineUserCount] = onlineUser;
    lectureInfo->onlineUserCount++;

    onlineUser->currentLectureID = lectureInfo->lecture.lectureID;

    return true;
}

bool userLeaveLecture(char *studentID, char *lectureName)
{
    bool findResult;
    LectureInfo *lectureInfo = findLectureInfoByName(lectureName, &findResult);
    if (findResult == false)
        return false;

    OnlineUser *onlineUser = findOnlineUserByID(studentID, &findResult);
    if (findResult == false)
        return false;

    for (int index = 0; index < lectureInfo->onlineUserCount; index++)
    {
        if (isSameUserID(&lectureInfo->onlineUser[index]->user, studentID))
        {
            lectureInfo->onlineUser[index] = lectureInfo->onlineUser[lectureInfo->onlineUserCount];
            // 초기화?
            lectureInfo->onlineUserCount--;

            onlineUser->currentLectureID = 0;

            return true;
        }
    }

    return false;
}

bool isSameUserID(User *user, char *studentID)
{
    return strcmp(user->studentID, studentID) == 0 ? true : false;
}

bool isSameLectureName(LectureInfo *lectureInfo, char *lectureName)
{
    return strcmp(lectureInfo->lecture.lectureName, lectureName) == 0 ? true : false;
}

OnlineUser *findOnlineUserByID(char *studentID, bool *result)
{
    for (int index = 0; index < UserCount; index++)
    {
        if (isSameUserID(&OnlineUserList[index].user, studentID))
        {
            *result = true;
            return &OnlineUserList[index];
        }
    }

    *result = false;
    return NULL;
}

LectureInfo *findLectureInfoByName(char *lectureName, bool *result)
{
    for (int index = 0; index < LectureCount; index++)
    {
        printMessage(MessageWindow, "'%s', '%s'\n", LectureInfoList[index].lecture.lectureName, lectureName);
        if (isSameLectureName(&LectureInfoList[index], lectureName))
        {
            *result = true;
            return &LectureInfoList[index];
        }
    }

    *result = false;
    return NULL;
}

LectureInfo *findLectureInfoByID(int lectureID, bool *result)
{
    for (int index = 0; index < LectureCount; index++)
    {
        if (LectureInfoList[index].lecture.lectureID == lectureID)
        {
            *result = true;
            return &LectureInfoList[index];
        }
    }

    *result = false;
    return NULL;
}

void setErrorMessage(DataPack *dataPack, char *message)
{
    dataPack->result = false;
    strncpy(dataPack->message, message, sizeof(dataPack->message));
}

bool hasPermission(UserInfo *userInfo)
{
    return ((userInfo->role == Admin) || (userInfo->role == Professor)) ? true : false;
}


