#include "database.h"

MYSQL *Connect;                                                         //MySQL 구조체를 핸들링 할 전역 변수+++


//에러가 발생하게되면 에러메시지를 띄우고 MYSQL 구조체함수 핸들링을 그만하게한다.
bool handlingError(MYSQL *Connect)
{
	fprintf(stderr,"%s\n",mysql_error(Connect));
	mysql_close(Connect);
}

//mysql_query함수에 에러처리까지 검사하는 함수가 자주쓰여서 만든 쿼리실행문 함수입니다.
bool excuteQuery(char *sql)
{
	if(mysql_query(Connect,sql) != 0)
	{
		handlingError(Connect);
		return false;
	}

	return true;
}

//MySQL 서버를 핸들링할 객체를 메모리에 할당하고 초기화 하는 함수.
//NULL 반환시 예외처리 시키고 종료한다.
bool initializeDatabase()
{
	Connect = mysql_init(NULL);

	if(Connect == NULL)
	{
		fprintf(stderr,"mysql_init() 함수 실패\n");
		return false;
	}

	return true;
}

/*database에 연결하는 함수
mysql_real_connect라는 함수를 이용한다. 만약 이 함수가 NULL을 반환하게되면 예외처리 시키고 종료한다.
첫번째 인자 : MySQL 객체를 핸들링 하는 변수
두번째 인자 : "@right.jbnu.ac.kr"을 의미, 즉 들어갈 서버의 호스트명을 의미한다.
세번째 인자 : "A201210927"은 서버에 들어갈 계정 이름을 의미한다.
네번째 인자 : "q1234"는 서버에 들어갈 계정의 비밀번호를 의미한다.
다섯번째 인자 : "A201210927" 두번째 인자와 햇갈릴 수 있지만 설명하자면, 다섯번째 인자는 "A201210927"의 이름을 가진 데이터베이스를 연결한다는 의미이다.
               해당 서버가 데이터베이스 이름을 이렇게 제공해주었고, 이것밖에 사용할 수 없기 때문에 우리는 이 안에서 테이블들을 관리해야한다.
여덟번째 인자 : CLIENT_MULTI_STATEMENTS는 기본 MySQL에서는 쿼리문을 단일로밖에 제공을 하지 않기때문에, 복수의 쿼리문을 입력하고 싶을때 쓰는 명령문이다. */
bool connectToDatabase()
{
	if(mysql_real_connect(Connect,"right.jbnu.ac.kr", "A201210927","q1234","A201210927",0,NULL,CLIENT_MULTI_STATEMENTS) == NULL)
	{
		handlingError(Connect);
		return false;
	}

	return true;
}

//데이터베이스를 종료시키는 함수.
//핸들링함수가 존재시에 종료시키고 true를 반환한다.
bool closeDatabase()
{
	if(Connect == NULL)
	{
		fprintf(stderr,"%s\n", mysql_error(Connect));
		return false;
	}
	mysql_close(Connect);
	return true;
}

/*4개의 테이블들을 만드는 함수, 성공하면 0 그렇지않으면 다른숫자를 발생시킨다.
Lecture의 table의 갯수와 구조체의 갯수가 다르다. 이유는 memberList배열을 데이터베이스에서는 뺐기 때문인데, memberList라는 table을 따로 만들어서
그 안에서 어떤 강의를 듣는지 구별 할 수 있는 값을 넣고 필요할때 그 값에 해당되는 모든 user를 출력하면 되기 때문이다.*/
bool makeTables()
{
	excuteQuery("CREATE TABLE IF NOT EXISTS User(studentID INT, hashedPassword TEXT, userName TEXT, role INT, registerDate TEXT)");
	excuteQuery("CREATE TABLE IF NOT EXISTS Lecture(lectureID INT, professorID INT, lectureName TEXT, createDate TEXT)");
	excuteQuery("CREATE TABLE IF NOT EXISTS AttendanceCheckLog(lectureID INT, studentID INT, IP TEXT, quizAnswer TEXT, checkDate TEXT)");
	excuteQuery("CREATE TABLE IF NOT EXISTS ChatLog(lectureID INT, userName TEXT, message TEXT, date TEXT)");

	return true;
}

/* 각 데이터베이스를 초기화 하는 함수이다.
DROP 명령문으로 기존 테이블을 삭제하고, 다시 새롭게 만드는 방식으로 초기화를 진행했다.
여기서 주의할것은 한번에 쿼리문을 작성하지 않았고 2개의 executeQuery()함수문으로 초기화를 시켰는데
이는 MySQL에서의 동기화 문제로 컴파일이 안되었기 때문에 어쩔 수 없이 이렇게 구현 할 수 밖에 없었다.*/
bool clearUser()
{
	excuteQuery("DROP TABLE User");
	excuteQuery("CREATE TABLE IF NOT EXISTS User(studentID INT, hashedPassword TEXT, userName TEXT, role INT, registerDate TEXT)");

	return true;
}
bool clearLecture()
{	
	excuteQuery("DROP TABLE Lecture");
	excuteQuery("CREATE TABLE IF NOT EXISTS Lecture(lectureID INT, professorID INT, lectureName TEXT, createDate TEXT)");

	return true;
}
bool clearAttendanceCheckLog()
{
	excuteQuery("DROP TABLE AttendanceCheckLog");
	excuteQuery("CREATE TABLE IF NOT EXISTS AttendanceCheckLog(lectureID INT, studentID INT, IP TEXT, quizAnswer TEXT, checkDate TEXT)");

	return true;
}
bool clearChatLog()
{
	excuteQuery("DROP TABLE ChatLog");
	excuteQuery("CREATE TABLE IF NOT EXISTS ChatLog(lectureID INT, userName TEXT, message TEXT, date TEXT)");

	return true;
}

//User구조체를 참조해서 데이터베이스 User 테이블에 값을 저장하는 함수.
bool registerUser(User *user)
{
	char strSQL[500];                                       //명령문을 담을 char 배열
    char strRole[2];										//Role형 role을 쿼리문에 입력하기위해 임시로 char형으로 바꿀 변수.
    char strRegisterDate[50];								//time_t형 registerDate를 char형으로 바꿔 데이터베이스에 저장시킬 변수.

	time(&user->registerDate);									
	strcpy(strRegisterDate,ctime(&user->registerDate));			//ctime함수로 인해 변형된 문자열을 strRegisterDate에 복사해 저장시키고 있다.
	sprintf(strRole, "%d", user->role);                         //쿼리문에는 char형으로 입력해야 하므로 int형 변수 role을 char형으로 변환 

	for(int i=0; strRegisterDate[i] != '\0'; i++)				//ctime으로 변환된 문자열은 '\n'문자를 포함하고있다. 따라서 '\n'문자를 제거하기위한 알고리즘이다.
	{
		if(strRegisterDate[i] == '\n')
		{
			strRegisterDate[i] = '\0';
			break;
		}
	}

	// INSERT INTO User VALUES(201210927,'password','장진성',2,'2018년 11월 29일 0시 45분'); 이와같은 형식으로 저장할 예정.
    strcpy(strSQL,"INSERT INTO User VALUES(");
    strcat(strSQL,user->studentID);	
	strcat(strSQL,", ");

	strcat(strSQL,"'");
	strcat(strSQL,user->hashedPassword);
	strcat(strSQL,"'");
    strcat(strSQL,", ");

	strcat(strSQL,"'");
	strcat(strSQL,user->userName);
	strcat(strSQL,"'");
   	strcat(strSQL,", ");
  
    strcat(strSQL,strRole);
	strcat(strSQL,", ");
    
	strcat(strSQL,"'");
   	strcat(strSQL,strRegisterDate);
	strcat(strSQL,"');");

	excuteQuery(strSQL);
	return true;
}

/*이 함수는 DB에서 studentID와 hashedPassword가 일치하는 사용자가 있는지 확인하는 함수이다.
학번과 비밀번호를 받아서, User Table에 해당 학번과 비밀번호가 일치한지 확인 한 다음 둘다 일치하면 true를, 그렇지않으면 false를 반환한다.*/
bool isLoginUser(char *studentID, char *hashedPassword)
{
	excuteQuery("SELECT * FROM User");					//User Table을 선택해야만 이 함수는 실행될 수 있기에, SELECT문으로 선택을 먼저했다.

	MYSQL_RES *result = mysql_store_result(Connect);	//데이터베이스의 원소들을 다루기 위한 핸들링 함수 result;
	if(result == NULL)
	{
		handlingError(Connect);
		return false;
	}

	MYSQL_ROW row;												//row 핸들링 변수 row;
	while( row= mysql_fetch_row(result)  )
	{
		int isItSameID = strcmp(studentID,row[0]);				//strcmp 함수를 통해, 두 문자가 같으면 0을 반환한다.
		int isItSamePW = strcmp(hashedPassword,row[1]);
		
		if( (isItSameID==0) & (isItSamePW==0) )					//두개 다 같다면 
		{
			printf("찾고자하는 ID와 PW가 일치하는 데이터가 이 테이블 안에 있습니다.\n");
			mysql_free_result(result);							//핸들링 변수 result를 닫는다.
			return true;										//true 반환
		}
	}

	printf("찾고자하는 ID와 PW가 일치하는 데이터가 이 테이블 안에 없습니다.\n");
	mysql_free_result(result);									//핸들링 변수 result를 닫는다.
	return false;												//두개 다 같은것을 찾지못하면 false반환.
}

//User 테이블에 studentID가 있으면 해당되는 행을 삭제하는 함수
bool removeUser(char *studentID)
{
	char strSQL[200];										//쿼리문을 배열에 저장하기 위해서.
	strcpy(strSQL,"DELETE FROM User WHERE studentID=");
    strcat(strSQL,studentID);	

	if(excuteQuery(strSQL))									//쿼리문을 돌리고 문제 없이 잘 작동되면 true 반환.
		return true;
}

//Database에서 studentID가 일치하는 사용자 구조체 반환하는 함수
//WHERE 조건문으로 하려고 했으나 계속 안되어서 어쩔수 없이 isLoginUser()함수와 비슷한 알고리즘으로 구현했다.
User loadUserByID(char *studentID)
{

	excuteQuery("SELECT * FROM User");

	User tempUser;										//return할 구조체를 임시로 생성
	MYSQL_RES *result = mysql_store_result(Connect);	//데이터베이스의 원소들을 다루기 위한 핸들링 함수 result;
	
	if(result == NULL)
	{
		handlingError(Connect);
		exit(0);
	}
		
	MYSQL_ROW row;												//row 핸들링 변수 row;
	while( row= mysql_fetch_row(result)  )
	{		
		int isItSameID = strcmp(studentID,row[0]);
			
		if(isItSameID==0)
		{
			strcpy(tempUser.studentID,row[0]);
    		strcpy(tempUser.hashedPassword,row[1]);
    		strcpy(tempUser.userName,row[2]);
    		tempUser.role = atoi(row[3]);
    		tempUser.registerDate = 0;							//일단 테스트용. time_t에 대해서 미숙하고 공부 아직 안함.

			mysql_free_result(result);
			return tempUser; 									//구조체를 반환한다.
		}
	}

	printf("loadUserByID() 함수 실패!.\n");
	exit(0);
}

int main(void)
{
	//마지막에 꼭 main함수에 내가 만든 함수 하나씩만 넣어서 실행시켜보기! (논리적인 오류가 있는지 확인하기 위해)
	//구조체에 임시로 데이터값 입력 테스트
	User u[10];
	strcpy(u[0].userName, "장진성");
   	strcpy(u[0].studentID, "201210927");
    strcpy(u[0].hashedPassword, "abcdefg");
   	u[0].role = Student;

	strcpy(u[1].userName, "홍길동");
   	strcpy(u[1].studentID, "1234567");
    strcpy(u[1].hashedPassword, "abcabc");
   	u[1].role = Student;

	//데이터베이스 초기화 세팅 테스트
	if(initializeDatabase())
		printf("초기화 성공!\n");
	if(connectToDatabase())
		printf("연결 성공!\n");
	if(makeTables())
		printf("테이블 생성 완료!\n");
		
	//테이블 리셋 테스트
	if(clearUser())
		printf("User 테이블 초기화 완료!\n");
	if(clearLecture())
		printf("Lecture 테이블 초기화 완료!\n");
	if(clearAttendanceCheckLog())
		printf("AttendanceCheckLog 테이블 초기화 완료!\n");
	if(clearChatLog())
		printf("ChatLog 테이블 초기화 완료!\n");

	//등록 테스트
	if(registerUser(&u[1]))
		printf("테이블에 데이터 등록 완료!\n");
	if(registerUser(&u[0]))
		printf("테이블에 데이터 등록 완료!\n");

	//isLoginUser 함수 테스트, ID PW로 일치하는지 확인
	if(isLoginUser("1234567","abcabc"))
		printf("isLoginUser 함수 작동 정상!\n");
	if(isLoginUser("201210927","abcdefg"))
		printf("isLoginUser 함수 작동 정상!\n");

	//removeUser 함수 테스트, 삭제 잘 되는지 확인
	if(removeUser("1234567"))
		printf("removeUser 함수 작동 정상!\n");

	//loadUserByID 함수 테스트, 반환 잘 되는지 확인
	printf("%s\n", loadUserByID("201210927").studentID);
	printf("%s\n", loadUserByID("201210927").hashedPassword);
	printf("%s\n", loadUserByID("201210927").userName);
	printf("%d\n", loadUserByID("201210927").role);
	//결과: 테스트 성공


	//데이터베이스 종료 테스트
	if(closeDatabase())
		printf("종료 성공!\n");
	printf("good bye.\n");
	return 0;
}