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
#ifndef CREPL_H
#define CREPL_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#if DUMMY_VIM_FIX_NEEDED
}
#endif // DUMMY_VIM_FIX_NEEDED
#include <stdlib.h>

struct crepl_t;
int crepl_create(struct crepl_t **p_context);
int crepl_eval(struct crepl_t *p_context, char *p_input);
void crepl_destroy(struct crepl_t *p_context);

int crepl_define(struct crepl_t *p_context, size_t var_size, char *p_defname);
void crepl_undef(struct crepl_t *p_context, char *p_defname);
int crepl_load(struct crepl_t *p_context, char *p_defname, void *p_data);
int crepl_store(struct crepl_t *p_context, char *p_defname, void *p_data);

int crepl_define_fn(struct crepl_t *p_context, char *p_fndefname, char *p_body);
int crepl_undef_fn(struct crepl_t *p_context, char *p_fndefname);

int crepl_pathhash(struct crepl_t *p_context);
int crepl_is_cmd(struct crepl_t *p_context, char *p_item);
int crepl_lib(struct crepl_t *p_context, char *p_dynlib);
int crepl_include(struct crepl_t *p_context, char *p_header);

typedef enum {
CREPL_STATE_ERROR = 0,
CREPL_STATE_CPARSER = 1,
CREPL_STATE_CMDSTART = 2,
CREPL_STATE_CMDARG = 3,
} crepl_state_t;
int crepl_parsedefs(struct crepl_t *p_context, char *p_line);

int crepl_finalize(char *p_unfinalized, char **pp_finalized);
int crepl_preparse(struct crepl_t *p_context, char *p_input, crepl_state_t *p_state);
#if DUMMY_VIM_FIX_NEEDED
{
#endif // DUMMY_VIM_FIX_NEEDED
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // CREPL_H

