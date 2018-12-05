#include "lecture.h"
#include "string.h"

Lecture createLecture(int lectureID, char *lectureName, char *professorID)
{
    Lecture lecture;
    resetLecture(&lecture);

    lecture.lectureID = lectureID;
    strncpy(lecture.lectureName, lectureName, sizeof(lecture.lectureName));
    strncpy(lecture.professorID, professorID, sizeof(lecture.professorID));
    time(&lecture.createDate);

    return lecture;
}

void buildLecture(Lecture *lecture, int lectureID, char *lectureName, char *professorID, time_t createDate)
{
    lecture->lectureID = lectureID;
    strncpy(lecture->lectureName, lectureName, sizeof(lecture->lectureName));
    strncpy(lecture->professorID, professorID, sizeof(lecture->professorID));
    lecture->createDate = createDate;
}

void buildLectureInfo(LectureInfo *lectureInfo, Lecture *lecture)
{
    resetLectureInfo(lectureInfo);

    memcpy(&lectureInfo->lecture, lecture, sizeof(lectureInfo->lecture));
}

void resetLecture(Lecture *lecture)
{
    memset(lecture, 0, sizeof(Lecture));
}

void resetLectureInfo(LectureInfo *lectureInfo)
{
    memset(lectureInfo, 0, sizeof(LectureInfo));
}

bool lectureAddMember(Lecture *lecture, char *studentID)
{
    if (lecture->memberCount >= MAX_LECTURE_MEMBER)
        return false;
    
    strncpy(lecture->memberList[lecture->memberCount], studentID, sizeof(lecture->memberList[lecture->memberCount]));
    lecture->memberCount++;

    return true;
}

bool lectureRemoveMember(Lecture *lecture, char *studentID)
{
    for (int index = 0; index < lecture->memberCount; index++)
    {
        if (strcmp(lecture->memberList[index], studentID) == 0)
        {
            lecture->memberCount--;
            strncpy(lecture->memberList[index], lecture->memberList[lecture->memberCount], sizeof(lecture->memberList[index]));
            memset(lecture->memberList[lecture->memberCount], 0, sizeof(lecture->memberList[lecture->memberCount]));
            return true;
        }
    }

    return false;
}

bool isLectureMember(Lecture *lecture, char *studentID)
{
    for (int index = 0; index < lecture->memberCount; index++)
    {
        if (strcmp(lecture->memberList[index], studentID) == 0)
        {
            return true;
        }
    }

    if (strcmp(lecture->professorID, studentID) == 0)
    {
        return true;
    }

    return false;
}

bool lectureAttendanceStart(LectureInfo *lectureInfo, int duration)
{
    if (lectureInfo->isAttendanceActive)
        return false;

    time(&lectureInfo->attendanceEndtime);
    lectureInfo->attendanceEndtime += duration * 60;
    lectureInfo->isAttendanceActive = true;

    for (int index = 0; index < lectureInfo->onlineUserCount; index++)
    {
        lectureInfo->onlineUser[index]->hasAttendanceChecked = false;
    }

    return true;
}

bool lectureAttendanceStop(LectureInfo *lectureInfo)
{
    if (lectureInfo->isAttendanceActive == false)
        return false;

    time(&lectureInfo->attendanceEndtime);
    lectureInfo->isAttendanceActive = false;

    return true;
}

bool lectureAttendanceExtend(LectureInfo *lectureInfo, int duration)
{
    if (lectureInfo->isAttendanceActive == false)
        return false;

    lectureInfo->attendanceEndtime += duration * 60;

    return true;
}