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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "usch.h"
#include "crepl.h"
#include "../external/linenoise/linenoise.h"

static struct crepl_t *p_global_context = NULL;

void spaceCompletion(const char *p_buf, linenoiseCompletions *lc) {
    char *p_full_completion = NULL;
    char *p_space_completion = NULL;
    crepl_state_t state = CREPL_STATE_CPARSER;
    size_t len = 0;
    const char space_completion[] = " ";

    len = strlen(p_buf);

    crepl_preparse(p_global_context, p_buf, &state);

    if (state == CREPL_STATE_CMDSTART)
    {
        const char completion[] = "(\"";
        p_full_completion = calloc(len + strlen(completion) + 1, 1);
        if (p_full_completion == NULL)
            goto end;

        memcpy(p_full_completion, p_buf, len);
        memcpy(&p_full_completion[len], completion, strlen(completion) + /* last NUL */ 1);

        linenoiseAddCompletion(lc, p_full_completion);
    }
    else if (state == CREPL_STATE_CMDARG)
    {
        const char completion[] = "\", \"";

        p_full_completion = calloc(len + strlen(completion) + 1, 1);
        if (p_full_completion == NULL)
            goto end;

        memcpy(p_full_completion, p_buf, len);
        memcpy(&p_full_completion[len], completion, strlen(completion) + /* last NUL */ 1);

        linenoiseAddCompletion(lc, p_full_completion);
    }
    p_space_completion = calloc(len + strlen(space_completion) + 1, 1);
    if (p_space_completion == NULL)
        goto end;

    memcpy(p_space_completion, p_buf, len);
    memcpy(&p_space_completion[len], space_completion, strlen(space_completion) + /* last NUL */ 1);

    linenoiseAddCompletion(lc, p_space_completion);
end:
    free(p_full_completion);
    free(p_space_completion);
}


static void siginthandler(int dummy)
{
    (void)dummy;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    char *p_line = NULL;
    struct crepl_t *p_crepl = NULL;
    size_t len = 0;
    char *p_history = NULL;
    crepl_create(&p_crepl);
    ustash_t s = {0};

    p_global_context = p_crepl;

    signal(SIGINT, siginthandler);
    signal(SIGQUIT, siginthandler);

    p_history = ustrjoin(&s, getenv("HOME"), "/.usch_history");

    linenoiseSetMultiLine(1);
    linenoiseHistorySetMaxLen(100);
    //linenoiseSetCompletionCallback(completion);
    linenoiseSetSpaceCompletionCallback(spaceCompletion);
    linenoiseHistoryLoad(p_history); /* Load the history at startup */

    while((p_line = linenoise("/* usch */ ")) != NULL) {
        if (p_line[0] != '\0') {
            crepl_eval(p_crepl, p_line);
            
            linenoiseHistoryAdd(p_line); /* Add to the history. */
            linenoiseHistorySave(p_history); /* Save the history on disk. */
        }
        free(p_line);
    }
    crepl_destroy(p_crepl);
    uclear(&s);
    return 0;
}
