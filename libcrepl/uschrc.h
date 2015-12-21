#include <usch.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <git2.h>

// this file is included in all statements that are evaluated by usch
// it can be user overridden by copying it to ~/.uschrc.h

// uncomment the following line to define a command "alias" with parameters
//#define rm(...) ucmd("rm", "-i", ##__VA_ARGS__)
#define ls(...) ucmd("ls", "--color=auto", ##__VA_ARGS__)

// the following function is run at usch startup
// it can be used to set environment variables
static inline void uschrc(ustash *p_mem)
{
    // uncomment the following line to set the $EDITOR variable to something sane ;)
    //setenv("EDITOR", "vim", 1);
    
    // uncomment the following line to give a nice welcoming message upon shell start.
    //printf("Usch says Hello!\n")

}

// quick and dirty git prompt function
char* git_prompt(ustash *s)
{
	static char *p_git_prompt_name = "";

	git_repository *repo = NULL;
	git_reference *ref = NULL;

	git_libgit2_init();
	git_repository_open_ext(&repo, ".", 0, NULL);
        if (repo == NULL)
		goto end;
	git_repository_head(&ref, repo);
        if (ref == NULL)
		goto end;
	git_ref_t type = git_reference_type(ref);
	if (type == GIT_REF_OID)
	{
		const git_oid *oid = git_reference_target(ref);
		const char *branch_name = NULL;
		git_branch_name(&branch_name, ref);
		if (branch_name)
		{
			p_git_prompt_name = ustrjoin(s, " (", branch_name, ")");
		}
		else
		{
			char shortsha[8] = {0};
			git_oid_tostr(shortsha, 7, oid);
			p_git_prompt_name = ustrjoin(s, " (", shortsha, "...)");
		}
	}
end:
	git_repository_free(repo);
	return p_git_prompt_name;
}

// the following function is run before each prompt is displayed
// the ustash is cleared before each call to this function and upon shell exit
static inline const char* prompt(ustash *p_prompt_stash)
{
    const char *p_prompt = NULL;
    const char *p_hostname = NULL;
    char *p_cwd = getcwd(NULL, INT_MAX);
    char *p_canon_cwd = realpath(p_cwd, NULL);
    char *p_prompt_cwd = NULL;
    
    if(ustrneq(getenv("HOME"), p_canon_cwd, strlen(getenv("HOME"))))
    {
        p_prompt_cwd = ustrjoin(p_prompt_stash, "~", &p_canon_cwd[strlen(getenv("HOME"))]);
    }
    else
    {
        p_prompt_cwd = p_canon_cwd;
    }

    p_hostname = ustrout(p_prompt_stash, "hostname", "-s");
    p_prompt = ustrjoin(p_prompt_stash, getenv("USER"), "@", p_hostname, ":", p_prompt_cwd, git_prompt(p_prompt_stash), "% ");
    free(p_cwd);
    free(p_canon_cwd);
    return p_prompt;
}


