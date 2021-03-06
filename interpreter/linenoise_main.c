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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <stddef.h>                     // for size_t
#include <sys/stat.h>                   // for stat, S_IFDIR, S_IFMT
#include <sys/ptrace.h>

#include "../usch_h/usch.h"
#include "crepl.h"
#include "../external/linenoise/linenoise.h"
#include "../external/commander/src/commander.h"

static crepl_options options;
static struct crepl *p_global_context = NULL;
static void handle_sigint(int sig)
{
    (void)sig;
    return;
}

int gdb_connected()
{
    int pid = fork();
    int status;
    int res;

    if (pid == -1)
    {
        perror("fork");
        return -1;
    }

    if (pid == 0)
    {
        int ppid = getppid();

        /* Child */
        if (ptrace(PTRACE_ATTACH, ppid, NULL, NULL) == 0)
        {
            /* Wait for the parent to stop and continue it */
            waitpid(ppid, NULL, 0);
            ptrace(PTRACE_CONT, NULL, NULL);

            /* Detach */
            ptrace(PTRACE_DETACH, getppid(), NULL, NULL);

            /* We were the tracers, so gdb is not present */
            res = 0;
        }
        else
        {
            /* Trace failed so gdb is present */
            res = 1;
        }
        exit(res);
    }
    else
    {
        waitpid(pid, &status, 0);
        res = WEXITSTATUS(status);
    }
    return res;
}

void spaceCompletion(const char *p_buf, linenoiseCompletions *lc)
{
    char *p_full_completion = NULL;
    char *p_space_completion = NULL;
    crepl_state_t state = CREPL_STATE_CPARSER;
    ustash s = {0};

    crepl_preparse(p_global_context, p_buf, &state);

    if (state == CREPL_STATE_CMDSTART)
    {
    p_full_completion = ustrjoin(&s, p_buf, "(\"");
    linenoiseAddCompletion(lc, p_full_completion);
    }
    else if (state == CREPL_STATE_CMDARG)
    {
        p_full_completion = ustrjoin(&s, p_buf, "\", \"");
        linenoiseAddCompletion(lc, p_full_completion);
    }

    p_space_completion = ustrjoin(&s, p_buf, " ");
    linenoiseAddCompletion(lc, p_space_completion);
    uclear(&s);
}

void reverseSpaceCompletion(const char *p_buf, linenoiseCompletions *lc)
{
    char *p_full_completion = NULL;
    char *p_space_completion = NULL;
    crepl_state_t state = CREPL_STATE_CPARSER;
    ustash s = {0};

    crepl_preparse(p_global_context, p_buf, &state);

    p_space_completion = ustrjoin(&s, p_buf, " ");
    linenoiseAddCompletion(lc, p_space_completion);

    if (state == CREPL_STATE_CMDSTART)
    {
    p_full_completion = ustrjoin(&s, p_buf, "(\"");
    linenoiseAddCompletion(lc, p_full_completion);
    }
    else if (state == CREPL_STATE_CMDARG)
    {
        p_full_completion = ustrjoin(&s, p_buf, "\", \"");
        linenoiseAddCompletion(lc, p_full_completion);
    }
    uclear(&s);
}
static void add_completion_callback(void *p_data, char *p_completion_string)
{
    if (p_data == NULL)
        return;
    if (p_completion_string == NULL)
        return;

    linenoiseCompletions *lc = (linenoiseCompletions*)p_data;
    linenoiseAddCompletion(lc, p_completion_string);
}

void tabCompletion(const char *p_buf, linenoiseCompletions *lc)
{
    E_CREPL estatus = E_CREPL_OK;
    DIR *p_dir = NULL;
    struct dirent *p_dirent = NULL;
    int i;
    int dirlen = 0;
    static ustash tab_completion_stash = {0};
    struct stat sb;
    
    if (p_buf == NULL)
        return;

    crepl_state_t state = CREPL_STATE_CPARSER;
    size_t len = 0;
    len = strlen(p_buf);

    crepl_preparse(p_global_context, p_buf, &state);

    if (state == CREPL_STATE_CMDARG)
    {
        const char *p_cmdarg = NULL;
        const char *p_dirarg = NULL;
        const char emptystr[] = "";
        const char *p_trailing_slash = emptystr;
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

        p_dirarg = udirname(&tab_completion_stash, p_cmdarg);
        dirlen = strlen(p_dirarg);
        if (p_dirarg[0] == '.')
        {
            if (p_dirarg[1] == '\0')
            {
                dirlen = 0;
            }
        }
        if (crepl_getoptions(p_global_context).verbosity >= 11)
            fprintf(stderr, "p_dirarg = %s\n", p_dirarg);

        p_dir = opendir(p_dirarg);
        if (p_dir == NULL)
            goto end;

        while ((p_dirent = readdir(p_dir)) != NULL) {
            if (p_cmdarg == NULL)
                goto end;
            if (p_dirent == NULL)
                goto end;
            if (crepl_getoptions(p_global_context).verbosity >= 11)
                fprintf(stderr, "&p_cmdarg[dirlen] (%s) == p_dirent->d_name (%s), len = %d\n", &p_cmdarg[dirlen], p_dirent->d_name, (int)(arglen - dirlen));
            if (strncmp(&p_cmdarg[dirlen], p_dirent->d_name, arglen - dirlen) == 0)
            {
                if (stat(ustrjoin(&tab_completion_stash, p_dirarg, "/", p_dirent->d_name), &sb) != -1)
                {
                    switch (sb.st_mode & S_IFMT) {
                        case S_IFDIR:
                        {
                            p_trailing_slash = "/";
                        break;
                        }
                        default:
                             break;
                    }
                }

                char *p_tab_completion = ustrjoin(&tab_completion_stash,
                                p_buf,
                                &p_dirent->d_name[arglen - dirlen],
                                p_trailing_slash);
                linenoiseAddCompletion(lc, p_tab_completion);
            }
        }

    }
    else
    {
        // do C completion here
        estatus = crepl_complete(p_global_context, p_buf, add_completion_callback, lc);
        if (!CREPL_OK(estatus))
                goto end;
    }
end:
    if (p_dir)
        closedir(p_dir);
    uclear(&tab_completion_stash);
}


static void verbose(command_t *self)
{
    (void)self;
    options.verbosity = 11;
}

static void single_instance(command_t *self)
{
    (void)self;
    options.single_instance = 1;
}


int main(int argc, char **pp_argv) {
    (void)argc;
    memset(&options, 0, sizeof(crepl_options));
    char *p_line = NULL;
    struct crepl *p_crepl = NULL;
    char *p_history = NULL;
    ustash s = {0};
    const char *p_prompt = NULL;
    command_t cmd;
    E_CREPL estatus = E_CREPL_OK;

    options.verbosity = 1;
    options.interactive = 1;

    command_init(&cmd, pp_argv[0], "0.0.1");
    command_option(&cmd, "-v", "--verbose", "enable verbose stuff", verbose);
    command_option(&cmd, "-s", "--single-instance", "enable single instance (for debugging)", single_instance);
    command_parse(&cmd, argc, pp_argv);

    if (gdb_connected())
    {
        fprintf(stderr, "usch: gdb detected, enabling single instance mode.\n");
        single_instance(NULL);
    }
    signal(SIGQUIT, handle_sigint);
    signal(SIGINT, handle_sigint);


    if (options.single_instance)
    {
        if(crepl_create(&p_crepl, options) != E_CREPL_OK)
            goto cleanup;

        p_global_context = p_crepl;

        //signal(SIGQUIT, handle_sigint);
        //signal(SIGINT, handle_sigint);

        p_history = ustrjoin(&s, getenv("HOME"), "/.usch_history");

        linenoiseSetMultiLine(1);
        linenoiseHistorySetMaxLen(100);
        linenoiseSetCompletionCallback(tabCompletion);
        linenoiseSetSpaceCompletionCallback(spaceCompletion);
        linenoiseSetReverseSpaceCompletionCallback(reverseSpaceCompletion);
        linenoiseHistoryLoad(p_history); /* Load the history at startup */

        if(crepl_getprompt(p_crepl, &p_prompt) != E_CREPL_OK)
            goto cleanup;

        while(1) 
        {
            free(p_line);
            p_line = linenoise(p_prompt);
            if(p_line == NULL)
                continue;
            estatus = crepl_eval(p_crepl, p_line);
            if (estatus == E_CREPL_SYNTAX_ERROR)
                continue;

            if (p_line[0] != '\0') {
                linenoiseHistoryAdd(p_line); /* Add to the history. */
                linenoiseHistorySave(p_history); /* Save the history on disk. */
            }
            if (crepl_getprompt(p_crepl, &p_prompt) != E_CREPL_OK)
                goto cleanup;
        }
    }
    else
    {
        while (ucmd(pp_argv[0], "--single-instance") != 0)
        {
            fprintf(stderr, "%s: CREPL crashed, all memory cleared\n", pp_argv[0]);
        }
    }
cleanup:
    uclear(&s);
    crepl_destroy(p_crepl);
    command_free(&cmd);
    return 0;
}
