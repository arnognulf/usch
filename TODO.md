DONE 1. globbing
 1.1 strexp()
DONE 4. line editing
DONE 5. line history (memory)
6. persistent line history: .usch_history
7. tab completion for dirs and files
8. prompt
9. ignore SIGSTOP/SIGCONT (CTRL+Z)
10. proper job control
11. piping.
12. redirect to file (tee() may be used as workaround if 11. is fixed)
14. usch_context_t *p_usch = {...}; usch_eval(p_usch, "int i = x + 32") (may need 13.)
15. function to set callback for prompt, whats needed? probably 13.
16. fix makefile to rebuild upon changes in any file.
17. enter blocks in repl with autmatic newlines
DONE 18. declare functions in repl with automatic newlines
19. shebang #!/bin/usch
20. uschshell_eval(uschshell_t *p_context)
21. line editing and printing of unicode characters, remember full-width: http://en.wikipedia.org/wiki/Halfwidth_and_fullwidth_forms#In_Unicode
DONE 22. push back quote chars if nothing entered after space
23. recursive pre-parser?
[identifier] [identifier [([arg...])]
24. handle line buffer separately: set_prompt(). add(uchar); insert_at(pos). erase(n). draw()
25. get LIBRARY_PATH: ld --verbose | grep SEARCH
    in lib(), search LIBRARY_PATH
