
bin_PROGRAMS = pjass

AM_CFLAGS = @CYGOPTS@ -O2 -I.

pjass_SOURCES = \
grammar.y \
token.l \
misc.c \
misc.h

pjass_DEPENDENCIES = grammar.tab.h grammar.tab.c token.c token.o misc.o

token.c: token.l grammar.tab.h

grammar.tab.c: grammar.y
	bison grammar.y

grammar.tab.h: grammar.y
	bison -d grammar.y

clean:
	-rm grammar.tab.c grammar.tab.h token.c misc.o grammar.o token.o grammar.c
