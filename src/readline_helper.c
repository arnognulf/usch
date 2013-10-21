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

static int xgetch() {
#if 1
    // http://zaemis.blogspot.se/2011/06/reading-unicode-utf-8-by-hand-in-c.html
    /* mask values for bit pattern of first byte in 
     * multi-byte UTF-8 sequences: 
     *   192 - 110xxxxx - for U+0080 to U+07FF 
     *   224 - 1110xxxx - for U+0800 to U+FFFF 
     *   240 - 11110xxx - for U+010000 to U+1FFFFF */
    //static unsigned short mask[] = {192, 224, 240};
    struct termios old;
    char buf = '0';
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
    return (int)buf;
#else // 0
#endif // 0
    return 'x';
}
static void initialize_readline()
{
   /* Allow conditional parsing of the ~/.inputrc file. */
   rl_readline_name = "usch_test";
   
   rl_getc_function = (*xgetch);

   /* Tell the completer that we want a crack first. */
   //rl_attempted_completion_function = fileman_completion;
}

static char *stripwhite(char *string)
{
   char *s, *t;

   for (s = string; isspace(*s); s++)
      ;

   if (*s == 0)
      return s;

   t = s + strlen(s) - 1;
   while (t > s && isspace(*t))
      t--;
   *++t = '\0';

   return s;
}

static void execute_line(struct uschshell_t *p_context, char *p_input)
{
    uschshell_eval(p_context, p_input);
}
void prompt(struct uschshell_t *p_context)
{
    char *p_line = NULL;
    char *p_s = NULL;

    setlocale(LC_CTYPE, "");

    initialize_readline();
    stifle_history(7);

    /* Loop reading and executing lines until the user quits. */
    for ( ;; )
    {
        //char c = '\0';

        //c = xgetch();
        //rl_stuff_char(c);

        //rl_redisplay();
        p_line = readline ("/* usch */ ");

        if (!p_line)
            break;

        /* Remove leading and trailing whitespace from the line.
         *          Then, if there is anything left, add it to the history list
         *                   and execute it. */
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
        }

        free(p_line);
    }
}

int main(int argc, char** p_argv)
{

    (void)p_argv;
    (void)argc;
    struct uschshell_t *p_uschshell = NULL;
    uschshell_create(&p_uschshell);
    prompt(p_uschshell);
    uschshell_destroy(p_uschshell);
}

