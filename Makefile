all: usch tests run_tests test_num_args

test_num_args:
		   gcc -g3 test_num_args.c -o $@

tests: tests.o
		   gcc -Wall -Wextra -g3 -rdynamic tests.c -ldl -o $@

usch: shell.o pmcurses.o eval.o
		   gcc -Wall -Wextra -g3 -rdynamic shell.o pmcurses.o eval.o -ldl -o $@

run_tests: tests
		  ./tests 

clean:
		rm -rf *o usch

# DO NOT DELETE
