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
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <libtcc.h>

#include "../usch_h/usch.h"
#include "crepl_debug.h"
#include "bufstr.h"
#include "crepl_parser.h"
#include "crepl_types.h"
#include "strutils.h"


#include "crepl.h"
#include "../external/uthash/src/uthash.h"


// TODO: DUPE of crepl_vars.c
// this function is a naive parser and should probably not be used anyway
static char *get_symname(char *p_defname)
{    
    if (p_defname[0] == '\0')
        return p_defname;

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


static int append_definitions(crepl *p_crepl, bufstr_t *p_bufstr, char *p_defs_line_in)
{
    (void)p_crepl;
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
            bufstradd(p_bufstr, "(void)crepl_define(p_crepl, sizeof(");
            bufstradd(p_bufstr, get_symname(&p_defs_line[start]));
            bufstradd(p_bufstr, "), \"");
            bufstradd(p_bufstr, &p_defs_line[start]);
            bufstradd(p_bufstr, "\");\n");
        }
        i++;
    }
    if (!has_semicolons && strlen(get_symname(&p_defs_line[start])) > 0)
    {
        bufstradd(p_bufstr, "(void)crepl_define(p_crepl, sizeof(");
        bufstradd(p_bufstr, get_symname(&p_defs_line[start]));
        bufstradd(p_bufstr, "), \"");
        bufstradd(p_bufstr, &p_defs_line[start]);
        bufstradd(p_bufstr, "\");\n");
    }
end:
    free(p_defs_line);
    return status;
}

static int append_storedefs(crepl *p_crepl, bufstr_t *p_bufstr, char *p_defs_line_in)
{
    int status = 0;
    (void)p_crepl;
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
            bufstradd(p_bufstr, "    crepl_store(p_crepl, \"");
            bufstradd(p_bufstr, &p_defs_line[start]);
            bufstradd(p_bufstr, "\", (void*)&");
            bufstradd(p_bufstr, get_symname(&p_defs_line[start]));
            bufstradd(p_bufstr, ");\n");

        }
        i++;
    }
    if (!has_semicolons && strlen(get_symname(&p_defs_line[start])) > 0)
    {
        bufstradd(p_bufstr, "    crepl_store(p_crepl, \"");
        bufstradd(p_bufstr, &p_defs_line[start]);
        bufstradd(p_bufstr, "\", (void*)&");
        bufstradd(p_bufstr, get_symname(&p_defs_line[start]));
        bufstradd(p_bufstr, ");\n");
    }
end:
    free(p_defs_line);
    return status;
}

static int write_definitions_h(crepl *p_crepl)
{
    int status = 0;
    FILE *p_definitions_h = NULL;
    crepl_def_t *p_defs = NULL;
    crepl_def_t *p_def = NULL;
    crepl_def_t *p_tmp = NULL;
    bufstr_t definitions_h;

    definitions_h.len = 1024;
    definitions_h.p_str = calloc(definitions_h.len, 1);
    FAIL_IF(definitions_h.p_str == NULL);
    (void)definitions_h.p_str;

    p_defs = p_crepl->p_defs;

    p_definitions_h = fopen(p_crepl->p_defs_h, "w+");
    FAIL_IF(p_definitions_h == NULL);
    bufstradd(&definitions_h, p_crepl->p_defs_line);
    bufstradd(&definitions_h, ";");
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            bufstradd(&definitions_h, p_def->defname);
            bufstradd(&definitions_h, ";\n");
        }
    }
    bufstradd(&definitions_h, "\nvoid crepl_load_vars(struct crepl *p_crepl)\n{\n");
    FAIL_IF(append_definitions(p_crepl, &definitions_h, p_crepl->p_defs_line));
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            bufstradd(&definitions_h, "    crepl_load(p_crepl, \"");
            bufstradd(&definitions_h, p_def->defname);
            bufstradd(&definitions_h, "\", (void*)&");
            bufstradd(&definitions_h, get_symname(p_def->defname));
            bufstradd(&definitions_h, ");\n");
        }
    }

    bufstradd(&definitions_h, "\n}\n");

    bufstradd(&definitions_h, "\nvoid crepl_store_vars(struct crepl *p_crepl)\n{\n");
    HASH_ITER(hh, p_defs, p_def, p_tmp)
    {
        if (strcmp(p_def->defname, "") != 0)
        {
            bufstradd(&definitions_h, "    crepl_store(p_crepl, \"");
            bufstradd(&definitions_h, p_def->defname);
            bufstradd(&definitions_h, "\", (void*)&");
            bufstradd(&definitions_h, get_symname(p_def->defname));
            bufstradd(&definitions_h, ");\n");
        }
    }
    FAIL_IF(append_storedefs(p_crepl, &definitions_h, p_crepl->p_defs_line));

    bufstradd(&definitions_h, "\n}\n");
    FAIL_IF(!fwrite_ok(definitions_h.p_str, p_definitions_h));
    
end:
    free(definitions_h.p_str);
    if(p_definitions_h)
        fclose(p_definitions_h);
    return status;
}

static int write_includes_h(crepl *p_crepl)
{
    int status = 0;
    FILE *p_includes_h = NULL;
    crepl_inc_t *p_incs = NULL;
    crepl_inc_t *p_inc = NULL;
    crepl_inc_t *p_tmp = NULL;

    p_incs = p_crepl->p_incs;

    p_includes_h = fopen(p_crepl->p_incs_h, "w+");
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
    return status;
}


static int write_trampolines_h(crepl *p_crepl)
{
    int status = 0;
    FILE *p_trampolines_h = NULL;
    crepl_dyfn_t *p_dyfns = NULL;
    crepl_dyfn_t *p_dyfn = NULL;
    crepl_dyfn_t *p_tmp = NULL;

    p_dyfns = p_crepl->p_dyfns;

    p_trampolines_h = fopen(p_crepl->p_trampolines_h, "w+");
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
    return status;
}

#define usch_shell_cc(...) ucmd("clang", ##__VA_ARGS__)

E_CREPL crepl_eval(crepl *p_crepl, char *p_input_line)
{
    int i = 0;
    int tccstatus = 0;
    E_CREPL estatus = E_CREPL_OK;
    usch_def_t definition = {0};
    FILE *p_stmt_file = NULL;
    int (*dyn_func)(crepl*);
    int (*set_context)(crepl*);
    int (*crepl_store_vars)(crepl*);
    int (*crepl_load_vars)(crepl*);
    char* (*uschrc_prompt)(ustash*);
    void (*uschrc_init)(ustash*);
    char **pp_cmds = NULL;
    bufstr_t input;
    // TODO: we need to determine wether stmt need to be usch-defined or not
    // declare dummy function to get overridden errors
    // use macro to call the real function
    ustash s = {NULL};
    TCCState *p_tcc = NULL;
    void *mem = NULL;

    char *p_pre_assign = NULL;
    char *p_post_assign = NULL;
    crepl_state_t state;
    char *p_stmt = NULL;

    E_FAIL_IF(p_crepl == NULL || p_input_line == NULL);

    input.p_str = NULL;
    E_FAIL_IF(crepl_finalize(p_crepl, p_input_line, &input.p_str));
    E_FAIL_IF(input.p_str == NULL);
    input.len = strlen(p_input_line);

    p_stmt_file = fopen(p_crepl->p_stmt_c, "w+");
    E_FAIL_IF(p_stmt_file == NULL);

    E_FAIL_IF(crepl_preparse(p_crepl, input.p_str, &state));
    if (state != CREPL_STATE_PREPROCESSOR)
    {
        E_FAIL_IF(crepl_parsedefs(p_crepl, input.p_str));
        free(input.p_str);
        input.p_str = p_crepl->p_nodef_line;
        input.len = strlen(p_crepl->p_nodef_line);
        p_crepl->p_nodef_line = NULL;
    }

    pp_cmds = p_crepl->pp_cmds;
    
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
            E_FAIL_IF(preproc_cmd.p_str == NULL);

            bufstradd(&preproc_cmd, "    (void)crepl_lib(p_crepl_context, \"");
            bufstradd(&preproc_cmd, &input.p_str[i+strlen("#lib ")]);
            bufstradd(&preproc_cmd, "\");");
            free(input.p_str);
            input.p_str = preproc_cmd.p_str;
            input.len = preproc_cmd.len;
        }
        else if (strncmp(&input.p_str[i], "#lib", strlen("#lib")) == 0)
        { 
             fprintf(stderr, "usch: invalid #lib macro directive. usage:\n#lib /path/to/library.so\n");
        }
        if (strncmp(&input.p_str[i], "#include ", strlen("#include ")) == 0)
        {
            preproc_cmd.len = 128;
            preproc_cmd.p_str = calloc(preproc_cmd.len, 1);
            E_FAIL_IF(preproc_cmd.p_str == NULL);

            bufstradd(&preproc_cmd, "    (void)crepl_include(p_crepl_context, \"");
            bufstradd(&preproc_cmd, &input.p_str[i+strlen("#include ")]);
            bufstradd(&preproc_cmd, "\");");
            free(input.p_str);
            input.p_str = preproc_cmd.p_str;
            input.len = preproc_cmd.len;

        }
        else if (strncmp(&input.p_str[i], "#include", strlen("#include")) == 0)
        { 
             fprintf(stderr, "usch: invalid #include macro directive. usage:\n#include <header.h>\nor:\n#include \"header.h\"\n");
        }
    }

    E_FAIL_IF(write_includes_h(p_crepl) != 0);
    E_FAIL_IF(write_definitions_h(p_crepl) != 0);
    E_FAIL_IF(write_trampolines_h(p_crepl) != 0);

    E_FAIL_IF(parse_line(input.p_str, &definition) < 1);

    p_stmt = ustrjoin(&s, p_crepl->p_stmt_header);

    if (pp_cmds != NULL)
    {
        for (i = 0; pp_cmds[i] != NULL; i++)
        {
            if (strcmp(pp_cmds[i], "cd") != 0 &&
                strcmp(definition.p_symname, "")   != 0) /* #include and #lib directives leaves the definiton empty */
            {
                p_stmt = ustrjoin(&s, p_stmt,
                                      "#ifndef ", definition.p_symname,"\n"
                                      "#define ", definition.p_symname,
                                      "(...) ucmd(\"", definition.p_symname, "\", ##__VA_ARGS__)\n",
                                      "#endif\n");

            }
        }
    }

    p_stmt = ustrjoin(&s, p_stmt,
		      "struct crepl;\n",
                      "int ", CREPL_DYN_FUNCNAME, "(struct crepl *p_crepl)\n", \
                      "{\n", \
                      CREPL_INDENT, input.p_str, ";\n", \
                      CREPL_INDENT, "return 0;\n", \
                      "}\n", \
                      "const char* uschrc_prompt(ustash *p_stash) { return prompt(p_stash);}\n", \
                      "void uschrc_init(ustash *p_stash) { uschrc(p_stash);}\n");

    if (crepl_getoptions(p_crepl).verbosity >= 11)
        fprintf(stderr, "p_stmt = \\\n%s\n", p_stmt);

    p_tcc = tcc_new();
    E_FAIL_IF(tcc_add_include_path(p_tcc, p_crepl->p_tmpdir) != 0);
    E_FAIL_IF(tcc_set_output_type(p_tcc, TCC_OUTPUT_MEMORY) != 0);
    tccstatus = tcc_compile_string(p_tcc, p_stmt);
    if (tccstatus != 0)
    {
	if (crepl_getoptions(p_crepl).verbosity >= 1)
            fprintf(stderr, "usch: compilation failed\n");
	E_QUIET_FAIL_IF(tccstatus != 0);
    }
    mem = malloc(tcc_relocate(p_tcc, NULL));
    E_FAIL_IF(mem == NULL);
    E_FAIL_IF(tcc_relocate(p_tcc, mem) != 0);

    *(void **) (&crepl_load_vars) = tcc_get_symbol(p_tcc, "crepl_load_vars");
    E_FAIL_IF(crepl_load_vars == NULL);
    (*crepl_load_vars)(p_crepl);

    *(void **) (&set_context) = tcc_get_symbol(p_tcc, "crepl_set_context");
    E_FAIL_IF(set_context == NULL);
    (*set_context)(p_crepl);

    if (!p_crepl->is_initialized && p_crepl->options.interactive)
    {
        *(void **) (&uschrc_init) = tcc_get_symbol(p_tcc, "uschrc_init");
	E_FAIL_IF(uschrc_init == NULL);
        (*uschrc_init)(&p_crepl->prompt_stash); p_crepl->is_initialized = 1; } 
    *(void **) (&dyn_func) = tcc_get_symbol(p_tcc, CREPL_DYN_FUNCNAME);
    (*dyn_func)(p_crepl);

    *(void **) (&crepl_store_vars) = tcc_get_symbol(p_tcc, "crepl_store_vars");
    E_FAIL_IF(crepl_store_vars == NULL);
    (*crepl_store_vars)(p_crepl);

    uclear(&p_crepl->prompt_stash);
    p_crepl->p_prompt = NULL;
    *(void **) (&uschrc_prompt) = tcc_get_symbol(p_tcc, "uschrc_prompt");
    E_FAIL_IF(uschrc_prompt == NULL);
    p_crepl->p_prompt = (*uschrc_prompt)(&p_crepl->prompt_stash);

end:
    tcc_delete(p_tcc);
    free(mem);
    free(definition.p_symname);
    free(p_pre_assign);
    free(p_post_assign);
    free(input.p_str);
    free(p_crepl->p_nodef_line);
    p_crepl->p_nodef_line = NULL;
    free(p_crepl->p_defs_line);
    p_crepl->p_defs_line = NULL;
    uclear(&s);

    if (p_stmt_file != NULL)
        fclose(p_stmt_file);

    return estatus;
}


