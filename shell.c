#include <stdio.h>
#include <ncurses.h>
#include <term.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <dlfcn.h>

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

int eval_stmt(char *p_input_tmp)
{
    FILE *p_stmt_c = NULL;
    void *handle;
    int (*dyn_func)();
    char *error;
    char pre_fn[] = "#include <stdio.h>\nint dyn_func() {\n";
    size_t pre_fn_len = strlen(pre_fn);
    char post_fn[] = ";return 0;\n}\n";
    size_t post_fn_len = strlen(post_fn);
    size_t input_length;
    size_t bytes_written;
    char p_input[] = "printf(\"herro\")";

    input_length = strlen(p_input);
    p_stmt_c = fopen("stmt.c", "w+");
    if (p_stmt_c == NULL)
    {
        fprintf(stderr, "file open fail\n");
        goto error;
    }


    bytes_written = fwrite(pre_fn, 1, strlen(pre_fn), p_stmt_c);
    if (bytes_written != strlen(pre_fn))
    {
        fprintf(stderr, "write error 1: %d != %d\n", bytes_written, strlen(pre_fn) + 1);
        
        goto error;
    }
    bytes_written = fwrite(p_input, 1, strlen(p_input), p_stmt_c);
    if (bytes_written != strlen(p_input))
    {
        fprintf(stderr, "write error 2\n");
        goto error;
    }
    bytes_written = fwrite(post_fn, 1, strlen(post_fn), p_stmt_c);
    if (bytes_written != strlen(post_fn))
    {
        fprintf(stderr, "write error 3\n");
        goto error;
    }
    fclose(p_stmt_c);
    if (system("gcc -rdynamic -shared -fPIC -o ./stmt stmt.c") != 0) 
    {
        
        fprintf(stderr, "compile error\n");
        goto error;
    }

    handle = dlopen("./stmt", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        goto error;
    }

    dlerror();

    *(void **) (&dyn_func) = dlsym(handle, "dyn_func");

    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        goto error;
    }

    printf("%d\n", (*dyn_func)());
    dlclose(handle);

    return 0;
error:
    return -1;
}

int main(void)
{
    int status = 0;
    char *p_input = NULL;
    int input_index = 0;
    char prompt[] = "/* usch */ ";
    int fn_startpos = 0;
    int c;
    int col, row;
    SCREEN *p_screen = newterm(NULL, stdin, stdout);
    USCH_FN_STATE fn_state = USCH_FN_START;

    if (p_screen == 0)
        return(-1);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    p_input = malloc(INPUT_BUFFER_MAX);
    if (p_input == NULL)
    {
        goto end;
    }
    printw("%s", prompt);

    while ((c = getch()) != EOF && c != CONTROL('d'))
    {
        switch (c)
        {
            case '(':
                {
                    fn_state = USCH_FN_BEGIN_PARAN;
                    printw("(");
                    p_input[input_index++] = '(';
                    break;
                }
            case '\"':
                {
                    fn_state = USCH_FN_DIV;
                    printw("\"");
                    p_input[input_index++] = '"';
                    break;
                }
            case KEY_BACKSPACE /* backspace */:
                {
                    getyx(stdscr, col, row);
                    row = MAX(strlen(prompt), row - 1);
                    move(col, row);
                    printw(" ");
                    move(col, row);
                    // TODO: fix backspace here
                    input_index--;
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
                                p_input[input_index++] = '(';
                                p_input[input_index++] = '"';
                                fn_state = USCH_FN_DIV;
                                break;
                            }
                        case USCH_FN_DIV:
                        case USCH_FN_DIV_CONT:
                            {
                                printw("\", \"", c);
                                p_input[input_index++] = '"';
                                p_input[input_index++] = ',';
                                p_input[input_index++] = '"';

                                fn_state = USCH_FN_DIV_CONT;
                                break;
                            }
                        case USCH_FN_DISABLED:
                            {
                                printw("\")", c);
                                p_input[input_index++] = '"';
                                p_input[input_index++] = ')';
                                break;
                            }
                        default:
                            {
                                printw("%d\n\n", (unsigned int)fn_state);
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
                                printw("\n%s", prompt);
                                break;
                            }
                        case USCH_FN_BEGIN:
                            {
                                printw("();\n%s", prompt);
                                p_input[input_index++] = '(';
                                p_input[input_index++] = ')';
                                p_input[input_index++] = ';';
                                break;
                            }
                        case USCH_FN_DIV:
                            {
                                getyx(stdscr, col, row);
                                row = MAX(strlen(prompt), row - 1);
                                move(col, row);
                                printw(" ");
                                move(col, row);
                                printw(");\n");
                                p_input[input_index++] = ')';
                                p_input[input_index++] = ';';
                                break;
                            }
                        case USCH_FN_DIV_CONT:
                            {
                                printw("\");\n%s", prompt);
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
                    printw("%s\n", prompt);

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
                    printw("%c", c);
                    break;
                }
        }
    }
end:
    printw("\nFIN!\n");
    getch();
    endwin();

    return 0;
}

