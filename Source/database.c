#include "database.h"

bool createNewDatabase() { return true; }
bool connectToDatabase() { return true; }
bool closeDatabase() { return true; }

bool loadUser(User *userList, int amount, int lectureID)
{
	for (int index = 0; index < amount; index++)
	{
		memset(&userList[index], 0, sizeof(User));
		sprintf(userList[index].studentID, "%s%05d", "2015", index);
		strncpy(userList[index].hashedPassword, "1234567890", sizeof(userList[index].hashedPassword));
		sprintf(userList[index].userName, "%04d", index);
		userList[index].role = (index % 3) + 1;
		time(&userList[index].registerDate);
	}
	return true;
}
bool loadUserByID(User *user, char *studentID)
{
	memset(user, 0, sizeof(User));
	strncpy(user->studentID, studentID, sizeof(user->studentID));
	strncpy(user->hashedPassword, "thisispassword", sizeof(user->hashedPassword));
	strncpy(user->userName, "김수환", sizeof(user->userName));
	user->role = DB_Student;
	time(&user->registerDate);
	
	return true;
}
bool registerUser(User *user) { return true; }
bool removeUser(char *studentID) { return true; }
bool loginUser(char *studentID, char *hashedPassword) { return true; }
bool clearUser() { return true; }

bool loadLecture(Lecture *lectureList, int amount)
{
	for (int index = 0; index < amount; index++)
	{
		memset(&lectureList[index], 0, sizeof(Lecture));
		lectureList[index].lectureID = index + 1;
		lectureList[index].memberCount = index;
		strncpy(lectureList[index].professorID, "201510743", sizeof(lectureList[index].professorID));
		strncpy(lectureList[index].lectureName, "201510743", sizeof(lectureList[index].lectureName));
		for (int memberIndex = 0; memberIndex < index; memberIndex++)
		{
			strncpy(lectureList[index].memberList[memberIndex], "201510743", sizeof(lectureList[index].memberList[memberIndex]));
		}
		time(&lectureList[index].createDate);
	}

	return true;
}
bool loadLectureByID(Lecture *lecture, int lectureID)
{
	memset(lecture, 0, sizeof(Lecture));
	lecture->lectureID = 10;
	lecture->memberCount = 2;
	strncpy(lecture->professorID, "201510743", sizeof(lecture->professorID));
	strncpy(lecture->lectureName, "201510743", sizeof(lecture->lectureName));
	for (int index = 0; index < lecture->memberCount; index++)
		strncpy(lecture->memberList[index], "201510743", sizeof(lecture->memberList[index]));
	time(&lecture->createDate);
	
	return true;
}
bool createLecture(Lecture *lecture) { return true; }
bool removeLecture(int lectureID) { return true; }
bool lecture_registerUser(int lectureID, char *studentID) { return true; }
bool lecture_deregisterUser(int lectureID, char *studentID) { return true; }
bool clearLecture() { return true; }

bool loadAttendanceCheckLog(AttendanceCheckLog *checkLogList, int amount, int lectureID)
{
	for (int index = 0; index < amount; index++)
	{
		memset(&checkLogList[index], 0, sizeof(AttendanceCheckLog));
	}
	return true;
}
bool saveAttendanceCheckLog(AttendanceCheckLog *checkLog) { return true; }
bool clearAttendanceCheckLog() { return true; }

bool loadChatLog(ChatLog *chatLog, int amount, int lectureID) { return true; }
bool saveChatLog(ChatLog *chatLog) { return true; }
bool clearChatLog() { return true; }

