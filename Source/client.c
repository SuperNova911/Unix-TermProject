#include "dataPack.h"
#include "interface.h"
#include "user.h"
#include "lecture.h"

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
    Default, Login, LectureBrowser, Lobby, Chat, Attendance
} CurrentClientStatus;

struct LoginInfo_t
{
    char studentID[16];
    char password[32];
} LoginInfo;

struct CurrentLectureInfo_t
{
    int lectureID;
    char lectureName[64];
} CurrentLectureInfo;

void initializeGlobalVariable();

// 서버
bool connectServer();
void async();
void receiveData();
bool sendDataPack(DataPack *dataPack);

bool composeDataPack(enum Command command);
bool decomposeDataPack(DataPack *dataPack);
void sendSampleDataPack();

void getUserInput();
void setInputGuide(char *inputGuide);
void receiveUserCommand();

void updateLayoutByStatus();
void updateCommandByStatus();
void updateInputByStatus();
void changeClientStatus(enum ClientStatus status);

bool receiveArgumentComplete();
void resetArgument();
void setArgumentGuide(char *guide1, char *guide2, char *guide3, char *guide4, char *guide5);

extern const char *UserRoleString[4];
extern const char *CommandString[37];

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

char UserInputGuide[64];
char UserInputBuffer[256];

enum Command CurrentRequest;
int TargetArgument, CurrentArgument;
bool ReceiveArgument;
char ArgumentGuide[5][64];
char Arguments[5][256];

int main()
{
    initializeGlobalVariable();

    atexit(onClose);
    initiateInterface();

    changeClientStatus(Default);

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

    strncpy(LoginInfo.studentID, "201510743", sizeof(LoginInfo.studentID));
    strncpy(LoginInfo.password, "suwhan77", sizeof(LoginInfo.password));

    changeClientStatus(Login);

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
    memset(&CurrentLectureInfo, 0, sizeof(struct CurrentLectureInfo_t));

    memset(UserInputGuide, 0, sizeof(UserInputGuide));
    memset(UserInputBuffer, 0, sizeof(UserInputBuffer));

    TargetArgument = 0;
    CurrentArgument = 0;
    ReceiveArgument = false;

    const int ARGS_NUM = 5;
    for (int index = 0; index < ARGS_NUM; index++)
    {
        memset(Arguments[index], 0, sizeof(Arguments[index]));
        memset(ArgumentGuide[index], 0, sizeof(ArgumentGuide[index]));
    }
}

// TCP/IP 소켓 서버와 연결
// [반환] true: 성공, false: 실패
bool connectServer()
{
    // TCP/IP 소켓
    ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocket == -1)
    {
        printMessage(MessageWindow, "socket: '%s'\n", strerror(errno));
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
        printMessage(MessageWindow, "connect: '%s'\n", strerror(errno));
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
    memset(&receivedDataPack, 0, sizeof(DataPack));

    readBytes = recv(ServerSocket, buffer, sizeof(buffer), 0);
    switch (readBytes)
    {
        // 예외 처리
        case -1:
            close(ServerSocket);
            FD_CLR(ServerSocket, &Master);
            printMessage(MessageWindow, "recv: '%s'\n", strerror(errno));
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
                memcpy(&receivedDataPack, buffer, sizeof(DataPack));
                decomposeDataPack(&receivedDataPack);
            }
            else
            {
                printMessage(MessageWindow, "Invalid data received, readBytes: '%d'\n", readBytes);
            }
            break;
    }
}

// 매개변수로 받은 Command에 따라 DataPack을 만들고 서버에 전송
// [매개변수] command: 전송할 DataPack의 Command 종류
// [반환] true: DataPack 전송 성공, false: 잘못된 Command 매개변수
bool composeDataPack(enum Command command)
{
    DataPack dataPack;
    memset(&dataPack, 0, sizeof(DataPack));

    dataPack.command = command;
    memcpy(&dataPack.userInfo, &CurrentUserInfo, sizeof(UserInfo));

    switch (dataPack.command)
    {
        case USER_LOGIN_REQUEST:
            buildDataPack(&dataPack, Arguments[0], Arguments[1], NULL, NULL, NULL);
            // memset(&LoginInfo, 0, sizeof(struct LoginInfo_t));   디버깅
            break;
        
        case USER_LOGOUT_REQUEST:
            break;

        case LECTURE_LIST_REQUEST:
            break;
        
        case LECTURE_CREATE_REQUEST:
            buildDataPack(&dataPack, Arguments[0], NULL, NULL, NULL, NULL);
            break;
        
        case LECTURE_REMOVE_REQUEST:
            buildDataPack(&dataPack, Arguments[0], NULL, NULL, NULL, NULL);
            break;
        
        case LECTURE_ENTER_REQUEST:
            buildDataPack(&dataPack, Arguments[0], NULL, NULL, NULL, NULL);
            // buildDataPack(&dataPack, Arguments[0], NULL, NULL, NULL, NULL);
            break;
        
        case LECTURE_LEAVE_REQUEST:
            buildDataPack(&dataPack, CurrentLectureInfo.lectureName, NULL, NULL, NULL, NULL);
            break;
        
        case LECTURE_REGISTER_REQUEST:
            buildDataPack(&dataPack, Arguments[0], NULL, NULL, NULL, NULL);
            break;
        
        case LECTURE_DEREGISTER_REQUEST:
            buildDataPack(&dataPack, Arguments[0], NULL, NULL, NULL, NULL);
            break;

        case ATTENDANCE_START_REQUEST:
            buildDataPack(&dataPack, Arguments[0], Arguments[2], NULL, NULL, Arguments[1]);
            break;
        
        case ATTNEDANCE_STOP_REQUEST:
            break;

        case ATTNEDANCE_EXTEND_REQUEST:
            buildDataPack(&dataPack, Arguments[0], NULL, NULL, NULL, NULL);
            break;
        
        case ATTENDANCE_RESULT_REQUEST:
            break;
        
        case ATTENDANCE_CHECK_REQUEST:
            buildDataPack(&dataPack, Arguments[0], NULL, NULL, NULL, UserInputBuffer);
            break;

        case CHAT_ENTER_REQUEST:
            break;
        
        case CHAT_LEAVE_REQUEST:
            break;
        
        case CHAT_USER_LIST_REQUEST:
            break;
        
        case CHAT_SEND_MESSAGE_REQUEST:
            buildDataPack(&dataPack, NULL, NULL, NULL, NULL, UserInputBuffer);
            break;

        default:
            return false;
    }

    if (sendDataPack(&dataPack))
        return true;
    else
        return false;
}

// 서버에게 전송 받은 DataPack의 Command에 따라 지정된 작업을 수행
// [매개변수] dataPack: 전송 받은 DataPack
// [반환] true: 성공적으로 DataPack 분석을 함, false: 잘못된 DataPack의 Command
bool decomposeDataPack(DataPack *dataPack)
{
    printMessage(MessageWindow, "command: '%d', message: '%s'\n", dataPack->command, dataPack->message);

    switch(dataPack->command)
    {
        case USER_LOGIN_RESPONSE:
            if (dataPack->result)
            {
                memcpy(&CurrentUserInfo, &dataPack->userInfo, sizeof(UserInfo));
                changeClientStatus(LectureBrowser);
                printMessage(MessageWindow, "로그인 성공, 학번: '%s', 이름: '%s', 역할: '%s'\n", dataPack->userInfo.studentID, dataPack->userInfo.userName, UserRoleString[dataPack->userInfo.role]);
            }
            else
                printMessage(MessageWindow, "로그인에 실패하였습니다. 오류: '%s'\n", dataPack->message);
            break;
        case USER_LOGOUT_RESPONSE:
            if (dataPack->result)
            {
                changeClientStatus(Login);
                printMessage(MessageWindow, "정상적으로 로그아웃 되었습니다.\n");
            }
            else
                printMessage(MessageWindow, "로그아웃을 시도하던 중 문제가 발생하였습니다. 오류: '%s'\n", dataPack->message);
            break;

        case LECTURE_LIST_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "[개설된 강의 목록]\n%s\n", dataPack->message);
            }
            else
                printMessage(MessageWindow, "강의 목록을 불러올 수 없습니다. 오류: '%s'\n", dataPack->message);
            break;
        case LECTURE_CREATE_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "새로운 강의가 개설되었습니다. 강의명: '%s'\n", dataPack->data1);
            }
            else
                printMessage(MessageWindow, "강의를 개설할 수 없습니다. 오류: '%s'\n", dataPack->message);
            break;
        case LECTURE_REMOVE_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "강의가 삭제되었습니다. 강의명: '%s'\n", dataPack->data1);
            }
            else
                printMessage(MessageWindow, "강의를 삭제할 수 없습니다. 오류: '%s'\n", dataPack->message);
            break;
        case LECTURE_ENTER_RESPONSE:
            if (dataPack->result)
            {
                changeClientStatus(Lobby);
                strncpy(CurrentLectureInfo.lectureName, dataPack->data1, sizeof(CurrentLectureInfo.lectureName));
                CurrentLectureInfo.lectureID = atoi(dataPack->data2);
                printMessage(MessageWindow, "강의실에 입장하였습니다. 강의명: '%s'\n", dataPack->data1);
            }
            else
                printMessage(MessageWindow, "강의실에 입장할 수 없습니다. 오류: '%s'\n", dataPack->message);
            break;
        case LECTURE_LEAVE_RESPONSE:
            if (dataPack->result)
            {
                changeClientStatus(LectureBrowser);
                printMessage(MessageWindow, "강의실에 퇴장하였습니다. 강의명: '%s'\n", dataPack->data1);
            }
            else
                printMessage(MessageWindow, "강의실에서 나가던 중 문제가 발생하였습니다. 오류: '%s'\n", dataPack->message);
            break;
        case LECTURE_REGISTER_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "강의 가입 요청을 보냈습니다. 강의명: '%s'\n", dataPack->data1);
            }
            else
                printMessage(MessageWindow, "가입 요청을 보내던 중 문제가 발생하였습니다. 오류: '%s'\n", dataPack->message);
            break;
        case LECTURE_DEREGISTER_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "강의 탈퇴 요청을 보냈습니다. 강의명: '%s'\n", dataPack->data1);
            }
            else
                printMessage(MessageWindow, "탈퇴 요청을 보내던 중 문제가 발생하였습니다. 오류: '%s'\n", dataPack->message);
            break;

        case ATTENDANCE_START_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "출석체크를 시작합니다. 출석체크는 '%s'분 간 진행됩니다.\n종료 시간: '%s'\n", dataPack->data1, dataPack->data2);
            }
            else
                printMessage(MessageWindow, "출석체크 시작 요청을 보내던 중 문제가 발생하였습니다. 오류: '%s'\n", dataPack->message);
            break;
        case ATTNEDANCE_STOP_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "출석체크를 종료합니다.\n");
            }
            else
                printMessage(MessageWindow, "출석체크를 종료 할 수 없습니다. 오류: '%s'\n", dataPack->message);
            break;
        case ATTNEDANCE_EXTEND_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "출석체크를 '%d'분 연장합니다.\n종료 시간: '%s'\n", dataPack->data1, dataPack->data2);
            }
            else
                printMessage(MessageWindow, "출석체크 시간 연장을 할 수 없습니다. 오류: '%s'\n", dataPack->message);
            break;
        case ATTENDANCE_RESULT_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "[출석체크 결과]\n%s\n", dataPack->message);
            }
            else
                printMessage(MessageWindow, "출석체크 결과를 요청할 수 없습니다. 오류: '%s'\n", dataPack->message);
            break;
        case ATTENDANCE_CHECK_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "성공적으로 출석체크를 하였습니다.\n");
            }
            else
                printMessage(MessageWindow, "출석체크를 하지 못했습니다. 오류: '%s'\n", dataPack->message);
            break;

        case CHAT_ENTER_RESPONSE:
            if (dataPack->result)
            {
                changeClientStatus(Chat);
                printMessage(MessageWindow, "강의 대화에 입장하였습니다. 현재 '%s'명이 대화 중 입니다.\n", dataPack->data1);
            }
            else
                printMessage(MessageWindow, "강의 대화에 입장 할 수 없습니다. 오류: '%s'\n", dataPack->message);
            break;
        case CHAT_LEAVE_RESPONSE:
            if (dataPack->result)
            {
                changeClientStatus(Lobby);
                printMessage(MessageWindow, "강의 대화에서 퇴장하였습니다.\n");
            }
            else
                printMessage(MessageWindow, "강의 대화에서 나가던 중 문제가 발생하였습니다. 오류: '%s'\n", dataPack->message);
            break;
        case CHAT_USER_LIST_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "[강의 대화에 접속중인 멤버 목록]\n%s\n", dataPack->message);
            }
            else
                printMessage(MessageWindow, "강의 대화에 접속중인 멤버 목록을 불러올 수 없습니다. 오류: '%s'\n", dataPack->message);
            break;
        case CHAT_SEND_MESSAGE_RESPONSE:
            if (dataPack->result)
            {
                printMessage(MessageWindow, "%s [%s] %s\n", dataPack->data2, dataPack->data1, dataPack->message);
            }
            else
                printMessage(MessageWindow, "메시지를 전송 할 수 없습니다. 오류: '%s'\n", dataPack->message);
            break;

        default:
            printMessage(MessageWindow, "Invaild DataPack received, command: '%d'\n", dataPack->command);
            return false;
    }

    return true;
}

// 서버에게 DataPack을 전송
// [매개변수] dataPack: 전송 할 DataPack
// [반환] true: 전송 성공, false: 전송 실패
bool sendDataPack(DataPack *dataPack)
{
    int sendBytes;

    sendBytes = send(ServerSocket, (char *)dataPack, sizeof(DataPack), 0);
    switch (sendBytes)
    {
        case -1:
            printMessage(MessageWindow, "데이터를 전송하는 중 문제가 발생하였습니다. send: '%s'\n", strerror(errno));
            break;
        
        case 0:
            printMessage(MessageWindow, "서버와 연결할 수 없습니다.\n");
            break;
        
        default:
            if (sendBytes == sizeof(DataPack))
                return true;
                
            printMessage(MessageWindow, "데이터를 완전히 전송하는데에 실패하였습니다. 잠시 후 다시 시도하세요.\n");
            break;
    }

    return false;
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
            receiveUserCommand();
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

    updateInputByStatus();
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

    if (ReceiveArgument)
    {
        strncpy(Arguments[CurrentArgument], UserInputBuffer, sizeof(Arguments[CurrentArgument]));
        CurrentArgument++;

        if (receiveArgumentComplete())
        {
            composeDataPack(CurrentRequest);
            resetArgument();
        }
        return;
    }

    switch (CurrentClientStatus)
    {
        case Login:
            setArgumentGuide("학번을 입력하세요", "비밀번호를 입력하세요", NULL, NULL, NULL);
            CurrentRequest = USER_LOGIN_REQUEST;
            ReceiveArgument = true;
            TargetArgument = 2;
            break;

        case LectureBrowser:
            switch (atoi(UserInputBuffer))
            {
                case 1:
                    composeDataPack(LECTURE_LIST_REQUEST);
                    break;
                case 2:
                    setArgumentGuide("입장할 강의 이름을 입력하세요", NULL, NULL, NULL, NULL);
                    CurrentRequest = LECTURE_ENTER_REQUEST;
                    ReceiveArgument = true;
                    TargetArgument = 1;
                    break;
                case 3:
                    if (CurrentUserInfo.role == USER_STUDENT)
                    {
                        setArgumentGuide("가입할 강의 이름을 입력하세요", NULL, NULL, NULL, NULL);
                        CurrentRequest = LECTURE_REGISTER_REQUEST;
                        ReceiveArgument = true;
                        TargetArgument = 1;
                    }
                    else
                    {
                        setArgumentGuide("개설할 강의 이름을 입력하세요", NULL, NULL, NULL, NULL);
                        CurrentRequest = LECTURE_CREATE_REQUEST;
                        ReceiveArgument = true;
                        TargetArgument = 1;
                    }
                    break;
                case 4:
                    if (CurrentUserInfo.role == USER_STUDENT)
                    {
                        setArgumentGuide("탈퇴할 강의 이름을 입력하세요", NULL, NULL, NULL, NULL);
                        CurrentRequest = LECTURE_DEREGISTER_REQUEST;
                        ReceiveArgument = true;
                        TargetArgument = 1;
                    }
                    else
                    {
                        setArgumentGuide("삭제할 강의 이름을 입력하세요", NULL, NULL, NULL, NULL);
                        CurrentRequest = LECTURE_REMOVE_REQUEST;
                        ReceiveArgument = true;
                        TargetArgument = 1;
                    }
                    break;
                case 9:
                    composeDataPack(USER_LOGOUT_REQUEST);
                    break;
                case 0:
                    //exitProgram();
                    break;
                default:
                    printMessage(MessageWindow, "잘못된 명령어 입력: '%s'\n", UserInputBuffer);
                    break;
            }
            break;
        
        case Lobby:
            switch (atoi(UserInputBuffer))
            {
                case 1:
                    if (CurrentUserInfo.role == USER_STUDENT)
                    {
                        // composeDataPack(LECTURE_NOTICE_REQUEST);
                    }
                    else
                    {
                        setArgumentGuide("등록할 공지사항을 입력하세요", NULL, NULL, NULL, NULL);
                        // CurrentRequest = LECTURE_NOTICE_SET_REQUEST;
                        ReceiveArgument = true;
                        TargetArgument = 1;
                    }
                    break;
                case 2:
                    if (CurrentUserInfo.role == USER_STUDENT)
                        composeDataPack(ATTENDANCE_CHECK_REQUEST);
                    else
                        changeClientStatus(Attendance);
                    break;
                case 3:
                    // quizRequest(QUIZ_REQUEST);
                    break;
                case 4:
                    composeDataPack(CHAT_ENTER_REQUEST);
                    break;
                case 8:
                    composeDataPack(LECTURE_LEAVE_REQUEST);
                    break;
                case 9:
                    composeDataPack(USER_LOGOUT_REQUEST);
                    break;
                case 0:
                    //exitPrograme();
                    break;
                default:
                    printMessage(MessageWindow, "잘못된 명령어 입력: '%s'\n", UserInputBuffer);
                    break;
            }
            break;

        case Chat:
            if (UserInputBuffer[0] == '!')
            {
                if (strcmp(UserInputBuffer, "!quit") == 0)
                {
                    composeDataPack(CHAT_LEAVE_REQUEST);
                }
                else if (strcmp(UserInputBuffer, "!user") == 0)
                {
                    composeDataPack(CHAT_USER_LIST_REQUEST);
                }
                else
                {
                    printMessage(MessageWindow, "잘못된 명령어 입력: '%s'\n", UserInputBuffer);
                }
            }
            else
            {
                composeDataPack(CHAT_SEND_MESSAGE_REQUEST);
            }
            break;

        case Attendance:
            switch (atoi(UserInputBuffer))
            {
                case 1:
                    setArgumentGuide("출석 체크를 진행할 시간(분)", "퀴즈 내용을 입력하세요", "퀴즈 정답을 입력하세요", NULL, NULL);
                    CurrentRequest = ATTENDANCE_START_REQUEST;
                    ReceiveArgument = true;
                    TargetArgument = 3;
                    break;
                case 2:
                    composeDataPack(ATTNEDANCE_STOP_REQUEST);
                    break;
                case 3:
                    setArgumentGuide("출석 체크를 연장할 시간을 입력하세요", NULL, NULL, NULL, NULL);
                    CurrentRequest = ATTNEDANCE_EXTEND_REQUEST;
                    ReceiveArgument = true;
                    TargetArgument = 1;
                    break;
                case 4:
                    composeDataPack(ATTENDANCE_RESULT_REQUEST);
                    break;
                case 8:
                    changeClientStatus(Lobby);
                    break;
                case 9:
                    composeDataPack(USER_LOGOUT_REQUEST);
                    break;
                case 0:
                    //exitPrograme();
                    break;
                default:
                    printMessage(MessageWindow, "잘못된 명령어 입력: '%s'\n", UserInputBuffer);
                    break;
            }

        default:
            printMessage(MessageWindow, "Invaild client status\n");
            break;
    }
}

void updateLayoutByStatus()
{
    switch (CurrentClientStatus)
    {
        case Login:
            // drawLoginLayout();
            drawDefaultLayout();
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
        case Attendance:
            drawLectureLayout();
            break;
        default:
            drawDefaultLayout();
            break;
    }
}

// CommandWindow의 명령어 목록을 ClientStatus에 따라 업데이트
void updateCommandByStatus()
{
    switch (CurrentUserInfo.role)
    {
        case USER_ADMIN:
            break;
        
        case USER_STUDENT:
            switch (CurrentClientStatus)
            {
                case Login:
                    break;

                case LectureBrowser:
                    updateCommand("1: 강의 목록 보기\n2: 강의 입장\n3. 강의 가입\n4. 강의 탈퇴\n\n\n\n\n9: 로그아웃\n0: 프로그램 종료");
                    break;
                        
                case Lobby:
                    updateCommand("1. 공지사항 보기\n2. 출석 체크\n3. 미니 퀴즈\n4. 강의 대화 입장\n\n\n\n8. 강의실 나가기\n9: 로그아웃\n0: 프로그램 종료");
                    break;
                                    
                case Chat:
                    updateCommand("!quit: 대화 나가기\n!user: 현재 사용자");
                    break;

                case Attendance:
                    break;

                default:
                    break;
            }
            break;
        
        case USER_PROFESSOR:
            switch (CurrentClientStatus)
            {
                case Login:
                    break;

                case LectureBrowser:
                    updateCommand("1: 강의 목록 보기\n2: 강의 입장\n3. 강의 개설\n4.강의 삭제\n\n\n\n\n9: 로그아웃\n0: 프로그램 종료");
                    break;
                        
                case Lobby:
                    updateCommand("1. 공지사항 등록\n2. 출석 체크\n3. 미니 퀴즈\n4. 강의 대화 입장\n\n\n\n8. 강의실 나가기\n9: 로그아웃\n0: 프로그램 종료");
                    break;
                                    
                case Chat:
                    updateCommand("!quit: 대화 나가기\n!user: 현재 사용자");
                    break;
                
                case Attendance:
                    updateCommand("1. 출석 체크 시작\n2. 출석 체크 종료\n3. 시간 연장\n4. 결과 보기\n\n\n\n8. 뒤로 가기\n9: 로그아웃\n0: 프로그램 종료");
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
} 

void updateInputByStatus()
{
    if (ReceiveArgument)
    {
        updateInput(ArgumentGuide[CurrentArgument], UserInputBuffer);
        return;
    }

    switch (CurrentClientStatus)
    {
        case Login:
            strncpy(UserInputGuide, "Input", sizeof(UserInputGuide));
            break;

        case LectureBrowser:
            strncpy(UserInputGuide, "명령어를 입력하세요", sizeof(UserInputGuide));
            break;

        case Lobby:
            strncpy(UserInputGuide, "명령어를 입력하세요", sizeof(UserInputGuide));
            break;

        case Chat:
            strncpy(UserInputGuide, "메시지를 입력하세요", sizeof(UserInputGuide));
            break;

        case Attendance:
            strncpy(UserInputGuide, "명령어를 입력하세요", sizeof(UserInputGuide));
            break;
    
        default:
            strncpy(UserInputGuide, "Input", sizeof(UserInputGuide));
            break;
    }

    updateInput(UserInputGuide, UserInputBuffer);
}

void changeClientStatus(enum ClientStatus status)
{
    CurrentClientStatus = status;

    updateLayoutByStatus();
    updateCommandByStatus();
    updateInputByStatus();

    switch (CurrentClientStatus)
    {
        case Login:
            break;

        case LectureBrowser:
            break;

        case Lobby:
            updateStatus();
            updateNotice("엄준식 엄톰식 엄흉길");
            updateEvent(true, false);
            break;

        case Chat:
            updateStatus();
            updateNotice("도파 압도 도파고");
            updateEvent(false, true);
            break;

        case Attendance:
            break;

        default:
            break;
    }
}

bool receiveArgumentComplete()
{
    return (ReceiveArgument && CurrentArgument == TargetArgument) ? true : false;
}

void resetArgument()
{
    const int ARGS_NUM = 5;
    for (int index = 0; index < ARGS_NUM; index++)
    {
        memset(Arguments[index], 0, sizeof(Arguments[index]));
        memset(ArgumentGuide[index], 0, sizeof(ArgumentGuide[index]));
    }
    
    CurrentRequest = NONE;
    CurrentArgument = 0;
    TargetArgument = 0;
    ReceiveArgument = false;
}

void setArgumentGuide(char *guide1, char *guide2, char *guide3, char *guide4, char *guide5)
{
    if (guide1 != NULL)
        strncpy(ArgumentGuide[0], guide1, sizeof(ArgumentGuide[0]));
    if (guide2 != NULL)
        strncpy(ArgumentGuide[1], guide2, sizeof(ArgumentGuide[1]));
    if (guide3 != NULL)
        strncpy(ArgumentGuide[2], guide3, sizeof(ArgumentGuide[2]));
    if (guide4 != NULL)
        strncpy(ArgumentGuide[3], guide4, sizeof(ArgumentGuide[3]));
    if (guide5 != NULL)
        strncpy(ArgumentGuide[4], guide5, sizeof(ArgumentGuide[4]));
}