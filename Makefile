CFLAGS = -w -O3 -flto -Os
VERSION := $(shell git rev-parse --short HEAD)

# when testing and releasing, we can't run both in parallel
# but we also don't want to test when we're just making the zip
# so this rule depends on test just when we're both releasing and testing
ifneq (,$(findstring release,$(MAKECMDGOALS)))
  ifneq (,$(findstring test,$(MAKECMDGOALS)))

pjass-git-$(VERSION).zip: | test

  endif
endif



.PHONY: all release clean debug

all:  pjass

debug: CFLAGS = -w -g
debug: pjass

pjass: lex.yy.o grammar.tab.o misc.o
	$(CC) $(CFLAGS) $^ -o $@ -DVERSIONSTR="\"git-$(VERSION)\""

lex.yy.o: lex.yy.c grammar.tab.h

misc.o: misc.c misc.h grammar.tab.h
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

lex.yy.c: token.l
	flex $<

%.tab.c %.tab.h: %.y
	bison -d $<

clean:
	rm -f grammar.tab.h grammar.tab.c lex.yy.c pjass.exe
	rm -f misc.o grammar.tab.o lex.yy.o
	rm -f pjass-git-*.zip

release: pjass-git-$(VERSION)-src.zip pjass-git-$(VERSION).zip

pjass-git-$(VERSION)-src.zip: grammar.y token.l misc.c misc.h Makefile notes.txt readme.txt
	zip -q pjass-git-$(VERSION)-src.zip $^

pjass-git-$(VERSION).zip: pjass
#ResourceHacker -addskip pjass.exe pjass.exe, pjass.res ,,,
	strip pjass.exe
	upx --best --ultra-brute pjass.exe > /dev/null
	zip -q pjass-git-$(VERSION).zip pjass.exe


SHOULD_FAIL := $(wildcard tests/should-fail/*.j)
SHOULD_CHECK := $(wildcard tests/should-check/*.j)

.PHONY: test print-test $(SHOULD_CHECK) $(SHOULD_FAIL)

$(SHOULD_CHECK): pjass print-test
	@./check.sh $@

$(SHOULD_FAIL): pjass print-test
	@./fail.sh $@

test: $(SHOULD_FAIL) $(SHOULD_CHECK)

print-test: pjass
	@echo 'Testing... '
