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
#ifndef USCHSHELL_H
#define USCHSHELL_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#if DUMMY_VIM_FIX_NEEDED
}
#endif // DUMMY_VIM_FIX_NEEDED

struct uschshell_t;
int uschshell_create(struct uschshell_t **p_context);
int uschshell_eval(struct uschshell_t *p_context, char *p_input);
void uschshell_destroy(struct uschshell_t *p_context);

int uschshell_define(struct uschshell_t *p_context, size_t var_size, char *p_defname);
void uschshell_undef(struct uschshell_t *p_context, char *p_defname);
int uschshell_load(struct uschshell_t *p_context, char *p_defname, void *p_data);
int uschshell_store(struct uschshell_t *p_context, char *p_defname, void *p_data);

int uschshell_define_fn(struct uschshell_t *p_context, char *p_fndefname, char *p_body);
int uschshell_undef_fn(struct uschshell_t *p_context, char *p_fndefname);

int uschshell_pathhash(struct uschshell_t *p_context);
int uschshell_is_cmd(struct uschshell_t *p_context, char *p_item);
int uschshell_lib(struct uschshell_t *p_context, char *p_dynlib);
int uschshell_include(struct uschshell_t *p_context, char *p_header);

typedef enum {
USCHSHELL_STATE_CPARSER = 0,
USCHSHELL_STATE_CMDSTART = 1,
USCHSHELL_STATE_CMDARG = 2,
} uschshell_state_t;

int uschshell_preparse(struct uschshell_t *p_context, char *p_line, uschshell_state_t *p_state);

#if DUMMY_VIM_FIX_NEEDED
{
#endif // DUMMY_VIM_FIX_NEEDED
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // USCHSHELL_H

