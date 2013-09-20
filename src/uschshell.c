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
#include <dirent.h>
#include "usch.h"
#include "usch_debug.h"
#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
typedef struct
{
    char *p_symname;
} usch_def_t;

#include "uschshell.h"
#include "../external/uthash/src/uthash.h"

static size_t get_type_len(char *p_defname)
{
    size_t i, last_type_pos;
    size_t deflen = strlen(p_defname);

    for (i = 0; i < deflen; i++)
    {
        if (p_defname[i] == ' ')
            last_type_pos = i;
        if (p_defname[i] == '*')
            last_type_pos = i;
    }
    return last_type_pos;
}

static char *get_symname(char *p_defname)
{
    char *p_tmp = p_defname;
    char *p_symname = p_defname;
    while (*p_tmp != '\0')
    {
        if (*p_tmp == ' ' || *p_tmp == '*')
        {
            p_symname = p_tmp;
        }
        p_tmp++;
    }
    return p_symname + 1;
}

static int parse_line(char *p_input, usch_def_t *p_definition)
{
    //usch_def_t definition;
    int status = 1;
    size_t i = 0;
    size_t length;
    char *p_symname = NULL;

    length = strlen(p_input);
    p_symname = calloc(length, 1);

    FAIL_IF(p_symname == NULL);

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

#define USCHSHELL_DEFINE_SIZE 8
typedef struct uschshell_def_t
{
    UT_hash_handle hh;
    size_t size;
    void *p_body_data;
    void *p_alloc_data;
    char data[USCHSHELL_DEFINE_SIZE];
    char defname[];
} uschshell_def_t;

typedef struct uschshell_cmd_t
{
    UT_hash_handle hh;
    char cmdname[];
} uschshell_cmd_t;

typedef struct uschshell_t 
{
    uschshell_def_t *p_defs;
    uschshell_cmd_t *p_cmds;
} uschshell_t;

int uschshell_define(uschshell_t *p_context, size_t var_size, char *p_defname)
{
    int status = 0;
    if (p_context == NULL || p_defname == NULL)
        return -1;
    uschshell_def_t *p_defs = p_context->p_defs;
    uschshell_def_t *p_def = NULL;
    void *p_alloc_data = NULL;
    HASH_FIND_STR(p_defs, p_defname, p_def);

    FAIL_IF(p_def != NULL);

    p_def = calloc(sizeof(uschshell_def_t) + strlen(p_defname) + 1, 1);
    FAIL_IF(p_def == NULL);

    if (var_size > USCHSHELL_DEFINE_SIZE)
    {
        p_alloc_data = calloc(var_size, 1);
        FAIL_IF(p_alloc_data == NULL);
        p_def->p_alloc_data = p_alloc_data;
    }
    p_def->size = var_size;
    strcpy(p_def->defname, p_defname);

    HASH_ADD_STR(p_context->p_defs, defname, p_def);
    p_def = NULL;
    p_alloc_data = NULL;
end:
    free(p_alloc_data);
    free(p_def);
    return status;
}
void uschshell_undef(uschshell_t *p_context, char *p_defname)
{
    if (p_context == NULL || p_defname == NULL)
        return;
    uschshell_def_t *p_def = NULL;
    uschshell_def_t *p_defs = p_context->p_defs;
    HASH_FIND_STR(p_defs, p_defname, p_def);
    if (p_def != NULL)
    {
        free(p_def->p_alloc_data);
        p_def->p_alloc_data = NULL;
        free(p_def->p_body_data);
        p_def->p_body_data = NULL;
        HASH_DEL(p_defs, p_def);
    }
    return;
}
int uschshell_load(uschshell_t *p_context, char *p_defname, void *p_data)
{
    int status = 0;
    if (p_context == NULL || p_defname == NULL)
        return -1;
    uschshell_def_t *p_def = NULL;
    uschshell_def_t *p_defs = p_context->p_defs;
    HASH_FIND_STR(p_defs, p_defname, p_def);
    if (p_def != NULL)
    {
        memcpy(p_data, p_def->data, p_def->size);
    }
    else
    {
        status = -1;
    }
    return status;
}

static void print_updated_variables(char *p_defname, void *p_data)
{
    size_t type_len;
    type_len = get_type_len(p_defname);

    if (strncmp(p_defname, "unsigned int", type_len) == 0)
        printf("%s = %d\n", get_symname(p_defname), *(unsigned int*)p_data);
    if (strncmp(p_defname, "char", type_len) == 0)
        printf("%s = %c\n", get_symname(p_defname), *(char*)p_data);
    else if (strncmp(p_defname, "int", type_len) == 0)
        printf("%s = %d\n", get_symname(p_defname), *(int*)p_data);
    else if (strncmp(p_defname, "double", type_len) == 0)
        printf("%s = %f\n", get_symname(p_defname), *(double*)p_data);
    else if (strncmp(p_defname, "float", type_len) == 0)
        printf("%s = %f\n", get_symname(p_defname), *(float*)p_data);
    else
        printf("%s = 0x%lx\n", get_symname(p_defname), *(uint64_t*)p_data);
}

int uschshell_store(uschshell_t *p_context, char *p_defname, void *p_data)
{
    int status = 0;
    if (p_context == NULL || p_defname == NULL)
        return -1;
    uschshell_def_t *p_def = NULL;
    uschshell_def_t *p_defs = p_context->p_defs;
    HASH_FIND_STR(p_defs, p_defname, p_def);
    if (p_def != NULL)
    {
        memcpy(p_def->data, p_data, p_def->size);

        print_updated_variables(p_defname, p_def->data);
    }
    else
    {
        status = -1;
    }
    return status;
}

int uschshell_define_fn(struct uschshell_t *p_context, char *p_fndefname, char *p_body)
{
    int status = 0;
    if (p_context == NULL || p_fndefname == NULL || p_body == NULL)
        return -1;
    uschshell_def_t *p_defs = p_context->p_defs;
    uschshell_def_t *p_def = NULL;
    void *p_body_data = NULL;
    HASH_FIND_STR(p_defs, p_fndefname, p_def);

    FAIL_IF(p_def != NULL);

    p_def = calloc(sizeof(uschshell_def_t) + strlen(p_fndefname) + 1, 1);
    FAIL_IF(p_def == NULL);

    p_body_data = calloc(strlen(p_body) + 1, 1);

    FAIL_IF(p_body_data == NULL);

    p_def->p_body_data = p_body_data;
    strcpy(p_def->defname, p_fndefname);

    HASH_ADD_STR(p_context->p_defs, defname, p_def);
    p_def = NULL;
    p_body_data = NULL;
end:
    free(p_body_data);
    free(p_def);
    return status;
}

int uschshell_create(uschshell_t **pp_context)
{
    int status = 0;
    uschshell_t *p_context = NULL;
    uschshell_def_t *p_def = NULL;
    uschshell_cmd_t *p_cmd = NULL;

    p_context = calloc(sizeof(uschshell_t), 1);
    FAIL_IF(p_context == NULL);

    p_def = calloc(sizeof(uschshell_def_t) + 1, 1);
    FAIL_IF(p_def == NULL);

    p_cmd = calloc(sizeof(uschshell_cmd_t) + 1, 1);
    FAIL_IF(p_cmd == NULL);

    HASH_ADD_STR(p_context->p_defs, defname, p_def);
    HASH_ADD_STR(p_context->p_cmds, cmdname, p_cmd);

    *pp_context = p_context;
    p_context = NULL;
    p_cmd = NULL;
    p_def = NULL;
end:
    free(p_cmd);
    free(p_def);
    free(p_context);
    return status;
}
void uschshell_destroy(uschshell_t *p_context)
{
    free(p_context);
    return;
}
static int fwrite_ok(char* p_str, FILE *p_file)
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
static int write_definitions_h(uschshell_t *p_context, char *p_tempdir)
{
    int status = 0;
    char definitions_h_filename[] = "definitions.h";
    size_t filename_length;
    size_t tempdir_len;
    char *p_definitionsfile = NULL;
    FILE *p_definitions_h = NULL;
    uschshell_def_t *p_defs = NULL;
    uschshell_def_t *p_def = NULL;
    uschshell_def_t *p_tmp = NULL;

    p_defs = p_context->p_defs;
    tempdir_len = strlen(p_tempdir);
    filename_length = tempdir_len + 1 + strlen(definitions_h_filename) + 1;
    p_definitionsfile = calloc(filename_length, 1);
    FAIL_IF(p_definitionsfile == NULL);

    strcpy(p_definitionsfile, p_tempdir);

    p_definitionsfile[tempdir_len] = '/';
    strcpy(&p_definitionsfile[tempdir_len + 1], definitions_h_filename);
    p_definitionsfile[filename_length-1] = '\0';
    p_definitions_h = fopen(p_definitionsfile, "w+");
    FAIL_IF(p_definitions_h == NULL);
    FAIL_IF(!fwrite_ok("struct uschshell_t;\n", p_definitions_h));
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            FAIL_IF(!fwrite_ok(p_def->defname, p_definitions_h));
            FAIL_IF(!fwrite_ok(";\n", p_definitions_h));
        }
    }
    FAIL_IF(!fwrite_ok("\nvoid uschshell_load_vars(struct uschshell_t *p_context)\n{\n", p_definitions_h));
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            FAIL_IF(!fwrite_ok("uschshell_load(p_context, \"", p_definitions_h));
            FAIL_IF(!fwrite_ok(p_def->defname, p_definitions_h));
            FAIL_IF(!fwrite_ok("\", (void*)&", p_definitions_h));
            FAIL_IF(!fwrite_ok(get_symname(p_def->defname), p_definitions_h));
            FAIL_IF(!fwrite_ok(");\n", p_definitions_h));
        }
    }

    FAIL_IF(!fwrite_ok("return;\n}\n", p_definitions_h));

end:
    if(p_definitions_h)
        fclose(p_definitions_h);
    free(p_definitionsfile);
    return status;
}
int uschshell_pathhash(uschshell_t *p_context)
{
    int status = 0;
    char **pp_path = NULL;
    int num_paths = 0;
    int i;
    DIR *p_dir;
    uschshell_cmd_t *p_cmds = NULL;
    uschshell_cmd_t *p_cmd = NULL;

    if (p_context == NULL)
        return -1;

    p_cmds = p_context->p_cmds;
    num_paths = usch_strsplit(getenv("PATH"), ":", &pp_path);
    FAIL_IF(num_paths < 1);

    for (i = 0; i < num_paths; i++)
    {
        struct dirent *p_ent;
        if ((p_dir = opendir(pp_path[i])) != NULL)
        {
            while ((p_ent = readdir(p_dir)) != NULL)
            {
                uschshell_cmd_t *p_found_cmd = NULL;

                if (strcmp(p_ent->d_name, "..") == 0)
                    continue;
                if (strcmp(p_ent->d_name, ".") == 0)
                    continue;

                HASH_FIND_STR(p_cmds, p_ent->d_name, p_found_cmd);
                if (p_found_cmd)
                    continue;
                p_cmd = calloc(sizeof(uschshell_cmd_t) + strlen(p_ent->d_name) + 1, 1);
                FAIL_IF(p_cmd == NULL);
                strcpy(p_cmd->cmdname, p_ent->d_name);

                HASH_ADD_STR(p_cmds, cmdname, p_cmd);
            }
        }
    }
end:
    if (p_dir)
        closedir(p_dir);
    free(pp_path);
    free(p_cmd);
    return status;
}
int uschshell_is_cmd(uschshell_t *p_context, char *p_item)
{
    int status = 0;
    uschshell_cmd_t *p_cmds = NULL;
    uschshell_cmd_t *p_found_cmd = NULL;

    if (p_context == NULL || p_item == NULL)
        return -1;

    HASH_FIND_STR(p_cmds, p_item, p_found_cmd);
    if (p_found_cmd)
        return 1;
    else
        return 0;
}
int uschshell_eval(uschshell_t *p_context, char *p_input)
{
    int status = 0;
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
    struct stat sb;
    char *p_fullpath_uschrc_h = NULL;
    char uschrc_h[] = "/.uschrc.h";

    char expr_c_filename[] = "expr.c";
    char dylib_filename[] = "dyn_stmt";
    char *p_tempdir = NULL;
    char *p_tempfile = NULL;
    char *p_tempdylib = NULL;
    size_t tempdir_len = 0;
    char dir_template[] = "/tmp/usch-XXXXXX";

    if (p_context == NULL || p_input == NULL)
        return -1;

    FAIL_IF(usch_strsplit(getenv("PATH"), ":", &pp_path) < 0);
    p_tempdir = mkdtemp(dir_template);
    FAIL_IF(p_tempdir == NULL);
    tempdir_len = strlen(p_tempdir);
    filename_length = tempdir_len + 1 + strlen(expr_c_filename) + 1;
    p_tempfile = malloc(filename_length);
    FAIL_IF(p_tempfile == NULL);
    strcpy(p_tempfile, p_tempdir);
    p_tempfile[tempdir_len] = '/';
    strcpy(&p_tempfile[tempdir_len + 1], expr_c_filename);
    p_tempfile[filename_length-1] = '\0';
    p_stmt_c = fopen(p_tempfile, "w+");
    FAIL_IF(p_stmt_c == NULL);

    FAIL_IF(write_definitions_h(p_context, p_tempdir) != 0);
    
    FAIL_IF(parse_line(p_input, &definition) < 1);
    
    FAIL_IF(!fwrite_ok("#include <usch.h>\n", p_stmt_c));
    FAIL_IF(!fwrite_ok("#include \"definitions.h\"\n", p_stmt_c));

    p_fullpath_uschrc_h = calloc(sizeof(getenv("HOME")) + sizeof(uschrc_h) + 2, 1);
    FAIL_IF(p_fullpath_uschrc_h == NULL);
    strcpy(p_fullpath_uschrc_h, getenv("HOME"));
    strcpy(p_fullpath_uschrc_h + sizeof(getenv("HOME")) + 2, uschrc_h);

    if (stat(p_fullpath_uschrc_h, &sb) != -1)
    {
        FAIL_IF(!fwrite_ok("#include \"", p_stmt_c));
        FAIL_IF(!fwrite_ok(getenv("HOME"), p_stmt_c));
        FAIL_IF(!fwrite_ok("/.uschrc.h\"\n", p_stmt_c));
    }

    if (strcmp(definition.p_symname, "cd") != 0)
    {
        FAIL_IF(!fwrite_ok("#define ", p_stmt_c));
        FAIL_IF(!fwrite_ok(definition.p_symname, p_stmt_c));
        FAIL_IF(!fwrite_ok("(...) usch_cmd(\"", p_stmt_c));
        FAIL_IF(!fwrite_ok(definition.p_symname, p_stmt_c));
        FAIL_IF(!fwrite_ok("\", ##__VA_ARGS__)\n", p_stmt_c));
    }
    FAIL_IF(!fwrite_ok("int dyn_func()\n    {\n        ", p_stmt_c));
    FAIL_IF(!fwrite_ok(p_input, p_stmt_c));

    FAIL_IF(!fwrite_ok(";return 0;\n}\n", p_stmt_c));
    fclose(p_stmt_c);
    p_stmt_c = NULL;
    dylib_length = tempdir_len + 1 + strlen(dylib_filename) + 1;
    p_tempdylib = malloc(dylib_length);
    FAIL_IF(p_tempdylib == NULL);
    strcpy(p_tempdylib, p_tempdir);
    p_tempdylib[tempdir_len] = '/';
    strcpy(&p_tempdylib[tempdir_len + 1], dylib_filename);
    p_tempdylib[dylib_length-1] = '\0';
    if (usch_shell_cc("-rdynamic", "-shared", "-fPIC", "-o", p_tempdylib, p_tempfile) != 0) 
    {
        fprintf(stderr, "usch: compile error\n");
        ENDOK_IF(1);
    }

    p_handle = dlopen(p_tempdylib, RTLD_LAZY);
    FAIL_IF(!p_handle);

    dlerror();

    *(void **) (&dyn_func) = dlsym(p_handle, "dyn_func");

    FAIL_IF((p_error = dlerror()) != NULL);
    (*dyn_func)();

end:
    free(definition.p_symname);
    if (p_handle)
        dlclose(p_handle);
    free(pp_path);
    free(p_tempfile);

    if (p_stmt_c != NULL)
        fclose(p_stmt_c);

    return status;
}
