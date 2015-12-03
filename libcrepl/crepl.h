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
#include <limits.h>

// CREPL error codes
typedef enum
{
    E_CREPL_MIN = INT_MIN,
    E_CREPL_WAT = -3,
    E_CREPL_PARAM = -2,
    E_CREPL_ALLOC_FAIL = -1,
    E_CREPL_UNDEFINED = 0,
    E_CREPL_OK = 1,
    E_CREPL_MAX = INT_MAX
} E_CREPL;

#define CREPL_OK(x) ((x) == E_CREPL_OK)

struct crepl;

typedef struct
{
    int interactive;
    int verbosity;
    int single_instance;
} crepl_options;

// CREPL parser states
typedef enum
{
CREPL_STATE_ERROR = 0,
CREPL_STATE_CPARSER = 1,
CREPL_STATE_CMDSTART = 2,
CREPL_STATE_CMDARG = 3,
CREPL_STATE_PREPROCESSOR = 4,
} crepl_state_t;


/**
 * @brief Create crepl context object
 *
 * Create crepl context object with specified options
 *
 * @param pp_crepl created context object
 * @param options creation options
 */
E_CREPL crepl_create(struct crepl **pp_crepl, crepl_options options);

E_CREPL crepl_eval(struct crepl *p_crepl, char *p_input);
void crepl_destroy(struct crepl *p_crepl);

crepl_options crepl_getoptions(struct crepl *p_crepl);

int crepl_define(struct crepl *p_crepl, size_t var_size, char *p_defname);
void crepl_undef(struct crepl *p_crepl, char *p_defname);
int crepl_load(struct crepl *p_crepl, char *p_defname, void *p_data);
int crepl_store(struct crepl *p_crepl, char *p_defname, void *p_data);

int crepl_define_fn(struct crepl *p_crepl, char *p_fndefname, char *p_body);
int crepl_undef_fn(struct crepl *p_crepl, char *p_fndefname);

int crepl_pathhash(struct crepl *p_crepl);
int crepl_is_cmd(struct crepl *p_crepl, char *p_item);
int crepl_lib(struct crepl *p_crepl, char *p_dynlib);
int crepl_include(struct crepl *p_crepl, char *p_header);
char **crepl_getldpath();

/**
 * @brief Get current prompt
 *
 * Get prompt to print
 *
 * @param p_crepl crepl context object
 * @param pp_prompt_out prompt string pointer to assign
 * @return E_CREPL_OK or specific crepl error
 * @remarks assigned pointer must not be free'd
 */
E_CREPL crepl_getprompt(struct crepl *p_crepl, const char **pp_prompt_out);

int crepl_parsedefs(struct crepl *p_crepl, char *p_line);

int crepl_finalize(struct crepl *p_crepl,
                   char *p_unfinalized,
                   char **pp_finalized);

int crepl_preparse(struct crepl *p_crepl,
                   const char *p_input,
                   crepl_state_t *p_state);

E_CREPL crepl_complete(struct crepl *p_crepl,
                       const char *p_input,
                       void(add_completion_callback(
                               void * /* user_data */,
                               char * /* completed_string */)),
                       void *p_callback_data);

E_CREPL crepl_reload_tu(struct crepl *p_crepl);

#if DUMMY_VIM_FIX_NEEDED
{
#endif // DUMMY_VIM_FIX_NEEDED
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // CREPL_H

