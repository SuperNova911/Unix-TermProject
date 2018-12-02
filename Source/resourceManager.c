#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "dataPack.h"
#include "database.h"

typedef struct LectureInfo_t
{
    int onlineUserCount;
    Lecture lecture;
    OnlineUser onlineUser[MAX_LECTURE_MEMBER];
} LectureInfo;

typedef struct OnlineUser_t
{
    int socket;
    int currentLectureID;
    User user;
} OnlineUser;

int UserCount;
int LectureCount;
OnlineUser OnlineUserList[MAX_CLIENT];
LectureInfo LectureInfoList[MAX_LECTURE];

bool addOnlineUser(User *user, int socket)
{
    if ((UserCount + 1) < MAX_CLIENT)
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
        if (isSameLectureName(LectureInfoList[index], lectureName))
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
    LectureInfo *lectureInfo;
    if (findLectureInfoByName(lectureInfo, lectureName) == false)
        return false;

    if ((lectureInfo->onlineUserCount + 1) == MAX_LECTURE_MEMBER)
        return false;

    OnlineUser *onlineUser;
    if (findOnlineUserByID(onlineUser, studentID) == false)
        return false;

    memcpy(&lectureInfo->onlineUser[lectureInfo->onlineUserCount], onlineUser, sizeof(OnlineUser));
    lectureInfo->onlineUserCount++;
    return true;
}

bool userLeaveLecture(char *studentID, char *lectureName)
{
    LectureInfo *lectureInfo;
    if (findLectureInfoByName(lectureInfo, lectureName) == false)
        return false;

    for (int index = 0; index < lectureInfo->onlineUserCount; index++)
    {
        if (isSameUserID(lectureInfo->onlineUser[index].user, studentID))
        {
            memcpy(lectureInfo->onlineUser[index], lectureInfo->onlineUser[lectureInfo->onlineUserCount], sizeof(OnlineUser));
            memset(lectureInfo->onlineUser[lectureInfo->onlineUserCount], 0, sizeof(OnlineUser));
            lectureInfo->onlineUserCount--;
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

bool findOnlineUserByID(OnlineUser *foundOnlineUser, char *studentID)
{
    for (int index = 0; index < UserCount; index++)
    {
        if (isSameUserID(&OnlineUserList[index].user, studentID))
        {
            foundOnlineUser = &OnlineUserList[index];
            return true;
        }
    }

    return false;
}

bool findLectureInfoByName(LectureInfo *foundLectureInfo, char *lectureName)
{
    for (int index = 0; index < LectureCount; index++)
    {
        if (isSameLectureName(LectureInfoList[index], lectureName))
        {
            foundLectureInfo = &LectureInfoList[index];
            return true;
        }
    }

    return false;
}

bool findLectureInfoByID(LectureInfo *foundLectureInfo, int lectureID)
{
    for (int index = 0; index < LectureCount; index++)
    {
        if (LectureInfoList[index].lecture.lectureID == lectureID)
        {
            foundLectureInfo = &LectureInfoList[index];
            return true;
        }
    }

    return false;
}

