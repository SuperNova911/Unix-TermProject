#include "database.h"

bool IsItUser;                          //유저들의 데이터베이스인지 확인하는 변수
bool IsItLecture;                       //강의들의 데이터베이스인지 확인하는 변수.
bool IsItAttendanceCheckLog;            //출석체크 기록 데이터베이스인지 확인하는 변수.
bool IsItChatLog;                       //채팅 기록 데이터베이스인지 확인하는 변수.

sqlite3 *Db;
char *err_msg = 0;                      //에러메시지 처리변수.

bool excuteQuery(char *sql)
{
    int returnCode = sqlite3_exec(dbUser,sql,0,0,&err_msg);
	
    if(returnCode != SQLITE_OK)
    {
		fprintf(stderr,"SQL Error: %s\n", err_msg);
	    sqlite3_free(err_msg);
	    sqlite3_close(Db);
	    return false;
	}

	return true;
}

bool connectToDatabase()
{
	int rc = sqlite3_open(DB_PATH, &Db);

    if(rc != SQLITE_OK)
    {
	    fprintf(stderr,"Cannot open database : %s\n", sqlite3_errmsg(Db));
	    sqlite3_close(Db);

	    return false;
	}

	if(IsItUser == true)
    {                           
		excuteQuery("CREATE TABLE IF NOT EXISTS User (studentID TEXT, hashedPassword TEXT, userName TEXT, role INT, registerDate INT);");
    }
	if(IsItLecture == true)
    {                           
		excuteQuery("CREATE TABLE IF NOT EXISTS Lecture (lectureID INT, professorID INT, lectureName TEXT, createDate INT );");
    }
    if(IsItAttendanceCheckLog == true)
    {
		excuteQuery("CREATE TABLE IF NOT EXISTS AttendanceCheckLog (lectureID INT, studentID INT, IP TEXT, quizAnswer TEXT, checkDate INT );");
	}
	if(IsItChatLog == true)
	{
		excuteQuery("CREATE TABLE IF NOT EXISTS ChatLog (lectureID INT, userName TEXT, message TEXT, date INT );");
	}
}

bool closeDatabase()
{
	int returnCode = sqlite3_close(Db);

	if(returnCode != SQLITE_OK)
	{
		printf("데이터베이스를 정상적으로 닫지 못했습니다. 치명적 오류발생!\n");
    	return false;
	}
}
//11월26일 수정시작! 10:51a.m
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
	strcat(strSQL,", ");

	strcat(strSQL,"'");
	strcat(strSQL,user->hashedPassword);
	strcat(strSQL,"'");
    strcat(strSQL,", ");

	strcat(strSQL,"'");
	strcat(strSQL,user->userName);
	strcat(strSQL,"'");
   	 strcat(strSQL,", ");

	//strcat(strSQL,"'");    
    strcat(strSQL,strRole);
	//strcat(strSQL,"'");
	strcat(strSQL,", ");
    
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