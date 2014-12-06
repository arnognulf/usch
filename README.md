USCH - The (permutated) tcsh successor
======================================

UNDER CONSTRUCTION
==================
Things are not working even at a basic level, take any code and docs with a grain of salt :(

TL;DR
-----

USCH is a shell where C is the command language. It has nothing to do with the harmfully considered csh or tcsh.

About
-----

USCH is a shell in early stages that aims to be a C Read-Evaluate-Print-Loop interpreter, useful enough for day-to-day use so the user can use a modern language like C instead of some deprecated oddjobb syntax from the sixties.

Commands are entered as-is with the REPL filling out the blanks (or more correct paranthesis).


What differs USCH from other C REPL implementations is that it's fully Free Software and has a fun little hackable codebase with portable C99 syntax.

Related C interpreters
----------------------

Cling: http://root.cern.ch/drupal/content/cling (no shell, C++ only, non-trivial to build last time I tried)

Ch: http://www.softintegration.com/ (no source for inspection but free to use, uses C99 superset)

Usage
-----
USCH consists of the usch.h shell API and the REPL interpreter crepl.
The USCH shell is started by entering:

    ./usch

At the shell prompt.



For any identifiers that have not yet been defined, the command path is searched for a command with the name of the identifier.


The command will then be defined as a variadic macro with zero or more arguments, where each argument is passed as a string.


Paranthesises and quotation marks will be inserted after such varadic macro when the spacebar is pressed, this is referred to as space-completion (in contrast to tab-completion), if a space character is required, press space twice.

Available commands
-------------------
Within the REPL, all commands not colliding with C functions or macros are declared as:

    int cmd("arg1", "arg2", ..., "arg n");

Within the REPL and when including usch.h in a standalone .c file the following functions/MACROs are declared:

    int cd("pwd");
    char** usch_strsplit(usch_stash_t *p_memstash, const char* p_in, const char* p_delims)
    char** usch_strexp(usch_stash_t *p_memstash, char *p_item1, ...)
    char** usch_strexp_arr(usch_stash_t *p_memstash, char **pp_items)
    char*  usch_strjoin(usch_stash_t *p_memstash, char *p_item1, ...)
    char*  usch_strjoin_arr(usch_stash_t *p_memstash, char **pp_items)

Using USCH as a scripting language
----------------------------------
Creating an "alias" or enabling to call a command from a C99 file can be done as follows:

    #define cmd(...) usch_cmd("cmd", __VA_ARGS__);

Similar to "for" in bash, iterating over a file matching pattern can be done as follows:

    // usch_stash_t is a linked list where allocations are stashed
    ustash_t s = {NULL};
    char **hdrs;
    // usch_strexp will never return NULL
    for (hdrs = ustrexp(&s, "*.h"); *hdrs != NULL; hdrs++)
    {
        printf("%s\n", hdrs[i]);
    }
    // clear allocations in stash list
    uclear(&s);
    // pp_exp is now already free'd 
    pp_exp = NULL;

BUGS
----
Usch cannot be compiled with -pedantic. This is a limitation of the 0-n arg macro.

Report bugs at: https://github.com/arnognulf/usch/issues

Contributing
------------
Create a fork, and send a pull request or patch either via Github or from your public git server.

License
-------
USCH is licensed under the MIT/X11 license. See LICENSE for more information.

