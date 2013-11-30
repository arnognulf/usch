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
#ifndef USCH_H
#define USCH_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#if NEED_VIM_WORKAROUND
}
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <unistd.h>

#define USCH_NULCHAR_LEN 1

// see: https://groups.google.com/forum/#!topic/comp.std.c/d-6Mj5Lko_s
#define USCH_ARGC(...) USCH_ARGC_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5,4,3,2,1)
#define USCH_ARGC_IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N

/**
 * <A short one line description>
 *  
 *  <Longer description>
 *  <May span multiple lines or paragraphs as needed>
 *   
 *   @param  Description of method's or function's input parameter
 *   @param  ...
 *   @return Description of the return value
 *   */
static inline int usch_strsplit(const char* p_in, char* p_delims, char*** ppp_out)
{
    char** pp_out = NULL;
    char* p_out = NULL;
    size_t len_in;
    size_t len_delims;
    size_t i, j;
    size_t num_str = 0;
    size_t size = 0;
    int out_pos = 0;

    if (p_in == NULL || p_delims == NULL || ppp_out == NULL)
        goto error;

    len_in = strlen(p_in);
    len_delims = strlen(p_delims);
    num_str = 1;
    for (i = 0; i < len_in; i++)
    {
        for (j = 0; j < len_delims; j++)
        {
            if (p_in[i] == p_delims[j])
            {
                num_str++;
            }
        }
    }

    size = (len_in + 1) * sizeof(char) + (num_str + 1) * sizeof(char*);
    pp_out = malloc(size);
    if (pp_out == NULL)
        goto error;

    p_out = (char*)(pp_out + num_str + 1);
    memcpy(p_out, p_in, len_in + 1);

    pp_out[out_pos++] = p_out;

    for (i = 0; i < len_in; i++)
    {
        for (j = 0; j < len_delims; j++)
        {
            if (p_out[i] == p_delims[j])
            {
                p_out[i] = '\0';
                pp_out[out_pos++] = &p_out[i+1];
            }
        }
    }

    *ppp_out = pp_out;
    return (int)num_str;

error:
    return -1;
}

//static inline int usch_strexp(char* p_in, char*** ppp_out, pattern1, pattern2, ...)
// for (i = 0, num_items = strexp(&pp_strings, "*.c", "*.h"); i < num_items; i++)
//
//
//hh
/**
 * <A short one line description>
 *  
 *  <Longer description>
 *  <May span multiple lines or paragraphs as needed>
 *   
 *   @param  Description of method's or function's input parameter
 *   @param  ...
 *   @return Description of the return value
 *   */
/*
 * typedef struct usch_stash_t
 * {
 *       usch_stash_t *p_next; 
 *       int error;
 *       char data[]; 
 * }
 *
 * const char* usch_strexp(usch_stash_t *p_stash, char *p_pattern, __VA_ARGS)
 * {
 * static char *p_str = NULL;
 * static char **pp_str = NULL;
 * pp_str = &p_str;
 *
 * 
 * }
 * const static http://stackoverflow.com/questions/453696/is-returning-a-pointer-to-a-static-local-variable-safe
 * 
 * for (char *p_i = strexp(&s, "*.txt); p_i[i] != NULL; i++)
 * {
 * }
 *
 */
struct usch_glob_list_t
{
    struct usch_glob_list_t *p_next;
    glob_t glob_data;
} usch_glob_list_t;

static inline int usch_cmd_impl(size_t num_args, char *p_name, ...)
{
	va_list p_ap;
	size_t i, j;
	char *p_actual_format = NULL;
	char **pp_argv = NULL;
	char **pp_orig_argv = NULL;
	char **pp_globbed_argv = NULL;
	pid_t child_pid;
	pid_t w;
	int child_status = -1;
	int status = 0;
	struct usch_glob_list_t *p_glob_list = NULL;
	struct usch_glob_list_t *p_current_glob_item = NULL;
	int orig_arg_idx = 0;
	int num_glob_items = 0;
	if (p_name == NULL)
	{
		return -1;
	}

	pp_orig_argv = calloc(num_args + 1, sizeof(char*));
	if (pp_orig_argv == NULL)
	{
		status = -1;
		goto end;
	}

	p_actual_format = calloc(num_args*2, sizeof(char));

	for (i = 0; i < num_args * 2; i += 2)
	{
		p_actual_format[i + 0] = '%';
		p_actual_format[i + 1] = 's';
	}
	p_name = p_actual_format;

	va_start(p_ap, p_name);

	for (i = 0; i < num_args; i++)
	{
		pp_orig_argv[i] = va_arg(p_ap, char *);
	}

	for (i = 0; i < num_args; i++)
	{
		if (strcmp(pp_orig_argv[i], "--") == 0)
		{
			orig_arg_idx++;
			break;
		}

		if (p_current_glob_item == NULL)
		{
			p_current_glob_item = calloc(1, sizeof(usch_glob_list_t));
			if (p_current_glob_item == NULL)
				goto end;
			p_glob_list = p_current_glob_item;
		}
		else
		{
			p_current_glob_item->p_next = calloc(1, sizeof(usch_glob_list_t));
			if (p_current_glob_item->p_next == NULL)
				goto end;
			p_current_glob_item = p_current_glob_item->p_next;
		}
		if (glob(pp_orig_argv[i], GLOB_MARK | GLOB_NOCHECK | GLOB_TILDE | GLOB_NOMAGIC | GLOB_BRACE, NULL, &p_current_glob_item->glob_data) != 0)
		{
			goto end;
		}
		num_glob_items += p_current_glob_item->glob_data.gl_pathc;
		orig_arg_idx++;
	}
	pp_argv = calloc(num_glob_items + num_args - orig_arg_idx + 1, sizeof(char*));
	p_current_glob_item = p_glob_list;

	i = 0;
	j = 0;
	while (p_current_glob_item != NULL)
	{
		for (j = 0; j < p_current_glob_item->glob_data.gl_pathc; j++, i++)
		{
			pp_argv[i] = p_current_glob_item->glob_data.gl_pathv[j];
		}
		p_current_glob_item = p_current_glob_item->p_next;
	}

	for (i = orig_arg_idx; i < num_args; i++)
	{
		pp_argv[j + i] = pp_orig_argv[i];
	}

	if (strcmp(pp_argv[0], "cd") == 0)
	{
		if (pp_argv[1] == NULL)
			chdir(getenv("HOME"));
		else
			chdir(pp_argv[1]);
	}
	else
	{
		child_pid = fork();
		if ( child_pid == -1 ) {
			perror("Cannot proceed. fork() error");
			return 1;
		}
		if(child_pid == 0)
		{

			i = 0;
			while (pp_argv[i] != NULL)
			{
				i++;
			}

			int execv_status = execvp(pp_argv[0], pp_argv);
			fprintf(stderr, "usch: no such file or directory; %s\n", pp_argv[0]);

			_exit(execv_status);
		}
		else
		{
			do
			{
				w = waitpid(child_pid, &child_status, WUNTRACED | WCONTINUED);
				if (w == -1)
				{
					perror("waitpid");
					exit(EXIT_FAILURE);
				}

				if (WIFEXITED(child_status)) 
				{
					status = WEXITSTATUS(child_status);
				}
				else if (WIFSIGNALED(child_status))
				{
					printf("killed by signal %d\n", WTERMSIG(child_status));
				}
				else if (WIFSTOPPED(child_status))
				{
					printf("stopped by signal %d\n", WSTOPSIG(child_status));
				}
				else if (WIFCONTINUED(child_status))
				{
					printf("continued\n");
				}
			} while (!WIFEXITED(child_status) && !WIFSIGNALED(child_status));
		}
	}

end:
	if (num_args > 1)
	{
		va_end(p_ap);
	}
	if (p_glob_list)
	{
		p_current_glob_item = p_glob_list;
		while (p_current_glob_item != NULL)
		{
			struct usch_glob_list_t *p_free_glob_item = p_current_glob_item;

			p_current_glob_item = p_current_glob_item->p_next;

			globfree(&p_free_glob_item->glob_data);
			free(p_free_glob_item);
		}
	}

	fflush(stdout);
	free(pp_globbed_argv);
	free(pp_orig_argv);
	free(p_actual_format);

	return status;
}
#define USCH_COUNT_ARGS(...) usch_cmd_impl(USCH_ARGC(__VA_ARGS__), "", ##__VA_ARGS__)
#define usch_cmd(cmd, ...) USCH_COUNT_ARGS(cmd, ##__VA_ARGS__)
#ifndef USCHSHELL_PARSER
#define cd(...) usch_cmd("cd", ##__VA_ARGS__)
#endif // USCHSHELL_PARSER

#if NEED_VIM_WORKAROUND
{
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // USCH_H

