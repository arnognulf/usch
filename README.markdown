USCH - The (permutated) tcsh successor
======================================

USCH is a shell in early stages that aims to be a C Read-Evaluate-Print-Loop interpreter, useful enough for day-to-day use so the user don't have to resort to ALGOL68 scripting.

Commands are entered as-is with the REPL filling out the blanks (or more correct paranthesis).

All commands not colliding with C functions or macros are declared as:

    int cmd("arg1", "arg2", ..., "arg n");

Additional commands
-------------------
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

