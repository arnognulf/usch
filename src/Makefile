# mkdtemp requires _BSD_SOURCE
CFLAGS=-D_BSD_SOURCE -std=gnu99 -Wall -Wextra -Werror -g3
all: usch tests run_tests test_num_args

test_num_args:
	$(CC) -pedantic -Wall -Wextra -Werror -g3 test_num_args.c -o $@

tests: tests.o uschshell.o pmcurses.o
		   $(CC) -pedantic -Wall -Wextra -Werror -g3 -rdynamic tests.o pmcurses.o uschshell.o -ldl -o $@

usch: shell.o pmcurses.o uschshell.o
		   $(CC) -pedantic -Wall -Wextra -Werror -g3 -rdynamic shell.o pmcurses.o uschshell.o -ldl -o $@

run_tests: tests
		  ./tests 

clean:
		rm -rf *o usch

# DO NOT DELETE
