all: usch

usch: usch.o
		   gcc -g3 usch.o -lncursesw -o $@

clean:
		rm -rf *o usch

