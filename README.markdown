USCH - The (permutated) tcsh successor
======================================

USCH is a shell in early stages that aims to be a C Read-Evaluate-Print-Loop interpreter, useful enough for day-to-day use so the user don't have to resort to ALGOL68 scripting.

Commands are entered as-is with the REPL filling out the blanks (or more correct paranthesis).

All commands not colliding with C functions or macros are declared as:

    int cmd("arg1", "arg2", ..., "arg n");

Additional commands
-------------------
    int cd("pwd");

Using USCH as a scripting language
----------------------------------
    #define cmd(...) usch_cmd(USCH_ARGC(__VA_ARGS__), "cmd", __VA_ARGS__);

