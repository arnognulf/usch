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
#include <clang-c/Index.h>

#include "../usch_h/usch.h"
#include "crepl_debug.h"
#include "crepl_parser.h"
#include "crepl_types.h"
#include "strutils.h"

#ifndef MAX
#define MAX(a,b) \
    ( __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; )
#endif // MAX

#include "crepl.h"
#include "../external/uthash/src/uthash.h"

static void copy_options(crepl* p_crepl, crepl_options *p_options);
static void set_filenames(crepl* p_crepl, const char *dir_template)
{
    p_crepl->p_tmpdir = ustrjoin(&p_crepl->stash, dir_template);
    p_crepl->p_stmt_c = ustrjoin(&p_crepl->stash, p_crepl->p_tmpdir, "/", CREPL_STMT_C_FN);
    p_crepl->p_defs_h = ustrjoin(&p_crepl->stash, p_crepl->p_tmpdir, "/", CREPL_DEFS_H_FN);
    p_crepl->p_incs_h = ustrjoin(&p_crepl->stash, p_crepl->p_tmpdir, "/", CREPL_INCS_H_FN);
    p_crepl->p_trampolines_h = ustrjoin(&p_crepl->stash, p_crepl->p_tmpdir, "/", CREPL_TRAMPOLINES_H_FN);

}

static char* find_uschrc(ustash *p_stash)
{
    struct stat sb;
    char *p_fullpath_uschrc_h = NULL;
    char *p_uschrc_cand = NULL;
    p_uschrc_cand = ustrjoin(p_stash, getenv("HOME"), "/.uschrc.h");
    if (stat(p_uschrc_cand, &sb) != -1)
    {
        p_fullpath_uschrc_h = p_uschrc_cand;
    }
    else
    {
        p_uschrc_cand = ustrjoin(p_stash, CMAKE_INSTALL_PREFIX, "/etc/uschrc.h");
        if (stat(p_uschrc_cand, &sb) != -1)
        {
            p_fullpath_uschrc_h = p_uschrc_cand;
        }
        else
        {
            p_uschrc_cand = ustrjoin(p_stash, "/etc/uschrc.h");
            if (stat(p_uschrc_cand, &sb) == -1)
            {
                goto error;
            }

        }
    }
    return p_fullpath_uschrc_h;
error:
    return NULL;
}

static void set_header(crepl* p_crepl)
{
    int status = 0;
    ustash s = {0};
    char *p_fullpath_uschrc_h = find_uschrc(&s);
    FAIL_IF(p_fullpath_uschrc_h == NULL);

    p_crepl->p_stmt_header = ustrjoin(&p_crepl->stash,
                    "struct crepl;\n", \
                    "#include <usch.h>\n", \
                    "#include <crepl.h>\n", \
                    "#include \"", CREPL_INCS_H_FN, "\"\n", \
                    "#include \"", CREPL_DEFS_H_FN, "\"\n", \
                    "#include \"", CREPL_TRAMPOLINES_H_FN, "\"\n", \
                    "#include \"", p_fullpath_uschrc_h, "\"\n", \
                    "#ifndef cd\n", \
                    "#define cd(...) ucmd(\"cd\", ##__VA_ARGS__)\n", \
                    "#endif // cd\n", \
                    "static struct crepl *p_crepl_context = NULL;\n", \
                    "void crepl_set_context(struct crepl *p_crepl)\n", \
                    "{\n", \
                    "    p_crepl_context = p_crepl;\n", \
                    "}\n");
end:
    (void)status;
    uclear(&s);
}

#if 0
static void set_footer(crepl* p_crepl)
{
    (void)p_crepl;

}
#endif // 0

E_CREPL crepl_create(crepl **pp_crepl, crepl_options options)
{
    E_CREPL estatus = E_CREPL_OK;
    crepl *p_crepl = NULL;
    crepl_def_t *p_def = NULL;
    crepl_sym_t *p_sym = NULL;
    crepl_dyfn_t *p_dyfn = NULL;
    crepl_inc_t *p_inc = NULL;
    char dir_template[] = "/tmp/crepl-XXXXXX";
    char *p_tempdir = NULL;
    char **pp_ldpath = NULL;
    CXIndex p_idx = NULL;

    if (pp_crepl == NULL)
        return E_CREPL_PARAM;

    p_crepl = calloc(sizeof(crepl), 1);
    E_FAIL_IF(p_crepl == NULL);

    p_idx = clang_createIndex(1, 0);
    E_FAIL_IF(p_idx == NULL);
    p_crepl->p_idx  = p_idx;

    p_tempdir = mkdtemp(dir_template);
    E_FAIL_IF(p_tempdir == NULL);

    p_def = calloc(sizeof(crepl_def_t) + 1, 1);
    E_FAIL_IF(p_def == NULL);

    p_sym = calloc(sizeof(crepl_sym_t) + 1, 1);
    E_FAIL_IF(p_sym == NULL);

    p_dyfn = calloc(sizeof(crepl_dyfn_t) + 1, 1);
    E_FAIL_IF(p_dyfn == NULL);

    p_inc = calloc(sizeof(crepl_inc_t) + 1, 1);
    E_FAIL_IF(p_inc == NULL);

    HASH_ADD_STR(p_crepl->p_defs, defname, p_def);
    HASH_ADD_STR(p_crepl->p_syms, symname, p_sym);
    HASH_ADD_STR(p_crepl->p_dyfns, dyfnname, p_dyfn);
    HASH_ADD_STR(p_crepl->p_incs, incname, p_inc);

    pp_ldpath = crepl_getldpath();
    E_FAIL_IF(pp_ldpath == NULL);
    p_crepl->pp_ldpath = pp_ldpath;

    copy_options(p_crepl, &options);

    set_header(p_crepl);
    set_filenames(p_crepl, dir_template);

    // create stmt.c and accompaning header files
    E_FAIL_IF(crepl_eval(p_crepl, "") != E_CREPL_OK);

    *pp_crepl = p_crepl;
    pp_ldpath = NULL;
    p_crepl = NULL;
    p_def = NULL;
    p_sym = NULL;
    p_dyfn = NULL;
    p_inc = NULL;
end:
    free(p_inc);
    free(p_dyfn);
    free(p_sym);
    free(p_def);
    free(p_crepl);

    return estatus;
}
// http://stackoverflow.com/questions/2256945/removing-a-non-empty-directory-programmatically-in-c-or-c
static int remove_directory(const char *p_path)
{
    DIR *p_d = opendir(p_path);
    size_t path_len = strlen(p_path);
    int r = -1;

    if (p_d)
    {
        struct dirent *p_de;

        r = 0;

        while (!r && (p_de=readdir(p_d)))
        {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p_de->d_name, ".") || !strcmp(p_de->d_name, ".."))
            {
                continue;
            }

            len = path_len + strlen(p_de->d_name) + 2; 
            buf = malloc(len);

            if (buf)
            {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", p_path, p_de->d_name);

                if (!stat(buf, &statbuf))
                {
                    if (S_ISDIR(statbuf.st_mode))
                    {
                        r2 = remove_directory(buf);
                    }
                    else
                    {
                        r2 = unlink(buf);
                    }
                }

                free(buf);
            }

            r = r2;
        }

        closedir(p_d);
    }

    if (!r)
    {
        r = rmdir(p_path);
    }

    return r;
}
void crepl_destroy(crepl *p_crepl)
{
    if (p_crepl)
    {
        crepl_lib_t *p_current_lib = NULL;
        crepl_sym_t *p_tmpsym = NULL;
        crepl_sym_t *p_sym = NULL;
        crepl_sym_t *p_syms = p_crepl->p_syms;
        HASH_ITER(hh, p_syms, p_sym, p_tmpsym) {
            HASH_DEL(p_syms, p_sym);
        }
        free(p_crepl->p_syms);

        crepl_def_t *p_tmpdef = NULL;
        crepl_def_t *p_def = NULL;
        crepl_def_t *p_defs = p_crepl->p_defs;

        HASH_ITER(hh, p_defs, p_def, p_tmpdef) {
            free(p_def->p_body_data);
            free(p_def->p_alloc_data);

            HASH_DEL(p_defs, p_def);
        }
        free(p_crepl->p_defs);

        crepl_inc_t *p_tmpinc = NULL;
        crepl_inc_t *p_inc = NULL;
        crepl_inc_t *p_incs = p_crepl->p_incs;

        HASH_ITER(hh, p_incs, p_inc, p_tmpinc) {
            HASH_DEL(p_incs, p_inc);
        }
        free(p_crepl->p_incs);

        crepl_dyfn_t *p_tmpdyfn = NULL;
        crepl_dyfn_t *p_dyfn = NULL;
        crepl_dyfn_t *p_dyfns = p_crepl->p_dyfns;

        HASH_ITER(hh, p_dyfns, p_dyfn, p_tmpdyfn) {
            HASH_DEL(p_dyfns, p_dyfn);
        }
        free(p_crepl->p_dyfns);

        p_current_lib = p_crepl->p_libs;
        while (p_current_lib != NULL)
        {
            crepl_lib_t *p_prev_lib = NULL;
            p_prev_lib = p_current_lib;
            p_current_lib = p_current_lib->p_next;
            free(p_prev_lib);
        }
        
        free(p_crepl->pp_cmds);
        (void)remove_directory(p_crepl->p_tmpdir);
        free(p_crepl->p_nodef_line);
        free(p_crepl->p_defs_line);
        free(p_crepl->pp_ldpath);
        (void)clang_disposeTranslationUnit(p_crepl->p_tu);
        uclear(&p_crepl->prompt_stash);
    }
    free(p_crepl);
    return;
}

E_CREPL crepl_getprompt(struct crepl *p_crepl, const char **pp_prompt_out)
{
    if (p_crepl == NULL ||
        pp_prompt_out == NULL)
        return E_CREPL_PARAM;
    if (p_crepl->p_prompt == NULL)
        return E_CREPL_WAT;

    if (p_crepl->p_prompt)
    {
        *pp_prompt_out = p_crepl->p_prompt;
    }
    return E_CREPL_OK;
}

crepl_options crepl_getoptions(crepl *p_crepl)
{
    crepl_options options;
    memset(&options, 0x0, sizeof(crepl_options));
    if (p_crepl == NULL)
        return options;

    return p_crepl->options;
}
static void copy_options(crepl* p_crepl, crepl_options *p_options)
{
    p_crepl->options = *p_options;
}

