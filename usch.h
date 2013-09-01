#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 42
#define MAX_SIZE 1149

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
static inline int usch_cmd(size_t num_args, char *p_name, ...)
{
    va_list ap = {{0}};
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

    va_start(ap, p_name);
    for (i = 0; i < num_args; i++)
    {
        pp_argv[i + 1] = va_arg(ap, char *);
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
    va_end(ap);

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
