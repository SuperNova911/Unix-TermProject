#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DB_PATH "데이터베이스 경로"
#define LECTURE_MAX_MEMBER 60

// 사용자 권한
typedef enum
{
    DB_None = 0, DB_Admin = 1, DB_Student = 2, DB_Professor = 3
} Role;                                 

// 사용자 정보 구조체
typedef struct User_t
{
    char studentID[16];
    char hashedPassword[64];
    char userName[16];
    Role role; 				    
    time_t registerDate;		
} User;

// 강의 정보 구조체
typedef struct Lecture_t
{
    int lectureID;
    int memberCount;
    char professorID[16];
    char lectureName[128];
    char memberList[LECTURE_MAX_MEMBER][16];
    time_t createDate;
} Lecture;

// 출석체크 기록 구조체
typedef struct AttendanceCheckLog_t
{
    int lectureID;
    char studentID[16];
    char IP[16];
    char quizAnswer[512];
    time_t checkDate;
} AttendanceCheckLog;

// 채팅 기록 구조체
typedef struct ChatLog_t
{
    int lectureID;
    char userName[16];
    char message[512];
    time_t date;
} ChatLog;

bool createNewDatabase();       // 새로운 데이터베이스 생성
//bool connectToDatabase();       // 데이터베이스에 연결
bool closeDatabase();           // 데이터베이스 닫기

bool loadUser(User *userList, int amount, int lectureID);   // DB에서 lectureID가 일치하는 사용자 구조체 배열 반환
bool loadUserByID(User *user, char *studentID);             // DB에서 studentID가 일치하는 사용자 구조체 반환
bool registerUser(User *user);                              // DB에 새로운 사용자 정보 저장
bool removeUser(char *studentID);                           // DB에서 studentID가 일치하는 사용자 삭제
bool loginUser(char *studentID, char *hashedPassword);      // DB에서 studentID와 hashedPassword가 일치하는 사용자가 있는지 확인
bool clearUser();                                           // User 테이블 초기화

bool loadLecture(Lecture *lectureList, int amount);             // DB에서 강의 구조체 배열 반환
bool loadLectureByID(Lecture *lecture, int lectureID);          // DB에서 lectureID가 일치하는 강의 정보 구조체 반환
bool createLecture(Lecture *lecture);                           // DB에 새로운 강의 정보 저장
bool removeLecture(int lectureID);                              // DB에서 lectureID가 일치하는 강의 삭제
bool lecture_registerUser(int lectureID, char *studentID);      // DB에서 lectureID가 일치하는 강의의 memberList에 studentID추가
bool lecture_deregisterUser(int lectureID, char *studentID);    // DB에서 lectureID가 일치하는 강의의 memberList에 studentID삭제
bool clearLecture();                                            // Lecture 테이블 초기화

bool loadAttendanceCheckLog(AttendanceCheckLog *checkLogList, int amount, int lectureID);   // DB에서 lectureID가 일치하는 출석체크 기록 구조체 배열 반환
bool saveAttendanceCheckLog(AttendanceCheckLog *checkLog);       // DB에 출석체크 기록 저장
bool clearAttendanceCheckLog();                                 // AttendanceCheckLog 테이블 초기화

bool loadChatLog(ChatLog *chatLog, int amount, int lectureID);       // DB에서 lectureID가 일치하는 채팅 기록 구조체 배열 반환
bool saveChatLog(ChatLog *chatLog);                                       // DB에 채팅 기록 저장
bool clearChatLog();


