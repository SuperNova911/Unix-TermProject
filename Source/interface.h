#include <locale.h>
#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define STATUS_HEIGHT 3
#define NOTICE_HEIGHT 5
#define EVENT_HEIGHT 5
#define EVENT_WIDTH 32
#define COMMAND_WIDTH 22
#define INPUT_HEIGHT 4

void initiateInterface();
void drawDefaultLayout();
void drawLoginLayout();
void drawLectureLayout();
void drawBorder(WINDOW *window, char *windowName);
void printMessage(WINDOW *window, const char *format, ...);
void updateStatus();
void updateNotice(char *notice);
void updateEvent(bool isAttendanceCheck, bool isQuiz);
void updateCommand(char *commandList);
void updateInput(char *inputGuide, char *userInput);
void onClose();
void signalResize();