#include <assert.h>
#include <string.h>
#include <malloc.h>

#include "pmcurses.h"
#include "eval.h"

#define INPUT_BUFFER_MAX 32676
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
    int status = 0;
    char *p_input = NULL;
    int input_index = 0;
    char prompt[] = "/* usch */ ";
    int fn_startpos = 0;
    int c;
    int col, row;
    //SCREEN *p_screen = newterm(NULL, stdin, stdout);
    USCH_FN_STATE fn_state = USCH_FN_START;

    //if (p_screen == 0)
    //    return(-1);
    //cbreak();
    //noecho();
    //keypad(stdscr, TRUE);

    p_input = malloc(INPUT_BUFFER_MAX);
    if (p_input == NULL)
    {
        goto end;
    }
    printf("%s", prompt);
    fflush(stdout);

    while ((c = getch()) != EOF && c != CONTROL('d'))
    {
        switch (c)
        {
            case '(':
                {
                    fn_state = USCH_FN_BEGIN_PARAN;
                    printf("(");
                    fflush(stdout);
                    p_input[input_index++] = '(';
                    break;
                }
            case '\"':
                {
                    fn_state = USCH_FN_DIV;
                    printf("\"");
                    fflush(stdout);
                    p_input[input_index++] = '"';
                    break;
                }
            case KEY_TAB:
                {
                    // TODO: do autocompletion stuffs
                    break;
                }

            case KEY_BACKSPACE:
                {
                    printf("back\n");
                    //getyx(stdscr, col, row);
                    row = MAX(strlen(prompt), row - 1);
                    //move(col, row);
                    printf(" ");
                    //move(col, row);
                    // TODO: fix backspace here
                    input_index--;
                    break;
                }
            case ' ':
                {
                    switch (fn_state)
                    {
                        case USCH_FN_START:
                            printf(" ");
                            fflush(stdout);
                            break;
                        case USCH_FN_BEGIN:
                            {
                                fn_startpos = col;
                                printf("(\"", c);
                                fflush(stdout);
                                p_input[input_index++] = '(';
                                p_input[input_index++] = '"';
                                fn_state = USCH_FN_DIV;
                                break;
                            }
                        case USCH_FN_DIV:
                        case USCH_FN_DIV_CONT:
                            {
                                printf("\", \"", c);
                                fflush(stdout);
                                p_input[input_index++] = '"';
                                p_input[input_index++] = ',';
                                p_input[input_index++] = '"';

                                fn_state = USCH_FN_DIV_CONT;
                                break;
                            }
                        case USCH_FN_DISABLED:
                            {
                                printf("\")", c);
                                p_input[input_index++] = '"';
                                p_input[input_index++] = ')';
                                break;
                            }
                        default:
                            {
                                printf("%d\n\n", (unsigned int)fn_state);
                                fflush(stdout);
                                goto end;
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
                                printf("\n%s", prompt);
                                fflush(stdout);
                                break;
                            }
                        case USCH_FN_BEGIN:
                            {
                                printf("();\n%s", prompt);
                                fflush(stdout);
                                p_input[input_index++] = '(';
                                p_input[input_index++] = ')';
                                p_input[input_index++] = ';';
                                break;
                            }
                        case USCH_FN_DIV:
                            {
                                //getyx(stdscr, col, row);
                                //row = MAX(strlen(prompt), row - 1);
                                //move(col, row);
                                printf(" ");
                                //move(col, row);
                                printf(");\n");
                                fflush(stdout);
                                p_input[input_index++] = ')';
                                p_input[input_index++] = ';';
                                break;
                            }
                        case USCH_FN_DIV_CONT:
                            {
                                printf("\");\n%s", prompt);
                                fflush(stdout);
                                p_input[input_index++] = '"';
                                p_input[input_index++] = ')';
                                p_input[input_index++] = ';';
                                break;
                            }
                        default:
                            {
                                goto end;
                            }
                    }
                    p_input[input_index++] = '\0';
                    status = eval_stmt(p_input);
                    if (status)
                    {
                        goto end;
                    }
                    printf("%s\n", prompt);
                    fflush(stdout);

                    fn_state = USCH_FN_START;
                    break;
                }
            default:
                {
                    if (fn_state == USCH_FN_START)
                    {
                        fn_state = USCH_FN_BEGIN;
                    }

                    p_input[input_index++] = c;
                    //printf("%c", c);
                    printf("%d", (unsigned int)c);
                    fflush(stdout);
                    break;
                }
        }
    }
end:
    printf("\nFIN!\n");
    getch();

    return 0;
}

