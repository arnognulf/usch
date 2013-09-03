#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include "pmcurses.h"

// poor-man's curses 
void move(int col, int row)
{

}
void backspace(int num)
{
    int i;
    printf("\033[%dD", num);
    for (i = 0; i < num; i++)
    {
        printf(" ");
    }
    printf("\033[%dD", num);
    fflush(stdout);
}
char getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror ("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror ("tcsetattr ~ICANON");
    return (buf);
}


