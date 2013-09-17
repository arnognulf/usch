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
typedef struct uschshell_t uschshell_t;
int uschshell_create(uschshell_t **p_context);
int uschshell_eval(uschshell_t *p_context, char *p_input);
void uschshell_destroy(uschshell_t *p_context);

int uschshell_load_impl(uschshell_t *p_context, void *p_type, char *p_symname);
int uschshell_store_impl(uschshell_t *p_context, char *p_decl, char *p_symname);
int uschshell_define(uschshell_t *p_context, size_t var_size, char *p_defname, void* p_data);
void uschshell_undef(uschshell_t *p_context, char *p_defname);
int uschshell_load(uschshell_t *p_context, char *p_defname, void *p_data);

#if DUMMY_VIM_FIX_NEEDED
{
#endif // DUMMY_VIM_FIX_NEEDED
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // USCHSHELL_H

