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

#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#include "crepl.h"
#include "../external/uthash/src/uthash.h"

int crepl_create(crepl_t **pp_context)
{
    int status = 0;
    crepl_t *p_context = NULL;
    crepl_def_t *p_def = NULL;
    crepl_sym_t *p_sym = NULL;
    crepl_dyfn_t *p_dyfn = NULL;
    crepl_inc_t *p_inc = NULL;
    char dir_template[] = "/tmp/crepl-XXXXXX";
    char *p_tempdir = NULL;

    p_context = calloc(sizeof(crepl_t) + strlen(dir_template) + 1, 1);
    FAIL_IF(p_context == NULL);

    p_tempdir = mkdtemp(dir_template);
    FAIL_IF(p_tempdir == NULL);
    strcpy(p_context->tmpdir, dir_template);

    p_def = calloc(sizeof(crepl_def_t) + 1, 1);
    FAIL_IF(p_def == NULL);

    p_sym = calloc(sizeof(crepl_sym_t) + 1, 1);
    FAIL_IF(p_sym == NULL);

    p_dyfn = calloc(sizeof(crepl_dyfn_t) + 1, 1);
    FAIL_IF(p_dyfn == NULL);

    p_inc = calloc(sizeof(crepl_inc_t) + 1, 1);
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
void crepl_destroy(crepl_t *p_context)
{

    if (p_context)
    {
        crepl_lib_t *p_current_lib = NULL;
        crepl_sym_t *p_tmpsym = NULL;
        crepl_sym_t *p_sym = NULL;
        crepl_sym_t *p_syms = p_context->p_syms;
        HASH_ITER(hh, p_syms, p_sym, p_tmpsym) {
            HASH_DEL(p_syms, p_sym);
        }

        crepl_def_t *p_tmpdef = NULL;
        crepl_def_t *p_def = NULL;
        crepl_def_t *p_defs = p_context->p_defs;

        HASH_ITER(hh, p_defs, p_def, p_tmpdef) {
            free(p_def->p_body_data);
            free(p_def->p_alloc_data);

            HASH_DEL(p_defs, p_def);
        }
        free(p_defs);

        crepl_inc_t *p_tmpinc = NULL;
        crepl_inc_t *p_inc = NULL;
        crepl_inc_t *p_incs = p_context->p_incs;

        HASH_ITER(hh, p_incs, p_inc, p_tmpinc) {
            HASH_DEL(p_incs, p_inc);
        }
        free(p_incs);

        crepl_dyfn_t *p_tmpdyfn = NULL;
        crepl_dyfn_t *p_dyfn = NULL;
        crepl_dyfn_t *p_dyfns = p_context->p_dyfns;

        HASH_ITER(hh, p_dyfns, p_dyfn, p_tmpdyfn) {
            HASH_DEL(p_dyfns, p_dyfn);
        }
        // TODO: free crepl_lib_t
        free(p_context->pp_cmds);
        free(p_context->p_dyfns);
        free(p_context->p_syms);
        
        p_current_lib = p_context->p_libs;
        while (p_current_lib != NULL)
        {
            crepl_lib_t *p_prev_lib = NULL;
            p_prev_lib = p_current_lib;
            p_current_lib = p_current_lib->p_next;
            free(p_prev_lib);
        }
        

    }
    free(p_context);
    return;
}

