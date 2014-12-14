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

#ifndef USCH_INSTALL_PREFIX
#define USCH_INSTALL_PREFIX "/usr/local"
#endif

#define CREPL_DYN_FUNCNAME "usch_dyn_func"

static char* find_uschrc(ustash *p_stash);

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
    int has_semicolons = 0;
    char *p_defs_line = NULL;
    if (p_defs_line_in == NULL)
        ENDOK_IF(1);
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
            has_semicolons = 1;
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
    if (!has_semicolons && strlen(get_symname(&p_defs_line[start])) > 0)
    {
        bufstradd(p_bufstr, "(void)crepl_define(p_context, sizeof(");
        bufstradd(p_bufstr, get_symname(&p_defs_line[start]));
        bufstradd(p_bufstr, "), \"");
        bufstradd(p_bufstr, &p_defs_line[start]);
        bufstradd(p_bufstr, "\");\n");
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
    int has_semicolons = 0;
    char *p_defs_line = NULL;
    if (p_defs_line_in == NULL)
        ENDOK_IF(1);

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
            has_semicolons = 1;
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
    if (!has_semicolons && strlen(get_symname(&p_defs_line[start])) > 0)
    {
        bufstradd(p_bufstr, "\tcrepl_store(p_context, \"");
        bufstradd(p_bufstr, &p_defs_line[start]);
        bufstradd(p_bufstr, "\", (void*)&");
        bufstradd(p_bufstr, get_symname(&p_defs_line[start]));
        bufstradd(p_bufstr, ");\n");
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
    FAIL_IF(append_definitions(p_context, &definitions_h, p_context->p_defs_line));
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
#define usch_shell_cc(...) ucmd("gcc", ##__VA_ARGS__)

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
    char *p_fullpath_uschrc_h = NULL;
    ustash s = {NULL};

    char expr_c_filename[] = "expr.c";
    char dylib_filename[] = "dyn_stmt";
    char *p_tempdir = NULL;
    char *p_tempfile = NULL;
    char *p_tempdylib = NULL;
    size_t tempdir_len = 0;
    char *p_pre_assign = NULL;
    char *p_post_assign = NULL;
    char *p_stmt = NULL;
    crepl_state_t state;

    if (p_context == NULL || p_input_line == NULL)
        return -1;

    input.p_str = NULL;
    FAIL_IF(crepl_finalize(p_input_line, &input.p_str));
    FAIL_IF(input.p_str == NULL);
    input.len = strlen(p_input_line);

    pp_path = ustrsplit(&s, getenv("PATH"), ":");
    FAIL_IF(pp_path[0] == NULL);
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
    if (state != CREPL_STATE_PREPROCESSOR)
    {
        FAIL_IF(crepl_parsedefs(p_context, input.p_str));
        free(input.p_str);
        input.p_str = p_context->p_nodef_line;
        input.len = strlen(p_context->p_nodef_line);
        p_context->p_nodef_line = NULL;
    }

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
    if (state == CREPL_STATE_PREPROCESSOR)
    {
        bufstr_t preproc_cmd;
        int i = 0;
        while (input.p_str[i] == ' ' || input.p_str[i] == '\t')
            i++;

        if (strncmp(&input.p_str[i], "#lib ", strlen("#lib ")) == 0)
        {
            preproc_cmd.len = 128;
            preproc_cmd.p_str = calloc(preproc_cmd.len, 1);
            FAIL_IF(preproc_cmd.p_str == NULL);

            bufstradd(&preproc_cmd, "\t(void)crepl_lib(p_crepl_context, \"");
            ///usr/lib/x86_64-linux-gnu/libm.so"); // "m"
            bufstradd(&preproc_cmd, &input.p_str[i+strlen("#lib ")]);
            bufstradd(&preproc_cmd, "\");");
            free(input.p_str);
            input.p_str = preproc_cmd.p_str;
            input.len = preproc_cmd.len;
        }
        else if (strncmp(&input.p_str[i], "#lib", strlen("#lib")) == 0)
        { 
             printf("usch: invalid #lib macro directive. usage:\n#lib /path/to/library.so\n");
        }
        if (strncmp(&input.p_str[i], "#include ", strlen("#include ")) == 0)
        {
            preproc_cmd.len = 128;
            preproc_cmd.p_str = calloc(preproc_cmd.len, 1);
            FAIL_IF(preproc_cmd.p_str == NULL);

            bufstradd(&preproc_cmd, "\t(void)crepl_include(p_crepl_context, \"");
            ///usr/lib/x86_64-linux-gnu/libm.so"); // "m"
            bufstradd(&preproc_cmd, &input.p_str[i+strlen("#include ")]);
            bufstradd(&preproc_cmd, "\");");
            free(input.p_str);
            input.p_str = preproc_cmd.p_str;
            input.len = preproc_cmd.len;

        }
        else if (strncmp(&input.p_str[i], "#include", strlen("#include")) == 0)
        { 
             printf("usch: invalid #include macro directive. usage:\n#include <header.h>\nor:\n#include \"header.h\"\n");
        }
    }


    FAIL_IF(write_includes_h(p_context, p_tempdir) != 0);
    FAIL_IF(write_definitions_h(p_context, p_tempdir) != 0);
    FAIL_IF(write_trampolines_h(p_context, p_tempdir) != 0);

    FAIL_IF(parse_line(input.p_str, &definition) < 1);

    p_stmt = ustrjoin(&s, "struct crepl_t;\n", \
                          "#include <usch.h>\n", \
                          "#include \"includes.h\"\n", \
                          "#include \"definitions.h\"\n", \
                          "#include \"trampolines.h\"\n");

    p_fullpath_uschrc_h = find_uschrc(&s);

    p_stmt = ustrjoin(&s, p_stmt,
                          "#include \"",
                          p_fullpath_uschrc_h,
                          "\"\n");

    p_stmt = ustrjoin(&s, p_stmt,
            "static struct crepl_t *p_crepl_context = NULL;\n",
            "void crepl_set_context(struct crepl_t *p_context)\n"
            "{\n",
            "\tp_crepl_context = p_context;\n",
            "}\n");

    if (pp_cmds != NULL)
    {
        for (i = 0; pp_cmds[i] != NULL; i++)
        {
            if (strcmp(pp_cmds[i], "cd") != 0)
            {
                p_stmt = ustrjoin(&s, p_stmt,
                                      "#define ",
                                      definition.p_symname,
                                      "(...) ucmd(\"",
                                      definition.p_symname,
                                      "\", ##__VA_ARGS__)\n");
            }
        }
    }

    p_stmt = ustrjoin(&s, p_stmt,
                          "int ",
                          CREPL_DYN_FUNCNAME,
                          "(struct crepl_t *p_context)\n{\n\t",
                          input.p_str,
                          ";\n\treturn 0;\n}\n");

    FAIL_IF(!fwrite_ok(p_stmt, p_stmt_c));

    fclose(p_stmt_c);
    p_stmt_c = NULL;
    p_stmt = NULL;
//    ucmd("cat", p_tempfile);
    dylib_length = tempdir_len + 1 + strlen(dylib_filename) + 1;
    p_tempdylib = malloc(dylib_length);
    FAIL_IF(p_tempdylib == NULL);
    strcpy(p_tempdylib, p_tempdir);
    p_tempdylib[tempdir_len] = '/';
    strcpy(&p_tempdylib[tempdir_len + 1], dylib_filename);
    p_tempdylib[dylib_length-1] = '\0';
    if (usch_shell_cc("-I/home/arno/Workspace/usch2/src", "-rdynamic", "-Werror", "-shared", "-fPIC", "-o", p_tempdylib, p_tempfile) != 0) 
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
    pp_path = NULL; // stashed
    free(definition.p_symname);
    if (p_handle)
        dlclose(p_handle);
    free(p_tempfile);
    free(p_pre_assign);
    free(p_post_assign);
    free(input.p_str);
    free(p_context->p_nodef_line);
    p_context->p_nodef_line = NULL;
    free(p_context->p_defs_line);
    p_context->p_defs_line = NULL;
    uclear(&s);

    //free(pp_cmds);

    if (p_stmt_c != NULL)
        fclose(p_stmt_c);

    return status;
}

static char* find_uschrc(ustash *p_stash)
{
    int error = 0;
    struct stat sb;
    char *p_fullpath_uschrc_h = NULL;
    char *p_uschrc_cand = NULL;
    p_uschrc_cand = ustrjoin(p_stash, getenv("HOME"), "/.uschrc.h");
    if (stat(p_fullpath_uschrc_h, &sb) != -1)
    {
        p_fullpath_uschrc_h = p_uschrc_cand;
    }
    else
    {
        p_uschrc_cand = ustrjoin(p_stash, USCH_INSTALL_PREFIX, "/etc/uschrc.h");
        if (stat(p_fullpath_uschrc_h, &sb) != -1)
        {
            p_fullpath_uschrc_h = p_uschrc_cand;
        }
        else
        {
            p_uschrc_cand = ustrjoin(p_stash, "/etc/uschrc.h");
            if (stat(p_fullpath_uschrc_h, &sb) == -1)
            {
                error = 1;
            }

        }
    }
    return p_fullpath_uschrc_h;
}

