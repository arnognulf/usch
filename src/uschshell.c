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
#include <limits.h>
#include <stdlib.h>

// /usr/lib/llvm-3.4/include/clang-c/Index.h
#include "clang-c/Index.h"
#include "usch.h"
#include "usch_debug.h"
#include "bufstr.h"
#include "parserutils.h"

#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#include "uschshell.h"
#include "../external/uthash/src/uthash.h"
#define USCHSHELL_DYN_FUNCNAME "usch_dyn_func"

#define usch_shell_cc(...) usch_cmd("gcc", ##__VA_ARGS__)

#define USCHSHELL_DEFINE_SIZE 8
typedef struct uschshell_def_t
{
    UT_hash_handle hh;
    size_t size;
    void *p_body_data;
    void *p_alloc_data;
    uint8_t data[USCHSHELL_DEFINE_SIZE];
    char defname[];
} uschshell_def_t;

typedef struct uschshell_cmd_t
{
    UT_hash_handle hh;
    char cmdname[];
} uschshell_cmd_t;

typedef struct uschshell_lib_t
{
    struct uschshell_lib_t *p_next;
    void *p_handle;
    char libname[];
} uschshell_lib_t;

typedef struct uschshell_sym_t
{
    UT_hash_handle hh;
    void *p_handle;
    char symname[];
} uschshell_sym_t;

typedef struct uschshell_dyfn_t
{
    UT_hash_handle hh;
    void *p_handle;
    char *p_dyfndef;
    char dyfnname[];
} uschshell_dyfn_t;

typedef struct uschshell_inc_t
{
    UT_hash_handle hh;
    char incname[];
} uschshell_inc_t;

typedef struct uschshell_t 
{
    uschshell_def_t *p_defs;
    uschshell_lib_t *p_libs;
    uschshell_sym_t *p_syms;
    uschshell_dyfn_t *p_dyfns;
    uschshell_inc_t *p_incs;
    char tmpdir[];
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
        printf("%%%s = %d\n", get_symname(p_defname), *(unsigned int*)p_data);
    if (strncmp(p_defname, "char", type_len) == 0)
        printf("%%%s = %c\n", get_symname(p_defname), *(char*)p_data);
    else if (strncmp(p_defname, "int", type_len) == 0)
        printf("%%%s = %d\n", get_symname(p_defname), *(int*)p_data);
    else if (strncmp(p_defname, "double", type_len) == 0)
        printf("%%%s = %f\n", get_symname(p_defname), *(double*)p_data);
    else if (strncmp(p_defname, "float", type_len) == 0)
        printf("%%%s = %f\n", get_symname(p_defname), *(float*)p_data);
    else
        printf("%%%s = 0x%lx\n", get_symname(p_defname), *(long unsigned int*)p_data);

}

int uschshell_store(uschshell_t *p_context, char *p_defname, void *p_data)
{
    int status = 0;
    uint8_t tmp[USCHSHELL_DEFINE_SIZE] = {0};
    int is_updated = 0;
    if (p_context == NULL || p_defname == NULL)
        return -1;
    uschshell_def_t *p_def = NULL;
    uschshell_def_t *p_defs = p_context->p_defs;
    HASH_FIND_STR(p_defs, p_defname, p_def);
    
    if (p_def != NULL)
    {
            memcpy(tmp, p_data, p_def->size);

            size_t i;
            for (i = 0; i < p_def->size; i++)
            {
                if (((uint8_t*)tmp)[i] != ((uint8_t*)p_def->data)[i])
                {
                    is_updated = 1;
                }
            }

            memcpy(p_def->data, p_data, p_def->size);
            if (is_updated)
                print_updated_variables(p_defname, p_data);
        //}
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
    uschshell_sym_t *p_sym = NULL;
    uschshell_dyfn_t *p_dyfn = NULL;
    uschshell_inc_t *p_inc = NULL;
    char dir_template[] = "/tmp/usch-XXXXXX";
    char *p_tempdir = NULL;

    p_context = calloc(sizeof(uschshell_t) + strlen(dir_template) + 1, 1);
    FAIL_IF(p_context == NULL);

    p_tempdir = mkdtemp(dir_template);
    FAIL_IF(p_tempdir == NULL);
    strcpy(p_context->tmpdir, dir_template);

    p_def = calloc(sizeof(uschshell_def_t) + 1, 1);
    FAIL_IF(p_def == NULL);

    p_sym = calloc(sizeof(uschshell_sym_t) + 1, 1);
    FAIL_IF(p_sym == NULL);

    p_dyfn = calloc(sizeof(uschshell_dyfn_t) + 1, 1);
    FAIL_IF(p_dyfn == NULL);

    p_inc = calloc(sizeof(uschshell_inc_t) + 1, 1);
    FAIL_IF(p_inc == NULL);

    HASH_ADD_STR(p_context->p_defs, defname, p_def);
    HASH_ADD_STR(p_context->p_syms, symname, p_sym);
    HASH_ADD_STR(p_context->p_dyfns, dyfnname, p_dyfn);
    HASH_ADD_STR(p_context->p_incs, incname, p_inc);
    strcpy(p_context->tmpdir, p_tempdir);

    *pp_context = p_context;
    p_context = NULL;
    p_def = NULL;
    p_sym = NULL;
    p_dyfn = NULL;
    p_inc = NULL;
end:
    free(p_inc);
    free(p_dyfn);
    free(p_sym);
    free(p_def);
    free(p_context);
    return status;
}
void uschshell_destroy(uschshell_t *p_context)
{

    if (p_context)
    {
        uschshell_sym_t *p_tmpsym = NULL;
        uschshell_sym_t *p_sym = NULL;
        uschshell_sym_t *p_syms = p_context->p_syms;

        HASH_ITER(hh, p_syms, p_sym, p_tmpsym) {
            HASH_DEL(p_syms, p_sym);
        }

        uschshell_def_t *p_tmpdef = NULL;
        uschshell_def_t *p_def = NULL;
        uschshell_def_t *p_defs = p_context->p_defs;

        HASH_ITER(hh, p_defs, p_def, p_tmpdef) {
            free(p_def->p_body_data);
            free(p_def->p_alloc_data);

            HASH_DEL(p_defs, p_def);
        }

        uschshell_dyfn_t *p_tmpdyfn = NULL;
        uschshell_dyfn_t *p_dyfn = NULL;
        uschshell_dyfn_t *p_dyfns = p_context->p_dyfns;

        HASH_ITER(hh, p_dyfns, p_dyfn, p_tmpdyfn) {
            HASH_DEL(p_dyfns, p_dyfn);
        }
        // TODO: free uschshell_lib_t

    }
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
            FAIL_IF(!fwrite_ok("\tuschshell_load(p_context, \"", p_definitions_h));
            FAIL_IF(!fwrite_ok(p_def->defname, p_definitions_h));
            FAIL_IF(!fwrite_ok("\", (void*)&", p_definitions_h));
            FAIL_IF(!fwrite_ok(get_symname(p_def->defname), p_definitions_h));
            FAIL_IF(!fwrite_ok(");\n", p_definitions_h));
        }
    }

    FAIL_IF(!fwrite_ok("return;\n}\n", p_definitions_h));

    FAIL_IF(!fwrite_ok("\nvoid uschshell_store_vars(struct uschshell_t *p_context)\n{\n", p_definitions_h));
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            FAIL_IF(!fwrite_ok("\tuschshell_store(p_context, \"", p_definitions_h));
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

static int write_includes_h(uschshell_t *p_context, char *p_tempdir)
{
    int status = 0;
    char includes_h_filename[] = "includes.h";
    size_t filename_length;
    size_t tempdir_len;
    char *p_includesfile = NULL;
    FILE *p_includes_h = NULL;
    uschshell_inc_t *p_incs = NULL;
    uschshell_inc_t *p_inc = NULL;
    uschshell_inc_t *p_tmp = NULL;

    p_incs = p_context->p_incs;
    tempdir_len = strlen(p_tempdir);
    filename_length = tempdir_len + 1 + strlen(includes_h_filename) + 1;
    p_includesfile = calloc(filename_length, 1);
    FAIL_IF(p_includesfile == NULL);

    strcpy(p_includesfile, p_tempdir);

    p_includesfile[tempdir_len] = '/';
    strcpy(&p_includesfile[tempdir_len + 1], includes_h_filename);
    p_includesfile[filename_length-1] = '\0';
    p_includes_h = fopen(p_includesfile, "w+");
    FAIL_IF(p_includes_h == NULL);
    HASH_ITER(hh, p_incs, p_inc, p_tmp)
    {
        if (strcmp(p_inc->incname, "") != 0)
        {
            FAIL_IF(!fwrite_ok("#include ", p_includes_h));
            if (p_inc->incname[0] == '<' || p_inc->incname[0] == '\"')
            {
                FAIL_IF(!fwrite_ok(p_inc->incname, p_includes_h));
            }
            else
            {
                FAIL_IF(!fwrite_ok("\"", p_includes_h));
                FAIL_IF(!fwrite_ok(p_inc->incname, p_includes_h));
                FAIL_IF(!fwrite_ok("\"", p_includes_h));
            }
        }
    }

end:
    if(p_includes_h)
        fclose(p_includes_h);
    free(p_includesfile);
    return status;
}


static int write_trampolines_h(uschshell_t *p_context, char *p_tempdir)
{
    int status = 0;
    char trampolines_h_filename[] = "trampolines.h";
    size_t filename_length;
    size_t tempdir_len;
    char *p_trampolinesfile = NULL;
    FILE *p_trampolines_h = NULL;
    uschshell_dyfn_t *p_dyfns = NULL;
    uschshell_dyfn_t *p_dyfn = NULL;
    uschshell_dyfn_t *p_tmp = NULL;

    p_dyfns = p_context->p_dyfns;
    tempdir_len = strlen(p_tempdir);
    filename_length = tempdir_len + 1 + strlen(trampolines_h_filename) + 1;
    p_trampolinesfile = calloc(filename_length, 1);
    FAIL_IF(p_trampolinesfile == NULL);

    strcpy(p_trampolinesfile, p_tempdir);

    p_trampolinesfile[tempdir_len] = '/';
    strcpy(&p_trampolinesfile[tempdir_len + 1], trampolines_h_filename);
    p_trampolinesfile[filename_length-1] = '\0';
    p_trampolines_h = fopen(p_trampolinesfile, "w+");
    FAIL_IF(p_trampolines_h == NULL);
    HASH_ITER(hh, p_dyfns, p_dyfn, p_tmp)
    {
        if (p_dyfn->p_dyfndef != NULL)
        {
            FAIL_IF(!fwrite_ok(p_dyfn->p_dyfndef, p_trampolines_h));
        }
    }

end:
    if(p_trampolines_h)
        fclose(p_trampolines_h);
    free(p_trampolinesfile);
    return status;
}

static void trim_end_space(char *p_input)
{
    size_t i;
    size_t len = strlen(p_input);

    for (i = len - 1; i > 0; i--)
    {
        if (p_input[i] == ' ' || p_input[i] == '\t')
            p_input[i] = '\0';
        else
             break;
    }
}

static int pre_assign(char *p_input, char **pp_pre_assign)
{
    int status = 0;
    char *p_pre_assign = NULL;
    int i = 0;

    p_pre_assign = strdup(p_input);

    FAIL_IF(p_pre_assign == NULL);

    i += count_spaces(p_pre_assign);
    while (p_pre_assign[i] != '\0')
    {
        if (p_input[i] == '=')
            p_pre_assign[i] = '\0';
        i++;
    }
    trim_end_space(p_pre_assign);
    *pp_pre_assign = p_pre_assign;
    p_pre_assign = NULL;
end:
    free(p_pre_assign);
    return status;
}

static int post_assign(char *p_input, char **pp_post_assign)
{
    int status = 0;
    char *p_post_assign = NULL;
    int i = 0;

    p_post_assign = calloc(strlen(p_input) + 1, 1);

    FAIL_IF(p_post_assign == NULL);

    while (p_input[i] != '\0')
    {
        if (p_input[i] == '=')
            strcpy(p_post_assign, &p_input[i+1]);
        i++;
    }
    *pp_post_assign = p_post_assign;
    p_post_assign = NULL;
end:
    free(p_post_assign);
    return status;
}

int uschshell_eval(uschshell_t *p_context, char *p_input_line)
{
    int status = 0;
    usch_def_t definition = {0};
    FILE *p_stmt_c = NULL;
    void *p_handle = NULL;
    int (*dyn_func)(uschshell_t*);
    int (*set_context)(uschshell_t*);
    int (*uschshell_store_vars)(uschshell_t*);
    int (*uschshell_load_vars)(uschshell_t*);
    char *p_error = NULL;
    char **pp_path = NULL;
    char **pp_cmds = NULL;
    size_t filename_length;
    size_t dylib_length;
    bufstr_t input;
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
    char *p_pre_assign = NULL;
    char *p_post_assign = NULL;
    bufstr_t stmt_c = {0, 0};
    uschshell_state_t state;

    if (p_context == NULL || p_input_line == NULL)
        return -1;

    input.p_str = NULL;
    input.p_str = strdup(p_input_line);
    FAIL_IF(input.p_str == NULL);
    input.len = strlen(p_input_line);

    stmt_c.p_str = calloc(1024, 1);
    FAIL_IF(stmt_c.p_str == NULL);
    stmt_c.len = 1024;
    FAIL_IF(usch_strsplit(getenv("PATH"), ":", &pp_path) < 0);
    p_tempdir = p_context->tmpdir;
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

    FAIL_IF(uschshell_preparse(p_context, input.p_str, &state, &pp_cmds));
    if (state == USCHSHELL_STATE_CMDSTART)
    {
        bufstradd(&input, "()");
    }
    if (state == USCHSHELL_STATE_CMDARG)
    {
        size_t i = 0;
        size_t quotes = 0;
        size_t rparens = 0;
        size_t lparens = 0;
        for (i = 0; i < input.len; i++)
        {
            if (input.p_str[i] != '\"')
                quotes++;
            if (input.p_str[i] == '(')
                lparens++;
            if (input.p_str[i] == ')')
                rparens++;
        }
        if ((quotes % 2) == 1)
        {
            bufstradd(&input, "\"");
        }

        if (lparens != rparens)
        {
            bufstradd(&input, ")");
        }


    }


    FAIL_IF(write_includes_h(p_context, p_tempdir) != 0);
    FAIL_IF(write_definitions_h(p_context, p_tempdir) != 0);
    FAIL_IF(write_trampolines_h(p_context, p_tempdir) != 0);

    FAIL_IF(parse_line(input.p_str, &definition) < 1);

    bufstradd(&stmt_c, "struct uschshell_t;\n");
    bufstradd(&stmt_c, "#include <usch.h>\n");
    bufstradd(&stmt_c, "#include \"includes.h\"\n");
    bufstradd(&stmt_c, "#include \"definitions.h\"\n");
    bufstradd(&stmt_c, "#include \"trampolines.h\"\n");

    p_fullpath_uschrc_h = calloc(sizeof(getenv("HOME")) + sizeof(uschrc_h) + 2, 1);
    FAIL_IF(p_fullpath_uschrc_h == NULL);
    strcpy(p_fullpath_uschrc_h, getenv("HOME"));
    strcpy(p_fullpath_uschrc_h + sizeof(getenv("HOME")) + 2, uschrc_h);

    if (stat(p_fullpath_uschrc_h, &sb) != -1)
    {
        bufstradd(&stmt_c, "#include \"");
        bufstradd(&stmt_c, getenv("HOME"));
        bufstradd(&stmt_c, "/.uschrc.h\"\n");
    }

    bufstradd(&stmt_c, "static struct uschshell_t *p_uschshell_context = NULL;\n");
    bufstradd(&stmt_c, "void uschshell_set_context(struct uschshell_t *p_context)\n{\np_uschshell_context = p_context;\t\n}\n");

    if (iscmd(input.p_str))
    {
        if (strcmp(definition.p_symname, "include") == 0)
        {
            bufstradd(&stmt_c, "#define include");
            bufstradd(&stmt_c, "(header) uschshell_include(p_uschshell_context, (header))\n");
        }
        else if (strcmp(definition.p_symname, "lib") == 0){
            bufstradd(&stmt_c, "#define lib");
            bufstradd(&stmt_c, "(libname) uschshell_lib(p_uschshell_context, (libname))\n");

        }
        else if (strcmp(definition.p_symname, "cd") != 0)
        {
            bufstradd(&stmt_c, "#define ");
            bufstradd(&stmt_c, definition.p_symname);
            bufstradd(&stmt_c, "(...) usch_cmd(\"");
            bufstradd(&stmt_c, definition.p_symname);
            bufstradd(&stmt_c, "\", ##__VA_ARGS__)\n");
        }
        bufstradd(&stmt_c, "int ");

        bufstradd(&stmt_c, USCHSHELL_DYN_FUNCNAME);
        bufstradd(&stmt_c, "(struct uschshell_t *p_context)\n{\n\t");
        bufstradd(&stmt_c, input.p_str);

        bufstradd(&stmt_c, ";\n\treturn 0;\n}\n");
    }
    else if(is_definition(input.p_str))
    {
        FAIL_IF(pre_assign(input.p_str, &p_pre_assign) != 0);
        bufstradd(&stmt_c, p_pre_assign);
        bufstradd(&stmt_c, ";\n");

        bufstradd(&stmt_c, "int \n");
        bufstradd(&stmt_c, USCHSHELL_DYN_FUNCNAME);
        bufstradd(&stmt_c, "(struct uschshell_t *p_context)\n{\n");
        bufstradd(&stmt_c, "\tuschshell_define(p_context, sizeof(");
        bufstradd(&stmt_c, get_symname(p_pre_assign));
        bufstradd(&stmt_c, "), \"");
        bufstradd(&stmt_c, p_pre_assign);
        bufstradd(&stmt_c, "\");\n");

        FAIL_IF(post_assign(input.p_str, &p_post_assign));
        if (strlen(p_post_assign) > 0)
        {
            bufstradd(&stmt_c, "\t");
            bufstradd(&stmt_c, get_symname(p_pre_assign));
            bufstradd(&stmt_c, " = ");
            bufstradd(&stmt_c, p_post_assign);
            bufstradd(&stmt_c, ";\n");

            bufstradd(&stmt_c, "\tuschshell_store(p_context, \"");
            bufstradd(&stmt_c, p_pre_assign);
            bufstradd(&stmt_c, "\", (void*)&");
            bufstradd(&stmt_c, get_symname(p_pre_assign));
            bufstradd(&stmt_c, ");\n");

        }

        bufstradd(&stmt_c, "\treturn 0;\n}\n");
    }
    else /* as is */   
    {
        bufstradd(&stmt_c, "int ");
        bufstradd(&stmt_c, USCHSHELL_DYN_FUNCNAME);
        bufstradd(&stmt_c, "(struct uschshell_t *p_context)\n{\n\t");
        bufstradd(&stmt_c, input.p_str);

        bufstradd(&stmt_c, ";\n\treturn 0;\n}\n");
    }

    FAIL_IF(!fwrite_ok(stmt_c.p_str, p_stmt_c));

    fclose(p_stmt_c);
    p_stmt_c = NULL;
    usch_cmd("cat", p_tempfile);
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

    *(void **) (&uschshell_load_vars) = dlsym(p_handle, "uschshell_load_vars");

    FAIL_IF((p_error = dlerror()) != NULL);
    (*uschshell_load_vars)(p_context);

    *(void **) (&set_context) = dlsym(p_handle, "uschshell_set_context");
    FAIL_IF((p_error = dlerror()) != NULL);
    (*set_context)(p_context);

    *(void **) (&dyn_func) = dlsym(p_handle, USCHSHELL_DYN_FUNCNAME);
    FAIL_IF((p_error = dlerror()) != NULL);
    (*dyn_func)(p_context);

    *(void **) (&uschshell_store_vars) = dlsym(p_handle, "uschshell_store_vars");
    FAIL_IF((p_error = dlerror()) != NULL);
    (*uschshell_store_vars)(p_context);


end:
    free(definition.p_symname);
    if (p_handle)
        dlclose(p_handle);
    free(pp_path);
    free(p_tempfile);
    free(p_pre_assign);
    free(p_post_assign);
    free(p_fullpath_uschrc_h);
    free(stmt_c.p_str);
    free(input.p_str);
    free(pp_cmds);

    if (p_stmt_c != NULL)
        fclose(p_stmt_c);

    return status;
}

int uschshell_lib(struct uschshell_t *p_context, char *p_libname)
{
    int status = 0;
    uschshell_lib_t *p_lib = NULL;
    uschshell_lib_t *p_current_lib = NULL;

    FAIL_IF(p_context == NULL || p_libname == NULL);
    
    p_lib = calloc(sizeof(uschshell_lib_t) + strlen(p_libname), 1);
    FAIL_IF(p_lib == NULL);

    strcpy(p_lib->libname, p_libname);

    p_lib->p_handle = dlopen(p_libname, RTLD_LAZY);

    p_current_lib = p_context->p_libs;

    if (p_current_lib != NULL)
    {
        while (p_current_lib->p_next != NULL)
        {
            p_current_lib = p_current_lib->p_next;
        }
        p_current_lib->p_next = p_lib;
    }
    else
    {
        p_context->p_libs = p_lib;
    }
    p_lib = NULL;
end:
    free(p_lib);
    return status;
}

static int has_symbol(struct uschshell_t *p_context, const char* p_sym) 
{
    int status = 0;
    int symbol_found = 0;
    void *p_handle = NULL;
    char *p_error = NULL;
    int (*dyn_func)();
    uschshell_lib_t *p_lib = NULL;

    p_lib = p_context->p_libs;
    
    FAIL_IF(p_lib == NULL);

    do 
    {
        p_handle = p_lib->p_handle;

        FAIL_IF(p_handle == NULL);

        *(void **) (&dyn_func) = dlsym(p_handle, p_sym);
        if ((p_error = dlerror()) != NULL)
            symbol_found = 1;
        p_lib = p_lib->p_next;
    } while (p_lib != NULL);

end:
    (void)status;
    return symbol_found;
}

void* uschshell_getdyfnhandle(uschshell_t *p_context, const char *p_id)
{
    int status = 0;
    uschshell_dyfn_t *p_dyfns = NULL;
    uschshell_dyfn_t *p_dyfn = NULL;
    void *p_handle = NULL;

    if (p_context == NULL || p_id == NULL)
        return NULL;

    FAIL_IF(p_context->p_dyfns == NULL);
    p_dyfns = p_context->p_dyfns;

    HASH_FIND_STR(p_dyfns, p_id, p_dyfn);

    p_handle = p_dyfn->p_handle;
end:
    (void)status;
    return p_handle;
}
static enum CXChildVisitResult clang_visitor(
        CXCursor cursor, 
        CXCursor parent, 
        CXClientData p_client_data)
{
    (void)parent;
    uschshell_dyfn_t *p_dyfns = NULL;
    bufstr_t bufstr = {NULL, 0};
    int status = 0;
    char *p_fnstr = NULL;
    uschshell_dyfn_t *p_dyfn = NULL;
    CXString cxretkindstr = {NULL, 0};
    enum CXChildVisitResult res = CXChildVisit_Recurse;
    CXString cxstr = {NULL, 0};
    CXString cxid = {NULL, 0}; 
    uschshell_t *p_context = NULL;

    cxstr = clang_getCursorSpelling(cursor);
    p_context = (uschshell_t*)p_client_data;
    FAIL_IF(p_context == NULL);
    p_dyfns = p_context->p_dyfns;
    (void)p_dyfns;
    switch (cursor.kind) 
    {
        case CXCursor_FunctionDecl:
            {
                int num_args = -1;
                int i;
                CXType return_type;

                cxstr = clang_getCursorSpelling(cursor);
                ENDOK_IF(strncmp(clang_getCString(cxstr), "__builtin_", strlen("__builtin_")) == 0);
                ENDOK_IF(!has_symbol(p_context, clang_getCString(cxstr)));
                bufstr.p_str = calloc(1024, 1);
                bufstr.len = 1024;
                FAIL_IF(bufstr.p_str == NULL);
                cxid = clang_getCursorDisplayName(cursor);

                return_type = clang_getCursorResultType(cursor);
                cxretkindstr = clang_getTypeSpelling(return_type);

                bufstradd(&bufstr, clang_getCString(cxretkindstr));
                bufstradd(&bufstr, " ");
                bufstradd(&bufstr, clang_getCString(cxstr));

                bufstradd(&bufstr, "(");
                num_args = clang_Cursor_getNumArguments(cursor);
                for (i = 0; i < num_args; i++)
                {
                    CXString cxkindstr = {NULL, 0};
                    CXString cxargstr = {NULL, 0};
                    CXCursor argCursor;
                    CXType argType;

                    argCursor = clang_Cursor_getArgument(cursor, i);
                    argType = clang_getCursorType(argCursor);
                    cxkindstr = clang_getTypeSpelling(argType);
                    cxargstr = clang_getCursorSpelling(argCursor);
                    bufstradd(&bufstr, clang_getCString(cxkindstr)); 
                    bufstradd(&bufstr, " "); 
                    bufstradd(&bufstr, clang_getCString(cxargstr)); 
                    if (i != (num_args - 1))
                    {
                        bufstradd(&bufstr, ", "); 
                    }
                    clang_disposeString(cxargstr);
                }
                bufstradd(&bufstr, ")\n{\n");

                bufstradd(&bufstr, "\t");
                bufstradd(&bufstr, clang_getCString(cxretkindstr));
                bufstradd(&bufstr, " (*handle)(");
                for (i = 0; i < num_args; i++)
                {
                    CXString cxkindstr = {NULL, 0};
                    CXString cxargstr = {NULL, 0};
                    CXCursor argCursor;
                    CXType argType;

                    argCursor = clang_Cursor_getArgument(cursor, i);
                    argType = clang_getCursorType(argCursor);
                    cxkindstr = clang_getTypeSpelling(argType);
                    cxargstr = clang_getCursorSpelling(argCursor);
                    bufstradd(&bufstr, clang_getCString(cxkindstr)); 

                    if (i != (num_args - 1))
                    {
                        bufstradd(&bufstr, ", "); 
                    }
                    clang_disposeString(cxargstr);
                }
                bufstradd(&bufstr, ");\n");
                bufstradd(&bufstr, "\thandle = uschshell_getdyfnhandle(p_uschshell_context, \"");
                bufstradd(&bufstr, clang_getCString(cxid));
                bufstradd(&bufstr, "\");\n");
                bufstradd(&bufstr, "\treturn handle(");
                for (i = 0; i < num_args; i++)
                {
                    CXString cxargstr = {NULL, 0};
                    CXCursor argCursor;

                    argCursor = clang_Cursor_getArgument(cursor, i);
                    cxargstr = clang_getCursorSpelling(argCursor);
                    bufstradd(&bufstr, clang_getCString(cxargstr));
                    if (i != (num_args - 1))
                    {
                        bufstradd(&bufstr, ","); 
                    }
                    clang_disposeString(cxargstr);
                }
                bufstradd(&bufstr, ");\n}\n");
                p_dyfn = calloc(strlen(clang_getCString(cxid)) + 1 + sizeof(uschshell_dyfn_t), 1);
                FAIL_IF(p_dyfn == NULL);
                strcpy(p_dyfn->dyfnname, clang_getCString(cxid));
                p_dyfn->p_dyfndef = bufstr.p_str;
                bufstr.p_str = NULL;

                break;
            }
        default:
            {
                res = CXChildVisit_Continue;
                break;
            }
    }
    p_dyfn = NULL;
    free(p_fnstr);
    p_fnstr = NULL;
end:
    clang_disposeString(cxstr);
    clang_disposeString(cxid);
    clang_disposeString(cxretkindstr);
    (void)status;
    free(bufstr.p_str);
    free(p_fnstr);
    free(p_dyfn);
    return res;
}


static int loadsyms_from_header_ok(uschshell_t *p_context, char *p_includefile)
{
    int status = 0;
    CXTranslationUnit p_tu = NULL;
    CXIndex p_idx = NULL;
    unsigned int visitorstatus = 0;

    p_idx = clang_createIndex(0, 0);
    FAIL_IF(p_idx == NULL);

    p_tu = clang_parseTranslationUnit(p_idx, p_includefile, NULL, 0, NULL, 0, 0);
    FAIL_IF(p_tu == NULL);

    visitorstatus = clang_visitChildren(
            clang_getTranslationUnitCursor(p_tu),
            clang_visitor,
            (void*)p_context);
    FAIL_IF(visitorstatus != 0);
end:
    clang_disposeTranslationUnit(p_tu);
    clang_disposeIndex(p_idx);
    return status;
}

int uschshell_include(struct uschshell_t *p_context, char *p_header)
{
    int status = 0;
    char *p_tmpheader = NULL;
    char tmp_h[] = "tmp.h";
    FILE *p_includefile = NULL;
    char *p_tmpdir = NULL;
    uschshell_inc_t *p_inc = NULL;
    uschshell_inc_t *p_incs = NULL;

    FAIL_IF(p_context == NULL || p_header == NULL);

    p_incs = p_context->p_incs;
    p_tmpdir = p_context->tmpdir;
    p_tmpheader = calloc(strlen(p_tmpdir) + 1 + strlen(tmp_h) + 1, 1);
    strcpy(p_tmpheader, p_tmpdir);
    p_tmpheader[strlen(p_tmpheader)] = '/';
    strcpy(&p_tmpheader[strlen(p_tmpheader)], tmp_h);
    printf("header: %s\n", p_tmpheader);

    p_includefile = fopen(p_tmpheader, "w");
    FAIL_IF(p_includefile == NULL);
    if (p_header[0] == '<')
    {
        FAIL_IF(!fwrite_ok("#include ", p_includefile));
        FAIL_IF(!fwrite_ok(p_header, p_includefile));
        FAIL_IF(!fwrite_ok("\n", p_includefile));
    }
    else
    {
        FAIL_IF(!fwrite_ok("#include \"", p_includefile));
        FAIL_IF(!fwrite_ok(p_header, p_includefile));
        FAIL_IF(!fwrite_ok("\"\n", p_includefile));
    }
    if (p_includefile)
        fclose(p_includefile);
    p_includefile = NULL;
    FAIL_IF(loadsyms_from_header_ok(p_context, p_tmpheader) != 0);

    p_inc = calloc(strlen(p_header) + 1 + sizeof(uschshell_inc_t), 1);
    FAIL_IF(p_inc == NULL);
    strcpy(p_inc->incname, p_header);
    HASH_ADD_STR(p_incs, incname, p_inc);
end:
    free(p_tmpheader);
    if (p_includefile)
        fclose(p_includefile);
    return status;
}

struct usch_ids
{
    struct usch_ids *p_usch_ids;
    char name[];
};

typedef struct 
{
char *p_cur_id;
int found_cur_id;
} preparse_userdata_t;

//static 
enum CXChildVisitResult clang_preparseVisitor(
        CXCursor cursor, 
        CXCursor parent, 
        CXClientData p_client_data)
{
    (void)parent;
    bufstr_t bufstr = {NULL, 0};
    int status = 0;
    char *p_fnstr = NULL;
    uschshell_dyfn_t *p_dyfn = NULL;
    enum CXChildVisitResult res = CXChildVisit_Recurse;
    CXString cxstr = {NULL, 0};
    preparse_userdata_t *p_userdata = NULL;

    cxstr = clang_getCursorSpelling(cursor);
    p_userdata = (preparse_userdata_t*)p_client_data;
    FAIL_IF(p_userdata == NULL);
    switch (cursor.kind) 
    {
        case CXCursor_FunctionDecl:
            {
                if ((strncmp(clang_getCString(cxstr), p_userdata->p_cur_id, strlen(p_userdata->p_cur_id)) == 0) && strlen(clang_getCString(cxstr)) == strlen(p_userdata->p_cur_id))
                {
                    //printf("found_cur_id = 1\n");
                    p_userdata->found_cur_id = 1;
                }
                res = CXChildVisit_Recurse;

                break;
            }
        default:
            {
                //printf("\nfound::: %s\n",clang_getCString(cxstr));
                res = CXChildVisit_Recurse;
                break;
            }
    }
    free(p_fnstr);
    p_fnstr = NULL;
end:
    clang_disposeString(cxstr);
    (void)status;
    free(bufstr.p_str);
    free(p_fnstr);
    free(p_dyfn);
    return res;
}

//static
int ends_with_identifier(char *p_line)
{
    size_t i = 0;
    size_t len = 0;
    int has_identifier = 0;
    len = strlen(p_line);
    if (len == 0)
        return 0;
    i = len - 1;
    do 
    {
        if (isdigit(p_line[i]) == 0)
        {
            if (isalpha(p_line[i]) == 0)
            {
                break;
            }
            else
            {
                has_identifier = 1;
                break;
            }
        }
        i--;
    } while(i != 0);
    return has_identifier;
}

//static
void set_preparsefile_content(bufstr_t *p_bufstr, char* p_line, char **pp_identifiers)
{
    int i;
    p_bufstr->p_str[0] = '\0';
    bufstradd(p_bufstr, "struct uschshell_t;\n");
    bufstradd(p_bufstr, "#include <usch.h>\n");
    bufstradd(p_bufstr, "#include \"includes.h\"\n");
    bufstradd(p_bufstr, "#include \"definitions.h\"\n");
    bufstradd(p_bufstr, "#include \"trampolines.h\"\n");

    for (i = 0; pp_identifiers[i] != NULL; i++)
    {
        // clang doesn't parse preprocessor #defines
        // define a function instead
        bufstradd(p_bufstr, "void ");
        bufstradd(p_bufstr, pp_identifiers[i]);
        bufstradd(p_bufstr, "(...) {};\n");
    }
    bufstradd(p_bufstr, "int ");
    bufstradd(p_bufstr, USCHSHELL_DYN_FUNCNAME);
    bufstradd(p_bufstr, "()\n");
    bufstradd(p_bufstr, "{\n");
    bufstradd(p_bufstr, "\t");
    bufstradd(p_bufstr, p_line);
    if (has_trailing_identifier(p_line, pp_identifiers))
    {
        bufstradd(p_bufstr, "()");
    }
    else if (has_trailing_open_parenthesis(p_line))
    {
        bufstradd(p_bufstr, ")");
    }
    bufstradd(p_bufstr, ";\n\treturn 0;\n");
    bufstradd(p_bufstr, "}\n");
}

static int resolve_identifier(char *p_parsefile_fullname,
                 bufstr_t *p_filecontent,
                 char *p_line,
                 preparse_userdata_t *p_userdata,
                 char **pp_definitions)
{
    int status = 0;
    CXTranslationUnit p_tu = NULL;
    CXIndex p_idx = NULL;
    unsigned int visitorstatus = 0;

    FILE *p_parsefile = NULL;
    if (pp_definitions != NULL)
    {
        set_preparsefile_content(p_filecontent, p_line, pp_definitions);
    }

    p_parsefile = fopen(p_parsefile_fullname, "w");
    FAIL_IF(p_parsefile == NULL);

    FAIL_IF(!fwrite_ok(p_filecontent->p_str, p_parsefile));
    fclose(p_parsefile);
    p_parsefile = NULL;
    p_idx = clang_createIndex(0, 0);
    FAIL_IF(p_idx == NULL);
    p_tu = clang_parseTranslationUnit(p_idx, p_parsefile_fullname, NULL, 0, NULL, 0, 0);
    FAIL_IF(p_tu == NULL);

    visitorstatus = clang_visitChildren(
            clang_getTranslationUnitCursor(p_tu),
            clang_preparseVisitor,
            (void*)p_userdata);
    FAIL_IF(visitorstatus != 0);
    clang_disposeTranslationUnit(p_tu);
    clang_disposeIndex(p_idx);

    p_tu = NULL;
    p_idx = NULL;

end:
    if (p_parsefile)
        fclose(p_parsefile);
    return status;
}

int uschshell_preparse(struct uschshell_t *p_context, char *p_input, uschshell_state_t *p_state, char ***ppp_cmds)
{
    int i;
    preparse_userdata_t userdata = {0};
    uschshell_state_t state;
    int status = 0;
    bufstr_t filecontent = {0};
    char preparse_filename[] = "preparse.c";
    char *p_parsefile_fullname = NULL;
    char *p_line_copy = NULL;
    char *p_line = NULL;
    int num_identifiers = 0;
    char **pp_identifiers = NULL;
    char **pp_cmds = NULL;

    p_line_copy = strdup(p_input);
    FAIL_IF(p_line_copy == NULL);
    p_line = stripwhite(p_line_copy);
    
    status = get_identifiers(p_line, &num_identifiers, &pp_identifiers);
    FAIL_IF(status != 0 || num_identifiers == 0);

    pp_cmds = calloc((num_identifiers + 1)* sizeof(char*) + (strlen(p_line) + 1) * sizeof(char), 1);
    FAIL_IF(pp_cmds == NULL);
    memcpy((char*)&pp_cmds[num_identifiers + 1], p_line, strlen(p_line));
    // NULL terminate vector before it's content
    pp_cmds[num_identifiers + 1] = NULL;

    filecontent.p_str = calloc(1, 1024);
    FAIL_IF(filecontent.p_str == NULL);
    filecontent.len = 1024;

    p_parsefile_fullname = calloc(strlen(p_context->tmpdir) + 1 + strlen(preparse_filename), 1);
    FAIL_IF(p_parsefile_fullname == NULL);
    strcpy(p_parsefile_fullname, p_context->tmpdir);
    p_parsefile_fullname[strlen(p_context->tmpdir)] = '/';
    strcpy(&p_parsefile_fullname[strlen(p_context->tmpdir) + 1], preparse_filename);
    
    for (i = 0; pp_identifiers[i] != NULL; i++)
    {
        state = USCHSHELL_STATE_CPARSER;

        userdata.p_cur_id = pp_identifiers[i];
        userdata.found_cur_id = 0;

        FAIL_IF(resolve_identifier(p_parsefile_fullname, &filecontent, p_line, &userdata, NULL));

        // identifier is not defined
        if (userdata.found_cur_id == 0)
        {
            if (iscmd(userdata.p_cur_id))
            {
                // the identifier is available as a system command
                // try to define the identifier as a function

                userdata.found_cur_id = 0;
                FAIL_IF(resolve_identifier(p_parsefile_fullname, &filecontent, p_line, &userdata, pp_identifiers));

                // unknown error
                if (userdata.found_cur_id == 0)
                {
                        state = USCHSHELL_STATE_ERROR;
                        break;
                }
                // TODO: if last identifer is a system command, we probably are in some parameter unless...
                // 
                // ... there is a nested command (ARGH!)
                else if (pp_identifiers[i+1] == NULL)
                {
                    if (has_trailing_closed_parenthesis(p_line))
                    {
                        state = USCHSHELL_STATE_CPARSER;
                    }
                    else if (has_trailing_open_parenthesis(p_line))
                    {
                        state = USCHSHELL_STATE_CMDARG;
                    } 
                    else
                    {
                        state = USCHSHELL_STATE_CMDSTART;
                    }
                    break;
                }
            }
            else
            {
                // the identifier could not be resolved, nor is it a system command
                state = USCHSHELL_STATE_ERROR;
                break;
            }
        }
        state = USCHSHELL_STATE_CPARSER;
    }
    *p_state = state;
    *ppp_cmds = pp_cmds;
    pp_cmds = NULL;
end:
    free(pp_identifiers);
    p_line = NULL;
    free(p_parsefile_fullname);
    free(p_line_copy);
    free(pp_cmds);

    free(filecontent.p_str);
    return status;
}

