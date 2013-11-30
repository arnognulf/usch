/*
 * USCH - The (permutated) tcsh successor
 * Copyright (c) 2013 Thomas Eriksson 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "minunit.h"
#include <dlfcn.h>
#include <ctype.h>

#include "usch.h"
#include "usch_debug.h"
#include <editline/readline.h>
#include <locale.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "uschshell.h"
#include "usch_debug.h"
static uschshell_state_t state = USCHSHELL_STATE_CPARSER;

static struct uschshell_t *p_global_context = NULL;
static int xgetch() 
{
    int status = 0;
    // http://zaemis.blogspot.se/2011/06/reading-unicode-utf-8-by-hand-in-c.html
    /* mask values for bit pattern of first byte in 
     * multi-byte UTF-8 sequences: 
     *   192 - 110xxxxx - for U+0080 to U+07FF 
     *   224 - 1110xxxx - for U+0800 to U+FFFF 
     *   240 - 11110xxx - for U+010000 to U+1FFFFF */
    //static unsigned short mask[] = {192, 224, 240};
    struct termios old;
    char buf = '0';
    char **pp_cmds = NULL;
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

    if (buf == ' ')
    {
        fprintf(stderr, "\nxgetch() 1: state=%d rl_line_buffer=%s\n", (int)state, rl_line_buffer);
        FAIL_IF(uschshell_preparse(p_global_context, rl_line_buffer, &state, &pp_cmds));
        free(pp_cmds);
        fprintf(stderr, "\nxgetch() 2: state=%d rl_line_buffer=%s\n", (int)state, rl_line_buffer);
        if (state == USCHSHELL_STATE_CMDSTART)
        {
            char text[] = "(";
            rl_insert_text(text);
            rl_redisplay();
            buf = '\"'; 
        }
        else if (state == USCHSHELL_STATE_CMDARG)
        {
            char text[] = "\", ";
            rl_insert_text(text);
            rl_redisplay();
            buf = '\"'; 
        }
    } 
end:
    (void)status;
    return (int)buf;
}

static void initialize_readline()
{
   rl_readline_name = "usch_test";
   rl_getc_function = (*xgetch);
}

static char* stripwhite(char *string)
{
   char *p_s, *p_t;

   for (p_s = string; isspace(*p_s); p_s++)
   {
      *p_s = '\0';
   }

   if (*p_s == 0)
      return p_s;

   p_t = p_s + strlen(p_s) - 1;
   while (p_t > p_s && isspace(*p_t))
   {
       *p_t = '\0';
       p_t--;
   }
   //*++p_t = '\0';

   return p_s;
}

static void execute_line(struct uschshell_t *p_context, char *p_input)
{
    uschshell_eval(p_context, p_input);
}
static int prompt(struct uschshell_t *p_context)
{
    (void)p_context;
    int status = 0;
    char *p_line = NULL;
    setlocale(LC_CTYPE, "");

    initialize_readline();
    stifle_history(7);

    rl_already_prompted = 0;
    for (;;)
    {
        //size_t clear_len = 0;
        char *p_s = NULL;
        state = USCHSHELL_STATE_CPARSER;
        rl_initialize();
        rl_set_prompt("/* usch */ ");
        p_line = readline ("/* usch */ ");
        //clear_len = strlen(p_line);
        //memset(rl_line_buffer, clear_len, '\0');

        FAIL_IF(p_line == NULL);
        p_s = stripwhite(p_line);

        if (*p_s) {

            char* p_expansion = NULL;
            int result;

            result = history_expand(p_s, &p_expansion);

            if (result < 0 || result == 2) {
                fprintf(stderr, "%s\n", p_expansion);
            } else {
                add_history(p_expansion);
                execute_line(p_context, p_expansion);
            }
            free(p_expansion);
            p_expansion = NULL;
        }

        free(p_line);
        p_line = NULL;
    }
end:
    free(p_line);
    return status;
}

int main(int argc, char** p_argv)
{

    (void)p_argv;
    (void)argc;
    struct uschshell_t *p_uschshell = NULL;
    uschshell_create(&p_uschshell);
    p_global_context = p_uschshell;
    prompt(p_uschshell);
    uschshell_destroy(p_uschshell);
}

