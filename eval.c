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

typedef struct {
    char *p_symname;
} usch_def_t;

int parse_line(char *p_input, usch_def_t *p_definition)
{
    usch_def_t definition = {0};
    int status = 1;
    size_t i = 0;
    size_t length;
    char *p_symname = NULL;

    length = strlen(p_input);
    p_symname = calloc(length, 0);
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

int eval_stmt(char *p_input)
{
    usch_def_t definition = {0};
    FILE *p_stmt_c = NULL;
    void *handle;
    int (*dyn_func)();
    char *p_error = NULL;
    char **pp_path = NULL;
    // TODO: we need to determine wether stmt need to be usch-defined or not
    // declare dummy function to get overridden errors
    // use macro to call the real function
    char usch_h[] = "\
#include \"usch.h\"\n";

                     char pre1[] = "\
#define ";
                     char pre2[] = "\
                     (...) usch_cmd(\"";
    char pre3[] = "\", ##__VA_ARGS__)\n";

    char dyn_func_def[] = "int dyn_func()\n\
    {\n\
        ";

        char post_fn[] = ";return 0;\n}\n";
        size_t post_fn_len = strlen(post_fn);
        size_t input_length;
        size_t bytes_written;
        char *p_fnname = NULL;

        if (usch_strsplit(getenv("PATH"), ":", &pp_path) < 0)
        {
            goto error;
        }
        input_length = strlen(p_input);
        p_stmt_c = fopen("stmt.c", "w+");
        if (p_stmt_c == NULL)
        {
            fprintf(stderr, "file open fail\n");
            goto error;
        }

        if (parse_line(p_input, &definition) < 1)
        {
            goto error;
        }
        bytes_written = fwrite(usch_h, 1, strlen(usch_h), p_stmt_c);
        if (strcmp(definition.p_symname, "cd") != 0)
        {
            bytes_written = fwrite(pre1, 1, strlen(pre1), p_stmt_c);
            bytes_written = fwrite(definition.p_symname, 1, strlen(definition.p_symname), p_stmt_c);
            bytes_written = fwrite(pre2, 1, strlen(pre2), p_stmt_c);
            bytes_written = fwrite(definition.p_symname, 1, strlen(definition.p_symname), p_stmt_c);
            bytes_written = fwrite(pre3, 1, strlen(pre3), p_stmt_c);
        }
        bytes_written = fwrite(dyn_func_def, 1, strlen(dyn_func_def), p_stmt_c);
        bytes_written = fwrite(p_input, 1, strlen(p_input), p_stmt_c);
        if (bytes_written != strlen(p_input))
        {
            fprintf(stderr, "write error 2\n");
            goto error;
        }
        printf("p_input: %s\n", p_input);

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

        if ((p_error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", p_error);
            goto error;
        }
        (*dyn_func)();

        dlclose(handle);
        free(pp_path);

        return 0;
error:
        return -1;
}
