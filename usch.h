#ifndef USCH_H
#define USCH_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <glob.h>

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
    j = 0;
    
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
static inline int usch_strexp(char *p_in, size_t num_args, char ***ppp_out, char *p_str, ...)
{
    va_list p_ap = {{0}};
    int i;
    char *s = NULL;
    char *p_actual_format = NULL;
    char **pp_argv = NULL;
    pid_t child_pid;
    int child_status;

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
}
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

static inline int usch_cd(char *p_dir)
{
    int error; 
    error = chdir(p_dir);
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
            perror("Unknown error");
            break;
    }
    return error;
}
#ifdef cd
#warning included header defines cd(), usch_cd() must be called directly
#else
#define cd(str) usch_cd((str))
#endif // cd
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

static inline int usch_whereis(char** pp_cached_path, int path_items, char* p_item, char** pp_dest)
{
    int i;
    if (pp_cached_path == NULL)
    {
        if (usch_strsplit(getenv("PATH"), ":", &pp_cached_path) < 1)
        {
            goto error;
        }
    }
    for (i = 0; i < path_items; i++)
    {
        char* p_cand = strcat(pp_cached_path[i], p_item);
    }
    return 0;
error:
    return -1;
}
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

static inline int usch_cmd(size_t num_args, char *p_name, ...)
{
    va_list p_ap = {{0}};
    int i;
    char *s = NULL;
    char *p_actual_format = NULL;
    char **pp_argv = NULL;
    pid_t child_pid;
    int child_status;

    if (p_name == NULL)
    {
        return -1;
    }

    pp_argv = calloc(num_args + 2, sizeof(char*));
    pp_argv[0] = p_name;

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
        pp_argv[i + 1] = va_arg(p_ap, char *);
    }

    child_pid = fork();
    if(child_pid == 0)
    {
        execv(pp_argv[0], pp_argv);
        if(errno == EACCES)
        {
            goto end;
        }
        if(errno == ENOEXEC)
        {
            goto end;
        }

        goto end;
    }
    waitpid(child_pid, &child_status, WEXITED);
end:
    va_end(p_ap);

    free(pp_argv);

    return 0;
}

#define USCH_ARGC(...) USCH_ARGC_IMPL(__VA_ARGS__, 5,4,3,2,1)
#define USCH_ARGC_IMPL(_1,_2,_3,_4,_5,N,...) N

#if 0
#define ls(...) usch_cmd(USCH_ARGC(__VA_ARGS__), "/bin/ls", __VA_ARGS__)

int main()
{
    //ls("-al");
    ls("-1", "/");
    exit(0);
}
#endif // 0
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // USCH_H

