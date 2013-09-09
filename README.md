USCH - The (permutated) tcsh successor
======================================

USCH is a shell in early stages that aims to be a C Read-Evaluate-Print-Loop interpreter, useful enough for day-to-day use so the user can use a modern language like C instead of some deprecated oddjobb syntax from the sixties.
Commands are entered as-is with the REPL filling out the blanks (or more correct paranthesis).

What differs USCH from other C REPL implementations is that it's fully Free Software and has a fun little hackable codebase with portable C99 syntax.

More complete and well-tested alternatives are:
Cling: http://root.cern.ch/drupal/content/cling (no shell, C++ only, non-trivial to build last time I tried)
Ch: http://www.softintegration.com/ (no source for inspection but free to use, uses C99 superset)

Available commands
-------------------
Within the REPL, all commands not colliding with C functions or macros are declared as:

    int cmd("arg1", "arg2", ..., "arg n");

Within the REPL and when including usch.h in a standalone .c file the following functions/MACROs are declared:

    int cd("pwd");
    int whereis(char **pp_cached_paths, char *p_item, char *pp_destination);
    int strsplit(char *p_str, char *p_delims, char ***ppp_out);
    int strexp(char ***ppp_out, char *p_str1, ...);

Using USCH as a scripting language
----------------------------------
Creating an "alias" or enabling to call a command from a C99 file can be done as follows:

    #define cmd(...) usch_cmd(USCH_ARGC(__VA_ARGS__), "cmd", __VA_ARGS__);

Similar to "for" in bash, iterating over a file matching pattern can be done as follows:

    int i;
    char **pp_list = NULL;
    for (i = 0, num_items = strexp(&pp_list, "*.h"); i < num_items; i++)
    {
        printf("%s\n", pp_list[i]);
    }
    free(pp_list);

License
-------
USCH is licensed under the MIT/X11 license. See LICENSE for more information.
