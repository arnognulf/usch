all: usch tests run_tests

tests: tests.o
		   gcc -g3 -rdynamic tests.c -ldl -o $@

usch: shell.o pmcurses.o eval.o
		   gcc -g3 -rdynamic shell.o pmcurses.o eval.o -ldl -o $@

run_tests: tests
		  ./tests 

clean:
		rm -rf *o usch

