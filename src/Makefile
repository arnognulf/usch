# mkdtemp requires _BSD_SOURCE
LLVM_DIR = $(shell llvm-config-3.6 --prefix 2>/dev/null)

DESTDIR=/usr/local
PREFIX=$(DESTDIR)

CFLAGS=-O0 -D_BSD_SOURCE -std=gnu99 -pedantic -Wno-gnu-zero-variadic-macro-arguments -I$(LLVM_DIR)/include/ -I../external/linenoise/ -Wall -Wextra -Werror -g3 -DUSCH_INSTALL_PREFIX=\"$(PREFIX)\"
LIBS=$(LLVM_DIR)/lib/libclang.so.1 -ldl
COMMON_SOURCES=../external/commander/src/commander.c ../external/linenoise/linenoise.c crepl.c bufstr.c crepl_parser.c crepl_linker.c crepl_vars.c strutils.c crepl_eval.c
TEST_SOURCES=tests.c $(COMMON_SOURCES)
MAIN_SOURCES=linenoise_main.c $(COMMON_SOURCES)
TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
MAIN_OBJECTS=$(MAIN_SOURCES:.c=.o)

all: usch test_num_args tests run_tests
test_num_args: test_num_args.o
	$(CC) $(CFLAGS) test_num_args.c $(LIBS) -o $@

tests: test_num_args $(TEST_OBJECTS) 
		   $(CC) $(CFLAGS) -rdynamic $(TEST_OBJECTS) $(LIBS) -o $@

usch: $(MAIN_OBJECTS) 
		   $(CC) $(CFLAGS) -rdynamic $(MAIN_OBJECTS) $(LIBS) -o $@

run_tests: tests usch
	LD_LIBRARY_PATH=$(LLVM_DIR)/lib ./tests 

clean:
		rm -rf $(MAIN_OBJECTS) $(TEST_OBJECTS) test_num_args tests

install: usch tests run_tests
	install -m 0644 usch.h $(PREFIX)/include
	install -m 0755 usch $(PREFIX)/bin
	install -m 0644 uschrc.h $(PREFIX)/etc

.PHONY: install
# DO NOT DELETE
#
