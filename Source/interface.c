#include "interface.h"

WINDOW *StatusWindow;
WINDOW *NoticeWindow, *NoticeWindowBorder;
WINDOW *EventWindow, *EventWindowBorder;
WINDOW *MessageWindow, *MessageWindowBorder;
WINDOW *CommandWindow, *CommandWindowBorder;
WINDOW *InputWindow, *InputWindowBorder;

bool isCursesMode;

// ncurses라이브러리를 이용한 사용자 인터페이스 초기화
void initiateInterface()
{
    setlocale(LC_CTYPE, "ko_KR.utf-8");
    initscr();
    noecho();
    curs_set(FALSE);

    isCursesMode = true;

    use_default_colors();
    start_color();
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);

    signal(SIGWINCH, signalResize);
}

// 기본 레이아웃 그리기
void drawDefaultLayout()
{
    int parentX, parentY;
    getmaxyx(stdscr, parentY, parentX);

    int commandWindowBorderWidth = 24;
    int userInputWindowBorderHeight = 4;

    wclear(stdscr);
    wrefresh(stdscr);

    MessageWindow = newwin(parentY - userInputWindowBorderHeight - 2, parentX - commandWindowBorderWidth - 4, 1, 2);
    MessageWindowBorder = newwin(parentY - userInputWindowBorderHeight, parentX - commandWindowBorderWidth, 0, 0);

    CommandWindow = newwin(parentY - userInputWindowBorderHeight - 2, commandWindowBorderWidth - 4, 1, parentX - commandWindowBorderWidth + 2);
    CommandWindowBorder = newwin(parentY - userInputWindowBorderHeight, commandWindowBorderWidth, 0, parentX - commandWindowBorderWidth);

    InputWindow = newwin(userInputWindowBorderHeight - 2, parentX - 4, parentY - userInputWindowBorderHeight + 1, 2);
    InputWindowBorder = newwin(userInputWindowBorderHeight, parentX, parentY - userInputWindowBorderHeight, 0);

    scrollok(MessageWindow, TRUE);
    scrollok(CommandWindow, TRUE);

    drawBorder(MessageWindowBorder, "메시지");
    drawBorder(CommandWindowBorder, "명령어 목록");
    drawBorder(InputWindowBorder, "사용자 입력");

    wrefresh(MessageWindow);
    wrefresh(CommandWindow);
    wrefresh(InputWindow);
}

void drawLoginLayout()
{

}

// 강의실 로비 레이아웃 그리기
void drawLectureLayout()
{
    int parentX, parentY;
    getmaxyx(stdscr, parentY, parentX);

    wclear(stdscr);
    wrefresh(stdscr);

    StatusWindow = newwin(STATUS_HEIGHT, parentX - 2, 0, 1);

    NoticeWindow = newwin(NOTICE_HEIGHT - 2, parentX - EVENT_WIDTH - 4, STATUS_HEIGHT + 1, 2);
    NoticeWindowBorder = newwin(NOTICE_HEIGHT, parentX - EVENT_WIDTH, STATUS_HEIGHT, 0);

    EventWindow = newwin(EVENT_HEIGHT - 2, EVENT_WIDTH - 4, STATUS_HEIGHT + 1, parentX - EVENT_WIDTH + 2);
    EventWindowBorder = newwin(EVENT_HEIGHT, EVENT_WIDTH, STATUS_HEIGHT, parentX - EVENT_WIDTH);

    MessageWindow = newwin(parentY - STATUS_HEIGHT - NOTICE_HEIGHT - INPUT_HEIGHT - 2, parentX - COMMAND_WIDTH - 4, STATUS_HEIGHT + NOTICE_HEIGHT + 1, 2);
    MessageWindowBorder = newwin(parentY - STATUS_HEIGHT - NOTICE_HEIGHT - INPUT_HEIGHT, parentX - COMMAND_WIDTH, STATUS_HEIGHT + NOTICE_HEIGHT, 0);

    CommandWindow = newwin(parentY - STATUS_HEIGHT - EVENT_HEIGHT - INPUT_HEIGHT - 2, COMMAND_WIDTH - 4, STATUS_HEIGHT + EVENT_HEIGHT + 1, parentX - COMMAND_WIDTH + 2);
    CommandWindowBorder = newwin(parentY - STATUS_HEIGHT - EVENT_HEIGHT - INPUT_HEIGHT, COMMAND_WIDTH, STATUS_HEIGHT + EVENT_HEIGHT, parentX - COMMAND_WIDTH);

    InputWindow = newwin(INPUT_HEIGHT - 2, parentX - 4, parentY - INPUT_HEIGHT + 1, 2);
    InputWindowBorder = newwin(INPUT_HEIGHT, parentX, parentY - INPUT_HEIGHT, 0);

    scrollok(NoticeWindow, TRUE);
    scrollok(MessageWindow, TRUE);

    drawBorder(NoticeWindowBorder, "공지 사항");
    drawBorder(EventWindowBorder, "진행중인 이벤트");
    drawBorder(MessageWindowBorder, "메시지");
    drawBorder(CommandWindowBorder, "명령어 목록");
    drawBorder(InputWindowBorder, "사용자 입력");

    wrefresh(StatusWindow);
    wrefresh(NoticeWindow);
    wrefresh(EventWindow);
    wrefresh(MessageWindow);
    wrefresh(CommandWindow);
    wrefresh(InputWindow);
}

// 윈도우 테두리 그리기
// [매개변수] window: 테두리를 그릴 윈도우, windowName: 상단에 보여줄 윈도우 이름
void drawBorder(WINDOW *window, char *windowName)
{
    int x, y;
    getmaxyx(window, y, x);

    // 테두리 그리기
    wattron(window, COLOR_PAIR(3));
    mvwprintw(window, 0, 0, "┌"); 
    mvwprintw(window, y - 1, 0, "└"); 
    mvwprintw(window, 0, x - 1, "┐"); 
    mvwprintw(window, y - 1, x - 1, "┘"); 
    for (int i = 1; i < (y - 1); i++)
    {
        mvwprintw(window, i, 0, "│"); 
        mvwprintw(window, i, x - 1, "│"); 
    }
    for (int i = 1; i < (x - 1); i++)
    {
        mvwprintw(window, 0, i, "─"); 
        mvwprintw(window, y - 1, i, "─"); 
    }
    wattroff(window, COLOR_PAIR(3));

    // 윈도우 이름 출력
    wattron(window, A_BOLD);
    mvwprintw(window, 0, 4, windowName); 
    wattroff(window, A_BOLD);

    refresh();

    wrefresh(window);
}

// 매개변수로 전달한 윈도우에 문자열 출력
// [매개변수] window: 문자열을 출력할 윈도우, format: 문자열 출력 포맷
void printMessage(WINDOW *window, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vwprintw(window, format, args);
    va_end(args);
    wrefresh(window);
}

void updateStatus(char *lectureName, bool isProfessorOnline, int onlineUserCount, char *clientStatus)
{
    int parentX, parentY;
    getmaxyx(StatusWindow, parentY, parentX);

    char timeString[48];
    char lectureString[64];
    char professorStatusString[32];
    char activeUserString[32];
    char lobbyString[32];

    struct tm *timeData;
    time_t timeNow;
    time(&timeNow);
    timeData = localtime(&timeNow);

    sprintf(timeString, "[현재 시간 %02d:%02d]", timeData->tm_hour, timeData->tm_min);
    sprintf(lectureString, "강의: %s", lectureName);
    sprintf(professorStatusString, "교수님 상태: %s", (isProfessorOnline ? "온라인" : "오프라인"));
    sprintf(activeUserString, "접속 중인 사용자: %2d명", onlineUserCount);
    sprintf(lobbyString, "%s", clientStatus);

    wclear(StatusWindow);

    wattron(StatusWindow, A_BOLD);
    mvwprintw(StatusWindow, 0, (((parentX + 1) / 2) - (strlen(timeString) / 2)), timeString);

    mvwprintw(StatusWindow, 1, 0, lectureString);
    if (isProfessorOnline)
    {
        wattron(StatusWindow, COLOR_PAIR(2));
        mvwprintw(StatusWindow, 2, 0, professorStatusString);
        wattroff(StatusWindow, COLOR_PAIR(2));
    }
    else
    {
        wattron(StatusWindow, COLOR_PAIR(1));
        mvwprintw(StatusWindow, 2, 0, professorStatusString);
        wattroff(StatusWindow, COLOR_PAIR(1));
    }
    mvwprintw(StatusWindow, 1, (parentX - 22), activeUserString);
    mvwprintw(StatusWindow, 2, (parentX - strlen(lobbyString) + 6), lobbyString);
    wattroff(StatusWindow, A_BOLD);

    wrefresh(StatusWindow);
}

void updateNotice(char *notice)
{
    wclear(NoticeWindow);
    wprintw(NoticeWindow, notice);
    wrefresh(NoticeWindow);
}

void updateEvent(bool isAttendanceActive, bool isQuizActive)
{
    wclear(EventWindow);

    wattron(EventWindow, A_BOLD);
    if (isAttendanceActive)
    {
        wattron(EventWindow, COLOR_PAIR(2));
        mvwprintw(EventWindow, 0, 0, "현재 출석체크중 입니다");
        wattroff(EventWindow, COLOR_PAIR(2));
    }
    else
    {
        wattron(EventWindow, COLOR_PAIR(1));
        mvwprintw(EventWindow, 0, 0, "진행중인 출석체크가 없습니다");
        wattroff(EventWindow, COLOR_PAIR(1));
    }

    if (isQuizActive)
    {
        wattron(EventWindow, COLOR_PAIR(2));
        mvwprintw(EventWindow, 1, 0, "현재 퀴즈가 진행중 입니다");
        wattroff(EventWindow, COLOR_PAIR(2));
    }
    else
    {
        wattron(EventWindow, COLOR_PAIR(1));
        mvwprintw(EventWindow, 1, 0, "진행중인 퀴즈가 없습니다");
        wattroff(EventWindow, COLOR_PAIR(1));
    }
    wattroff(EventWindow, A_BOLD);

    wrefresh(EventWindow);
}

void updateCommand(char *commandList)
{
    wclear(CommandWindow);
    wprintw(CommandWindow, commandList);
    wrefresh(CommandWindow);
}

void updateInput(char *inputGuide, char *userInput)
{
    wclear(InputWindow);
    wprintw(InputWindow, "%s: %s", inputGuide, userInput);
    wrefresh(InputWindow);
}

// 프로그램 종료시 수행
void onClose()
{
    isCursesMode = false;
    // ncurses윈도우 종료
    delwin(StatusWindow);
    delwin(NoticeWindow);
    delwin(NoticeWindowBorder);
    delwin(EventWindow);
    delwin(EventWindowBorder);
    delwin(MessageWindow);
    delwin(MessageWindowBorder);
    delwin(CommandWindow);
    delwin(CommandWindowBorder);
    delwin(InputWindow);
    delwin(InputWindowBorder);
    endwin();
}

void signalResize()
{
    endwin();
    refresh();
    drawLectureLayout();
    // updateStatus();
    // updateNotice("Vigenere sample source code has been uploaded onto our class khub. You may use it for your Lab 4. Vigenere 소스코드를 khub에 올려놓았으니 참고하세요.");
    // updateEvent(true, false);
}

void enterCursesMode()
{
    if (isCursesMode)
        return;
    
    isCursesMode = true;
    
    reset_prog_mode();
    refresh();
}

void leaveCursesMode()
{
    if (isCursesMode == false)
        return;

    isCursesMode = false;

    def_prog_mode();
    endwin();
}