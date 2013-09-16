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
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <ctype.h>
#include <assert.h>
#include "usch.h"

typedef struct
{
    char *p_symname;
} usch_def_t;

#include "uschshell.h"
#include "external/uthash/src/uthash.h"

static int parse_line(char *p_input, usch_def_t *p_definition)
{
    //usch_def_t definition;
    int status = 1;
    size_t i = 0;
    size_t length;
    char *p_symname = NULL;

    length = strlen(p_input);
    p_symname = calloc(length, 1);
    if (p_symname == NULL)
    {
        status = -1;
        goto end;
    }
    while (p_input[i] == ' ' || p_input[i] == '\t')
        i++;



    for (; i < length; i++)
    {
        char c = p_input[i];
        switch (p_input[i])
        {
            case '\t':
            case ' ':
                {
                    break;
                }
            case '(':
                {

                    break;
                }
            default:
                {
                    if (isalnum(c) != c)
                    {
                        p_symname[i] = c;
                    }
                    else
                    {
                        assert(0);
                    }
                    break;
                }
        }
    }
    p_definition->p_symname = p_symname;
end:
    return status;
}

#define usch_shell_cc(...) usch_cmd("gcc", ##__VA_ARGS__)
typedef struct uschshell_t 
{
    int foo;
} uschshell_t;
int uschshell_create(uschshell_t **pp_context)
{
    uschshell_t *p_context = NULL;
    p_context = calloc(sizeof(uschshell_t), 1);
    *pp_context = p_context;
    return 0;
}
void uschshell_destroy(uschshell_t *p_context)
{
    free(p_context);
    return;
}
int fwrite_ok(char* p_str, FILE *p_file)
{
    size_t bytes_written;
    size_t bytes_to_write;
    bytes_to_write = strlen(p_str);
    bytes_written = fwrite(p_str, sizeof(char), bytes_to_write, p_file);
    if (bytes_to_write != bytes_written)
    {
        return 0;
    }
    else
    {
        return 1;
    }

}
int uschshell_eval(uschshell_t *p_context, char *p_input)
{
    // TODO: unused for now
    (void)p_context;

    usch_def_t definition = {0};
    FILE *p_stmt_c = NULL;
    void *p_handle = NULL;
    int (*dyn_func)();
    char *p_error = NULL;
    char **pp_path = NULL;
    size_t filename_length;
    size_t dylib_length;
    // TODO: we need to determine wether stmt need to be usch-defined or not
    // declare dummy function to get overridden errors
    // use macro to call the real function
    char usch_h[] = "\
#include <usch.h>\n";

                     char pre1[] = "\
#define ";
                     char pre2[] = "(...) usch_cmd(\"";
    char pre3[] = "\", ##__VA_ARGS__)\n";

    char dyn_func_def[] = "int dyn_func()\n\
    {\n\
        ";

        char expr_c_filename[] = "expr.c";
        char dylib_filename[] = "dyn_stmt";
        char post_fn[] = ";return 0;\n}\n";
        //char *p_fnname = NULL;
        char *p_tempdir = NULL;
        char *p_tempfile = NULL;
        char *p_tempdylib = NULL;
        size_t tempdir_len = 0;
        char dir_template[] = "/tmp/usch-XXXXXX";

        if (usch_strsplit(getenv("PATH"), ":", &pp_path) < 0)
        {
            goto end;
        }
        p_tempdir = mkdtemp(dir_template);
        if (p_tempdir == NULL)
        {
            fprintf(stderr, "dir creation failed\n");
            goto end;
        }
        tempdir_len = strlen(p_tempdir);
        filename_length = tempdir_len + 1 + strlen(expr_c_filename) + 1;
        p_tempfile = malloc(filename_length);
        if (p_tempfile == NULL)
        {
            goto end;
        }
        strcpy(p_tempfile, p_tempdir);
        p_tempfile[tempdir_len] = '/';
        strcpy(&p_tempfile[tempdir_len + 1], expr_c_filename);
        p_tempfile[filename_length-1] = '\0';
        p_stmt_c = fopen(p_tempfile, "w+");
        if (p_stmt_c == NULL)
        {
            printf("file open fail\n");
            goto end;
        }

        if (parse_line(p_input, &definition) < 1)
            goto end;
        if (!fwrite_ok(usch_h, p_stmt_c))
            goto end;
        if (strcmp(definition.p_symname, "cd") != 0)
        {
            if (!fwrite_ok(pre1, p_stmt_c))
                goto end;
            if (!fwrite_ok(definition.p_symname, p_stmt_c))
                goto end;
            if (!fwrite_ok(pre2, p_stmt_c))
                goto end;
            if (!fwrite_ok(definition.p_symname, p_stmt_c))
                goto end;
            if (!fwrite(pre3, 1, strlen(pre3), p_stmt_c))
                goto end;
        }
        if (!fwrite_ok(dyn_func_def, p_stmt_c))
            goto end;
        if (!fwrite_ok(p_input, p_stmt_c))
            goto end;

        if (!fwrite_ok(post_fn, p_stmt_c))
            goto end;
        fclose(p_stmt_c);
        p_stmt_c = NULL;
        dylib_length = tempdir_len + 1 + strlen(dylib_filename) + 1;
        p_tempdylib = malloc(dylib_length);
        if (p_tempdylib == NULL)
        {
            goto end;
        }
        strcpy(p_tempdylib, p_tempdir);
        p_tempdylib[tempdir_len] = '/';
        strcpy(&p_tempdylib[tempdir_len + 1], dylib_filename);
        p_tempdylib[dylib_length-1] = '\0';
        if (usch_shell_cc("-rdynamic", "-shared", "-fPIC", "-o", p_tempdylib, p_tempfile) != 0) 
        {
            fprintf(stderr, "usch: compile error\n");
            goto end;
        }

        p_handle = dlopen(p_tempdylib, RTLD_LAZY);
        if (!p_handle) {
            fprintf(stderr, "%s\n", dlerror());
            goto end;
        }

        dlerror();

        *(void **) (&dyn_func) = dlsym(p_handle, "dyn_func");

        if ((p_error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", p_error);
            goto end;
        }
        (*dyn_func)();

end:
        free(definition.p_symname);
        if (p_handle)
            dlclose(p_handle);
        free(pp_path);
        free(p_tempfile);

        if (p_stmt_c != NULL)
            fclose(p_stmt_c);

        return 0;
}

