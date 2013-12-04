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

struct usch_stash_mem
{
    struct usch_stash_mem *p_next;
    size_t size;
    char str[];
};

typedef struct
{
    struct usch_stash_mem *p_next;
} usch_stash_t;
    
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
static inline int usch_strsplit(const char* p_in, const char* p_delims, char ***ppp_out)
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
#if 0
static inline int usch_strexp(char *p_in, size_t num_args, char ***ppp_out, char *p_str, ...)
{
    va_list p_ap;
    size_t i;
    //char *s = NULL;
    char *p_actual_format = NULL;
    char **pp_argv = NULL;
    //pid_t child_pid;
    //int child_status;

    if (p_str == NULL)
    {
        return -1;
    }

    pp_argv = calloc(num_args + 2, sizeof(char*));
    pp_argv[0] = p_str;

    p_actual_format = calloc(num_args*2, sizeof(char));

    for (i = 0; i < num_args * 2; i += 2)
    {
        p_actual_format[i + 0] = '%';
        p_actual_format[i + 1] = 's';
    }
    p_str = p_actual_format;

    va_start(p_ap, p_str);
    for (i = 0; i < num_args; i++)
    {
        pp_argv[i + 1] = va_arg(p_ap, char *);
    }

    va_end(p_ap);

    free(pp_argv);

    return 0;
    return -1;
}
#endif // 0

#if 0
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

static inline int usch_cached_whereis(char** pp_cached_path, int path_items, char* p_search_item, char** pp_dest)
{
    int status = 0;
    size_t i;
    char *p_dest = NULL;
    char **pp_path = NULL;
    char **pp_relarray = NULL;
    size_t num_items = 0;
    char *p_item = NULL;
    char *p_item_copy = NULL;
    size_t item_length = strlen(p_search_item);

    if (p_search_item[0] == '/' || p_search_item[0] == '.')
    {
        int basename_index = -1;
        pp_relarray = (char**)calloc(1, sizeof(char*));
        if (pp_relarray == NULL)
        {
            status = -1;
            goto end;
        }
        p_item_copy = calloc(strlen(p_search_item), 1);
        if (p_item_copy == NULL)
        {
            status = -1;
            goto end;
        }

        for (i = 0; i < item_length; i++)
        {
            if (p_item_copy[i] == '/')
            {
                basename_index = i;
            }
        }
        if (basename_index < 0)
        {
            status = -1;
            goto end;
        }
        p_item_copy[basename_index] = '\0';

        pp_relarray[0] = p_item_copy;
        pp_path = pp_relarray;
        num_items = 1;
        p_item = &p_item_copy[basename_index + 1];
    }
    else
    {
        p_item = p_search_item;
        pp_path = pp_cached_path;
        num_items = path_items;
    }

    for (i = 0; i < num_items; i++)
    {
        size_t dir_length = strlen(pp_path[i]);
        char new_path[dir_length + 1 + item_length + 1];
        struct stat sb;

        memcpy(new_path, pp_path[i], dir_length);
        new_path[dir_length] = '/';
        memcpy(&new_path[dir_length + 1], p_item, item_length);
        new_path[dir_length + 1 + item_length] = '\0';
        if (stat(new_path, &sb) == -1)
            continue;

        status = 1;
        // TODO: discard if not executable
        //printf("%s %lo\n", new_path, sb.st_mode);
        p_dest = (char*)malloc(dir_length + 1 + item_length + 1);
        if (p_dest == NULL)
        {
            status = -1;
            goto end;
        }
        memcpy(p_dest, new_path, dir_length + item_length + 1);
        *pp_dest = p_dest;
        p_dest = NULL;
        goto end;
    }
end:
    free(pp_relarray);
    free(p_item_copy);
    free(p_dest);

    return status;
}
static inline int usch_whereis(char* p_item, char** pp_dest)
{
    char **pp_path = NULL;
    int status = 0;
    int num_items = 0;
    num_items = usch_strsplit(getenv("PATH"), ":", &pp_path);
    if (num_items < 1)
    {
        status = -1;
        goto end;
    }
    status = usch_cached_whereis(pp_path, num_items, p_item, pp_dest);
end:
    free(pp_path);
    return status;
}
#endif // 0
struct usch_glob_list_t
{
    struct usch_glob_list_t *p_next;
    glob_t glob_data;
} usch_glob_list_t;

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
static inline int usch_cmd_arr(struct usch_stash_mem **pp_in, 
                               struct usch_stash_mem **pp_out,
                               struct usch_stash_mem **pp_err,
                               size_t num_args,
                               char **pp_orig_argv)
{
    (void)pp_in;
    (void)pp_err;
    struct usch_stash_mem *p_out = NULL;
    int pipefd[2] = {0, 0};
	size_t i, j;
	struct usch_glob_list_t *p_glob_list = NULL;
	struct usch_glob_list_t *p_current_glob_item = NULL;
	int num_glob_items = 0;
	int orig_arg_idx = 0;
	char **pp_argv = NULL;
	pid_t child_pid;
    int status = 0;

	pid_t w;
	int child_status = -1;

    if (pp_out != NULL)
    {
        pipe(pipefd);
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
            if (pp_out != NULL)
            {
                close(pipefd[0]); // close reading end in the child
                dup2(pipefd[1], 1); // send stdout to the pipe
                close(pipefd[1]); // this descriptor is no longer needed
            }

            int execv_status = execvp(pp_argv[0], pp_argv);
            fprintf(stderr, "usch: no such file or directory; %s\n", pp_argv[0]);

            _exit(execv_status);
        }
        else
        {
            if (pp_out != NULL)
            {
                size_t i = 0;
                size_t read_size = 1024;
                p_out = malloc(read_size + sizeof(struct usch_stash_mem));
                if (p_out == NULL)
                    goto end;
                p_out->p_next = NULL;
                p_out->size = read_size;

                close(pipefd[1]);  // close the write end of the pipe in the parent

                while (read(pipefd[0], &p_out->str[i], 1) != 0)
                {
                    i++;
                    if (i >= read_size)
                    {
                        read_size *= 2;
                        p_out = realloc(p_out, read_size + sizeof(struct usch_stash_mem));
                        if (p_out == NULL)
                            goto end;
                        p_out->size = read_size;
                    }
                }
                p_out->str[i] = '\0';
            }
            else
                do
                {

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
                    }
                } while (!WIFEXITED(child_status) && !WIFSIGNALED(child_status));
        }
    }
    if (pp_out != NULL)
    {
        *pp_out = p_out;
    }
end:
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
    free(pp_argv);

    return status;
}

static inline int usch_cmd_impl(size_t num_args, char *p_name, ...)
{
    va_list p_ap;
    size_t i;
    char **pp_orig_argv = NULL;
    int status = 0;
    char *p_actual_format = NULL;

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


    status = usch_cmd_arr(NULL, NULL, NULL, num_args, pp_orig_argv);

end:
    if (num_args > 1)
    {
        va_end(p_ap);
    }

    fflush(stdout);
    free(pp_orig_argv);
    free(p_actual_format);

    return status;
}
static inline int usch_stash(usch_stash_t *p_memstash, struct usch_stash_mem *p_memblob)
{
    int status = 0;

    if (p_memstash == NULL || p_memblob == NULL)
        return -1;

    p_memblob->p_next = p_memstash->p_next;
    p_memstash->p_next = p_memblob;

    return status;
}
static inline void usch_stashfree(usch_stash_t *p_memstash)
{
    struct usch_stash_mem *p_current = NULL;
    if (p_memstash == NULL)
        return;
    if (p_memstash->p_next == NULL)
        return;

    p_current = p_memstash->p_next;

    while (p_current != NULL)
    {
        struct usch_stash_mem *p_prev = p_current;
        p_current = p_current->p_next;
        free(p_prev);
    }
    p_memstash->p_next = NULL;
}

static inline char* usch_strout_impl(usch_stash_t *p_memstash, size_t num_args, char *p_name, ...)
{
    (void)p_memstash;
    (void)num_args;
    (void)p_name;


    char *p_strout = NULL;
    va_list p_ap;
    size_t i;
    char **pp_orig_argv = NULL;
    char *p_actual_format = NULL;
    struct usch_stash_mem *p_out = NULL;
    static char emptystr[1];

    emptystr[0] = '\0';

    p_strout = emptystr;


    if (p_name == NULL)
    {
        goto end;
    }

    pp_orig_argv = calloc(num_args + 1, sizeof(char*));
    if (pp_orig_argv == NULL)
    {
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
    (void)usch_cmd_arr(NULL, &p_out, NULL, num_args, pp_orig_argv);
    if (usch_stash(p_memstash, p_out) != 0)
    {
        printf("stash failed, ohnoes!\n");
        goto end;
    }

    p_strout = p_out->str;
    p_out = NULL;
end:
    if (num_args > 1)
    {
        va_end(p_ap);
    }

    fflush(stdout);
    free(pp_orig_argv);
    free(p_actual_format);
    free(p_out);

    return p_strout;
}

#define USCH_STROUT_ARGS(p_memstash, ...) usch_strout_impl((p_memstash), USCH_ARGC(__VA_ARGS__), "", ##__VA_ARGS__)
#define usch_strout(p_memstash, cmd, ...) USCH_STROUT_ARGS((p_memstash), (cmd), ##__VA_ARGS__)

#define USCH_CMD_ARGS(...) usch_cmd_impl(USCH_ARGC(__VA_ARGS__), "", ##__VA_ARGS__)
#define usch_cmd(cmd, ...) USCH_CMD_ARGS(cmd, ##__VA_ARGS__)
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

