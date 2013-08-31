all: usch

usch: shell.o
		   gcc -g3 shell.o -lncursesw -o $@

clean:
		rm -rf *o usch

