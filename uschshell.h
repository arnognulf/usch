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

struct uschshell_t;
typedef struct uschshell_t uschshell_t;
int uschshell_create(uschshell_t **p_context);
int uschshell_eval(uschshell_t *p_context, char *p_input);
void uschshell_destroy(uschshell_t *p_context);

int uschshell_load_impl(uschshell_t *p_context, void *p_type, char *p_symname);
int uschshell_store_impl(uschshell_t *p_context, char *p_decl, char *p_symname);
#define usch_eval_load(p_context, type, synname)
#define usch_eval_store(p_context, type, symname) usch_eval_store_impl((p_context), #type, sizeof((symname)), (void*)&(symname))

#ifdef __cplusplus
    }
#endif // __cplusplus
#endif // USCHSHELL_H

