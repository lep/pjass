
bin_PROGRAMS = pjass

AM_CFLAGS = -mno-cygwin -O2

pjass_SOURCES = \
misc.c \
misc.h \
token.l \
grammar.y

#pjass_DEPENDENCIES = token.c grammar.tab.c grammar.tab.h misc.o

token.c: token.l

grammar.tab.c: grammar.y

grammar.tab.h: grammar.y
	bison -d $<

clean:
	-rm grammar.tab.c grammar.tab.h token.c misc.o grammar.o token.o grammar.c
