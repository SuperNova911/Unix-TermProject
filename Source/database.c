#include "database.h"

//-----------------------------------------장진성이 추가한 변수 및 함수내용----------------------------------------------------------//

bool isItUser;                          //유저들의 데이터베이스인지 확인하는 변수
bool isItLecture;                       //강의들의 데이터베이스인지 확인하는 변수.
bool isItAttendanceCheckLog;            //출석체크 기록 데이터베이스인지 확인하는 변수.
bool isItChatLog;                       //채팅 기록 데이터베이스인지 확인하는 변수.

sqlite3 *dbUser;                        //각 구조체에 대한 데이터베이스 변수 4개
sqlite3 *dbLecture;
sqlite3 *dbAttendanceCheckLog;
sqlite3 *dbChatLog;
char *err_msg = 0;                      //에러메시지 처리변수.

bool createNewDatabase()
{
	if(isItUser == true)
    {
        sqlite3 *dbUser;                            
	    int rc = sqlite3_open("User.db", &dbUser);

    	if(rc != SQLITE_OK)
    	{
	    	fprintf(stderr,"Cannot open database : %s\n", sqlite3_errmsg(db));
	    	sqlite3_close(dbUser);

	    	return false;
	    }

	    char *sql =	"DROP TABLE IF EXISTS User;"
			"CREATE TABLE User (studentID INT, hashedPassword TEXT, userName TEXT, role INT, registerDate INT);";

        rc = sqlite3_exec(dbUser,sql,0,0,&err_msg);
	
        if(rc != SQLITE_OK)
        {
	       fprintf(stderr,"SQL Error: %s\n", err_msg);
	       sqlite3_free(err_msg);
	       sqlite3_close(dbUser);

	       return false;
	    }
    }
    if(isItLecture == true)
    {
        sqlite3 *dbLecture;
        int rc = sqlite3_open("Lecture.db", &dbLecture);

    	if(rc != SQLITE_OK)
    	{
	    	fprintf(stderr,"Cannot open database : %s\n", sqlite3_errmsg(dbLecture));
	    	sqlite3_close(dbLecture);

	    	return false;
	    }

	    char *sql =	"DROP TABLE IF EXISTS Lecture;"
			"CREATE TABLE Lecture (lectureID INT, professorID INT, lectureName TEXT, , createDate INT );";        //미완성(인자값하나 못넣음)
                        
        rc = sqlite3_exec(dbLecture,sql,0,0,&err_msg);
	
        if(rc != SQLITE_OK)
        {
	       fprintf(stderr,"SQL Error: %s\n", err_msg);
	       sqlite3_free(err_msg);
	       sqlite3_close(dbLecture);

	       return false;
	    }
    }
    if(isItAttendanceCheckLog == true)
    {
        sqlite3 *dbAttendanceCheckLog;
        int rc = sqlite3_open("AttendanceCheckLog.db", &dbAttendanceCheckLog);

    	if(rc != SQLITE_OK)
    	{
	    	fprintf(stderr,"Cannot open database : %s\n", sqlite3_errmsg(dbAttendanceCheckLog));
	    	sqlite3_close(dbAttendanceCheckLog);

	    	return false;
	    }

	    char *sql =	"DROP TABLE IF EXISTS AttendanceCheckLog;"
			"CREATE TABLE AttendanceCheckLog (lectureID INT, studentID INT, IP TEXT, quizAnswer TEXT, checkDate INT );"; 
                        
        rc = sqlite3_exec(dbAttendanceCheckLog,sql,0,0,&err_msg);
	
        if(rc != SQLITE_OK)
        {
	       fprintf(stderr,"SQL Error: %s\n", err_msg);
	       sqlite3_free(err_msg);
	       sqlite3_close(dbAttendanceCheckLog);

	       return false;
	    }
    }
    if(isItChatLog == true)
    {
        sqlite3 *dbChatLog;
        int rc = sqlite3_open("ChatLog.db", &dbChatLog);

    	if(rc != SQLITE_OK)
    	{
	    	fprintf(stderr,"Cannot open database : %s\n", sqlite3_errmsg(dbChatLog));
	    	sqlite3_close(dbChatLog);

	    	return false;
	    }

	    char *sql =	"DROP TABLE IF EXISTS ChatLog;"
			"CREATE TABLE ChatLog (lectureID INT, userName TEXT, message TEXT, date INT );";
                        
        rc = sqlite3_exec(dbChatLog,sql,0,0,&err_msg);
	
        if(rc != SQLITE_OK)
        {
	       fprintf(stderr,"SQL Error: %s\n", err_msg);
	       sqlite3_free(err_msg);
	       sqlite3_close(dbChatLog);

	       return false;
	    }
    }
    return true;
}

bool closeDatabase()
{
    if(isItUser == true)
    {
        sqlite3_close(dbUser);
        return true;
    }
    if(isItLecture == true)
    {
        sqlite3_close(dbLecture);
        return true;
    }
    if(isItAttendanceCheckLog == true)
    {
        sqlite3_close(dbAttendanceCheckLog);
        return true;
    }
    if(isItChatLog == true)
    {
        sqlite3_close(dbChatLog);
        return true;
    }

    printf("데이터베이스를 정상적으로 닫지 못했습니다. 치명적 오류발생!\n");
    exit(1);
    return false;
}

User *loadUser(User user[], int amount, int lectureID);     // DB에서 lectureID가 일치하는 사용자 구조체 배열 반환
User loadUserByID(char *studentID);                         // DB에서 studentID가 일치하는 사용자 구조체 반환
bool registerUser(User *user);                              // DB에 새로운 사용자 정보 저장
bool removeUser(char *studentID);                           // DB에서 studentID가 일치하는 사용자 삭제
bool loginUser(char *studentID, char *hashedPassword);      // DB에서 studentID와 hashedPassword가 일치하는 사용자가 있는지 확인
bool clearUser();                                           // User 테이블 초기화