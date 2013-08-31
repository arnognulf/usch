all: usch

usch: shell.o
		   gcc -g3 -rdynamic shell.o -lncursesw -ldl -o $@

clean:
		rm -rf *o usch

