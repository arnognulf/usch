USCH - C shell with vanilla C syntax
====================================

USCH is an experimental shell to make scripting and command execution in a REPL possible in vanilla C.

About
-----

USCH is a shell in early stages that aims to be a C Read-Evaluate-Print-Loop interpreter, useful enough for day-to-day use so the user can use a modern language like C instead of some deprecated oddjobb syntax from the sixties.


Commands are entered as-is with the REPL filling out the blanks and any unclosed paranthesis.


What differs USCH from other C REPL implementations is that it's fully Free Software and has a fun little hackable codebase with portable C99 syntax.

Usage
-----
USCH consists of the usch.h shell API and the REPL interpreter crepl.
The USCH shell is started by entering the following at the shell prompt:

    usch

A familar "c-shell" style prompt will be displayed:

    user@host:/home/user% 

This prompt can be changed by copying uschrc.h to your home directory as .uschrc.h and modifying it.


When entering a command, any identifiers that have not yet been defined, the command path is searched for a command with the name of the identifier.


The command will then be defined as a variadic macro with zero or more arguments, where each argument is passed as a string.


Paranthesises and quotation marks will be inserted after such varadic macro when the spacebar is pressed, this is referred to as space-completion (in contrast to tab-completion), if a space character is required, press space twice:

    ls<SPACE>-al<SPACE>*.h

Will produce the following function in your REPL:

    ls("-al", "*.h

Pressing Return will complete the statement and fill in any unbalanced quotes and paranthesis.

Variables
---------
A variable can be declared at the prompt:

    float a = 42.0;

If no initial value is given, the variable gets zero as its value.


If the variable has been updated, the value gets printed as follows:

    a = 32
    %a = 32.000000

However, writing the same value to the variable, does not cause an update write:

    a = 32
    

Loading dynamic libraries and headers
-------------------------------------
Dynamic libraries can be loaded at the command prompt, with the #lib preprocessor directive:

    #lib m

After a library has been successfully loaded, its header can be loaded:

    #include <math.h>

After the header has been loaded, all symbols will be resolved and trampoline functions calling into the dynamic library will be defined.

After this, you can do stuff like:

    float a = 0
    a = cos(0.5)
    // and recieve the updated variable reply:
    %a = 0.877583

REPL-friendly string functions
------------------------------
A few REPL-friendly functions to simplify string concatenation in the REPL environment is included in usch.h:

Pros:
 * not having to declare and keep track of returned strings.
 * being able to free strings at a later point. 
 * use 1-n argument macro (as used in ucmd) to enable concatenation of multiple strings at once.
 * operate on regular C strings.
 * returns empty string upon error.
 * treats input strings as immutable.

Cons:
 * always allocates new strings (performance/memory gotcha)

    ustash s = {0};
    char myfile[] = "my_stuffs.txt";

    cat((ustrjoin(&s, "/tmp/", myfile));
    
    uclear(&s);

Available commands
-------------------
Within the REPL, all commands that can be represented as valid identifiers (that means no "7z", "./configure" and "gcc-4.8") and not colliding with C functions or macros are declared as:

    int cmd("arg1", "arg2", ..., "arg n");

Within the REPL and when including usch.h in a standalone .c file the following functions/MACROs are declared:

    int cd("pwd");
    // join 1-n strings into one buffer
    char*  ustrjoin(usch_stash_t *p_memstash, char *p_item1, ...)
    // split a string by delimeters, into an array
    char** ustrsplit(usch_stash_t *p_memstash, const char* p_in, const char* p_delims)
    // create an array from 1-n strings with glob expansion (eg. *.h)
    char** ustrexp(usch_stash_t *p_memstash, char *p_item1, ...)
    // dirname version
    char* udirname(usch_stash_t *p_memstash, char *p_dir)
    // run a command, and return stdout as a buffer
    char* ustrout(usch_stash_t *p_memstash, char *p_item1, ...)

To run commands that can not be represented as valid identifiers, the ucmd() macro may be called with the command as its arguments:

    ucmd("gcc-4.8", "foo.c", "-o", "bar.c")


Using USCH as a scripting language
----------------------------------
Creating an "alias" or enabling to call a command from a C99 file can be done as follows:

    #define cmd(...) usch_cmd("cmd", __VA_ARGS__);

Similar to "for" in bash, iterating over a file matching pattern can be done as follows:

    // ustash_t is a linked list where allocations are stashed
    ustash_t s = {NULL};
    char **hdrs;
    // ustrexp will never return NULL
    for (hdrs = ustrexp(&s, "*.h"); *hdrs != NULL; hdrs++)
    {
        printf("%s\n", hdrs[i]);
    }
    // clear allocations in stash list
    uclear(&s);
    // hdrs is now already free'd 
    hdrs = NULL;

Debugging USCH
--------------
USCH forks off a process which will be respawned if crashed; thus to debug USCH; start gdb with:

    gdb --args ./usch -s

BUGS
----
Report bugs at: https://github.com/arnognulf/usch/issues

Contributing
------------
Create a fork, and send a pull request or patch either via Github or from your public git server.

Related C interpreters
----------------------

Cling: http://root.cern.ch/drupal/content/cling (no shell, C++ only, non-trivial to build last time I tried)

Ch: http://www.softintegration.com/ (no source for inspection but free to use, uses C99 superset)

License
-------
USCH is licensed under the MIT/X11 license. See LICENSE for more information.

<script src="js/jr.js"></script>

