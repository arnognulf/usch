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
#include "usch.h"
#include "crepl_debug.h"
#include "bufstr.h"
#include "crepl_parser.h"
#include "crepl_types.h"
#include "strutils.h"


#include "crepl.h"
#include "../external/uthash/src/uthash.h"


#define CREPL_DYN_FUNCNAME "usch_dyn_func"

// TODO: DUPE of crepl_vars.c
// this function is a naive parser and should probably not be used anyway
static char *get_symname(char *p_defname)
{
    int len = (int)strlen(p_defname);
    int i = len - 1;
    char *p_symname = p_defname;
    if (len == 0)
        return &p_defname[len-1];
    p_symname = &p_defname[len];
    while (i > 0 && !isalnum(p_defname[i]) && p_defname[i] != ';')
        i--;
    while (i >= 0)
    {
        if (isalnum(p_defname[i]))
        {
            i--;
        }
        else
        {
            if (isalpha(p_defname[i+1]))
            {
                p_symname = &p_defname[i+1];
            }
            break;
        }
    }
    if (i == 0 && isalpha(p_defname[i]))
    {
        p_symname = &p_defname[i];
    }
    return p_symname;
}


static int append_definitions(crepl_t *p_context, bufstr_t *p_bufstr, char *p_defs_line_in)
{
    (void)p_context;
    int status = 0;
    int i = 0;
    int start = 0;
    char *p_defs_line = NULL;
    p_defs_line = strdup(p_defs_line_in);
    FAIL_IF(p_defs_line == NULL);

    while (p_defs_line[i] == '\t' || p_defs_line[i] == ' ')
    {
        i++;
    }
    start = i;
    while (p_defs_line[i] != '\0')
    {
        if (p_defs_line[i] == ';')
        {
            int nulpos = i;
            while ((nulpos-1) >= 0)
            {
                if (p_defs_line[nulpos-1] == ' ' || p_defs_line[nulpos-1] == '\t')
                {
                    nulpos--;
                }
                else
                {
                    break;
                }
            }
            p_defs_line[nulpos] = '\0';
            if (strlen(get_symname(&p_defs_line[start])) == 0)
            {
                i++;
                continue;
            }
            bufstradd(p_bufstr, "(void)crepl_define(p_context, sizeof(");
            bufstradd(p_bufstr, get_symname(&p_defs_line[start]));
            bufstradd(p_bufstr, "), \"");
            bufstradd(p_bufstr, &p_defs_line[start]);
            bufstradd(p_bufstr, "\");\n");
        }
        i++;
    }
end:
    free(p_defs_line);
    return status;
}

static int append_storedefs(crepl_t *p_context, bufstr_t *p_bufstr, char *p_defs_line_in)
{
    int status = 0;
    (void)p_context;
    int i = 0;
    int start = 0;
    char *p_defs_line = NULL;
    p_defs_line = strdup(p_defs_line_in);
    FAIL_IF(p_defs_line == NULL);

    while (p_defs_line[i] == '\t' || p_defs_line[i] == ' ')
    {
        i++;
    }
    start = i;
    while (p_defs_line[i] != '\0')
    {
        if (p_defs_line[i] == ';')
        {
            int nulpos = i;
            while ((nulpos-1) >= 0)
            {
                if (p_defs_line[nulpos-1] == ' ' || p_defs_line[nulpos-1] == '\t')
                {
                    nulpos--;
                }
                else
                {
                    break;
                }
            }
            p_defs_line[nulpos] = '\0';
            if (strlen(get_symname(&p_defs_line[start])) == 0)
            {
                i++;
                continue;
            }
            bufstradd(p_bufstr, "\tcrepl_store(p_context, \"");
            bufstradd(p_bufstr, &p_defs_line[start]);
            bufstradd(p_bufstr, "\", (void*)&");
            bufstradd(p_bufstr, get_symname(&p_defs_line[start]));
            bufstradd(p_bufstr, ");\n");

        }
        i++;
    }
end:
    free(p_defs_line);
    return status;
}



static int write_definitions_h(crepl_t *p_context, char *p_tempdir)
{
    int status = 0;
    char definitions_h_filename[] = "definitions.h";
    size_t filename_length;
    size_t tempdir_len;
    char *p_definitionsfile = NULL;
    FILE *p_definitions_h = NULL;
    crepl_def_t *p_defs = NULL;
    crepl_def_t *p_def = NULL;
    crepl_def_t *p_tmp = NULL;
    bufstr_t definitions_h;

    definitions_h.len = 1024;
    definitions_h.p_str = calloc(definitions_h.len, 1);
    FAIL_IF(definitions_h.p_str == NULL);
    (void)definitions_h.p_str;

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
    bufstradd(&definitions_h, p_context->p_defs_line);
    bufstradd(&definitions_h, ";");
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            bufstradd(&definitions_h, p_def->defname);
            bufstradd(&definitions_h, ";\n");
        }
    }
    bufstradd(&definitions_h, "\nvoid crepl_load_vars(struct crepl_t *p_context)\n{\n");
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            bufstradd(&definitions_h, "\tcrepl_load(p_context, \"");
            bufstradd(&definitions_h, p_def->defname);
            bufstradd(&definitions_h, "\", (void*)&");
            bufstradd(&definitions_h, get_symname(p_def->defname));
            bufstradd(&definitions_h, ");\n");
        }
    }
    FAIL_IF(append_definitions(p_context, &definitions_h, p_context->p_defs_line));

    bufstradd(&definitions_h, "\n}\n");

    bufstradd(&definitions_h, "\nvoid crepl_store_vars(struct crepl_t *p_context)\n{\n");
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            bufstradd(&definitions_h, "\tcrepl_store(p_context, \"");
            bufstradd(&definitions_h, p_def->defname);
            bufstradd(&definitions_h, "\", (void*)&");
            bufstradd(&definitions_h, get_symname(p_def->defname));
            bufstradd(&definitions_h, ");\n");
        }
    }
    FAIL_IF(append_storedefs(p_context, &definitions_h, p_context->p_defs_line));

    bufstradd(&definitions_h, "\n}\n");
    FAIL_IF(!fwrite_ok(definitions_h.p_str, p_definitions_h));
    
end:
    free(definitions_h.p_str);
    if(p_definitions_h)
        fclose(p_definitions_h);
    free(p_definitionsfile);
    return status;
}

static int write_includes_h(crepl_t *p_context, char *p_tempdir)
{
    int status = 0;
    char includes_h_filename[] = "includes.h";
    size_t filename_length;
    size_t tempdir_len;
    char *p_includesfile = NULL;
    FILE *p_includes_h = NULL;
    crepl_inc_t *p_incs = NULL;
    crepl_inc_t *p_inc = NULL;
    crepl_inc_t *p_tmp = NULL;

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


static int write_trampolines_h(crepl_t *p_context, char *p_tempdir)
{
    int status = 0;
    char trampolines_h_filename[] = "trampolines.h";
    size_t filename_length;
    size_t tempdir_len;
    char *p_trampolinesfile = NULL;
    FILE *p_trampolines_h = NULL;
    crepl_dyfn_t *p_dyfns = NULL;
    crepl_dyfn_t *p_dyfn = NULL;
    crepl_dyfn_t *p_tmp = NULL;

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

#if 0
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
#endif // 0
#define usch_shell_cc(...) usch_cmd("gcc", ##__VA_ARGS__)

int crepl_eval(crepl_t *p_context, char *p_input_line)
{
    int i = 0;
    int status = 0;
    usch_def_t definition = {0};
    FILE *p_stmt_c = NULL;
    void *p_handle = NULL;
    int (*dyn_func)(crepl_t*);
    int (*set_context)(crepl_t*);
    int (*crepl_store_vars)(crepl_t*);
    int (*crepl_load_vars)(crepl_t*);
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
    crepl_state_t state;

    if (p_context == NULL || p_input_line == NULL)
        return -1;

    input.p_str = NULL;
    FAIL_IF(crepl_finalize(p_input_line, &input.p_str));
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

    FAIL_IF(crepl_preparse(p_context, input.p_str, &state));
    FAIL_IF(crepl_parsedefs(p_context, input.p_str));
    free(input.p_str);
    input.p_str = p_context->p_nodef_line;
    input.len = strlen(p_context->p_nodef_line);
    p_context->p_nodef_line = NULL;

    pp_cmds = p_context->pp_cmds;
    
    if (state == CREPL_STATE_CMDSTART)
    {
        bufstradd(&input, "()");
    }
    if (state == CREPL_STATE_CMDARG)
    {
        size_t quotes = 0;
        size_t rparens = 0;
        size_t lparens = 0;
        for (i = 0; i < (int)input.len; i++)
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

    bufstradd(&stmt_c, "struct crepl_t;\n");
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

    bufstradd(&stmt_c, "static struct crepl_t *p_crepl_context = NULL;\n");
    bufstradd(&stmt_c, "void crepl_set_context(struct crepl_t *p_context)\n{\np_crepl_context = p_context;\t\n}\n");

    for (i = 0; pp_cmds[i] != NULL; i++)
    {
        if (strcmp(pp_cmds[i], "cd") == 0)
        {
            ;
        }
        else if (strcmp(pp_cmds[i], "lib") == 0)
        {
            ;
        }
        else if (strcmp(pp_cmds[i], "header") == 0)
        {
            ;
        }
        else
        {
            bufstradd(&stmt_c, "#define ");
            bufstradd(&stmt_c, definition.p_symname);
            bufstradd(&stmt_c, "(...) usch_cmd(\"");
            bufstradd(&stmt_c, definition.p_symname);
            bufstradd(&stmt_c, "\", ##__VA_ARGS__)\n");
        }
    }

#if 0
    if (iscmd(input.p_str))
    {
        if (strcmp(definition.p_symname, "include") == 0)
        {
            bufstradd(&stmt_c, "#define include");
            bufstradd(&stmt_c, "(header) crepl_include(p_crepl_context, (header))\n");
        }
        else if (strcmp(definition.p_symname, "lib") == 0){
            bufstradd(&stmt_c, "#define lib");
            bufstradd(&stmt_c, "(libname) crepl_lib(p_crepl_context, (libname))\n");

        }

        bufstradd(&stmt_c, "int ");

        bufstradd(&stmt_c, CREPL_DYN_FUNCNAME);
        bufstradd(&stmt_c, "(struct crepl_t *p_context)\n{\n\t");
        bufstradd(&stmt_c, input.p_str);

        bufstradd(&stmt_c, ";\n\treturn 0;\n}\n");
    }
    else if(is_definition(input.p_str))
    {
        FAIL_IF(pre_assign(input.p_str, &p_pre_assign) != 0);
        bufstradd(&stmt_c, p_pre_assign);
        bufstradd(&stmt_c, ";\n");

        bufstradd(&stmt_c, "int \n");
        bufstradd(&stmt_c, CREPL_DYN_FUNCNAME);
        bufstradd(&stmt_c, "(struct crepl_t *p_context)\n{\n");
        bufstradd(&stmt_c, "\tcrepl_define(p_context, sizeof(");
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

            bufstradd(&stmt_c, "\tcrepl_store(p_context, \"");
            bufstradd(&stmt_c, p_pre_assign);
            bufstradd(&stmt_c, "\", (void*)&");
            bufstradd(&stmt_c, get_symname(p_pre_assign));
            bufstradd(&stmt_c, ");\n");

        }

        bufstradd(&stmt_c, "\treturn 0;\n}\n");
    }
    else /* as is */   
#endif // 0
    {
        bufstradd(&stmt_c, "int ");
        bufstradd(&stmt_c, CREPL_DYN_FUNCNAME);
        bufstradd(&stmt_c, "(struct crepl_t *p_context)\n{\n\t");
        bufstradd(&stmt_c, input.p_str);

        bufstradd(&stmt_c, ";\n\treturn 0;\n}\n");
    }

    FAIL_IF(!fwrite_ok(stmt_c.p_str, p_stmt_c));

    fclose(p_stmt_c);
    p_stmt_c = NULL;
//    usch_cmd("cat", p_tempfile);
    dylib_length = tempdir_len + 1 + strlen(dylib_filename) + 1;
    p_tempdylib = malloc(dylib_length);
    FAIL_IF(p_tempdylib == NULL);
    strcpy(p_tempdylib, p_tempdir);
    p_tempdylib[tempdir_len] = '/';
    strcpy(&p_tempdylib[tempdir_len + 1], dylib_filename);
    p_tempdylib[dylib_length-1] = '\0';
    if (usch_shell_cc("-rdynamic", "-Werror", "-shared", "-fPIC", "-o", p_tempdylib, p_tempfile) != 0) 
    {
        fprintf(stderr, "usch: compile error\n");
        ENDOK_IF(1);
    }

    p_handle = dlopen(p_tempdylib, RTLD_LAZY);
    FAIL_IF(!p_handle);

    dlerror();

    *(void **) (&crepl_load_vars) = dlsym(p_handle, "crepl_load_vars");

    FAIL_IF((p_error = dlerror()) != NULL);
    (*crepl_load_vars)(p_context);

    *(void **) (&set_context) = dlsym(p_handle, "crepl_set_context");
    FAIL_IF((p_error = dlerror()) != NULL);
    (*set_context)(p_context);

    *(void **) (&dyn_func) = dlsym(p_handle, CREPL_DYN_FUNCNAME);
    FAIL_IF((p_error = dlerror()) != NULL);
    (*dyn_func)(p_context);

    *(void **) (&crepl_store_vars) = dlsym(p_handle, "crepl_store_vars");
    FAIL_IF((p_error = dlerror()) != NULL);
    (*crepl_store_vars)(p_context);


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
    //free(pp_cmds);

    if (p_stmt_c != NULL)
        fclose(p_stmt_c);

    return status;
}


