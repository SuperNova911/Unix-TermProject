#include "database.h"

MYSQL *Connection; 

void printError()
{
	printf("[ERROR] code: '%d', message: '%s'\n", mysql_errno(Connection), mysql_error(Connection));
}

bool executeQuery(char *query)
{
	if (mysql_query(Connection, query) != 0)
	{
		printError();
		return false;
	}

	return true;
}

bool initializeDatabase()
{
	Connection = mysql_init(NULL);
	if (Connection == NULL)
	{
		printf("Insufficient memory\n");
		return false;
	}

	return true;
}

void createTable()
{
	executeQuery("CREATE TABLE IF NOT EXISTS `User` (`studentID` VARCHAR(16) NOT NULL, `hashedPassword` VARCHAR(64) NOT NULL, `userName` VARCHAR(16) NOT NULL, `role` INT NOT NULL, `registerDate` BIGINT NOT NULL, PRIMARY KEY (`studentID`))");
	executeQuery("CREATE TABLE IF NOT EXISTS `Lecture` (`lectureID` INT NOT NULL, `lectureName` VARCHAR(64) NOT NULL, `professorID` VARCHAR(16) NOT NULL, `memberCount` INT NOT NULL, `createDate` BIGINT NOT NULL, PRIMARY KEY (`lectureID`))");
	executeQuery("CREATE TABLE IF NOT EXISTS `LectureMember` (`lectureID` INT NOT NULL, `studentID` VARCHAR(16) NOT NULL)");
	executeQuery("CREATE TABLE IF NOT EXISTS `AttendanceCheckLog` (`lectureID` INT NOT NULL, `studentID` VARCHAR(16) NOT NULL, `IP` VARCHAR(16) NOT NULL, `quizAnswer` VARCHAR(512) NOT NULL, `checkDate` BIGINT NOT NULL)");
	executeQuery("CREATE TABLE IF NOT EXISTS `ChatLog` (`lectureID` INT NOT NULL, `userName` VARCHAR(16) NOT NULL, `message` VARCHAR(512) NOT NULL, `date` BIGINT NOT NULL)");
}

bool connectToDatabase()
{
	const char *HOST = "right.jbnu.ac.kr";
	const char *USER = "A201210927";
	const char *PASSWORD = "q1234";
	const char *DATABASE = "A201210927";
	const unsigned int PORT = 0;
	const char *SOCKET = NULL;
	const unsigned long CLIENT_FLAG = CLIENT_MULTI_STATEMENTS;

	if (mysql_real_connect(Connection, HOST, USER, PASSWORD, DATABASE, PORT, SOCKET, CLIENT_FLAG) == NULL)
	{
		printError();
		return false;
	}

	return true;
}

bool closeDatabase()
{
	if (Connection == NULL)
	{
		fprintf(stderr, "closeDatabase: Already closed\n");
		return false;
	}

	mysql_close(Connection);
	return true;
}

User loadUserByID(char *studentID, bool *dbResult)
{
	char query[512];
	sprintf(query, "SELECT * FROM `User` WHERE studentID = '%s'", studentID);
	executeQuery(query);

	User user;
	resetUser(&user);

	MYSQL_RES *result;
	result = mysql_store_result(Connection);
	if (result == NULL)
	{
		printError();
		*dbResult = false;
		return user;
	}

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)) != NULL)
	{
		buildUser(&user, row[0], row[1], row[2], atoi(row[3]), atol(row[4]));
		mysql_free_result(result);

		*dbResult = true;
		return user;
	}

	*dbResult = false;
	return user;
}

bool saveUser(User *user)
{
	char query[512];
	sprintf(query, "REPLACE INTO `User` (`studentID`, `hashedPassword`, `userName`, `role`, `registerDate`) VALUES ('%s', '%s', '%s', '%d', '%ld')",
		user->studentID, user->hashedPassword, user->userName, user->role, user->registerDate);
	
	return executeQuery(query);
}

bool removeUser(char *studentID)
{
	char query[512];
	sprintf(query, "DELETE FROM `User` WHERE studentID = '%s'", studentID);

	return executeQuery(query);
}

bool clearUser()
{
	return executeQuery("DELETE FROM `User`");
}

bool isUserMatch(char *studentID, char *hashedPassword)
{
	char query[512];
	sprintf(query, "SELECT * FROM `User` WHERE studentID = '%s' AND hashedPassword = '%s'", studentID, hashedPassword);
	executeQuery(query);

	MYSQL_RES *result;
	result = mysql_store_result(Connection);
	if (result == NULL)
	{
		printError();
		return false;
	}

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)) != NULL)
	{
		return true;
	}

	return false;
}

Lecture loadLectureByName(char *lectureName, bool *dbResult)
{
	char query[512];
	sprintf(query, "SELECT * FROM `Lecture` WHERE lectureName = '%s'", lectureName);
	executeQuery(query);

	Lecture lecture;
	resetLecture(&lecture);

	MYSQL_RES *result;
	result = mysql_store_result(Connection);
	if (result == NULL)
	{
		printError();
		*dbResult = false;
		return lecture;
	}

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)) != NULL)
	{
		buildLecture(&lecture, atoi(row[0]), row[1], row[2], atol(row[4]));
		loadLectureMemberList(&lecture);

		mysql_free_result(result);

		*dbResult = true;
		return lecture;
	}

	*dbResult = false;
	return lecture;
}

int loadLectureList(Lecture *lectureList, int amount)
{
	char query[512];
	sprintf(query, "SELECT * FROM `Lecture` LIMIT %d", amount);
	executeQuery(query);

	int index = 0;

	MYSQL_RES *result;
	result = mysql_store_result(Connection);
	if (result == NULL)
	{
		printError();
		return index;
	}

	MYSQL_ROW row;
	while (((row = mysql_fetch_row(result)) != NULL) && index < amount)
	{
		resetLecture(&lectureList[index]);
		buildLecture(&lectureList[index], atoi(row[0]), row[1], row[2], atol(row[4]));
		loadLectureMemberList(&lectureList[index]);
		index++;
	}

	mysql_free_result(result);
	return index;
}

bool saveLecture(Lecture *lecture)
{
	char query[512];
	sprintf(query, "REPLACE INTO `Lecture` (`lectureID`, `lectureName`, `professorID`, `memberCount`, `createDate`) VALUES ('%d', '%s', '%s', '%d', '%ld')",
		lecture->lectureID, lecture->lectureName, lecture->professorID, lecture->memberCount, lecture->createDate);
	
	saveLectureMemberList(lecture);
	
	return executeQuery(query);
}

bool removeLectureByID(int lectureID)
{
	char query1[512], query2[512];
	sprintf(query1, "DELETE FROM `Lecture` WHERE lectureID = '%d'", lectureID);
	sprintf(query2, "DELETE FROM `LectureMember` WHERE lectureID = '%d'", lectureID);

	return executeQuery(query1) && executeQuery(query2);
}

bool loadLectureMemberList(Lecture *lecture)
{
	char query[512];
	sprintf(query, "SELECT * FROM `LectureMember` WHERE lectureID = '%d'", lecture->lectureID);
	executeQuery(query);

	MYSQL_RES *result;
	result = mysql_store_result(Connection);
	if (result == NULL)
	{
		printError();
		return false;
	}

	MYSQL_ROW row;
	for (int index = 0; ((row = mysql_fetch_row(result)) != NULL) && lecture->memberCount < MAX_LECTURE_MEMBER; index++)
	{
		strncpy(lecture->memberList[index], row[1], sizeof(lecture->memberList[index]));
		lecture->memberCount++;
	}

	return true;
}

void saveLectureMemberList(Lecture *lecture)
{
	char query[512];
	for (int index = 0; index < lecture->memberCount; index++)
	{
		sprintf(query, "REPLACE INTO `LectureMember` (`lectureID`, `studentID`) VALUES ('%d', '%s')",
			lecture->lectureID, lecture->memberList[index]);
		executeQuery(query);
	}
}

bool clearLecture()
{
	return executeQuery("DELETE FROM `Lecture`");
}

bool clearLectureMember()
{
	return executeQuery("DELETE FROM `LectureMember`");
}

int loadAttendanceCheckLogList(AttendanceCheckLog *checkLogList, int amount, int lectureID, time_t date)
{
	const int SEARCH_RANGE = 12 * 60 * 60;		// 12시간

	char query[512];
	sprintf(query, "SELECT * FROM `AttendanceCheckLog` WHERE lectureID = '%d' AND checkDate BETWEEN '%ld' AND '%ld' LIMIT %d", 
		lectureID, date - SEARCH_RANGE, date + SEARCH_RANGE, amount);

	executeQuery(query);

	int index = 0;

	MYSQL_RES *result;
	result = mysql_store_result(Connection);
	if (result == NULL)
	{
		printError();
		return index;
	}

	MYSQL_ROW row;
	while (((row = mysql_fetch_row(result)) != NULL) && index < amount)
	{
		checkLogList[index].lectureID = atoi(row[0]);
		strncpy(checkLogList[index].studentID, row[1], sizeof(checkLogList[index].studentID));
		strncpy(checkLogList[index].IP, row[2], sizeof(checkLogList[index].IP));
		strncpy(checkLogList[index].quizAnswer, row[3], sizeof(checkLogList[index].quizAnswer));
		checkLogList[index].checkDate = atol(row[4]);

		index++;
	}

	mysql_free_result(result);
	return index;
}

bool saveAttendanceCheckLog(AttendanceCheckLog *checkLog)
{
	char query[1024];
	sprintf(query, "INSERT INTO `AttendanceCheckLog` (`lectureID`, `studentID`, `IP`, `quizAnswer`, `checkDate`) VALUES ('%d', '%s', '%s', '%s', '%ld')",
		checkLog->lectureID, checkLog->studentID, checkLog->IP, checkLog->quizAnswer, checkLog->checkDate);
	
	return executeQuery(query);
}

bool removeAttendanceCheckLog(int lectureID, time_t date)
{
	const int SEARCH_RANGE = 12 * 60 * 60;		// 12시간

	char query[512];
	sprintf(query, "DELETE FROM `AttendanceCheckLog` WHERE lectureID = '%d' AND checkDate BETWEEN '%ld' AND '%ld'", 
		lectureID, date - SEARCH_RANGE, date + SEARCH_RANGE);

	return executeQuery(query);
}

bool clearAttendanceCheckLog()
{
	return executeQuery("DELETE FROM `AttendanceCheckLog`");
}

int loadChatLogList(ChatLog *chatLogList, int amount, int lectureID)
{
	char query[512];
	sprintf(query, "SELECT * FROM `ChatLog` WHERE lectureID = '%d' LIMIT %d", lectureID, amount);
	executeQuery(query);

	int index = 0;

	MYSQL_RES *result;
	result = mysql_store_result(Connection);
	if (result == NULL)
	{
		printError();
		return index;
	}

	MYSQL_ROW row;
	while (((row = mysql_fetch_row(result)) != NULL) && index < amount)
	{
		chatLogList[index].lectureID = atoi(row[0]);
		strncpy(chatLogList[index].userName, row[1], sizeof(chatLogList[index].userName));
		strncpy(chatLogList[index].message, row[2], sizeof(chatLogList[index].message));
		chatLogList[index].date = atol(row[3]);

		index++;
	}

	mysql_free_result(result);
	return index;
}

bool saveChatLog(ChatLog *chatLog)
{
	char query[1024];
	sprintf(query, "INSERT INTO `ChatLog` (`lectureID`, `userName`, `message`, `date`) VALUES ('%d', '%s', '%s', '%ld')",
		chatLog->lectureID, chatLog->userName, chatLog->message, chatLog->date);
	
	return executeQuery(query);
}

bool removeChatLog(int lectureID)
{
	char query[512];
	sprintf(query, "DELETE FROM `ChatLog` WHERE lectureID = '%d'", lectureID);

	return executeQuery(query);
}

bool clearChatLog()
{
	return executeQuery("DELETE FROM `ChatLog`");
}