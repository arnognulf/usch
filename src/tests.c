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

#include "usch.h"
#include "uschshell.h"
#include "../external/uthash/src/uthash.h"

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
    uschshell_t *p_context = NULL;
    error = uschshell_create(&p_context);
    mu_assert("error: uschshell_create(&p_context) != 0", error == 0);
    error = uschshell_define(p_context, sizeof(int_test), "int int_test", (void*)&int_test);
    mu_assert("error: uschshell_define(p_context, sizeof(int_test) != 0", error == 0);
    uschshell_undef(p_context, "int int_test");
    error = uschshell_load(p_context, "int int_test", (void*)&int_test);
    mu_assert("error: uschshell_load(p_context, \"int int_test\", (void*)&int_test) == 0", error != 0);

    error = uschshell_define(p_context, sizeof(int_test), "int int_test", (void*)&int_test);
    mu_assert("error: uschshell_define(p_context, sizeof(int_test) != 0", error == 0);

    error = uschshell_load(p_context, "int int_test", (void*)&int_test);
    mu_assert("error: uschshell_load(p_context, \"int int_test\", (void*)&int_test) == 0", error != 0);


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
    usch_test_vars_t *p_s, *p_tmp, *p_users = NULL;
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
        free(p_s);
    }
    return p_message;
}      
static char * all_tests()
{
    mu_run_test(test_strsplit);
    //mu_run_test(test_whereis);
    mu_run_test(test_usch_cmd);
    mu_run_test(test_usch_chdir);
    mu_run_test(test_uschshell_vars);
    mu_run_test(test_uthash);
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

