#pragma once

#include "user.h"
#include <time.h>

#define MAX_LECTURE_MEMBER 40
#define MAX_LECTURE_NAME_LENGTH 64

typedef struct Lecture_t
{
    int lectureID;
    char lectureName[MAX_LECTURE_NAME_LENGTH];
    char professorID[16];
    int memberCount;
    char memberList[MAX_LECTURE_MEMBER][16];
    time_t createDate;
} Lecture;

typedef struct LectureInfo_t
{
    Lecture lecture;
    int onlineUserCount;
    OnlineUser *onlineUser[MAX_LECTURE_MEMBER];
    bool isProfessorOnline;
    bool isAttendanceActive;
    time_t attendanceEndtime;
    bool isQuizActive;
    char quiz[512];
    char quizAnswer[128];
} LectureInfo;

Lecture createLecture(int lectureID, char *lectureName, char *professorID);
void buildLecture(Lecture *lecture, int lectureID, char *lectureName, char *professorID, time_t createDate);
void buildLectureInfo(LectureInfo *lectureInfo, Lecture *lecture);

void resetLecture(Lecture *lecture);
void resetLectureInfo(LectureInfo *lectureInfo);

bool lectureAddMember(Lecture *lecture, char *studentID);
bool lectureRemoveMember(Lecture *lecture, char *studentID);
bool isLectureMember(Lecture *lecture, char *studentID);

bool lectureAttendanceStart(LectureInfo *lectureInfo, int duration);
bool lectureAttendanceStop(LectureInfo *lectureInfo);
bool lectureAttendanceExtend(LectureInfo *lectureInfo, int duration);