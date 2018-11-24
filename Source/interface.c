#include "interface.h"

WINDOW *StatusWindow;
WINDOW *NoticeWindow, *NoticeWindowBorder;
WINDOW *EventWindow, *EventWindowBorder;
WINDOW *MessageWindow, *MessageWindowBorder;
WINDOW *CommandWindow, *CommandWindowBorder;
WINDOW *UserInputWindow, *UserInputWindowBorder;

// ncurses라이브러리를 이용한 사용자 인터페이스 초기화
void initiateInterface()
{
    setlocale(LC_CTYPE, "ko_KR.utf-8");
    initscr();
    noecho();
    curs_set(FALSE);

    signal(SIGWINCH, signalResize);
}

// 기본 레이아웃 그리기
void drawDefaultLayout()
{
    int parentX, parentY;
    getmaxyx(stdscr, parentY, parentX);

    int commandWindowBorderWidth = 22;
    int userInputWindowBorderHeight = 4;

    MessageWindow = newwin(parentY - userInputWindowBorderHeight - 2, parentX - commandWindowBorderWidth - 2, 1, 1);
    MessageWindowBorder = newwin(parentY - userInputWindowBorderHeight, parentX - commandWindowBorderWidth, 0, 0);

    CommandWindow = newwin(parentY - userInputWindowBorderHeight - 2, commandWindowBorderWidth - 2, 1, parentX - commandWindowBorderWidth + 1);
    CommandWindowBorder = newwin(parentY - userInputWindowBorderHeight, commandWindowBorderWidth, 0, parentX - commandWindowBorderWidth);

    UserInputWindow = newwin(userInputWindowBorderHeight - 2, parentX - 2, parentY - userInputWindowBorderHeight + 1, 1);
    UserInputWindowBorder = newwin(userInputWindowBorderHeight, parentX, parentY - userInputWindowBorderHeight, 0);

    scrollok(MessageWindow, TRUE);
    scrollok(CommandWindow, TRUE);

    drawBorder(MessageWindowBorder, "MESSAGE");
    drawBorder(CommandWindowBorder, "COMMAND");
    drawBorder(UserInputWindowBorder, "USER INPUT");

    wrefresh(MessageWindow);
    wrefresh(CommandWindow);
    wrefresh(UserInputWindow);
}

// 강의실 로비 레이아웃 그리기
void drawLectureLobbyLayout()
{
    int parentX, parentY;
    getmaxyx(stdscr, parentY, parentX);

    StatusWindow = newwin(STATUS_HEIGHT, parentX, 0, 0);

    NoticeWindow = newwin(NOTICE_HEIGHT - 2, parentX - EVENT_WIDTH - 2, STATUS_HEIGHT + 1, 1);
    NoticeWindowBorder = newwin(NOTICE_HEIGHT, parentX - EVENT_WIDTH, STATUS_HEIGHT, 0);

    EventWindow = newwin(EVENT_HEIGHT - 2, EVENT_WIDTH - 2, STATUS_HEIGHT + 1, parentX - EVENT_WIDTH + 1);
    EventWindowBorder = newwin(EVENT_HEIGHT, EVENT_WIDTH, STATUS_HEIGHT, parentX - EVENT_WIDTH);

    MessageWindow = newwin(parentY - STATUS_HEIGHT - NOTICE_HEIGHT - INPUT_HEIGHT - 2, parentX - COMMAND_WIDTH - 2, STATUS_HEIGHT + NOTICE_HEIGHT + 1, 1);
    MessageWindowBorder = newwin(parentY - STATUS_HEIGHT - NOTICE_HEIGHT - INPUT_HEIGHT, parentX - COMMAND_WIDTH, STATUS_HEIGHT + NOTICE_HEIGHT, 0);

    CommandWindow = newwin(parentY - STATUS_HEIGHT - EVENT_HEIGHT - INPUT_HEIGHT - 2, COMMAND_WIDTH - 2, STATUS_HEIGHT + EVENT_HEIGHT + 1, parentX - COMMAND_WIDTH + 1);
    CommandWindowBorder = newwin(parentY - STATUS_HEIGHT - EVENT_HEIGHT - INPUT_HEIGHT, COMMAND_WIDTH, STATUS_HEIGHT + EVENT_HEIGHT, parentX - COMMAND_WIDTH);

    UserInputWindow = newwin(INPUT_HEIGHT - 2, parentX - 2, parentY - INPUT_HEIGHT + 1, 1);
    UserInputWindowBorder = newwin(INPUT_HEIGHT, parentX, parentY - INPUT_HEIGHT, 0);

    scrollok(NoticeWindow, TRUE);
    scrollok(MessageWindow, TRUE);

    drawBorder(NoticeWindowBorder, "공지 사항");
    drawBorder(EventWindowBorder, "진행중인 이벤트");
    drawBorder(MessageWindowBorder, "메시지");
    drawBorder(CommandWindowBorder, "명령어");
    drawBorder(UserInputWindowBorder, "사용자 입력");

    wrefresh(StatusWindow);
    wrefresh(NoticeWindow);
    wrefresh(EventWindow);
    wrefresh(MessageWindow);
    wrefresh(CommandWindow);
    wrefresh(UserInputWindow);
}

// 윈도우 테두리 그리기
// [매개변수] window: 테두리를 그릴 윈도우, windowName: 상단에 보여줄 윈도우 이름
void drawBorder(WINDOW *window, char *windowName)
{
    int x, y;
    getmaxyx(window, y, x);

    // 테두리 그리기
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

    // 윈도우 이름 출력
    mvwprintw(window, 0, 4, windowName); 

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

// 프로그램 종료시 수행
void onClose()
{
    // ncurses윈도우 종료
    delwin(MessageWindow);
    delwin(MessageWindowBorder);
    delwin(CommandWindow);
    delwin(CommandWindowBorder);
    delwin(UserInputWindow);
    delwin(UserInputWindowBorder);
    endwin();
}

void signalResize()
{
    endwin();
    refresh();
    drawLectureLobbyLayout();
}