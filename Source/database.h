#pragma once

#include "user.h"
#include "lecture.h"

#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mysql.h>

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

bool initializeDatabase();
void createTable();
bool connectToDatabase();
bool closeDatabase();

User loadUserByID(char *studentID, bool *dbResult);
bool saveUser(User *user);
bool removeUser(char *studentID);
bool clearUser();
bool isUserMatch(char *studentID, char *hashedPassword);

Lecture loadLectureByName(char *lectureName, bool *dbResult);
int loadLectureList(Lecture *lectureList, int amount);
bool saveLecture(Lecture *lecture);
bool removeLectureByID(int lectureID);
bool loadLectureMemberList(Lecture *lecture);
void saveLectureMemberList(Lecture *lecture);
void removeLectureMemberList(Lecture *lecture);
int loadLectureNotice(char noticeList[][512], int amount, int lectureID);
bool saveLectureNotice(int lectureID, char *message);
bool clearLecture();
bool clearLectureMember();
bool clearLectureNotice();

int loadAttendanceCheckLogList(AttendanceCheckLog *checkLogList, int amount, int lectureID, time_t date);
bool saveAttendanceCheckLog(AttendanceCheckLog *checkLog);
bool removeAttendanceCheckLog(int lectureID, time_t date);
bool clearAttendanceCheckLog();

int loadChatLogList(ChatLog *chatLogList, int amount, int lectureID);
bool saveChatLog(ChatLog *chatLog);
bool removeChatLog(int lectureID);
bool clearChatLog();