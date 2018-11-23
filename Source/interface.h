#include <locale.h>
#include <ncurses.h>
#include <signal.h>

void initiateInterface();
void drawDefaultLayout();
void drawBorder(WINDOW *window, char *windowName);
void printMessage(WINDOW *window, const char *format, ...);
void onClose();
void signalResize();