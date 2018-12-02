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

void resetLecture(Lecture *lecture)
{
    memset(lecture, 0, sizeof(Lecture));
}