#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define MAX_ARGS 42
#define MAX_SIZE 1149

typedef struct usch_alloc_t {
    struct usch_alloc_t* p_next;
    void* p_alloc;
} usch_alloc_t;

// usch_t is not stable and must not be allocated! only declared on the stack
typedef struct usch_t {
    usch_alloc_t *p_allocs;
    unsigned int return_status;
    char empty_string[1];
} usch_t;

usch_t usch_context = {0};
static void usch_cleanup(usch_t *p_context);
static void usch_add_alloc(usch_t *p_current_allocs, void *p_alloc);
static char* stdout_usch(usch_t *p_usch_context, size_t num_args, char* p_name, ...);
static char* strout_usch(usch_t *p_usch_context, size_t num_args, char* p_name, ...);

static void usch_add_alloc(usch_t *p_context, void *p_alloc) {
    usch_alloc_t *p_node = NULL;
    usch_alloc_t *p_list = NULL;

    if(p_context == NULL)
    {
        return;
    }

    p_node = calloc(1, sizeof(usch_alloc_t));
    if(p_node == NULL)
    {
        return;
    }
    p_node->p_alloc = p_alloc;
    p_list = p_context->p_allocs;
    while(p_list != NULL)
    {
        p_list = p_list->p_next;
    }
    p_list->p_next = p_node;
}

static void usch_cleanup(usch_t *p_context) {
    usch_alloc_t *p_next;
    usch_alloc_t *p_current;

    if(p_context == NULL) {
        return;
    }
    
    p_next = p_context->p_allocs;
    p_current = p_next;

    while (p_next != NULL)
    {
        p_current = p_next;
        p_next = p_next->p_next;
        free(p_current);
        p_current = NULL;
    }
    free(p_current);
}

static char *usch_tee(int size) {
   int shmid = shmget(IPC_PRIVATE, size + 1, 0660 | IPC_CREAT);
   int pipe_fds[2];
   pipe(pipe_fds);

   switch (fork()) {
      case -1:                      // = error
         perror("fork");
         exit(EXIT_FAILURE);
      case 0: {                     // = child
         char *out = shmat(shmid, 0, 0), c;
         int i = 0;
         out[0] = 0;
         dup2(pipe_fds[0], 0);      // redirect pipe to child's stdin
         setvbuf(stdout, 0, _IONBF, 0);
         while (read(0, &c, 1) == 1 && i < size) {
            printf("<%c>", c);      // pass parent's stdout to real stdout,
            out[i++] = c;           // and then buffer in mycapture buffer
            out[i] = 0;             // (the extra <> are just for clarity)
         }
         _exit(EXIT_SUCCESS);
      }
      default:                      // = parent
         dup2(pipe_fds[1], 1);      // replace stdout with output to child
         setvbuf(stdout, 0, _IONBF, 0);
         return shmat(shmid, 0, 0); // return the child's capture buffer
   }
}
static char* stdout_usch(usch_t *p_usch_context, size_t num_args, char *p_name, ...)
{
    va_list ap = {{0}};
    int i;
    char c;
    char *s = NULL;
    char *p_actual_format = NULL;
    //char *pp_argv[num_args + 1];
    char **pp_argv = NULL;
    pid_t child_pid;
    int child_status;

    if (p_usch_context == NULL || p_name == NULL)
    {
        return NULL;
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

    // make sure we always return a valid string (even in OOM cases)
    // the reason for this is that we do not expect 
    // rigorous error handling by the caller, 
    // eg. if a caller does not check for NULL when trying 
    // to use the string we might get a NULL dereference.
    return p_usch_context->empty_string;
}

static char* strout_usch(usch_t *p_usch_context, size_t num_args, char *p_name, ...)
{
    va_list ap = {{0}};
    int i;
    char c;
    char *s = NULL;
    char *p_actual_format = NULL;
    //char *pp_argv[num_args + 1];
    char **pp_argv = NULL;
    pid_t child_pid;
    int child_status;

    if (p_usch_context == NULL || p_name == NULL)
    {
        return NULL;
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

    // make sure we always return a valid string (even in OOM cases)
    // the reason for this is that we do not expect 
    // rigorous error handling by the caller, 
    // eg. if a caller does not check for NULL when trying 
    // to use the string we might get a NULL dereference.
    return p_usch_context->empty_string;
}
static char* usch(usch_t *p_usch_context, size_t num_args, char *p_name, ...)
{
    printf("usch called");
    return NULL;
}
#if 0
static char* strout_usch(usch_t *p_usch_context, size_t num_args, char *p_name, ...)
{
    va_list ap = {0};
    int d, i;
    char c, *s, *actual_format = NULL;
    char *pp_argv[num_args];
    int out_pipe[2];
    int saved_stdout;
    saved_stdout = dup(STDOUT_FILENO);
    char* stdout_buffer = NULL;
    pid_t child_pid;
    int child_status;
    char* p_dummy_format = p_name;
    size_t name_length = strlen(p_name);
    char p_arg0[name_length+1];

    strcpy(p_arg0, p_name);

    actual_format = calloc(num_args*2, sizeof(char));

    for(i=0; i < num_args*2; i+=2) {
        actual_format[i+0] = '%';
        actual_format[i+1] = 's';
    }
    p_dummy_format = actual_format;

    va_start(ap, p_name);
    pp_argv[0] = p_arg0;
    for(i=0;i<num_args;i++) {
        pp_argv[i+1] = va_arg(ap, char *);
    }
    if(pipe(out_pipe) != 0) {
        goto end;
    }
    if(p_usch_context->p_allocs != NULL)
    {
        dup2(out_pipe[1], STDOUT_FILENO);
    }

    child_pid = fork();
    if(child_pid == 0) {
        execve(pp_argv[0], pp_argv, NULL);
        goto end;
    } else {
        if(p_usch_context->p_allocs == NULL) {
            ssize_t readbytes;
            ssize_t alloc_size = MAX_SIZE;

            do {
                fflush(NULL);
                stdout_buffer = (void*)realloc(stdout_buffer, alloc_size);
                if(stdout_buffer == NULL) {
                    goto end;
                }

                readbytes = read(out_pipe[0], stdout_buffer, MAX_SIZE - 1);
                if(readbytes == -1) {
                    goto end;
                }
                // needed?
                printf("\0");
                fflush(NULL);

                stdout_buffer[alloc_size-1] = '\0';
                alloc_size+=MAX_SIZE;
            } while ( readbytes == MAX_SIZE);
            waitpid(child_pid, &child_status, WEXITED);
        }
    }
end:
    va_end(ap);
    if(p_usch_context->p_allocs != NULL) {
        dup2(saved_stdout, STDOUT_FILENO);
    }
    // make sure we always return a valid string (even in OOM cases)
    // the reason for this is that we do not expect 
    // rigorous error handling by the caller, 
    // eg. if a caller does not check for NULL when trying 
    // to use the string we might get a NULL dereference.
    if(p_usch_context->p_allocs == NULL) {
        return p_usch_context->empty_string;
    } else {
        // TODO: save stdout_buffer to list
        return stdout_buffer;
    }
    return NULL;
}
#endif // 0
#define USCH_ALLOC_SCOPE(cmd)  usch_context.p_allocs == NULL ? stdout_##cmd : (cmd) 
#define USCH_VA_NUM_ARGS(...) USCH_VA_NUM_ARGS_IMPL(__VA_ARGS__, 5,4,3,2,1)
#define USCH_VA_NUM_ARGS_IMPL(_1,_2,_3,_4,_5,N,...) N

#include "usch-alias.h"
int main()
{
    //ls("-al");
    ls("-1", "/");
    char* p_s = usch_tee(1024);
    printf("%s\n", p_s);
    exit(0);
}
