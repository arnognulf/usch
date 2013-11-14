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
#include "parserutils.h"

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
static char *test_uschshell_parse()
{
    int status;
    char *p_message = NULL;
    int error;
    int num_ids = 0;
    char **pp_ids = NULL;
    struct uschshell_t *p_context = NULL;
    uschshell_state_t state = USCHSHELL_STATE_CPARSER;
    error = uschshell_create(&p_context);
    mu_assert("error != 0", error == 0);

    get_identifiers("a_1", &num_ids, &pp_ids);
    mu_assert("get_identifiers(..) != 1", num_ids == 1);
    mu_assert("strcmp(..) != a_1", strcmp(pp_ids[0], "a_1") == 0);
    free(pp_ids);
    pp_ids = NULL;
    num_ids = get_identifiers("2f", &num_ids, &pp_ids);
    mu_assert("get_identifiers(..) != 0", num_ids == 0);
    pp_ids = NULL;
    status = get_identifiers(" = bc()", &num_ids, &pp_ids);
    mu_assert("get_identifiers(..) != 1", num_ids == 1);
    mu_assert("strcmp(..) != bc", strcmp(pp_ids[0], "bc") == 0);
    get_identifiers("c3->fa", &num_ids, &pp_ids);
    mu_assert("get_identifiers(..) != 1", num_ids == 1);
    mu_assert("strcmp(..) = c3", strcmp(pp_ids[0], "c3") == 0);
    status = get_identifiers("g.h", &num_ids, &pp_ids);
    mu_assert("get_identifiers(..) != 1", num_ids == 1);
    mu_assert("strcmp(..) = g", strcmp(pp_ids[0], "g") == 0);
    free(pp_ids);
    status = get_identifiers("\"test\"", &num_ids, &pp_ids);
    mu_assert("get_identifiers(..) != 0", num_ids == 0);
    free(pp_ids);
    pp_ids = NULL;
    status = get_identifiers("\'x\'", &num_ids, &pp_ids);
    mu_assert("get_identifiers(..) != 0", num_ids == 0);
    free(pp_ids);
    pp_ids = NULL;

    status = get_identifiers("a1 2f = bc(); c3->f4 g.h", &num_ids, &pp_ids);
    mu_assert("get_identifiers(..) != 4", num_ids == 4);
    mu_assert("strcmp(..) = a1", strcmp(pp_ids[0], "a1") == 0);
    mu_assert("strcmp(..) = bc", strcmp(pp_ids[1], "bc") == 0);
    mu_assert("strcmp(..) = c3", strcmp(pp_ids[2], "c3") == 0);
    mu_assert("strcmp(..) = g", strcmp(pp_ids[3], "g") == 0);
    mu_assert("strcmp(..) = NULL", pp_ids[4] == NULL);
    free(pp_ids);
    pp_ids = NULL;
    //(void)state;
    error = uschshell_preparse(p_context, "ls", &state);
    printf("state: %d\n", (int)state);
    mu_assert("error != 0", error == 0);
    mu_assert("uschshell_preparse(p_context, \"ls\", &state) != USCHSHELL_STATE_CMDSTART", state == USCHSHELL_STATE_CMDSTART);
cleanup:
    (void)status;
    uschshell_destroy(p_context);
    return p_message;
}

static char *test_parserutils()
{
    char *p_message = NULL;
    char id[] = "1 + id";
    char nonid[] = "1 + 2f";
    char prespace[] = "   xyz";
    char postspace[] = "xyz ";
    char nospace[] = "ls";
    mu_assert("error identifier_pos(\"1 + id\")", id[identifier_pos(id)] == 'i');
    mu_assert("error identifier_pos(\"1 + 2f\")", nonid[identifier_pos(nonid)] == '\0');
    mu_assert("error stripwhite()", strcmp(stripwhite(prespace), "xyz") == 0);
    mu_assert("error stripwhite()", strcmp(stripwhite(postspace), "xyz") == 0);
    mu_assert("error stripwhite()", strcmp(stripwhite(nospace), "ls") == 0);
    mu_assert("error stripwhite()", stripwhite(postspace)[4] == '\0');

cleanup:
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

static char * all_tests()
{
    mu_run_test(test_strsplit);
    mu_run_test(test_usch_cmd);
    mu_run_test(test_usch_chdir);
    mu_run_test(test_uthash);
    mu_run_test(test_uthash1);
    mu_run_test(test_uschshell_vars);
    mu_run_test(test_uschshell_dyld);
    mu_run_test(test_parserutils);
    mu_run_test(test_uschshell_parse);
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

