#include <fcntl.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SERVER_PORT 10743

enum Command
{
    SampleCommand1, SampleCommand2
};

struct DataPack
{
    enum Command command;
    char message[508];
};

// 서버
bool initiateServer();

// 통신
bool receiveDataPack(int sender, struct DataPack *dataPack);
bool sendDataPack(int receiver, struct DataPack *dataPack);

// 인터페이스
void initiateInterface();
void drawBorder(WINDOW *window, char *windowName);
void onClose();

int ServerSocket;

int Fdmax;
fd_set Master, Reader;

WINDOW *MessageWindow, *MessageWindowBorder;
WINDOW *CommandWindow, *CommandWindowBorder;
WINDOW *UserInputWindow, *UserInputWindowBorder;

int main()
{
    atexit(onClose);
    initiateInterface();

    initiateServer();

    sleep(10);
    
    return 0;
}

bool initiateServer()
{
    return false;
}

bool receiveDataPack(int sender, struct DataPack *dataPack)
{
    return false;
}

bool sendDataPack(int receiver, struct DataPack *dataPack)
{
    return false;
}

void initiateInterface()
{
    initscr();
    noecho();
    curs_set(FALSE);

    int parentX, parentY;
    getmaxyx(stdscr, parentY, parentX);

    int commandWindowBorderWidth = 20;
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

void drawBorder(WINDOW *window, char *windowName)
{
    int x, y;
    getmaxyx(window, y, x);
    mvwprintw(window, 0, 0, "+"); 
    mvwprintw(window, y - 1, 0, "+"); 
    mvwprintw(window, 0, x - 1, "+"); 
    mvwprintw(window, y - 1, x - 1, "+"); 

    for (int i = 1; i < (y - 1); i++)
    {
        mvwprintw(window, i, 0, "|"); 
        mvwprintw(window, i, x - 1, "|"); 
    }
    for (int i = 1; i < (x - 1); i++)
    {
        mvwprintw(window, 0, i, "-"); 
        mvwprintw(window, y - 1, i, "-"); 
    }

    mvwprintw(window, 0, 4, windowName); 

    wrefresh(window);
}

void onClose()
{
    delwin(MessageWindow);
    delwin(MessageWindowBorder);
    delwin(CommandWindow);
    delwin(CommandWindowBorder);
    delwin(UserInputWindow);
    delwin(UserInputWindowBorder);
    endwin();
}