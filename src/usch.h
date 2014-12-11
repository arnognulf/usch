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
#endif // NEED_VIM_WORKAROUND

#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
 
/******************************* public declarations **********************************/

/**
 * Forward declaration for private stash struct.
 */
struct ustash_item;
typedef struct ustash ustash_t;

/**
 * @brief free allocated memory referenced by p_ustash.
 *  
 * Clear allocated memory referenced by an ustash_t structure.
 * uclear() can be called any number of times with the same ustash_t pointer.
 *   
 *   @param Pointer to an ustash_t (preferably on the stack)
 *   */

static inline void uclear(ustash_t *p_ustash);


/**
 * @brief splits a string
 *  
 *  <Longer description>
 *  <May span multiple lines or paragraphs as needed>
 *   
 *   @param p_ustash Stash holding allocations.
 *   @param p_in A string to split.
 *   @param p_delims Delimiting characters where the string should be split.
 *   @return A NULL-terminated array of pointers to the substrings.
 *   @remarks return value must not be freed.
 *   @remarks return value is never NULL.
 *   */
static inline char **ustrsplit(ustash_t *p_ustash, const char* p_in, const char* p_delims);

#define ustrout(p_ustash, cmd, ...) PRIV_USCH_STROUT_ARGS((p_ustash), (cmd), ##__VA_ARGS__)
#define ustrexp(p_ustash, item, ...) PRIV_USCH_STREXP_ARGS((p_ustash), (item), ##__VA_ARGS__)
#define ustrjoin(p_ustash, str1, ...) PRIV_USCH_STRJOIN_ARGS((p_ustash), (str1), ##__VA_ARGS__)
#define ucmd(cmd, ...) PRIV_USCH_CMD_ARGS(cmd, ##__VA_ARGS__)

#ifndef CREPL_PARSER
#define cd(...) ucmd("cd", ##__VA_ARGS__)
#endif // CREPL_PARSER

/******************************* private APIs, may change without notice  **********************************/
/**
 * @brief A structure that holds a pointer to a linked list of allocations
 *  
 *  
 *  Returned allocated memory by usch functions should not be explicitly free'd.
 *
 *  Instead the uclear() function should be called.
 *   */
typedef struct ustash
{
    struct ustash_item *p_list;
} ustash_t;


struct uglob_list_t;

static inline int priv_usch_stash(ustash_t *p_ustash, struct ustash_item *p_stashitem);
static inline char **priv_usch_globexpand(char **pp_orig_argv, size_t num_args, /* out */ struct uglob_list_t **pp_glob_list);
static inline void   priv_usch_free_globlist(struct uglob_list_t *p_glob_list);
static inline int    priv_usch_cmd_arr(struct ustash_item **pp_in, 
                               struct ustash_item **pp_out,
                               struct ustash_item **pp_err,
                               size_t num_args,
                               char **pp_orig_argv);
static inline char **priv_usch_strexp_impl(ustash_t *p_ustash, size_t num_args, char *p_str, ...);
static inline int    priv_usch_cmd_impl(size_t num_args, char *p_name, ...);
static inline int priv_usch_cached_whereis(char** pp_cached_path, int path_items, char* p_search_item, char** pp_dest);
#define PRIV_USCH_ARGC(...) PRIV_USCH_ARGC_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5,4,3,2,1)
#define PRIV_USCH_ARGC_IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N
#define PRIV_USCH_STROUT_ARGS(p_ustash, ...) priv_usch_strout_impl((p_ustash), PRIV_USCH_ARGC(__VA_ARGS__), "", ##__VA_ARGS__)
#define PRIV_USCH_STREXP_ARGS(p_ustash, ...) priv_usch_strexp_impl((p_ustash), PRIV_USCH_ARGC(__VA_ARGS__), "", ##__VA_ARGS__)
#define PRIV_USCH_STRJOIN_ARGS(p_ustash, ...) priv_usch_strjoin_impl((p_ustash), PRIV_USCH_ARGC(__VA_ARGS__), "", ##__VA_ARGS__)
#define PRIV_USCH_CMD_ARGS(...) priv_usch_cmd_impl(PRIV_USCH_ARGC(__VA_ARGS__), "", ##__VA_ARGS__)

struct uglob_list_t
{
    struct uglob_list_t *p_next;
    glob_t glob_data;
} uglob_list_t;

struct ustash_item
{
    struct ustash_item *p_next;
    unsigned char error;
    char str[];
};

static int priv_usch_run(char **pp_argv, int input, int first, int last, int *p_child_pid, struct ustash_item **pp_out);
static int n = 0; /* number of calls to 'priv_usch_command' */
static int priv_usch_command(char **pp_argv, int input, int first, int last, int *p_child_pid, struct ustash_item **pp_out);

static int priv_usch_waitforall(int n);
 
#define READ  0
#define WRITE 1
 
/******************************* implementations **********************************/

static inline int priv_usch_stash(ustash_t *p_ustash, struct ustash_item *p_stashitem)
{
    int status = 0;

    if (p_ustash == NULL || p_stashitem == NULL)
        return -1;

    p_stashitem->p_next = p_ustash->p_list;
    p_ustash->p_list = p_stashitem;

    return status;
}

static inline void uclear(ustash_t *p_ustash)
{
    struct ustash_item *p_current = NULL;
    if (p_ustash == NULL)
        return;
    if (p_ustash->p_list == NULL)
        return;

    p_current = p_ustash->p_list;

    while (p_current != NULL)
    {
        struct ustash_item *p_prev = p_current;
        p_current = p_current->p_next;
        free(p_prev);
    }
    p_ustash->p_list = NULL;
}

static inline char **ustrsplit(ustash_t *p_ustash, const char* p_in, const char* p_delims)
{
    struct ustash_item *p_stashitem = NULL;
    char** pp_out = NULL;
    char* p_out = NULL;
    size_t len_in;
    size_t len_delims;
    size_t i, j;
    size_t num_str = 0;
    size_t size = 0;
    int out_pos = 0;

    if (p_ustash == NULL || p_in == NULL || p_delims == NULL)
        goto end;

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

    size = sizeof(struct ustash_item) + (len_in + 1) * sizeof(char) + (num_str + 1) * sizeof(char*);
    p_stashitem = calloc(size, 1);
    if (p_stashitem == NULL)
        goto end;

    pp_out = (char**)p_stashitem->str;
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

    if (priv_usch_stash(p_ustash, p_stashitem) != 0)
    {
        printf("stash failed, ohnoes!\n");
        goto end;
    }
    p_stashitem = NULL;
end:
    free(p_stashitem);
    return pp_out;
}

static inline char **priv_usch_strexp_impl(ustash_t *p_ustash, size_t num_args, char *p_str, ...)
{
    char **pp_strexp = NULL;
    va_list p_ap;
    size_t i;
    char **pp_orig_argv = NULL;
    char *p_actual_format = NULL;
    struct ustash_item *p_blob = NULL;
    static char* emptyarr[1];
	struct uglob_list_t *p_glob_list = NULL;
    size_t total_len = 0;
    char **pp_strexp_copy = NULL;
    char **pp_strexp_extmem = NULL;
    size_t pos = 0;
    size_t num_globbed_args = 0;
    char *p_strexp_data = NULL;


    emptyarr[0] = NULL;

    pp_strexp = emptyarr;

    if (p_str == NULL)
    {
        goto end;
    }

    pp_orig_argv = calloc(num_args + 1, sizeof(char*));
    if (pp_orig_argv == NULL)
    {
        goto end;
    }

    p_actual_format = calloc(num_args*2, sizeof(char));
    if (p_actual_format == NULL)
        goto end;

    for (i = 0; i < num_args * 2; i += 2)
    {
        p_actual_format[i + 0] = '%';
        p_actual_format[i + 1] = 's';
    }
    p_str = p_actual_format;

    va_start(p_ap, p_str);

    for (i = 0; i < num_args; i++)
    {
        pp_orig_argv[i] = va_arg(p_ap, char *);
    }
    pp_strexp_extmem = priv_usch_globexpand(pp_orig_argv, num_args, &p_glob_list);
    if (pp_strexp_extmem == NULL)
        goto end;

    for (i = 0; pp_strexp_extmem[i] != NULL; i++)
        total_len += strlen(pp_strexp_extmem[i]) + 1;

    num_globbed_args = i;

    p_blob = calloc(sizeof(struct ustash_item) + (num_globbed_args + 1) * sizeof(char*) + total_len, 1);
    if (p_blob == NULL)
        goto end;

    pp_strexp_copy = (char**)p_blob->str;
    pp_strexp_copy[num_globbed_args] = NULL;
    p_strexp_data = (char*)&pp_strexp_copy[num_globbed_args+1];
    // TODO: debug
    memset(p_strexp_data, 0x0, total_len);
    for (i = 0; pp_strexp_extmem[i] != NULL; i++)
    {
        size_t len = strlen(pp_strexp_extmem[i]);

        pp_strexp_copy[i] = &p_strexp_data[pos];
        pos += len + 1;
        memcpy(pp_strexp_copy[i], pp_strexp_extmem[i], len);
    }
    if (priv_usch_stash(p_ustash, p_blob) != 0)
    {
        printf("stash failed, ohnoes!\n");
        goto end;
    }

    pp_strexp = pp_strexp_copy;
end:
    pp_strexp_copy = NULL;
    if (num_args > 1)
    {
        va_end(p_ap);
    }
    priv_usch_free_globlist(p_glob_list);

    fflush(stdout);
    free(pp_orig_argv);
    free(p_actual_format);
    free(pp_strexp_extmem);

    return pp_strexp;
}

static inline char *udirname(ustash_t *p_ustash, const char *p_str)
{
    static char emptystr[] = "\0";
    char *p_dirname = emptystr;
    struct ustash_item* p_blob = NULL;
    int i = 0;
    int end = 0;

    while (p_str[i] != '\0')
    {
        if (p_str[i] == '/')
            end = i;
        i++;
    }
    if (end == 0)
    {
        end = i - 1;
    }
    p_blob = calloc(end + 1 + sizeof(struct ustash_item), 1);
    memcpy(p_blob->str, p_str, end);
    p_blob->str[end] = '\0';
    fprintf(stderr, "dirname %s\n", p_blob->str);

    if (priv_usch_stash(p_ustash, p_blob) != 0)
    {
        goto end;
    }
    p_dirname = p_blob->str;
    p_blob = NULL;
end:
    return p_dirname;
}

static inline char *ustrtrim(ustash_t *p_ustash, const char *p_str)
{
    static char emptystr[] = "\0";
    char *p_trim = emptystr;
    struct ustash_item* p_blob = NULL;
    int i = 0;
    int start = 0;
    int end = 0;
    size_t len;

    if (p_str == NULL)
        goto end;

    while (p_str[i] == ' ')
    {
        start = i+1;
        i++;
    }
    len = strlen(p_str);
    i = len - 1;
    while (p_str[i] == ' ' && i > start)
    {
        i--;
    }
    end = i+1;

    p_blob = calloc(end - start + 1 + sizeof(struct ustash_item), 1);
    memcpy(p_blob->str, &p_str[start], end-start);
    p_blob->str[end] = '\0';

    if (priv_usch_stash(p_ustash, p_blob) != 0)
    {
        goto end;
    }
    p_trim = p_blob->str;
    p_blob = NULL;
end:
    return p_trim;
}

static inline char *priv_usch_strjoin_impl(ustash_t *p_ustash, size_t num_args, char *p_str1, ...)
{
    static char emptystr[1];
    char *p_strjoin_retval = emptystr;
    char *p_strjoin = NULL;
    char *p_dststr = NULL;
    char **pp_orig_argv = NULL;

    emptystr[0] = '\0';

    va_list p_ap;
    size_t i;
    char *p_actual_format = NULL;
    struct ustash_item *p_blob = NULL;
    size_t total_len = 0;

    p_strjoin_retval = emptystr;

    if (p_str1 == NULL)
    {
        goto end;
    }

    pp_orig_argv = calloc(num_args + 1, sizeof(char*));
    if (pp_orig_argv == NULL)
    {
        goto end;
    }

    p_actual_format = calloc(num_args*2, sizeof(char));
    if (p_actual_format == NULL)
        goto end;

    for (i = 0; i < num_args * 2; i += 2)
    {
        p_actual_format[i + 0] = '%';
        p_actual_format[i + 1] = 's';
    }
    p_str1 = p_actual_format;

    va_start(p_ap, p_str1);

    for (i = 0; i < num_args; i++)
    {
        pp_orig_argv[i] = va_arg(p_ap, char *);
    }
    for (i = 0; pp_orig_argv[i] != NULL; i++)
    {
        total_len += strlen(pp_orig_argv[i]);
    }

    p_blob = calloc(sizeof(struct ustash_item) + sizeof(char) * total_len + 1, 1);
    if (p_blob == NULL)
        goto end;

    p_dststr = p_blob->str; 

    for (i = 0; pp_orig_argv[i] != NULL; i++)
    {
        size_t len = strlen(pp_orig_argv[i]);
        memcpy(p_dststr, pp_orig_argv[i], len);
        p_dststr += len;
    }

    *p_dststr = '\0';

    if (priv_usch_stash(p_ustash, p_blob) != 0)
    {
        printf("stash failed, ohnoes!\n");
        goto end;
    }

    p_strjoin_retval = p_blob->str;
    p_strjoin = NULL;
end:
    if (num_args > 1)
    {
        va_end(p_ap);
    }

    free(pp_orig_argv);
    free(p_actual_format);
    free(p_strjoin);

    return p_strjoin_retval;
}

static inline int priv_usch_cached_whereis(char** pp_cached_path, int path_items, char* p_search_item, char** pp_dest)
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
#if 0
static inline int uwhereis(char* p_item, char** pp_dest)
{
    char **pp_path = NULL;
    int status = 0;
    int num_items = 0;
    ustash_t s = {NULL};

    ustrsplit(&s, getenv("PATH"), ":", &pp_path);

    if (num_items < 1)
    {
        status = -1;
        goto end;
    }
    status = priv_usch_cached_whereis(pp_path, num_items, p_item, pp_dest);
end:
    free(pp_path);
    return status;
}
#endif // 0
static inline char **priv_usch_globexpand(char **pp_orig_argv, size_t num_args, struct uglob_list_t **pp_glob_list)
{
    char **pp_expanded_argv = NULL;
	struct uglob_list_t *p_glob_list = NULL;
	struct uglob_list_t *p_current_glob_item = NULL;
	size_t i, j;
	int orig_arg_idx = 0;
	int num_glob_items = 0;

	for (i = 0; i < num_args; i++)
	{
		if (strcmp(pp_orig_argv[i], "--") == 0)
		{
			orig_arg_idx++;
			break;
		}

		if (p_current_glob_item == NULL)
		{
			p_current_glob_item = calloc(1, sizeof(uglob_list_t));
			if (p_current_glob_item == NULL)
				goto end;
			p_glob_list = p_current_glob_item;
		}
		else
		{
			p_current_glob_item->p_next = calloc(1, sizeof(uglob_list_t));
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
	pp_expanded_argv = calloc(num_glob_items + num_args - orig_arg_idx + 1, sizeof(char*));
	p_current_glob_item = p_glob_list;

	i = 0;
	j = 0;
	while (p_current_glob_item != NULL)
	{
		for (j = 0; j < p_current_glob_item->glob_data.gl_pathc; j++, i++)
		{
			pp_expanded_argv[i] = p_current_glob_item->glob_data.gl_pathv[j];
		}
		p_current_glob_item = p_current_glob_item->p_next;
	}

	for (i = orig_arg_idx; i < num_args; i++)
	{
		pp_expanded_argv[j + i] = pp_orig_argv[i];
	}
    *pp_glob_list = p_glob_list;
    p_glob_list = NULL;

end:
    return pp_expanded_argv;
}
static inline void priv_usch_free_globlist(struct uglob_list_t *p_glob_list)
{
    if (p_glob_list)
    {
        struct uglob_list_t *p_current_glob_item = p_glob_list;
        while (p_current_glob_item != NULL)
        {
            struct uglob_list_t *p_free_glob_item = p_current_glob_item;

            p_current_glob_item = p_current_glob_item->p_next;

            globfree(&p_free_glob_item->glob_data);
            free(p_free_glob_item);
        }
    }
}

static inline int priv_usch_cmd_arr(struct ustash_item **pp_in, 
                               struct ustash_item **pp_out,
                               struct ustash_item **pp_err,
                               size_t num_args,
                               char **pp_orig_argv)
{
    (void)pp_in;
    (void)pp_err;
	struct uglob_list_t *p_glob_list = NULL;
	char **pp_argv = NULL;
    int argc = 0;
    int status = 0;
    int i = 0;
    int child_pid = 0;

    pp_argv = priv_usch_globexpand(pp_orig_argv, num_args, &p_glob_list);
    if (pp_argv == NULL)
        goto end;

    while (pp_argv[argc] != NULL)
    {
        if (*pp_argv[argc] == '|')
        {
            pp_argv[argc] = NULL;
        }
        argc++;
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
		int input = 0;
		int first = 1;
#if 0
        for (i = 0;  i < argc; i++)
        {
            printf("argv[%d] = %s\n", i, pp_argv[i]);
        }
#endif // 0
        i = 0;
        int j = 0;
        int last = 0;
        while (i < argc)
        {
            last = 1;
            j = i;
            while (j < argc)
            {
                if (pp_argv[j] == '\0')
                    last = 0;
                j++;
            }
			input = priv_usch_run(&pp_argv[i], input, first, last, &child_pid, pp_out);
 
			first = 0;
            while (i < argc && pp_argv[i] != NULL)
            {
                i++;
            }
            if (pp_argv[i] == NULL && i != argc)
            {
                i++;
            }
		}
        if (pp_argv[i] == NULL && pp_out == NULL)
        {
            input = priv_usch_run(&pp_argv[i], input, first, 1, &child_pid, pp_out);
        }

		status = priv_usch_waitforall(child_pid);
		n = 0;
    }
end:
    priv_usch_free_globlist(p_glob_list);
    free(pp_argv);

    return status;
}

static inline int priv_usch_cmd_impl(size_t num_args, char *p_name, ...)
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


    status = priv_usch_cmd_arr(NULL, NULL, NULL, num_args, pp_orig_argv);

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
static inline char* priv_usch_strout_impl(ustash_t *p_ustash, size_t num_args, char *p_name, ...)
{
    (void)p_ustash;
    (void)num_args;
    (void)p_name;


    char *p_strout = NULL;
    va_list p_ap;
    size_t i;
    char **pp_orig_argv = NULL;
    char *p_actual_format = NULL;
    struct ustash_item *p_out = NULL;
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
    (void)priv_usch_cmd_arr(NULL, &p_out, NULL, num_args, pp_orig_argv);
    if (priv_usch_stash(p_ustash, p_out) != 0)
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

/*
 * Handle commands separatly
 * input: return value from previous priv_usch_command (useful for pipe file descriptor)
 * first: 1 if first command in pipe-sequence (no input from previous pipe)
 * last: 1 if last command in pipe-sequence (no input from previous pipe)
 *
 * EXAMPLE: If you type "ls | grep shell | wc" in your shell:
 *    fd1 = command(0, 1, 0), with args[0] = "ls"
 *    fd2 = command(fd1, 0, 0), with args[0] = "grep" and args[1] = "shell"
 *    fd3 = command(fd2, 0, 1), with args[0] = "wc"
 *
 * So if 'command' returns a file descriptor, the next 'command' has this
 * descriptor as its 'input'.
 */
static int priv_usch_command(char **pp_argv, int input, int first, int last, int *p_child_pid, struct ustash_item **pp_out)
{
    struct ustash_item *p_ustash_item = NULL;
	int pipettes[2];
    pid_t pid;
   
	pipe(pipettes);
	pid = fork();
 
	/*
	 SCHEME:
	 	STDIN --> O --> O --> O --> STDOUT
	*/
 
	if (pid == 0) {
		if (first == 1 && last == 0 && input == 0) {
			// First command
			dup2(pipettes[WRITE], STDOUT_FILENO );
		} else if (first == 0 && last == 0 && input != 0) {
			// Middle command
			dup2(input, STDIN_FILENO);
			dup2(pipettes[WRITE], STDOUT_FILENO);
		} else {
			// Last command
            if (pp_out)
                dup2(pipettes[WRITE], STDOUT_FILENO );
            else
                dup2(input, STDIN_FILENO);
		}
 
		if (execvp(pp_argv[0], pp_argv) == -1)
        {
            printf("execv failed!\n");
			_exit(EXIT_FAILURE); // If child fails
        }
	}
 
	if (input != 0) 
		close(input);
 
	// Nothing more needs to be written
	close(pipettes[WRITE]);

    if (pp_out != NULL && last == 1)
    {
        size_t i = 0;
        size_t read_size = 1024;
        p_ustash_item = calloc(read_size + sizeof(struct ustash_item), 1);
        if (p_ustash_item == NULL)
            goto end;
        p_ustash_item->p_next = NULL;

        while (read(pipettes[READ], &p_ustash_item->str[i], 1) != 0)
        {
            i++;
            if (i >= read_size)
            {
                read_size *= 2;
                p_ustash_item = realloc(p_ustash_item, read_size + sizeof(struct ustash_item));
                if (p_ustash_item == NULL)
                    goto end;
            }
        }
        p_ustash_item->str[i] = '\0';
        if (i > 0)
        {
            if (p_ustash_item->str[i - 1] == '\n')
            {
                p_ustash_item->str[i - 1] = '\0';
            }
        }

    }

	// If it's the last command, nothing more needs to be read
	if (last == 1)
    {
		close(pipettes[READ]);
    }

    *p_child_pid = pid;

    if (pp_out)
    {
        *pp_out = p_ustash_item;
    }
    p_ustash_item = NULL;
end:
    free(p_ustash_item);
	return pipettes[READ];
}
 

/* @brief priv_usch_waitforall
 *
 * Wait for processes to terminate.
 *
 * @param  child_pid.
 * @return child error status.
 */
static int priv_usch_waitforall(int child_pid)
{
    pid_t wpid;
    int status;
    int child_status;
    do {
        wpid = waitpid(child_pid, &status, WUNTRACED
#ifdef WCONTINUED       /* Not all implementations support this */
                | WCONTINUED
#endif
                );
        if (wpid == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }


        if (WIFEXITED(status)) {
            //printf("child exited, status=%d\n", WEXITSTATUS(status));
            child_status = WEXITSTATUS(status);


        } else if (WIFSIGNALED(status)) {
            //printf("child killed (signal %d)\n", WTERMSIG(status));
            child_status = -1;


        } else if (WIFSTOPPED(status)) {
            //printf("child stopped (signal %d)\n", WSTOPSIG(status));
            child_status = -1;


#ifdef WIFCONTINUED     /* Not all implementations support this */
        } else if (WIFCONTINUED(status)) {
            //printf("child continued\n");
            child_status = -1;
#endif
        } else {    /* Non-standard case -- may never happen */
            //printf("Unexpected status (0x%x)\n", status);
            child_status = -1;
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    return child_status;
}
 
 
static int priv_usch_run(char **pp_argv, int input, int first, int last, int *p_child_pid, struct ustash_item **pp_out)
{
	if (pp_argv[0] != NULL) {
		n += 1;
		return priv_usch_command(pp_argv, input, first, last, p_child_pid, pp_out);
	}
	return 0;
}
 

#if NEED_VIM_WORKAROUND
{
#endif // NEED_VIM_WORKAROUND

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // USCH_H

