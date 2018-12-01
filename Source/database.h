#include <time.h>
#include <stdbool.h>
#include <mysql.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LECTURE_MAX_MEMBER 60

// 사용자 권한
typedef enum
{
    None = 0, Admin, Student, Professor
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
    char lectureName[128];
    char professorID[16];
    int memberCount;
    char memberList[LECTURE_MAX_MEMBER][16];
    time_t createDate;
} Lecture;

// 출석체크 기록 구조체
typedef struct AttendanceCheckLog_t
{
    int lectureID;
    int studentID[16];
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

bool initializeDatabase();      // 새로운 데이터베이스 생성+++
bool connectToDatabase();       // 데이터베이스에 연결+++
bool closeDatabase();           // 데이터베이스 닫기+++

User *loadUser(User user[], int amount, int lectureID);     // DB에서 lectureID가 일치하는 사용자 구조체 배열 반환
User loadUserByID(char *studentID);                         // DB에서 studentID가 일치하는 사용자 구조체 반환
bool registerUser(User *user);                              // DB에 새로운 사용자 정보 저장+++
bool removeUser(char *studentID);                           // DB에서 studentID가 일치하는 사용자 삭제+++
bool isLoginUser(char *studentID, char *hashedPassword);    // DB에서 studentID와 hashedPassword가 일치하는 사용자가 있는지 확인+++
void clearUser();                                           // User 테이블 초기화+++

Lecture *loadLecture(Lecture lecture[], int amount);            // DB에서 강의 구조체 배열 반환
Lecture loadLectureByID(int lectureID);                         // DB에서 lectureID가 일치하는 강의 정보 구조체 반환
bool createLecture(Lecture *lecture);                           // DB에 새로운 강의 정보 저장
bool removeLecture(int lectureID);                              // DB에서 lectureID가 일치하는 강의 삭제
bool lecture_registerUser(int lectureID, char *studentID);      // DB에서 lectureID가 일치하는 강의의 memberList에 studentID추가
bool lecture_deregisterUser(int lectureID, char *studentID);    // DB에서 lectureID가 일치하는 강의의 memberList에 studentID삭제
void clearLecture();                                            // Lecture 테이블 초기화+++

AttendanceCheckLog *loadAttendanceCheckLog(AttendanceCheckLog checkLog[], int amount, int lectureID);   // DB에서 lectureID가 일치하는 출석체크 기록 구조체 배열 반환
bool saveAttendanceCheckLog(AttendanceCheckLog checkLog);                                               // DB에 출석체크 기록 저장
void clearAttendanceCheckLog();                                                                         // AttendanceCheckLog 테이블 초기화+++

ChatLog *loadChatLog(ChatLog chatLog[], int amount, int lectureID);       // DB에서 lectureID가 일치하는 채팅 기록 구조체 배열 반환
bool saveChatLog(ChatLog *chatLog);                                       // DB에 채팅 기록 저장
void clearChatLog();                                                      // CahtLog 테이블 초기화+++

// 11.28 추가
// h파일이랑 c파일 함수 선언 순서좀 맞춰주세요
void handlingError();                                     //에러 발생시 실행되는 함수+++
void createTable();                                                      //Database에 4개의 테이블을 만드는 함수+++
bool executeQuery(char *query); //쿼리문을 실행시키고 동시에 에러검사 까지하는 함수+++

// by Dopa
void saveLectureMemberList(Lecture *lecture);
bool loadLectureMemberList(Lecture *lecture);
void timeToString(char *result, int size, time_t *time);
void stringToTime(time_t *result, char *timeString);