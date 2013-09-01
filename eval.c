#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

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
