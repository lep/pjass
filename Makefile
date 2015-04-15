CC=gcc
CFLAGS=-w

.PHONY: all

all:  pjass

pjass: lex.yy.c grammar.tab.c grammar.tab.h misc.c misc.h
	$(CC) $(CFLAGS) lex.yy.c grammar.tab.c misc.c -o $@ -O2 -DVERSIONSTR="\"git-$(shell git rev-parse --short HEAD)\""


lex.yy.c: token.l
	flex $<

%.tab.c %.tab.h: grammar.y
	bison -d $<

clean:
	rm grammar.tab.h grammar.tab.c lex.yy.c pjass.exe
