#include <string.h>
#include <malloc.h>

#include "crepl.h"
#include "crepl_debug.h"

#define INPUT_BUFFER_MAX 32676
#define INPUT_HISTORY_BUFFERS 15
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
    struct crepl_t *p_context = NULL;
    char *p_history_input[INPUT_HISTORY_BUFFERS] = {0};
    int history_index;
    char buf[PMCURSES_GETCH_BUFSZ] = {0};

    for (history_index = 0; history_index < INPUT_HISTORY_BUFFERS; history_index++)
    {
        p_history_input[history_index] = calloc(INPUT_BUFFER_MAX * sizeof(char), 1);
        FAIL_IF(p_history_input[history_index] == NULL);
    }

    history_index = 0;
    p_input = p_history_input[history_index];
    FAIL_IF(p_input == NULL);
    printf("%s", prompt);
    fflush(stdout);
    
    FAIL_IF(crepl_create(&p_context) != 0);
    FAIL_IF(crepl_pathhash(p_context) != 0);

    while ((c = pmcurses_getch(buf)) != EOF && c != CONTROL('d'))
    {
        switch (c)
        {
            case '\033':
                {
#if 0
                    FAIL_IF(d == EOF);
                    // ^A EMACS HOME
                    if (d == 'A')
                    {};
                    // ^E EMACS END
                    if (d == 'E')
                    {};
                    // ^C SIGINT 
                    if (d == 'C')
                    {};
                    FAIL_IF(e == EOF);

                    // BACK
                    if (d == '[' && e == 'D')
                    {};
                    // FORW
                    if (d == '[' && e == 'C')
                    {};
                    // UP
                    if (d == '[' && e == 'A')
                    {};
                    // DOWN
                    if (d == '[' && e == 'B')
                    {};
                    FAIL_IF(f == EOF);

                    // HOME
                    if (d == '[' && e == 'O' && f == 'H')
                    {};
                    // END
                    if (d == '[' && e == 'O' && f == 'F')
                    {};
#endif // 0
                    break;
                }

            case '(':
                {
                    fn_state = USCH_FN_BEGIN_PARAN;
                    printf("(");
                    fflush(stdout);
                    p_input[input_index++] = '(';
                    p_input[input_index+1] = '\0';
                    break;
                }
            case '\"':
                {
                    fn_state = USCH_FN_DIV;
                    printf("\"");
                    fflush(stdout);
                    p_input[input_index++] = '"';
                    p_input[input_index+1] = '\0';
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
                        pmcurses_backspace(1);
                        input_index--;
                    }
                    break;
                }
            case KEY_SPACE:
                {
                    if (crepl_is_cmd(p_context, p_input))
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
                                    p_input[input_index+1] = '\0';
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
                                    p_input[input_index+1] = '\0';

                                    fn_state = USCH_FN_DIV_CONT;
                                    break;
                                }
                            case USCH_FN_DISABLED:
                                {
                                    printf("\")");
                                    p_input[input_index++] = '"';
                                    p_input[input_index++] = ')';
                                    p_input[input_index+1] = '\0';
                                    break;
                                }
                            default:
                                {
                                    printf("%d\n\n", (unsigned int)fn_state);
                                    fflush(stdout);
                                    FAIL_IF(1);
                                }
                        }
                    }
                    else
                    {
                        printf(" ");
                        p_input[input_index++] = ' ';
                        p_input[input_index+1] = '\0';
                        fflush(stdout);
                    }
                    break;
                }
            case KEY_NEWLINE:
                {
                    if (crepl_is_cmd(p_context, p_input))
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
                                    p_input[input_index+1] = '\0';
                                    break;
                                }
                            case USCH_FN_DIV_CONT:
                                {
                                    printf(" ");
                                    printf(");\n");
                                    fflush(stdout);
                                    p_input[input_index++] = ')';
                                    p_input[input_index++] = ';';
                                    p_input[input_index+1] = '\0';
                                    break;
                                }
                            case USCH_FN_DIV:
                                {
                                    printf("\");\n");
                                    fflush(stdout);
                                    p_input[input_index++] = '"';
                                    p_input[input_index++] = ')';
                                    p_input[input_index++] = ';';
                                    p_input[input_index+1] = '\0';
                                    break;
                                }
                            default:
                                {
                                    FAIL_IF(1);
                                }
                        }
                    }
                    else
                    {
                        printf("\n");
                        fflush(stdout);
                    }

                    p_input[input_index++] = '\0';
                    if (strlen(p_input) > 0)
                    {
                        status = crepl_eval(p_context, p_input);
                    }
                    p_input[0] = '\0';
                    input_index = 0;
                    FAIL_IF(status < 0);
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
                    p_input[input_index+1] = '\0';
                    printf("%c", c);
                    fflush(stdout);
                    break;
                }
        }
    }
end:

    for (history_index = 0; history_index < INPUT_HISTORY_BUFFERS; history_index++)
    {
        free(p_history_input[history_index]);
    }
    crepl_destroy(p_context);
    free(p_input);
    return 0;
}

