#include <assert.h>
#include <string.h>
#include <malloc.h>

#include "pmcurses.h"
#include "uschshell.h"

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
    int c;
    USCH_FN_STATE fn_state = USCH_FN_START;

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
                    if (input_index > 0)
                    {
                        backspace(1);
                        input_index--;
                    }
                    break;
                }
            case KEY_SPACE:
                {
                    switch (fn_state)
                    {
                        case USCH_FN_START:
                            printf(" ");
                            fflush(stdout);
                            break;
                        case USCH_FN_BEGIN:
                            {
                                printf("(\"");
                                fflush(stdout);
                                p_input[input_index++] = '(';
                                p_input[input_index++] = '"';
                                fn_state = USCH_FN_DIV;
                                break;
                            }
                        case USCH_FN_DIV:
                        case USCH_FN_DIV_CONT:
                            {
                                printf("\", \"");
                                fflush(stdout);
                                p_input[input_index++] = '"';
                                p_input[input_index++] = ',';
                                p_input[input_index++] = '"';

                                fn_state = USCH_FN_DIV_CONT;
                                break;
                            }
                        case USCH_FN_DISABLED:
                            {
                                printf("\")");
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
            case KEY_NEWLINE:
                {
                    switch (fn_state)
                    {
                        case USCH_FN_START:
                            {
                                printf("\n");
                                fflush(stdout);
                                break;
                            }
                        case USCH_FN_BEGIN:
                            {
                                printf("();\n");
                                fflush(stdout);
                                p_input[input_index++] = '(';
                                p_input[input_index++] = ')';
                                p_input[input_index++] = ';';
                                break;
                            }
                        case USCH_FN_DIV_CONT:
                            {
                                printf(" ");
                                printf(");\n");
                                fflush(stdout);
                                p_input[input_index++] = ')';
                                p_input[input_index++] = ';';
                                break;
                            }
                        case USCH_FN_DIV:
                            {
                                printf("\");\n");
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
                    if (strlen(p_input) > 0)
                    {
                        status = uschshell_eval(NULL, p_input);
                    }
                    p_input[0] = '\0';
                    input_index = 0;
                    if (status)
                    {
                        goto end;
                    }
                    printf("%s", prompt);
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
                    printf("%c", c);
                    fflush(stdout);
                    break;
                }
        }
    }
end:
    free(p_input);
    return 0;
}

