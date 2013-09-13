all: usch tests run_tests test_num_args

test_num_args:
		   $(CC) -g3 test_num_args.c -o $@

tests: tests.o
		   $(CC) -Wall -Wextra -g3 -rdynamic tests.c -ldl -o $@

usch: shell.o pmcurses.o usch_eval.o
		   $(CC) -Wall -Wextra -g3 -rdynamic shell.o pmcurses.o usch_eval.o -ldl -o $@

run_tests: tests
		  ./tests 

clean:
		rm -rf *o usch

# DO NOT DELETE
