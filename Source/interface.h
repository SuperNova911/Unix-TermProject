#include <locale.h>
#include <ncurses.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define STATUS_HEIGHT 3
#define NOTICE_HEIGHT 5
#define EVENT_HEIGHT 5
#define EVENT_WIDTH 30
#define COMMAND_WIDTH 22
#define INPUT_HEIGHT 4

void initiateInterface();
void drawDefaultLayout();
void drawLectureLobbyLayout();
void drawBorder(WINDOW *window, char *windowName);
void printMessage(WINDOW *window, const char *format, ...);
void updateStatus();
void onClose();
void signalResize();