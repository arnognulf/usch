all: usch

usch: shell.o pmcurses.o eval.o
		   gcc -g3 -rdynamic shell.o pmcurses.o eval.o -ldl -o $@

clean:
		rm -rf *o usch

