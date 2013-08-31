#include <stdio.h>
#include <ncurses.h>
#include <term.h>
#include <assert.h>

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif // MAX

#define CONTROL(x)  ((x) & 0x1F)
typedef enum {
    USCH_FN_START,
    USCH_FN_BEGIN,
    USCH_FN_BEGIN_PARAN,
    USCH_FN_DIV,
    USCH_FN_END,
    USCH_FN_DISABLED,
} USCH_FN_STATE;

int main(void)
{
    FILE *fp = fopen("x", "w");
    if (fp == 0)
        return(-1);
    SCREEN *p_screen = newterm(NULL, stdin, stdout);
    if (p_screen == 0)
        return(-1);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    USCH_FN_STATE fn_state = USCH_FN_START;
    int fn_startpos = 0;
    printw("/* usch */ ");

    int c;
    int col, row;
    while ((c = getch()) != EOF && c != CONTROL('d'))
    {
        switch (c)
        {
            case '(':
                {
                    fn_state = USCH_FN_BEGIN_PARAN;
                    printw("(");
                    break;
                }
            case '\"':
                {
                    fn_state = USCH_FN_DIV;
                    printw("\"");
                    break;
                }


            case KEY_BACKSPACE /* backspace */:
                {
                    getyx(stdscr, col, row);
                    row = MAX(0, row - 1);
                    move(col, row);
                    printw(" ");
                    move(col, row);
                    break;
                }
            case ' ':
                {
                    switch (fn_state)
                    {
                        case USCH_FN_BEGIN:
                            {
                                getyx(stdscr, col, row);
                                fn_startpos = col;
                                printw("(\"", c);
                                fn_state = USCH_FN_DIV;
                                break;
                            }
                        case USCH_FN_DIV:
                            {
                                printw("\", \"", c);
                                break;
                            }
                        case USCH_FN_DISABLED:
                            {
                                printw("\")", c);
                            }
                        default:
                        {
                            assert(0);
                            break;
                        }
                    }
                    break;
                }
            default:
                {
                    printw("%c", c);
                    break;
                }
        }
    }

    endwin();

    return 0;
}

