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
#include <locale.h>                     // for NULL
#include <stdio.h>                      // for printf
#include <stdlib.h>                     // for free
#include <string.h>                     // for strcmp
#include "../external/uthash/src/uthash.h"  // for UT_hash_handle
#include "crepl.h"                      // for crepl_define, crepl_load, etc
#include "crepl_parser.h"               // for stripwhite, identifier_pos
#include "minunit.h"                    // for mu_assert, mu_run_test
#include "../usch_h/usch.h"             // for ucmd, uclear, etc

int tests_run = 0;
static char * test_strsplit() {
    char *p_message = NULL;
    char *p_test1 = "foo bar baz";
    ustash s = {NULL};
    {
        char **pp_out = NULL;
        pp_out = ustrsplit(&s, p_test1, " ");
        mu_assert("error: pp_out[0] != foo", strcmp(pp_out[0], "foo") == 0);
        mu_assert("error: pp_out[1] != bar", strcmp(pp_out[1], "bar") == 0);
        mu_assert("error: pp_out[2] != baz", strcmp(pp_out[2], "baz") == 0);
        mu_assert("error: pp_out[3] != NULL", pp_out[3] == NULL);
    }
cleanup:
    uclear(&s);
    return p_message;
}

static char * test_strexp()
{
    char *p_message = NULL;
    ustash s = {NULL};

    {
        char **pp_out = ustrexp(&s, "foo", "bar", "baz");
        mu_assert("error: pp_out[0] != foo", strcmp(pp_out[0], "foo") == 0);
        mu_assert("error: pp_out[1] != bar", strcmp(pp_out[1], "bar") == 0);
        mu_assert("error: pp_out[2] != baz", strcmp(pp_out[2], "baz") == 0);
        mu_assert("error: pp_out[3] != NULL", pp_out[3] == NULL);
    }
    {
        char **pp_out = ustrexp(&s, "CMakeCa*.txt");
        mu_assert("error: pp_out[0] != CMakeCache.txt", strcmp(pp_out[0], "CMakeCache.txt") == 0);
        mu_assert("error: pp_out[1] != NULL", pp_out[1] == NULL);
    }
cleanup: 
    uclear(&s);
    return p_message;
}

#if 0
static char * test_whereis()
{
    char *p_message = NULL;
    char *p_dest = NULL;
    int status = uwhereis("sh", &p_dest);
    mu_assert("error: uwhereis(\"sh\", &p_dest) < 1", status > 0);
    mu_assert("error: uwhereis(\"sh\", &p_dest), p_dest != \"/bin/sh\"", strcmp(p_dest, "/bin/sh") == 0);
    free(p_dest);
    p_dest = NULL;
    status = uwhereis("/bin/sh", &p_dest);
    mu_assert("error: uwhereis(\"sh\", &p_dest) < 1", status > 0);
    mu_assert("error: uwhereis(\"/bin/sh\", &p_dest), p_dest != \"/bin/sh\"", strcmp(p_dest, "/bin/sh") == 0);
    free(p_dest);
    p_dest = NULL;

    return NULL;
cleanup:
    free(p_dest);
    return p_message;
}
#endif // 0

#define test_num_args(...) ucmd("test/test_num_args", ##__VA_ARGS__)

static char *test_ucmd()
{
    char *p_message = NULL;
    int num_args;
    num_args = test_num_args("../test/testdir/*");
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

#define test_ls(...) ucmd("ls", ##__VA_ARGS__)

static char *test_ucmd_pipe()
{
    char *p_message = NULL;
    int num_args;
    num_args = test_ls("../test/testdir/*", "|", "wc", "-l");
    mu_assert("error: test_num_args(\"testdir/*\") != 4", num_args == 0);

    return NULL;
cleanup:
    return p_message;
}

#define test_upwd(...) ucmd("pwd", ##__VA_ARGS__)

static char *test_uchdir()
{
    char *p_message = NULL;
    int error;
    error = ucmd("cd", ".");
    mu_assert("error: cd(\".\") != 0", error == 0);
    error = test_upwd();
    mu_assert("error: test_upwd() != 0", error == 0);
    return NULL;
cleanup:
    return p_message;
}
static char *test_ustrout()
{
    char *p_message = NULL;
    char *p_str = NULL;
    ustash s = {NULL};

    p_str = ustrout(&s, "echo", "foo");
    mu_assert("error: ustrout(\"echo\", \"foo\") != 0", p_str != NULL);
    mu_assert("error: ustrout(\"echo\", \"foo\") != foo", strcmp(p_str, "foo") == 0);
    uclear(&s);
cleanup:
    return p_message;
}

static char *test_udirname()
{
    char *p_message = NULL;
    char *p_str = NULL;
    ustash s = {NULL};

    p_str = udirname(&s, "/foo/bar/baz.txt");
    mu_assert("error: udirname(\"/foo/bar/baz.txt\",) != 0", strcmp(p_str, "/foo/bar") == 0);

    p_str = udirname(&s, "/foo");
    mu_assert("error: udirname(\"/foo\",) != 0", strcmp(p_str, "/") == 0);

    p_str = udirname(&s, "foo/bar/baz.txt");
    mu_assert("error: udirname(\"foo/bar/baz.txt\",) != 0", strcmp(p_str, "foo/bar") == 0);

    p_str = udirname(&s, "baz.txt");
    mu_assert("error: udirname(\"baz.txt\",) != 0", strcmp(p_str, ".") == 0);

    uclear(&s);

cleanup:
    return p_message;
}

static char *test_ustrneq()
{
    char *p_message = NULL;

    mu_assert("error: ustrneq(\"FOO\", \"FOOBAR\"", ustrneq("FOO", "FOOBAR", strlen("FOO")) == USCH_TRUE);
    mu_assert("error: ustrneq(\"FOO\", \"BAR\"", ustrneq("FOO", "BAR", strlen("FOO")) == USCH_FALSE);
    mu_assert("error: ustrneq(\"FOO\", \"FOOBAR\", strlen(\"FOOBAR\")", ustrneq("FOO", "FOOBAR", strlen("FOOBAR")) == USCH_FALSE);
    mu_assert("error: ustrneq(\"FOO\", NULL, strlen(\"FOOBAR\")", ustrneq("FOO", NULL, strlen("FOOBAR")) == USCH_FALSE);
    mu_assert("error: ustrneq(NULL, \"FOO\", strlen(\"FOOBAR\")", ustrneq(NULL, "FOO", strlen("FOOBAR")) == USCH_FALSE);
cleanup:
    return p_message;
}


static char *test_ustrtrim()
{
    char *p_message = NULL;
    char *p_str = NULL;
    ustash s = {NULL};

    p_str = ustrtrim(&s, "   abc   ");
    mu_assert("error: ustrtrim(\"   abc   \",) != 0", strcmp(p_str, "abc") == 0);
    uclear(&s);
cleanup:
    return p_message;
}


static char *test_crepl_vars()
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
    struct crepl *p_context = NULL;
    crepl_options options;
    memset(&options, 0, sizeof(crepl_options));

    mu_assert("error: crepl_create(&p_context) != 0", CREPL_OK(crepl_create(&p_context, options)));
    error = crepl_define(p_context, sizeof(int_test), "int int_test");
    mu_assert("error: crepl_define(p_context, sizeof(int_test) != 0", error == 0);
    crepl_undef(p_context, "int int_test");
    error = crepl_load(p_context, "int int_test", (void*)&int_test);
    mu_assert("error: crepl_load(p_context, \"int int_test\", (void*)&int_test) == 0", error != 0);

    error = crepl_define(p_context, sizeof(int_test), "int int_test");
    mu_assert("error: crepl_define(p_context, sizeof(int_test) != 0", error == 0);

    int_test = 42;
    error = crepl_store(p_context, "int int_test", (void*)&int_test);
    mu_assert("error: crepl_store(p_context, \"int int_test\", (void*)&int_test) != 0", error == 0);

    int_test = 994;

    error = crepl_load(p_context, "int int_test", (void*)&int_test);
    mu_assert("error: crepl_load(p_context, \"int int_test\", (void*)&int_test) != 0", error == 0);
    mu_assert("error: int_test != 42", int_test == 42);

    float_test = 3.14f;
    crepl_define(p_context, sizeof(float_test), "float float_test");
    crepl_store(p_context, "float float_test", (void*)&float_test);
    float_test = 994.0f;
    crepl_load(p_context, "float float_test", (void*)&float_test);
    mu_assert("error: float_test != 3.14", float_test == 3.14f);

    p_str = right_str;
    crepl_define(p_context, sizeof(p_str), "char *p_str");
    crepl_store(p_context, "char *p_str", (void*)&p_str);
    p_str = wrong_str;
    crepl_load(p_context, "char *p_str", (void*)&p_str);
    mu_assert("error: p_str != right_str", strcmp(p_str, right_str) == 0);

    double_test = 3.14;
    crepl_define(p_context, sizeof(double_test), "double double_test");
    crepl_store(p_context, "double double_test", (void*)&double_test);
    double_test = 994.0f;
    crepl_load(p_context, "double double_test", (void*)&double_test);
    mu_assert("error: double_test != 3.14", double_test == 3.14);

    char_test = 'a';
    crepl_define(p_context, sizeof(char_test), "char char_test");
    crepl_store(p_context, "char char_test", (void*)&char_test);
    char_test = 'f';
    crepl_load(p_context, "char char_test", (void*)&char_test);
    mu_assert("error: char_test != 3.14", char_test == 'a');

cleanup:
    crepl_destroy(p_context);
    return p_message;
}
static char *test_crepl_dyld()
{
    char *p_message = NULL;
    int error = 0;
    struct crepl *p_context = NULL;
    crepl_options options;
    memset(&options, 0, sizeof(crepl_options));

    mu_assert("error: crepl_create(&p_context) != 0", CREPL_OK(crepl_create(&p_context, options)));
#ifdef __x86_64__
    error = crepl_lib(p_context, "/usr/lib/x86_64-linux-gnu/libm.so"); // "m"
#else
    error = crepl_lib(p_context, "/usr/lib/i386-linux-gnu/libm.so");
#endif // 0
    mu_assert("error: crepl_lib() != 0", error == 0);
    error = crepl_include(p_context, "<math.h>");
    mu_assert("error: crepl_include(p_context, \"<math.h>\") != 0", error == 0);
    crepl_destroy(p_context);
    p_context = NULL;

    error = crepl_create(&p_context, options);
    error = crepl_lib(p_context, "m");
    mu_assert("error: crepl_lib() != 0", error == 0);
    error = crepl_include(p_context, "<math.h>");
    mu_assert("error: crepl_include(p_context, \"<math.h>\") != 0", error == 0);

    crepl_destroy(p_context);
    p_context = NULL;

    error = crepl_create(&p_context, options);
    error = crepl_lib(p_context, "xapian");
    mu_assert("error: crepl_lib() != 0", error == 0);
    crepl_destroy(p_context);
    p_context = NULL;
cleanup:
    crepl_destroy(p_context);
    return p_message;
}

typedef struct utest_vars_t {
    int id;
    UT_hash_handle hh;
    char name[];
} utest_vars_t;

static char *test_crepl_parse()
{
    int status;
    char *p_message = NULL;
    int error;
    crepl_options options;
    memset(&options, 0, sizeof(crepl_options));

    struct crepl *p_context = NULL;
    error = crepl_create(&p_context, options);
    crepl_state_t state = CREPL_STATE_CPARSER;
#if 0
    int num_ids = 0;
    char **pp_ids = NULL;
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
    (void)error;
    (void)state;
    mu_assert("!has_trailing_open_parenthesis(\"foo(\")", has_trailing_open_parenthesis("foo("));
    mu_assert("!has_trailing_closed_parenthesis(\"foo()\")", has_trailing_closed_parenthesis("foo()"));
#endif // 0
#if 0
#endif // 0

    error = crepl_preparse(p_context, "ls", &state);
    mu_assert("error != 0", error == 0);
    mu_assert("crepl_preparse(p_context, \"ls\", &state) != CREPL_STATE_CMDSTART", state == CREPL_STATE_CMDSTART);
    error = crepl_preparse(p_context, "ls()", &state);
    mu_assert("error != 0", error == 0);
    mu_assert("crepl_preparse(p_context, \"ls()\", &state) != CREPL_STATE_CPARSER", state == CREPL_STATE_CPARSER);
    error = crepl_preparse(p_context, "ls(); int i = 0; i++; sizeof int; int a; char **p = NULL; char* q = NULL; long long int *p = NULL", &state);
    mu_assert("error != 0", error == 0);

cleanup:
    (void)status;
    crepl_destroy(p_context);
    return p_message;
}

#if 0
static char *test_crepl_parsedefs()
{
    char *p_message = NULL;
    int error;
    struct crepl *p_context = NULL;
    error = crepl_create(&p_context);
    mu_assert("error != 0", error == 0);
    error = crepl_parsedefs(p_context, "ls(); int i = 0; i++; sizeof int; int a; char **p = NULL; char* q = NULL; long long int *p = NULL");
    //mu_assert("error != 0", error == 0);

cleanup:
 //   (void)status;
    crepl_destroy(p_context);
    return p_message;
}
#endif // 0

static char *test_crepl_finalize()
{
    int status;
    char *p_message = NULL;
    int error;
    crepl_options options;
    memset(&options, 0, sizeof(crepl_options));

    struct crepl *p_context = NULL;
    error = crepl_create(&p_context, options);
    char *p_finalized = NULL;
    if (error)
        goto cleanup;

    error = crepl_finalize(p_context, "foo(((", &p_finalized);
    mu_assert("error != 0", error == 0);
    mu_assert("crepl_finalized(p_context, \"foo(((\") != \"foo((()))\"", strcmp("foo((()))", p_finalized) == 0);
    free(p_finalized);p_finalized = NULL;
    (void)status;
cleanup:
    free(p_finalized);
    crepl_destroy(p_context);
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

static char *test_ustrjoin()
{
    ustash s = {0};
    char *p_message = NULL;
    mu_assert("error ustrjoin()", strcmp(ustrjoin(&s, "a", "b", "c"), "abc") == 0);
    mu_assert("error ustrjoin()", strcmp(ustrjoin(&s, "a"), "a") == 0);
cleanup:
    uclear(&s);
    return p_message;
}

static char *test_ufiletostrv()
{
    ustash s = {0};
    char *p_message = NULL;
    char **pp_strv = ufiletostrv(&s, "../test/testdir/a", "\n");

    mu_assert("error ufiletostrv()", strcmp(pp_strv[0], "abc") == 0);
    mu_assert("error ufiletostrv()", strcmp(pp_strv[1], "def") == 0);
    mu_assert("error ufiletostrv()", strcmp(pp_strv[2], "ghj") == 0);
    mu_assert("error ufiletostrv()", pp_strv[3] == NULL);

cleanup:
    uclear(&s);
    return p_message;
}

static char *test_ustrvtofile()
{
    ustash s = {0};
    char dir_template[] = "/tmp/test-XXXXXX";
    char *p_message = NULL;
    char *p_tempdir = mkdtemp(dir_template);
    mu_assert("p_tempdir == NULL", p_tempdir != NULL);

    char *p_tempfile = ustrjoin(&s, p_tempdir, "/tempfile.txt");
    char *p_strv[] = {"abc", "def", "ghj", NULL};
    if (ustrvtofile((const char**)p_strv, p_tempfile, "\n") != 0)
        mu_assert("ustrvtofile() failed\n", 0);
    char **pp_verify = ufiletostrv(&s, p_tempfile, "\n");

    mu_assert("error ustrvtofile()", strcmp(pp_verify[0], "abc") == 0);
    mu_assert("error ustrvtofile()", strcmp(pp_verify[1], "def") == 0);
    mu_assert("error ustrvtofile()", strcmp(pp_verify[2], "ghj") == 0);
    mu_assert("error ustrvtofile()", pp_verify[3] == NULL);

    ucmd("rm", "-f", p_tempfile);
    ucmd("rm", "-d", p_tempdir);

cleanup:
    uclear(&s);
    return p_message;
}

static void add_completion_callback(void *p_data, char *p_completed_string)
{
    char *p_message = NULL;
    (void)p_message;
    char **p_str = (char**)p_data;
    mu_assert("p_data == NULL", p_data != NULL);
    *p_str = strdup(p_completed_string);
cleanup:
    return;
}

static char *test_complete()
{
    char *p_message = NULL;
    (void)p_message;
    struct crepl *p_crepl = NULL;
    crepl_options options;
    memset(&options, 0, sizeof(crepl_options));
    mu_assert("error creating crepl", CREPL_OK(crepl_create(&p_crepl, options)));
    char *p_str = NULL;
    mu_assert("error completing", CREPL_OK(crepl_complete(p_crepl, "crepl_creat", add_completion_callback, &p_str)));
    //mu_assert("completion incorrect", ustreq(p_str, "crepl_create(/*struct crepl **pp_crepl*/"));

cleanup:
    free(p_str);
    crepl_destroy(p_crepl);
    
    return NULL;
}

static char * all_tests()
{
    mu_run_test(test_strexp);
    mu_run_test(test_strsplit);
    mu_run_test(test_ucmd);
    mu_run_test(test_ucmd_pipe);
    mu_run_test(test_uchdir);
    mu_run_test(test_crepl_vars);
    mu_run_test(test_crepl_dyld);
    mu_run_test(test_parserutils);
    mu_run_test(test_crepl_parse);
    mu_run_test(test_crepl_finalize);
    mu_run_test(test_ustrout);
    mu_run_test(test_udirname);
    mu_run_test(test_ustrtrim);
    mu_run_test(test_ustrjoin);
    mu_run_test(test_ufiletostrv);
    mu_run_test(test_ustrvtofile);
    mu_run_test(test_ustrneq);
    mu_run_test(test_complete);
    //mu_run_test(test_crepl_parsedefs);
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

