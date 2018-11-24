#include "database.h"

bool isItUser;                          //유저들의 데이터베이스인지 확인하는 변수
bool isItLecture;                       //강의들의 데이터베이스인지 확인하는 변수.
bool isItAttendanceCheckLog;            //출석체크 기록 데이터베이스인지 확인하는 변수.
bool isItChatLog;                       //채팅 기록 데이터베이스인지 확인하는 변수.

sqlite3 *dbUser;   
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
	    	fprintf(stderr,"Cannot open database : %s\n", sqlite3_errmsg(dbUser));
	    	sqlite3_close(dbUser);

	    	return false;
	    }

	    char *sql =	"DROP TABLE IF EXISTS User;"
			        "CREATE TABLE User (studentID TEXT, hashedPassword TEXT, userName TEXT, role INT, registerDate INT);";   //role과 Date는 INT로 임시로 잡음
	    printf("%s\n",sql);
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
		        	"CREATE TABLE Lecture (lectureID INT, professorID INT, lectureName TEXT, createDate INT );";        //미완성(인자값하나 못넣음)
                        
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

//새로추가함 11/24 15:25 p.m
bool registerUser(User *user)                                  // DB에 새로운 사용자 정보 저장
{
    char strSQL[500];                                       // 명령문을 담을 임시 char 배열
    char strRole[10];
    char strRegisterDate[50];


    sprintf(strRole, "%d", user->role);                         //정수형을 char형으로 변환
    sprintf(strRegisterDate, "%d", user->registerDate);         //정수형을 char형으로 변환
 
    strcpy(strSQL,"INSERT INTO User VALUES(");

	strcat(strSQL,"'");
    strcat(strSQL,user->studentID);	
	strcat(strSQL,"'");
	strcat(strSQL,",");

	strcat(strSQL,"'");
	strcat(strSQL,user->hashedPassword);
	strcat(strSQL,"'");
    strcat(strSQL,",");

	strcat(strSQL,"'");
	strcat(strSQL,user->userName);
	strcat(strSQL,"'");
   	 strcat(strSQL,",");

	//strcat(strSQL,"'");    
    strcat(strSQL,strRole);
	//strcat(strSQL,"'");
	strcat(strSQL,",");
    
	//strcat(strSQL,"'");
   	strcat(strSQL,strRegisterDate);
	//strcat(strSQL,"'");
	strcat(strSQL,");");

	printf("%s\n",strSQL);                                                              //테스트용, 잘 출력되나

	//char *ssql = "INSERT INTO User VALUES('201210927','ZZZZZZ','장진성',2,3);";

	int rc = sqlite3_exec(dbUser,strSQL,0,0,&err_msg);                                  //지금 여기서 막힘. 18:42 p.m 11/24
	if(rc != SQLITE_OK)
	{
		fprintf(stderr,"SQL error: %s\n", err_msg);

		sqlite3_free(err_msg);
		sqlite3_close(dbUser);

		return false;
	}

    return true;
}

int main(void)                                                                           //테스트용.
{
	User u1;
	strcpy(u1.userName, "장진성");
   	strcpy(u1.studentID, "201210927");
    strcpy(u1.hashedPassword, "abcdefg");
   	u1.role = 1;
   	u1.registerDate = 1234;

	isItUser = true;
	createNewDatabase();
	registerUser(&u1);
	closeDatabase();

	return 0;
}