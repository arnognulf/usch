# mkdtemp requires _BSD_SOURCE
LLVM_DIR=/usr/lib/llvm-3.4
CFLAGS=-D_BSD_SOURCE -std=gnu99 -I$(LLVM_DIR)/include/ -I../external/linenoise/ -Wall -Wextra -Werror -g3 -fno-builtin 
LIBS=$(LLVM_DIR)/lib/libclang.so.1 `pkg-config --libs --cflags` -ldl
COMMON_SOURCES=../external/linenoise/linenoise.c crepl.c bufstr.c crepl_parser.c crepl_linker.c crepl_vars.c strutils.c crepl_eval.c
TEST_SOURCES=tests.c $(COMMON_SOURCES)
MAIN_SOURCES=linenoise_main.c $(COMMON_SOURCES)
TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
MAIN_OBJECTS=$(MAIN_SOURCES:.c=.o)

all: usch tests run_tests test_num_args
test_num_args: test_num_args.o
	$(CC) $(CFLAGS) test_num_args.c $(LIBS) -o $@

tests: $(TEST_OBJECTS) 
		   $(CC) $(CFLAGS) -rdynamic $(TEST_OBJECTS) $(LIBS) -o $@

usch: $(MAIN_OBJECTS) 
		   $(CC) $(CFLAGS) -rdynamic $(MAIN_OBJECTS) $(LIBS) -o $@

run_tests: tests
	LD_LIBRARY_PATH=$(LLVM_DIR)/lib ./tests 

clean:
		rm -rf *o usch

# DO NOT DELETE
