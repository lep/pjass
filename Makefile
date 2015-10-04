CC=gcc
CFLAGS=-w -O2

VERSION:=$(shell git rev-parse --short HEAD)

.PHONY: all release clean

all:  pjass

pjass: lex.yy.c grammar.tab.c grammar.tab.h misc.c misc.h
	$(CC) $(CFLAGS) lex.yy.c grammar.tab.c misc.c -o $@ -DVERSIONSTR="\"git-$(VERSION)\""


lex.yy.c: token.l
	flex $<

%.tab.c %.tab.h: %.y
	bison -d $<

clean:
	rm grammar.tab.h grammar.tab.c lex.yy.c pjass.exe
	rm pjass-git-*.zip

release: pjass-git-$(VERSION)-src.zip pjass-git-$(VERSION).zip

pjass-git-$(VERSION)-src.zip: grammar.y token.l misc.c misc.h Makefile notes.txt readme.txt
	zip -q pjass-git-$(VERSION)-src.zip $^

pjass-git-$(VERSION).zip: pjass
#ResourceHacker -addskip pjass.exe pjass.exe, pjass.res ,,,
	strip pjass.exe
	upx --best pjass.exe > /dev/null
	zip -q pjass-git-$(VERSION).zip pjass.exe
