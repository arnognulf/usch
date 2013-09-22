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
static inline int usch_strsplit(char* p_in, char* p_delims, char*** ppp_out)
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
static inline int usch_cd_impl(size_t num_args, char *p_dummy, ...)
{
    int error = 0;
    glob_t glob_data;
    va_list p_ap;
    char *p_dir = NULL;
    char *p_chdir = NULL;
    char format[] = "%s";

    printf("%d\n", (int)num_args);
    if (num_args > 1)
    {
        p_dummy = format;
        va_start(p_ap, p_dummy);
        p_dir = va_arg(p_ap, char *);
    }
    else
    {
        p_dir = getenv("HOME");
    }

    if (glob(p_dir, GLOB_MARK | GLOB_NOCHECK | GLOB_TILDE | GLOB_NOMAGIC | GLOB_BRACE, NULL, &glob_data) == 0)
    {
        p_chdir = glob_data.gl_pathv[0];
    }
    else
    {
        p_chdir = p_dir;
    }
    error = chdir(p_chdir);

    switch (error)
    {
        case EACCES:
            {
                perror("Permission denied.");
                break;
            }
        case EFAULT:
            {
                perror("Path points outside your accessible address space.");
                break;
            }

        case EIO:
            {
                perror("An I/O error occurred.");
                break;
            }
        case ELOOP:
            {
                perror("Too many symbolic links were encountered in resolving path.");
                break;
            }
        case ENAMETOOLONG:
            {
                perror("path is too long.");
                break;
            }
        case ENOENT:
            {
                perror("The file does not exist.");
                break;
            }
        case ENOMEM:
            {
                perror("Insufficient kernel memory was available.");
                break;
            }
        case ENOTDIR:
            {
                perror("A component of path is not a directory.");
                break;
            }
        default:
            break;
    }
    printf("cd: %s\n", p_chdir);
    globfree(&glob_data);
    if (num_args > 0)
    {
        va_end(p_ap);
    }
    return error;
}
#define usch_cd(...) usch_cd_impl(USCH_ARGC(__VA_ARGS__), "", ##__VA_ARGS__)
#ifndef cd
#define cd(...) usch_cd(__VA_ARGS__)
#else
#warning included header defines cd(), usch_cd() must be called directly
#endif // cd
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

#if NEED_VIM_WORKAROUND
{
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // USCH_H

