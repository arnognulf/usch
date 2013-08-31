#include <stdio.h>
#include <ncurses.h>
#include <term.h>
#include <assert.h>
#include <string.h>

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif // MAX

#define CONTROL(x)  ((x) & 0x1F)
typedef enum {
    USCH_FN_START,
    USCH_FN_BEGIN,
    USCH_FN_BEGIN_PARAN,
    USCH_FN_DIV,
    USCH_FN_DIV_CONT,
    USCH_FN_END,
    USCH_FN_DISABLED,
} USCH_FN_STATE;

int main(void)
{
    char prompt[] = "/* usch */ ";
    SCREEN *p_screen = newterm(NULL, stdin, stdout);
    if (p_screen == 0)
        return(-1);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    USCH_FN_STATE fn_state = USCH_FN_START;
    int fn_startpos = 0;
    printw("%s", prompt);

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
                    row = MAX(strlen(prompt), row - 1);
                    move(col, row);
                    printw(" ");
                    move(col, row);
                    break;
                }
            case ' ':
                {
                    switch (fn_state)
                    {
                        case USCH_FN_START:
                            printw(" ");
                            break;
                        case USCH_FN_BEGIN:
                            {
                                fn_startpos = col;
                                printw("(\"", c);
                                fn_state = USCH_FN_DIV;
                                break;
                            }
                        case USCH_FN_DIV:
                        case USCH_FN_DIV_CONT:
                            {
                                printw("\", \"", c);
                                fn_state = USCH_FN_DIV_CONT;
                                break;
                            }
                        case USCH_FN_DISABLED:
                            {
                                printw("\")", c);
                            }
                        default:
                            {
                                printw("%d\n\n", (unsigned int)fn_state);
                                goto end;
                                break;
                            }
                    }
                    break;
                }
            case '\n':
                {
                    switch (fn_state)
                    {
                        case USCH_FN_START:
                            {
                                printw("\n%s", prompt);
                                break;
                            }
                        case USCH_FN_BEGIN:
                            {
                                printw("();\n%s", prompt);
                                break;
                            }
                        case USCH_FN_DIV:
                            {
                                getyx(stdscr, col, row);
                                row = MAX(strlen(prompt), row - 1);
                                move(col, row);
                                printw(" ");
                                move(col, row);
                                printw(");\n%s", prompt);
                                break;
                            }
                        case USCH_FN_DIV_CONT:
                            {
                                printw("\");\n%s", prompt);
                                break;
                            }
                        default:
                            {
                                goto end;
                            }
                    }


                    fn_state = USCH_FN_START;
                    break;
                }
            default:
                {
                    if (fn_state == USCH_FN_START)
                    {
                        fn_state = USCH_FN_BEGIN;
                    }
                    printw("%c", c);
                    break;
                }
        }
    }
end:
    printw("\nTHE END\n");
    getch();
    endwin();

    return 0;
}

