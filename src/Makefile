# mkdtemp requires _BSD_SOURCE
LLVM_DIR=/usr/lib/llvm-3.4
CFLAGS=-D_BSD_SOURCE -std=gnu99 -I$(LLVM_DIR)/include/ -Wall -Wextra -Werror -g3
all: usch tests run_tests test_num_args

test_num_args:
	$(CC) -pedantic -Wall -Wextra -Werror -g3 test_num_args.c -o $@

tests: tests.o uschshell.o pmcurses.o
		   $(CC) -pedantic -Wall -Wextra -Werror -g3 -rdynamic tests.o pmcurses.o uschshell.o  /usr/lib/llvm-3.4/lib/libclang.so.1 `pkg-config --libs --cflags libedit` -ldl -o $@

usch: pmcurses.o uschshell.o readline_helper.o
		   $(CC) -pedantic -Wall -Wextra -Werror -g3 -rdynamic pmcurses.o uschshell.o readline_helper.o /usr/lib/llvm-3.4/lib/libclang.so.1 -ldl `pkg-config --libs --cflags libedit` -o $@

run_tests: tests
	LD_LIBRARY_PATH=$(LLVM_DIR)/lib ./tests 

clean:
		rm -rf *o usch

# DO NOT DELETE
