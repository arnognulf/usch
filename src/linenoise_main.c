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
#include <dirent.h>

#include "usch.h"
#include "crepl.h"
#include "../external/linenoise/linenoise.h"

static struct crepl_t *p_global_context = NULL;
static void handle_sigint(int sig)
{
    return;
}
void spaceCompletion(const char *p_buf, linenoiseCompletions *lc)
{
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

void tabCompletion(const char *p_buf, linenoiseCompletions *lc)
{
    DIR *p_dir = NULL;
    struct dirent *p_dirent = NULL;
    int i;
    static ustash_t tab_completion_stash = {0};

    crepl_state_t state = CREPL_STATE_CPARSER;
    size_t len = 0;
    len = strlen(p_buf);

    crepl_preparse(p_global_context, p_buf, &state);

    if (state == CREPL_STATE_CMDARG)
    {
        const char *p_cmdarg = NULL;
        const char *p_dirarg = NULL;
        size_t arglen = 0;

        for (i = len-1; i >= 0; i--)
        {
            if (p_buf[i] == '"')
            {
                p_cmdarg = &p_buf[i+1];
                arglen = strlen(p_cmdarg);
                break;
            }
        }

        p_dirarg = udirname(&tab_completion_stash, ustrtrim(&tab_completion_stash, p_cmdarg));
        fprintf(stderr, "xarg: %s\n", p_dirarg);
        if (p_dirarg[0] == '/' || (p_dirarg[0] == '.' && p_dirarg[1] == '.'))
        {
            p_dir = opendir(p_dirarg);
        }
        else
        {
            p_dir = opendir(".");
        }

        while ((p_dirent = readdir(p_dir)) != NULL) {
            //fprintf(stderr, "ABC: %s %s\n", p_dirent->d_name, p_cmdarg);
            if (strncmp(p_cmdarg, p_dirent->d_name, arglen) == 0)
            {
                char *p_tab_completion = ustrjoin(&tab_completion_stash, p_buf, &p_dirent->d_name[arglen]);
                //fprintf(stderr, "XYZ: %s\n", p_tab_completion);
                linenoiseAddCompletion(lc, p_tab_completion);
            }
        }

    }
    if (p_dir)
        closedir(p_dir);
    uclear(&tab_completion_stash);
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
    ustash_t prompt = {0};
    char *p_prompt = NULL;
    char *p_hostname = NULL;
    int i = 0;

    p_global_context = p_crepl;

    signal(SIGQUIT, handle_sigint);
    signal(SIGINT, handle_sigint);

    p_history = ustrjoin(&s, getenv("HOME"), "/.usch_history");

    linenoiseSetMultiLine(1);
    linenoiseHistorySetMaxLen(100);
    linenoiseSetCompletionCallback(tabCompletion);
    linenoiseSetSpaceCompletionCallback(spaceCompletion);
    linenoiseHistoryLoad(p_history); /* Load the history at startup */

    p_hostname = ustrout(&s, "hostname", "-s");
    p_prompt = ustrjoin(&prompt, getenv("USER"), "@", p_hostname, ":", ustrout(&prompt, "pwd"), "% ");
    while((p_line = linenoise(p_prompt)) != NULL) 
    {
        uclear(&prompt);
        if (p_line[0] != '\0') {
            crepl_eval(p_crepl, p_line);
            
            linenoiseHistoryAdd(p_line); /* Add to the history. */
            linenoiseHistorySave(p_history); /* Save the history on disk. */
        }
        free(p_line);
        p_prompt = ustrjoin(&prompt, getenv("USER"), "@", p_hostname, ":", ustrout(&prompt, "pwd"), "% ");
    }
    crepl_destroy(p_crepl);
    uclear(&s);
    return 0;
}
