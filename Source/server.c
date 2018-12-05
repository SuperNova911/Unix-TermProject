#include "database.h"
#include "dataPack.h"
#include "interface.h"
#include "user.h"
#include "lecture.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
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
#include <sys/time.h>
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
void decomposeDataPack(int sender, DataPack *dataPack);

void userLogin(int sender, char *studentID, char *password);
void userLogout(int sender, UserInfo *userInfo);
void userRegister(int sender, char *studentID, char *password, UserInfo *userInfo);
void lectureList(int sender);
void lectureCreate(int sender, char *lectureName, UserInfo *userInfo);
void lectureRemove(int sender, char *lectureName, UserInfo *userInfo);
void lectureNotice(int sender, char *lectureName);
void lectureNoticeSet(int sender, char *lectureName, char *message, UserInfo *userInfo);
void lectureEnter(int sender, char *lectureName, UserInfo *userInfo);
void lectureLeave(int sender, char *lectureName, UserInfo *userInfo);
void lectureRegister(int sender, char *lectureName, UserInfo *userInfo);
void lectureDeregister(int sender, char *lectureName, UserInfo *userInfo);
void lectureStatus(int sender, char *lectureName);
// void attendanceStart(int sender, char *duration, char *answer, char *quiz, UserInfo *userInfo);
void attendanceStart(int sender, char *duration, UserInfo *userInfo);
void attendanceStop(int sender, UserInfo *userInfo);
void attendanceExtend(int sender, char *duration, UserInfo *userInfo);
void attendanceResult(int sender, UserInfo *userInfo);
void attendanceCheck(int sender, char *IP, char *answer, UserInfo *userInfo);
void chatEnter(int sender, UserInfo *userInfo);
void chatLeave(int sender, UserInfo *userInfo);
void chatUserList(int sender, UserInfo *userInfo);
void chatSendMessage(int sender, char *message, UserInfo *userInfo);

bool addOnlineUser(User *user, int socket);
bool removeOnlineUserBySocket(int socket);
bool removeOnlineUserByID(char *studentID);
bool addLectureInfo(Lecture *lecture);
bool removeLectureInfoByName(char *lectureName);
bool userEnterLecture(char *studentID, char *lectureName);
bool userLeaveLecture(char *studentID, char *lectureName);
bool isSameUserID(User *user, char *studentID);
bool isSameLectureName(LectureInfo *lectureInfo, char *lectureName);
OnlineUser *findOnlineUserByID(char *studentID, bool *result);
OnlineUser *findOnlineUserBySocket(int socket, bool *result);
LectureInfo *findLectureInfoByName(char *lectureName, bool *result);
LectureInfo *findLectureInfoByID(int lectureID, bool *result);
void setErrorMessage(DataPack *dataPack, char *message);

void disconnectUser(int socket);
void loadLectureListFromDB();
int getNextLectureID();
void attendanceSignalHandler();
void sendMessageToLectureMember(LectureInfo *lectureInfo, char *sender, char *message, bool onChat);
void sendDataPackToLectureMember(LectureInfo *lectureInfo, DataPack *dataPack, bool onChat);
void initializeTimer();
LectureStatusPack getLectureStatusPack(LectureInfo *lectureInfo);

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

void test()
{
    printMessage(MessageWindow, "test\n");
    sleep(1);
}

int main()
{
    initializeGlobalVariable();

    atexit(onClose);
    initiateInterface();
    drawDefaultLayout();

    if (initializeDatabase() && connectToDatabase() == false)
    {
        printMessage(MessageWindow, "데이터베이스 초기화 실패");
        getchar();
        return 0;
    }
    createTable();
    loadLectureListFromDB();

    initiateServer();

    sigset(SIGALRM, attendanceSignalHandler);
    initializeTimer();

    async();

    closeDatabase();
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
            if (errno != EINTR)
            {
                printMessage(MessageWindow, "select: '%s'\n", strerror(errno));
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
            disconnectUser(socket);
            close(socket);
            FD_CLR(socket, &Master);
            printMessage(MessageWindow, "recv: %s, socket: %d\n", strerror(errno), socket);
            break;

        // 클라이언트와 연결 종료
        case 0:
            disconnectUser(socket);
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
void decomposeDataPack(int sender, DataPack *dataPack)
{
    printMessage(MessageWindow, "sender: '%d', command: '%d', data1: '%s', data2: '%s', message: '%s'\n",
        sender, dataPack->command, dataPack->data1, dataPack->data2, dataPack->message);
    printMessage(MessageWindow, "role: '%d', userName: '%s', studentID: '%s'\n\n", dataPack->userInfo.role, dataPack->userInfo.userName, dataPack->userInfo.studentID);

    switch(dataPack->command)
    {
        case USER_LOGIN_REQUEST:
            userLogin(sender, dataPack->data1, dataPack->data2);
            break;
        case USER_LOGOUT_REQUEST:
            userLogout(sender, &dataPack->userInfo);
            break;
        case USER_REGISTER_REQUEST:
            userRegister(sender, dataPack->data1, dataPack->data2, &dataPack->userInfo);
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
        case LECTURE_NOTICE_REQUEST:
            lectureNotice(sender, dataPack->data1);
            break;
        case LECTURE_NOTICE_SET_REQUEST:
            lectureNoticeSet(sender, dataPack->data1, dataPack->message, &dataPack->userInfo);
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
        case LECTURE_STATUS_REQUEST:
            lectureStatus(sender, dataPack->data1);
            break;

        case ATTENDANCE_START_REQUEST:
            attendanceStart(sender, dataPack->data1, &dataPack->userInfo);
            break;
        case ATTNEDANCE_STOP_REQUEST:
            attendanceStop(sender, &dataPack->userInfo);
            break;
        case ATTNEDANCE_EXTEND_REQUEST:
            attendanceExtend(sender, dataPack->data1, &dataPack->userInfo);
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

    if (isUserMatch(studentID, password) == false)
    {
        setErrorMessage(&dataPack, "학번 또는 비밀번호가 틀립니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    bool dbResult;
    User user = loadUserByID(studentID, &dbResult);
    if (dbResult == false)
    {
        setErrorMessage(&dataPack, "사용자 정보를 불러올 수 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    for (int index = 0; index < UserCount; index++)
    {
        if (strcmp(OnlineUserList[index].user.studentID, studentID) == 0)
        {
            setErrorMessage(&dataPack, "이미 로그인 중 입니다");
            sendDataPack(sender, &dataPack);
            return;
        }
    }

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

    LectureInfo *lectureInfo = findLectureInfoByID(onlineUser->currentLectureID, &findResult);

    disconnectUser(sender);

    dataPack.result = true;
    sendDataPack(sender, &dataPack);

    if (findResult == false)
    {
        return;
    }

    resetDataPack(&dataPack);
    dataPack.command = LECTURE_STATUS_RESPONSE;
    dataPack.result = true;
    LectureStatusPack statusPack = getLectureStatusPack(lectureInfo);
    memcpy(&dataPack.message, &statusPack, sizeof(LectureStatusPack));
    sendDataPackToLectureMember(lectureInfo, &dataPack, false);
}

void userRegister(int sender, char *studentID, char *password, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = USER_REGISTER_RESPONSE;

    bool dbResult;
    loadUserByID(studentID, &dbResult);

    if (dbResult)
    {
        setErrorMessage(&dataPack, "해당 학번으로 등록된 계정이 이미 있습니다.");
        sendDataPack(sender, &dataPack);
        return;
    }

    User user;
    if (!(strlen(studentID) < sizeof(user.studentID)) || !(strlen(password) < sizeof(user.hashedPassword)))
    {
        setErrorMessage(&dataPack, "최대 길이 초과, 학번: 16Bytes, 비밀번호: 64Bytes");
        sendDataPack(sender, &dataPack);
        return;
    }

    user = createUser(studentID, password, userInfo->userName, userInfo->role);
    if (saveUser(&user) == false)
    {
        setErrorMessage(&dataPack, "DB에 저장하던 중 문제가 발생하였습니다.");
        sendDataPack(sender, &dataPack);
        return;
    }

    UserInfo newUserInfo = getUserInfo(&user);
    memcpy(&dataPack.userInfo, &newUserInfo, sizeof(dataPack.userInfo));
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

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    for (int index = 0; index < LectureCount; index++)
    {
        sprintf(buffer, "%s%s [%d/%d]\n", 
            buffer, LectureInfoList[index].lecture.lectureName, LectureInfoList[index].lecture.memberCount, MAX_LECTURE_MEMBER);
        buildDataPack(&dataPack, NULL, NULL, buffer);
    }

    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureCreate(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_CREATE_RESPONSE;

    if (hasPermission(userInfo->role) == false)
    {
        setErrorMessage(&dataPack, "강의를 개설할 권한이 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    Lecture newLecture = createLecture(getNextLectureID(), lectureName, userInfo->studentID);

    if (addLectureInfo(&newLecture) == false)
    {
        setErrorMessage(&dataPack, "더 이상 강의를 개설할 수 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (saveLecture(&newLecture) == false)
    {
        removeLectureInfoByName(newLecture.lectureName);
        setErrorMessage(&dataPack, "강의를 개설하던 중 문제가 발생하였습니다");
        sendDataPack(sender, &dataPack);
        return;
    }   

    buildDataPack(&dataPack, newLecture.lectureName, NULL, NULL);
    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureRemove(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_REMOVE_RESPONSE;

    if (hasPermission(userInfo->role) == false)
    {
        setErrorMessage(&dataPack, "강의를 삭제할 권한이 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    bool findResult;
    LectureInfo *lectureInfo = findLectureInfoByName(lectureName, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "해당 이름이 일치하는 강의가 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (removeLectureByID(lectureInfo->lecture.lectureID) == false)
    {
        setErrorMessage(&dataPack, "removeLectureByID 실패");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (removeLectureInfoByName(lectureName) == false)
    {
        setErrorMessage(&dataPack, "해당 이름이 일치하는 강의가 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    buildDataPack(&dataPack, lectureName, NULL, NULL);
    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureNotice(int sender, char *lectureName)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_NOTICE_RESPONSE;

    bool findResult;
    LectureInfo *lectureInfo = findLectureInfoByName(lectureName, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "현재 입장중인 강의 정보가 잘못되었습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    const int NOTICE_NUM = 3;
    char noticeList[NOTICE_NUM][512];
    int loaded;
    loaded = loadLectureNotice(noticeList, NOTICE_NUM, lectureInfo->lecture.lectureID);

    if (loaded == 0)
    {
        setErrorMessage(&dataPack, "등록된 공지사항이 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    for (int index = loaded - 1; index >= 0; index--)
    {
        buildDataPack(&dataPack, NULL, NULL, noticeList[index]);
        dataPack.result = true;
        sendDataPack(sender, &dataPack);
    }
}

void lectureNoticeSet(int sender, char *lectureName, char *message, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_NOTICE_SET_RESPONSE;

    if (hasPermission(userInfo->role) == false)
    {
        setErrorMessage(&dataPack, "공지를 등록할 권한이 없습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    bool findResult;
    LectureInfo *lectureInfo = findLectureInfoByName(lectureName, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "현재 입장중인 강의 정보가 잘못되었습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (saveLectureNotice(lectureInfo->lecture.lectureID, message) == false)
    {
        setErrorMessage(&dataPack, "DB에 저장중에 문제가 발생하였습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    dataPack.result = true;
    sendDataPackToLectureMember(lectureInfo, &dataPack, false);

    resetDataPack(&dataPack);
    dataPack.command = LECTURE_STATUS_RESPONSE;
    dataPack.result = true;
    LectureStatusPack statusPack = getLectureStatusPack(lectureInfo);
    memcpy(&dataPack.message, &statusPack, sizeof(LectureStatusPack));
    sendDataPackToLectureMember(lectureInfo, &dataPack, false);
}

void lectureEnter(int sender, char *lectureName, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_ENTER_RESPONSE;

    bool findResult;
    LectureInfo *lectureInfo = findLectureInfoByName(lectureName, &findResult);

    if (findResult == false)
    {
        setErrorMessage(&dataPack, "존재하지 않는 강의 이름 입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (isLectureMember(&lectureInfo->lecture, userInfo->studentID) == false)
    {
        setErrorMessage(&dataPack, "가입하지 않은 강의 입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (userEnterLecture(userInfo->studentID, lectureName) == false)
    {
        setErrorMessage(&dataPack, "강의실이 가득 찼습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    buildDataPack(&dataPack, lectureName, NULL, NULL);
    dataPack.result = true;
    sendDataPack(sender, &dataPack);

    resetDataPack(&dataPack);
    dataPack.command = LECTURE_STATUS_RESPONSE;
    dataPack.result = true;
    LectureStatusPack statusPack = getLectureStatusPack(lectureInfo);
    memcpy(&dataPack.message, &statusPack, sizeof(LectureStatusPack));
    sendDataPackToLectureMember(lectureInfo, &dataPack, false);
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

    LectureInfo *lectureInfo = findLectureInfoByName(lectureName, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "강의 정보가 잘못되었습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (onlineUser->onChat)
    {
        onlineUser->onChat = false;

        char message[64];
        sprintf(message, "'%s'님이 강의 대화를 나갔습니다", userInfo->userName);
        sendMessageToLectureMember(lectureInfo, userInfo->userName, message, true);
    }

    if (userLeaveLecture(userInfo->studentID, lectureName) == false)
    {
        setErrorMessage(&dataPack, "강의를 이미 나갔습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    buildDataPack(&dataPack, lectureInfo->lecture.lectureName, NULL, NULL);
    dataPack.result = true;
    sendDataPack(sender, &dataPack);

    resetDataPack(&dataPack);
    dataPack.command = LECTURE_STATUS_RESPONSE;
    dataPack.result = true;
    LectureStatusPack statusPack = getLectureStatusPack(lectureInfo);
    memcpy(&dataPack.message, &statusPack, sizeof(LectureStatusPack));
    sendDataPackToLectureMember(lectureInfo, &dataPack, false);
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

    if (isLectureMember(&lectureInfo->lecture, userInfo->studentID))
    {
        setErrorMessage(&dataPack, "이미 가입한 강의 입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (lectureAddMember(&lectureInfo->lecture, userInfo->studentID) == false)
    {
        setErrorMessage(&dataPack, "강의가 가득찼습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (saveLecture(&lectureInfo->lecture) == false)
    {
        lectureRemoveMember(&lectureInfo->lecture, userInfo->studentID);
        setErrorMessage(&dataPack, "DB에 저장 실패");
        sendDataPack(sender, &dataPack);
        return;
    }

    buildDataPack(&dataPack, lectureInfo->lecture.lectureName, NULL, NULL);

    dataPack.result = true;
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

    if (isLectureMember(&lectureInfo->lecture, userInfo->studentID) == false)
    {
        setErrorMessage(&dataPack, "가입하지 않은 강의입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (lectureRemoveMember(&lectureInfo->lecture, userInfo->studentID) == false)
    {
        setErrorMessage(&dataPack, "가입하지 않은 강의입니다(2)");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (saveLecture(&lectureInfo->lecture) == false)
    {
        lectureRemoveMember(&lectureInfo->lecture, userInfo->studentID);
        setErrorMessage(&dataPack, "DB에 저장 실패");
        sendDataPack(sender, &dataPack);
        return;
    }

    buildDataPack(&dataPack, lectureInfo->lecture.lectureName, NULL, NULL);

    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void lectureStatus(int sender, char *lectureName)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = LECTURE_STATUS_RESPONSE;

    bool findResult;
    LectureInfo *lectureInfo = findLectureInfoByName(lectureName, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "현재 강의 정보가 잘못되었습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    LectureStatusPack statusPack = getLectureStatusPack(lectureInfo);

    memcpy(&dataPack.message, &statusPack, sizeof(LectureStatusPack));
    dataPack.result = true;
    sendDataPack(sender, &dataPack);
}

void attendanceStart(int sender, char *duration, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_START_RESPONSE;

    if (hasPermission(userInfo->role) == false)
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

    if (lectureAttendanceStart(lectureInfo, atoi(duration)) == false)
    {
        setErrorMessage(&dataPack, "이미 출석 체크가 진행중 입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    removeAttendanceCheckLog(lectureInfo->lecture.lectureID, lectureInfo->attendanceEndtime);

    struct tm *timeData;
    timeData = localtime(&lectureInfo->attendanceEndtime);
    char timeString[128];
    sprintf(timeString, "%02d시 %02d분", timeData->tm_hour, timeData->tm_min);

    buildDataPack(&dataPack, duration, timeString, NULL);
    dataPack.result = true;

    sendDataPackToLectureMember(lectureInfo, &dataPack, false);

    resetDataPack(&dataPack);
    dataPack.command = LECTURE_STATUS_RESPONSE;
    dataPack.result = true;
    LectureStatusPack statusPack = getLectureStatusPack(lectureInfo);
    memcpy(&dataPack.message, &statusPack, sizeof(LectureStatusPack));
    sendDataPackToLectureMember(lectureInfo, &dataPack, false);
}

void attendanceStop(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTNEDANCE_STOP_RESPONSE;

    if (hasPermission(userInfo->role) == false)
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

    if (lectureAttendanceStop(lectureInfo) == false)
    {
        setErrorMessage(&dataPack, "출석체크가 진행중이 아닙니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    dataPack.result = true;

    sendDataPackToLectureMember(lectureInfo, &dataPack, false);

    resetDataPack(&dataPack);
    dataPack.command = LECTURE_STATUS_RESPONSE;
    dataPack.result = true;
    LectureStatusPack statusPack = getLectureStatusPack(lectureInfo);
    memcpy(&dataPack.message, &statusPack, sizeof(LectureStatusPack));
    sendDataPackToLectureMember(lectureInfo, &dataPack, false);
}

void attendanceExtend(int sender, char *duration, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTNEDANCE_EXTEND_RESPONSE;

    if (hasPermission(userInfo->role) == false)
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

    if (lectureInfo->isAttendanceActive == false)
    {
        setErrorMessage(&dataPack, "출석 체크 중이 아닙니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (lectureAttendanceExtend(lectureInfo, atoi(duration)) == false)
    {
        setErrorMessage(&dataPack, "출석체크가 진행중이 아닙니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    struct tm *timeData;
    timeData = localtime(&lectureInfo->attendanceEndtime);
    char timeString[128];
    sprintf(timeString, "%02d시 %02d분", timeData->tm_hour, timeData->tm_min);

    buildDataPack(&dataPack, duration, timeString, NULL);
    dataPack.result = true;

    sendDataPackToLectureMember(lectureInfo, &dataPack, false);
}

void attendanceResult(int sender, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_RESULT_RESPONSE;

    if (hasPermission(userInfo->role) == false)
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

    if (lectureInfo->isAttendanceActive)
    {
        setErrorMessage(&dataPack, "출석 체크가 아직 진행 중 입니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    // collect result
    AttendanceCheckLog checkLogList[MAX_LECTURE_MEMBER];
    time_t timeNow;
    time(&timeNow);
    int loaded = loadAttendanceCheckLogList(checkLogList, MAX_LECTURE_MEMBER, lectureInfo->lecture.lectureID, timeNow);

    char resultMessage[4096];
    memset(resultMessage, 0 ,sizeof(resultMessage));

    sprintf(resultMessage, "[출석체크 결과]\n'%d'명의 학생 중 '%d'명이 출석체크 하였습니다.\n", 
        lectureInfo->lecture.memberCount, loaded);

    char buffer[128];
    struct tm *timeData;
    for (int index = 0; index < loaded; index++)
    {
        timeData = localtime(&checkLogList[index].checkDate);
        memset(buffer, 0 ,sizeof(buffer));
        sprintf(buffer, "[CHECK] Date: '%02d/%02d %02d:%02d:%02d', ID: '%s', IP: '%s'\n", 
            timeData->tm_mon + 1, timeData->tm_mday, timeData->tm_hour, timeData->tm_min, timeData->tm_sec, checkLogList[index].studentID, checkLogList[index].IP);
        
        strcat(resultMessage, buffer);
    }

    sprintf(dataPack.data1, "%ld", strlen(resultMessage));
    dataPack.result = true;
    for (int index = 0; index < (int)sizeof(resultMessage); index += (sizeof(dataPack.message) - 1))
    {
        memcpy(dataPack.message, &resultMessage[index], sizeof(dataPack.message) - 1);
        if (strlen(dataPack.message) == 0)
            break;

        sendDataPack(sender, &dataPack);
    }
}

void attendanceCheck(int sender, char *IP, char *answer, UserInfo *userInfo)
{
    DataPack dataPack;
    resetDataPack(&dataPack);
    dataPack.command = ATTENDANCE_CHECK_RESPONSE;

    if (userInfo->role != USER_STUDENT)
    {
        setErrorMessage(&dataPack, "출석 체크 대상이 아닙니다");
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

    time_t timeNow;
    time(&timeNow);

    if (lectureInfo->isAttendanceActive == false || lectureInfo->attendanceEndtime < timeNow)
    {
        setErrorMessage(&dataPack, "출석 체크가 진행 중이 아닙니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    if (onlineUser->hasAttendanceChecked)
    {
        setErrorMessage(&dataPack, "이미 출석 체크를 했습니다");
        sendDataPack(sender, &dataPack);
        return;
    }

    onlineUser->hasAttendanceChecked = true;

    AttendanceCheckLog checkLog;
    checkLog.lectureID = lectureInfo->lecture.lectureID;
    strncpy(checkLog.studentID, userInfo->studentID, sizeof(checkLog.studentID));
    strncpy(checkLog.IP, IP, sizeof(checkLog.IP));
    strncpy(checkLog.quizAnswer, answer, sizeof(checkLog.quizAnswer));
    time(&checkLog.checkDate);

    saveAttendanceCheckLog(&checkLog);

    dataPack.result = true;
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

    LectureInfo *lectureInfo = findLectureInfoByID(onlineUser->currentLectureID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "입장중인 강의가 없습니다");
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

    char message[64];
    sprintf(message, "'%s'님이 강의 대화에 입장했습니다", userInfo->userName);
    sendMessageToLectureMember(lectureInfo, userInfo->userName, message, true);

    int chatUserCount = 0;
    for (int index = 0; index < lectureInfo->onlineUserCount; index++)
    {
        if (lectureInfo->onlineUser[index]->onChat)
        {
            chatUserCount++;
        }
    }

    sprintf(dataPack.data1, "%d", chatUserCount);
    dataPack.result = true;
    sendDataPack(sender, &dataPack);
    
    const int LOG_COUNT = 5;
    ChatLog chatLogList[LOG_COUNT];
    int loaded;
    loaded = loadChatLogList(chatLogList, LOG_COUNT, lectureInfo->lecture.lectureID);

    char timeString[16];
    struct tm *timeData;

    resetDataPack(&dataPack);
    dataPack.command = CHAT_SEND_MESSAGE_RESPONSE;
    dataPack.result = true;
    for (int index = loaded - 1; index >= 0; index--)
    {
        timeData = localtime(&chatLogList[index].date);
        sprintf(timeString, "%02d:%02d", timeData->tm_hour, timeData->tm_min);
        sprintf(dataPack.message, "%s [%s] %s", timeString, chatLogList[index].userName, chatLogList[index].message);
        sendDataPack(sender, &dataPack);
    }
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

    LectureInfo *lectureInfo = findLectureInfoByID(onlineUser->currentLectureID, &findResult);
    if (findResult == false)
    {
        setErrorMessage(&dataPack, "입장중인 강의가 없습니다");
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
    
    char message[64];
    sprintf(message, "'%s'님이 강의 대화를 나갔습니다", userInfo->userName);
    sendMessageToLectureMember(lectureInfo, userInfo->userName, message, true);

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

    char timeString[16];
    sprintf(timeString, "%02d:%02d", timeData->tm_hour, timeData->tm_min);

    sprintf(dataPack.message, "%s [%s] %s", timeString, userInfo->userName, message);

    for (int index = 0; index < lectureInfo->onlineUserCount; index++)
    {
        if (lectureInfo->onlineUser[index]->onChat && FD_ISSET(lectureInfo->onlineUser[index]->socket, &Master))
        {
            dataPack.result = true;
            sendDataPack(lectureInfo->onlineUser[index]->socket, &dataPack);
        }
    }

    ChatLog chatLog;
    chatLog.lectureID = lectureInfo->lecture.lectureID;
    strncpy(chatLog.userName, userInfo->userName, sizeof(chatLog.userName));
    strncpy(chatLog.message, message, sizeof(chatLog.message));
    time(&chatLog.date);
    saveChatLog(&chatLog);
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
            UserCount--;
            memcpy(&OnlineUserList[index], &OnlineUserList[UserCount], sizeof(OnlineUser));
            memset(&OnlineUserList[UserCount], 0, sizeof(OnlineUser));
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
            UserCount--;
            memcpy(&OnlineUserList[index], &OnlineUserList[UserCount], sizeof(OnlineUser));
            memset(&OnlineUserList[UserCount], 0, sizeof(OnlineUser));
            return true;
        }
    }

    return false;
}

bool addLectureInfo(Lecture *lecture)
{
    if (LectureCount >= MAX_LECTURE)
        return false;
    
    buildLectureInfo(&LectureInfoList[LectureCount], lecture);
    LectureCount++;
    return true;
}

bool removeLectureInfoByName(char *lectureName)
{
    for (int index = 0; index < LectureCount; index++)
    {
        if (isSameLectureName(&LectureInfoList[index], lectureName))
        {
            LectureCount--;
            memcpy(&LectureInfoList[index], &LectureInfoList[LectureCount], sizeof(LectureInfo));
            memset(&LectureInfoList[LectureCount], 0, sizeof(LectureInfo));
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

    for (int index = 0; index < lectureInfo->onlineUserCount; index++)
    {
        if (strcmp(lectureInfo->onlineUser[index]->user.studentID, studentID) == 0)
        {
            userLeaveLecture(studentID, lectureName);
            break;
        }
    }

    if (lectureInfo->onlineUserCount >= MAX_LECTURE_MEMBER)
        return false;

    OnlineUser *onlineUser = findOnlineUserByID(studentID, &findResult);
    if (findResult == false)
        return false;

    if (strcmp(lectureInfo->lecture.professorID, studentID) == 0)
    {
        lectureInfo->isProfessorOnline = true;
    }

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
            if (strcmp(lectureInfo->lecture.professorID, studentID) == 0)
            {
                lectureInfo->isProfessorOnline = false;
            }
            lectureInfo->onlineUserCount--;
            lectureInfo->onlineUser[index] = lectureInfo->onlineUser[lectureInfo->onlineUserCount];

            onlineUser->currentLectureID = 0;
            onlineUser->onChat = false;

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

OnlineUser *findOnlineUserBySocket(int socket, bool *result)
{
    for (int index = 0; index < UserCount; index++)
    {
        if (OnlineUserList[index].socket == socket)
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

void disconnectUser(int socket)
{
    bool findResult;

    OnlineUser *onlineUser = findOnlineUserBySocket(socket, &findResult);
    if (findResult == false)
        return;

    LectureInfo *lectureInfo = findLectureInfoByID(onlineUser->currentLectureID, &findResult);
    if (findResult)
    {
        userLeaveLecture(onlineUser->user.studentID, lectureInfo->lecture.lectureName);
    }

    if (onlineUser->currentLectureID != 0 && onlineUser->onChat)
    {
        onlineUser->onChat = false;

        UserInfo userInfo;
        strncpy(userInfo.studentID, onlineUser->user.studentID, sizeof(userInfo.studentID));
        strncpy(userInfo.userName, onlineUser->user.userName, sizeof(userInfo.userName));
        userInfo.role = onlineUser->user.role;

        char message[64];
        sprintf(message, "'%s'님이 강의 대화를 나갔습니다", onlineUser->user.userName);
        sendMessageToLectureMember(lectureInfo, onlineUser->user.userName, message, true);
    }

    removeOnlineUserBySocket(socket);
}

void loadLectureListFromDB()
{
    Lecture lectureList[MAX_LECTURE];
    LectureCount = loadLectureList(lectureList, MAX_LECTURE);

    for (int index = 0; index < LectureCount; index++)
    {
        buildLectureInfo(&LectureInfoList[index], &lectureList[index]);
    }
}

int getNextLectureID()
{
    int max = 0;
    for (int index = 0; index < LectureCount; index++)
    {
        if (LectureInfoList[index].lecture.lectureID > max)
        {
            max = LectureInfoList[index].lecture.lectureID;
        }
    }

    return max + 1;
}

void attendanceSignalHandler()
{
    DataPack dataPack;

    time_t timeNow;
    time(&timeNow);

    for (int index = 0; index < LectureCount; index++)
    {
        if (LectureInfoList[index].isAttendanceActive && LectureInfoList[index].attendanceEndtime <= timeNow)
        {
            lectureAttendanceStop(&LectureInfoList[index]);
            sendMessageToLectureMember(&LectureInfoList[index], "강의", "출석체크가 종료되었습니다", false);

            resetDataPack(&dataPack);
            dataPack.command = LECTURE_STATUS_RESPONSE;
            dataPack.result = true;
            LectureStatusPack statusPack = getLectureStatusPack(&LectureInfoList[index]);
            memcpy(&dataPack.message, &statusPack, sizeof(LectureStatusPack));
            sendDataPackToLectureMember(&LectureInfoList[index], &dataPack, false);
        }
    }
}

void sendMessageToLectureMember(LectureInfo *lectureInfo, char *sender, char *message, bool onChat)
{
    DataPack dataPack;
    dataPack.command = CHAT_SEND_MESSAGE_RESPONSE;
    dataPack.result = true;

    struct tm *timeData;
    time_t timeNow;
    time(&timeNow);
    timeData = localtime(&timeNow);

    char timeString[16];
    sprintf(timeString, "%02d:%02d", timeData->tm_hour, timeData->tm_min);

    buildDataPack(&dataPack, sender, timeString, message);

    sendDataPackToLectureMember(lectureInfo, &dataPack, onChat);
}

void sendDataPackToLectureMember(LectureInfo *lectureInfo, DataPack *dataPack, bool onChat)
{
    for (int index = 0; index < lectureInfo->onlineUserCount; index++)
    {
        if (FD_ISSET(lectureInfo->onlineUser[index]->socket, &Master) && (onChat ? lectureInfo->onlineUser[index]->onChat : true))
        {
            sendDataPack(lectureInfo->onlineUser[index]->socket, dataPack);
        }
    }
}

void initializeTimer()
{
    time_t timeNow;
    time(&timeNow);

    int delay = (60 - timeNow % 60) + 2;

    struct itimerval itimer;
    itimer.it_value.tv_sec = delay;
    itimer.it_value.tv_usec = 0;
    itimer.it_interval.tv_sec = 15;
    itimer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itimer, (struct itimerval *)NULL);
}

LectureStatusPack getLectureStatusPack(LectureInfo *lectureInfo)
{
    const int NOTICE_NUM = 1;
    char noticeList[NOTICE_NUM][512];
    memset(noticeList, 0, sizeof(noticeList[0]));

    int loaded;
    loaded = loadLectureNotice(noticeList, NOTICE_NUM, lectureInfo->lecture.lectureID);

    if (loaded == 0)
    {
        strncpy(noticeList[0], "등록된 공지사항이 없습니다", sizeof(noticeList[0]));
    }

    return createLectureStatusPack(lectureInfo->onlineUserCount, lectureInfo->isProfessorOnline, lectureInfo->isAttendanceActive, lectureInfo->isQuizActive, noticeList[0]);
}
