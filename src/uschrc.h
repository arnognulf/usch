#include <usch.h>

// this file is included in all statements that are evaluated by usch
// it can be user overridden by copying it to ~/.uschrc.h

// uncomment the following line to define a command "alias" with parameters
//#define rm(...) ucmd("rm", "-i", ##__VA_ARGS__)

// the following function is run at usch startup
// it can be used to set environment variables
static inline void uschrc(ustash *p_mem)
{
    // uncomment the following line to set the $EDITOR variable to something sane ;)
    //setenv("EDITOR", "vim", 1);
    
    // uncomment the following line to give a nice welcoming message upon shell start.
    //printf("Usch says Hello!\n")
}

// the following function is run before each prompt is displayed
// the ustash is cleared before each call to this function and upon shell exit
static inline const char* prompt(ustash *p_prompt_stash)
{
    const char *p_prompt = NULL;
    const char *p_hostname = NULL;

    p_hostname = ustrout(p_prompt_stash, "hostname", "-s");
    p_prompt = ustrjoin(p_prompt_stash, getenv("USER"), "@", p_hostname, ":", ustrout(p_prompt_stash, "pwd"), "% ");
    return p_prompt;
}


