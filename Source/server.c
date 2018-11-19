#include <errno.h>
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
#define SERVER_ADDRESS "192.168.98.128"
#define MAX_CLIENT 64

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
void async();
void acceptClient(int socket);
void receiveData(int socket);

// 통신
bool receiveDataPack(int sender, struct DataPack *dataPack);
bool sendDataPack(int receiver, struct DataPack *dataPack);

// 인터페이스
void initiateInterface();
void drawBorder(WINDOW *window, char *windowName);
void onClose();

int ServerSocket;
time_t ServerUpTime;

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
    async();
    
    return 0;
}

bool initiateServer()
{
    wprintw(MessageWindow, "Initiate server\n");
    wrefresh(MessageWindow);

    ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocket == -1)
    {
        wprintw(MessageWindow, "socket: %s\n", strerror(errno));
        wrefresh(MessageWindow);
        return false;
    }

    int socketOption = 1;
    setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, &socketOption, sizeof(socketOption));

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(struct sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

    if (bind(ServerSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)) == -1)
    {
        wprintw(MessageWindow, "bind: %s\n", strerror(errno));
        wrefresh(MessageWindow);
        return false;
    }

    if (listen(ServerSocket, MAX_CLIENT) == -1)
    {
        wprintw(MessageWindow, "listen: %s\n", strerror(errno));
        wrefresh(MessageWindow);
        return false;
    }

    time(&ServerUpTime);

    return true;
}

void async()
{
    FD_ZERO(&Master);
    FD_ZERO(&Reader);
    FD_SET(ServerSocket, &Master);
    Fdmax = ServerSocket;

    while (true)
    {
        wprintw(MessageWindow, "Wait for data...\n");
        wrefresh(MessageWindow);

        Reader = Master;
        if (select(Fdmax + 1, &Reader, NULL, NULL, NULL) == -1)
        {
            wprintw(MessageWindow, "select: %s\n", strerror(errno));
            wrefresh(MessageWindow);
            return;
        }

        for (int fd = 0; fd <= Fdmax; fd++)
        {
            if (FD_ISSET(fd, &Reader) == 0)
                continue;

            if (fd == ServerSocket)
            {
                acceptClient(fd);
            }
            else if (fd == 0)   // stdin
            {

            }
            else
            {
                receiveData(fd);
            }
        }
    }
}

void acceptClient(int socket)
{
    struct sockaddr_in clientAddr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    socket = accept(ServerSocket, (struct sockaddr *)&clientAddr, &addrlen);
    if (socket == -1)
    {
        wprintw(MessageWindow, "accept: %s\n", strerror(errno));
        wrefresh(MessageWindow);
    }
    else
    {
        FD_SET(socket, &Master);
        if (socket > Fdmax)
        {
            Fdmax = socket;
        }
        wprintw(MessageWindow, "Accept new client, socket: '%d'\n", socket);
        wrefresh(MessageWindow);
    }
}

void receiveData(int socket)
{
    int readBytes;
    struct DataPack *receivedDataPack;
    // memset(receivedDataPack, 0, sizeof(struct DataPack));
    char buffer[sizeof(struct DataPack)] = { 0, };

    readBytes = recv(socket, buffer, sizeof(buffer), 0);
    switch (readBytes)
    {
        case -1:
            close(socket);
            FD_CLR(socket, &Master);
            wprintw(MessageWindow, "recv: %s, socket: %d\n", strerror(errno), socket);
            wrefresh(MessageWindow);
            break;

        case 0:
            close(socket);
            FD_CLR(socket, &Master);
            wprintw(MessageWindow, "Client disconnected, socket: '%d'\n", socket);
            wrefresh(MessageWindow);
            break;

        default:
            if (readBytes == sizeof(struct DataPack))
            {
                receivedDataPack = (struct DataPack *)buffer;
                receiveDataPack(socket, receivedDataPack);
            }
            else
            {
                wprintw(MessageWindow, "Invalid data received, socket: '%d', readBytes: '%d'\n", socket, readBytes);
                wrefresh(MessageWindow);
            }
            break;
    }
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