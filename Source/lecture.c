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
            strncpy(lecture->memberList[index], lecture->memberList[lecture->memberCount], sizeof(lecture->memberList[index]));
            memset(lecture->memberList[lecture->memberCount], 0, sizeof(lecture->memberList[lecture->memberCount]));
            lecture->memberCount--;
            return true;
        }
    }

    return false;
}