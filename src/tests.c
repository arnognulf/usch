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
#include "clang-c/Index.h"
#include <dlfcn.h>
#include <ctype.h>
#include "../external/uthash/src/uthash.h"

#include "usch.h"
#include "uschshell.h"
#include "usch_debug.h"
#include "pmcurses.h"
#include <editline/readline.h>
#include <locale.h>

int tests_run = 0;
static char * test_strsplit() {
    char *p_message = NULL;
    char **pp_out = NULL;
    char *p_test1 = "foo bar baz";
    int num_items = 0;
    num_items = usch_strsplit(p_test1, " ", &pp_out);
    mu_assert("num_items > 0", num_items > 0);
    mu_assert("error: pp_out[0] != foo", strcmp(pp_out[0], "foo") == 0);
    mu_assert("error: pp_out[1] != bar", strcmp(pp_out[1], "bar") == 0);
    mu_assert("error: pp_out[2] != baz", strcmp(pp_out[2], "baz") == 0);
    mu_assert("error: pp_out[3] != NULL", pp_out[3] == NULL);
    free(pp_out);
    return 0;
cleanup:
    free(pp_out);
    return p_message;
}

#if 0
static char * test_strexp()
{
    return "not implemented";
}

static char * test_cd()
{
    return "not implemented";
}
static char * test_whereis()
{
    char *p_message = NULL;
    char *p_dest = NULL;
    int status = usch_whereis("sh", &p_dest);
    mu_assert("error: usch_whereis(\"sh\", &p_dest) < 1", status > 0);
    mu_assert("error: usch_whereis(\"sh\", &p_dest), p_dest != \"/bin/sh\"", strcmp(p_dest, "/bin/sh") == 0);
    free(p_dest);
    p_dest = NULL;
    status = usch_whereis("/bin/sh", &p_dest);
    mu_assert("error: usch_whereis(\"sh\", &p_dest) < 1", status > 0);
    mu_assert("error: usch_whereis(\"/bin/sh\", &p_dest), p_dest != \"/bin/sh\"", strcmp(p_dest, "/bin/sh") == 0);
    free(p_dest);
    p_dest = NULL;

    return NULL;
cleanup:
    free(p_dest);
    return p_message;
}
#endif // 0

#define test_num_args(...) usch_cmd("./test_num_args", ##__VA_ARGS__)

static char *test_usch_cmd()
{
    char *p_message = NULL;
    int num_args;
    num_args = test_num_args("testdir/*");
    mu_assert("error: test_num_args(\"testdir/*\") != 4", num_args == 4);

    num_args = test_num_args("arg1");
    mu_assert("error: test_num_args(\"arg1\") != 2", num_args == 2);

    num_args = test_num_args("arg1", "arg2");
    mu_assert("error: test_num_args(\"arg1\", \"arg2\") != 3", num_args == 3);

    num_args = test_num_args("arg1", "arg2", "arg3");
    mu_assert("error: test_num_args(\"arg1\", \"arg2\", \"arg3\") != 4", num_args == 4);

    return NULL;
cleanup:
    return p_message;
}

#define test_usch_pwd(...) usch_cmd("pwd", ##__VA_ARGS__)

static char *test_usch_chdir()
{
    char *p_message = NULL;
    int error;
    error = cd(".");
    mu_assert("error: cd(\".\") != 0", error == 0);
    error = test_usch_pwd();
    mu_assert("error: test_usch_pwd() != 0", error == 0);
    return NULL;
cleanup:
    return p_message;
}
static char *test_uschshell_vars()
{
    char *p_message = NULL;
    int error;
    int int_test;
    float float_test;
    double double_test;
    char char_test;
    char right_str[] = "right";
    char wrong_str[] = "wrong";
    char *p_str = NULL;
    struct uschshell_t *p_context = NULL;

    error = uschshell_create(&p_context);
    mu_assert("error: uschshell_create(&p_context) != 0", error == 0);
    error = uschshell_define(p_context, sizeof(int_test), "int int_test");
    mu_assert("error: uschshell_define(p_context, sizeof(int_test) != 0", error == 0);
    uschshell_undef(p_context, "int int_test");
    error = uschshell_load(p_context, "int int_test", (void*)&int_test);
    mu_assert("error: uschshell_load(p_context, \"int int_test\", (void*)&int_test) == 0", error != 0);

    error = uschshell_define(p_context, sizeof(int_test), "int int_test");
    mu_assert("error: uschshell_define(p_context, sizeof(int_test) != 0", error == 0);

    int_test = 42;
    error = uschshell_store(p_context, "int int_test", (void*)&int_test);
    mu_assert("error: uschshell_store(p_context, \"int int_test\", (void*)&int_test) != 0", error == 0);

    int_test = 994;

    error = uschshell_load(p_context, "int int_test", (void*)&int_test);
    mu_assert("error: uschshell_load(p_context, \"int int_test\", (void*)&int_test) != 0", error == 0);
    mu_assert("error: int_test != 42", int_test == 42);

    float_test = 3.14f;
    uschshell_define(p_context, sizeof(float_test), "float float_test");
    uschshell_store(p_context, "float float_test", (void*)&float_test);
    float_test = 994.0f;
    uschshell_load(p_context, "float float_test", (void*)&float_test);
    mu_assert("error: float_test != 3.14", float_test == 3.14f);

    p_str = right_str;
    uschshell_define(p_context, sizeof(p_str), "char *p_str");
    uschshell_store(p_context, "char *p_str", (void*)&p_str);
    p_str = wrong_str;
    uschshell_load(p_context, "char *p_str", (void*)&p_str);
    mu_assert("error: p_str != right_str", strcmp(p_str, right_str) == 0);

    double_test = 3.14;
    uschshell_define(p_context, sizeof(double_test), "double double_test");
    uschshell_store(p_context, "double double_test", (void*)&double_test);
    double_test = 994.0f;
    uschshell_load(p_context, "double double_test", (void*)&double_test);
    mu_assert("error: double_test != 3.14", double_test == 3.14);

    char_test = 'a';
    uschshell_define(p_context, sizeof(char_test), "char char_test");
    uschshell_store(p_context, "char char_test", (void*)&char_test);
    char_test = 'f';
    uschshell_load(p_context, "char char_test", (void*)&char_test);
    mu_assert("error: char_test != 3.14", char_test == 'a');


    uschshell_destroy(p_context);
    return NULL;
cleanup:
    uschshell_destroy(p_context);
    return p_message;
}
static char *test_uschshell_dyld()
{
    char *p_message = NULL;
    int error = 0;
    struct uschshell_t *p_context = NULL;

    error = uschshell_create(&p_context);
    mu_assert("error: uschshell_create(&p_context) != 0", error == 0);
#ifdef __x86_64__
    error = uschshell_lib(p_context, "/usr/lib/x86_64-linux-gnu/libm.so"); // "m"
#else
    error = uschshell_lib(p_context, "/usr/lib/i386-linux-gnu/libm.so");
#endif // 0
    mu_assert("error: uschshell_lib() != 0", error == 0);
    error = uschshell_include(p_context, "<math.h>");
    //error = uschshell_include(p_context, "<xmmintrin.h>");
    mu_assert("error: uschshell_include(p_context, \"<math.h>\") != 0", error == 0);

    uschshell_destroy(p_context);
    return NULL;
cleanup:
    uschshell_destroy(p_context);
    return p_message;
}

typedef struct usch_test_vars_t {
    int id;
    UT_hash_handle hh;
    char name[];
} usch_test_vars_t;

static char *test_uthash()
{
    char *p_message = NULL;
    char **n, *names[] = { "joe", "bob", "betty", NULL };
    usch_test_vars_t *p_s = NULL;
    usch_test_vars_t *p_tmp = NULL;
    usch_test_vars_t *p_users = NULL;
    int i=0;

    for (n = names; *n != NULL; n++) {
        p_s = (usch_test_vars_t*)calloc(sizeof(usch_test_vars_t) + strlen(*n) + 1,1);
        strncpy(p_s->name, *n,strlen(*n));
        p_s->id = i++;
        HASH_ADD_STR( p_users, name, p_s );
    }

    HASH_FIND_STR(p_users, "betty", p_s);
    mu_assert("error: HASH_FIND_STR(users, \"betty\", p_s) != NULL", p_s != NULL);

cleanup:

    /* free the hash table contents */
    HASH_ITER(hh, p_users, p_s, p_tmp) {
        HASH_DEL(p_users, p_s);
    }
    return p_message;
}
static char *test_uthash1()
{
    char *p_message = NULL;
    char **n, *names[] = { "mccleoad", NULL };
    usch_test_vars_t *p_s = NULL;
    usch_test_vars_t *p_tmp = NULL;
    usch_test_vars_t *p_users = NULL;
    int i=0;
    for (n = names; *n != NULL; n++) {
        p_s = (usch_test_vars_t*)calloc(sizeof(usch_test_vars_t) + strlen(*n) + 1,1);
        strncpy(p_s->name, *n, strlen(*n));
        p_s->id = i++;
        HASH_ADD_STR( p_users, name, p_s );
    }

    HASH_FIND_STR(p_users, "mccleoad", p_s);

    mu_assert("error: HASH_FIND_STR(users, \"mccleoad\", p_s) != NULL", p_s != NULL);

cleanup:

    /* free the hash table contents */
    HASH_ITER(hh, p_users, p_s, p_tmp) {
        HASH_DEL(p_users, p_s);
    }
    return p_message;
}
#if 0
void initialize_readline ()
{
   /* Allow conditional parsing of the ~/.inputrc file. */
   rl_readline_name = "usch_test";

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

static void execute_line(char *p_expansion)
{
    printf("%s\n", p_expansion);
}
#endif // 0

#if 0
static char *test_editline()
{
    char *p_message = NULL;
    char *p_line = NULL;
    char *p_s = NULL;

    setlocale(LC_CTYPE, "");

    initialize_readline();
    stifle_history(7);

   /* Loop reading and executing lines until the user quits. */
   for ( ;; )
   {
      p_line = readline ("/* usch */ ");

      if (!p_line)
         break;

      /* Remove leading and trailing whitespace from the line.
         Then, if there is anything left, add it to the history list
         and execute it. */
      p_s = stripwhite(p_line);

      if (*p_s) {

         char* p_expansion = NULL;
         int result;

         result = history_expand(p_s, &p_expansion);

         if (result < 0 || result == 2) {
            fprintf(stderr, "%s\n", p_expansion);
         } else {
            add_history(p_expansion);
            execute_line(p_expansion);
         }
         free(p_expansion);
      }

      free(p_line);
   }

    mu_assert("error: HASH_FIND_STR(users, \"mccleoad\", p_s) != NULL", p_s != NULL);

cleanup:

    return p_message;
}
static char *test_pmcurses()
{
    char *p_message = NULL;
    int status = 0;
    pmcurses_t *p_pmcurses = NULL;
    char *p_str;
    char *p_tok;
    char *p_line = NULL;

    p_line = calloc(256, 1);
    mu_assert("error: calloc() == 0", p_line != 0);

    status = pmcurses_create(&p_pmcurses);
    mu_assert("error: pmcurses_create(&p_pmcurses) != 0", status == 0);
    mu_assert("error: p_pmcurses == NULL", p_pmcurses != NULL);
    status = pmcurses_insert(p_pmcurses, "a");
    status = pmcurses_insert(p_pmcurses, "bb");
    status = pmcurses_insert(p_pmcurses, "ccc");
    mu_assert("error: pmcurses_insert(p_pmcurses) != 0", status == 0);
    p_str = pmcurses_gettok(p_pmcurses, 0);
    mu_assert("error: strcmp(p_str, \"a\") == 0", strcmp(p_str, "a") == 0);
    p_str = pmcurses_gettok(p_pmcurses, 1);
    mu_assert("error: pmcurses_gettok(p_pmcurses, 0) == NULL", p_str != NULL);
    mu_assert("error: strcmp(p_str, \"bb\") == 0", strcmp(p_str, "bb") == 0);
    p_str = pmcurses_gettok(p_pmcurses, 2);
    mu_assert("error: strcmp(p_str, \"ccc\") == 0", strcmp(p_str, "ccc") == 0);
    status = pmcurses_writeline(p_pmcurses, p_line);
    mu_assert("error: strcmp(p_line, \"abbccc\") != 0", strcmp(p_line, "abbccc") == 0);
    status = pmcurses_draw(p_pmcurses, 10, 10);
    printf("\n");
    mu_assert("error: pmcurses_draw(p_pmcurses, 10, 10) != 0", status == 0);
    status = pmcurses_back(p_pmcurses);
    status = pmcurses_insert(p_pmcurses, "dddd");
    status = pmcurses_writeline(p_pmcurses, p_line);
    status = pmcurses_draw(p_pmcurses, 10, 10);
    printf("\n");
    mu_assert("error: strcmp(p_line, \"abbddddccc\") != 0", strcmp(p_line, "abbddddccc") == 0);
    pmcurses_back(p_pmcurses);
    printf("\n");
    status = pmcurses_draw(p_pmcurses, 10, 10);
    p_tok = pmcurses_getcurtok(p_pmcurses);
    printf("%s\n", p_tok);
    mu_assert("error: strcmp(p_line, \"abbddddccc\") != 0", strcmp(p_tok, "dddd") == 0);

cleanup:

    free(p_line);
    return p_message;
}
static char *test_input()
{
    char *p_message = NULL;
    int status = 0;
    pmcurses_t *p_pmcurses = NULL;
    char *p_line = NULL;
    char c = 0;
    char buf[5] = {0};

    p_line = calloc(256, 1);
    mu_assert("error: calloc() == 0", p_line != 0);

    status = pmcurses_create(&p_pmcurses);
    mu_assert("error: pmcurses_create(&p_pmcurses) != 0", status == 0);
    mu_assert("error: p_pmcurses == NULL", p_pmcurses != NULL);
    while ((c = pmcurses_getch(buf)) != EOF)
    {
        switch (pmcurses_parsekey(buf))
        {
            case PMCURSES_PRINTABLE:
                {
                    pmcurses_insert(p_pmcurses, buf);
                }
                break;
            case PMCURSES_BACK:
                {
                    pmcurses_back(p_pmcurses);
                }
                break;
            case PMCURSES_FORWARD:
                {
                    pmcurses_forward(p_pmcurses);
                }
                break;
            default:
                break;
        }
        pmcurses_draw(p_pmcurses, 124, 28);
    }
cleanup:

    free(p_line);
    return p_message;
}
#endif // 0

static char * all_tests()
{
    mu_run_test(test_strsplit);
    mu_run_test(test_usch_cmd);
    mu_run_test(test_usch_chdir);
    mu_run_test(test_uthash);
    mu_run_test(test_uthash1);
    mu_run_test(test_uschshell_vars);
    mu_run_test(test_uschshell_dyld);
    //mu_run_test(test_pmcurses);
    //mu_run_test(test_input);
//    mu_run_test(test_editline);
    return 0;
}

int main()
{
     char *result = all_tests();
     if (result != 0)
     {
         printf("\033[31m EPIC FAIL!\033[0m %s\n", result);
     }
     else
     {
         printf("\033[32m A WINNER IS YOU!\033[0m ALL TESTS PASSED\n");
     }
     printf(" Tests run: %d\n", tests_run);
 
     return result != 0;
 }

