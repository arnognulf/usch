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
#include "usch.h"

int eval_stmt(char *p_input)
{
    FILE *p_stmt_c = NULL;
    void *handle;
    int (*dyn_func)();
    char *p_error = NULL;
    char **pp_path = NULL;
    // TODO: we need to determine wether stmt need to be usch-defined or not
    // declare dummy function to get overridden errors
    // use macro to call the real function
    char pre_fn[] = "\
                     #include \"usch.h\"\n\
                     int dyn_func()\n\
    {\n\
";

    size_t pre_fn_len = strlen(pre_fn);
    char post_fn[] = ";return 0;\n}\n";
    size_t post_fn_len = strlen(post_fn);
    size_t input_length;
    size_t bytes_written;
    //char p_input[] = "printf(\"herro\")";

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

    if ((p_error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", p_error);
        goto error;
    }

    printf("%d\n", (*dyn_func)());
    dlclose(handle);
    free(pp_path);

    return 0;
error:
    return -1;
}
