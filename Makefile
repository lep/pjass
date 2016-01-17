CFLAGS = -O4
VERSION := $(shell git rev-parse --short HEAD)

# when testing and releasing, we can't run both in parallel
# but we also don't want to test when we're just making the zip
# additionaly we want to test before we make any zip file
# so these rules depend on test just when we're both releasing and testing
ifneq (,$(findstring release,$(MAKECMDGOALS)))
  ifneq (,$(findstring test,$(MAKECMDGOALS)))

pjass-git-$(VERSION).zip: | test
pjass-git-$(VERSION)-src.zip: | test

  endif
endif


.PHONY: all release clean debug prof clean-release-files clean-prof-files clean-build-files

all:  pjass

debug: CFLAGS = -w -g
debug: pjass

prof: CFLAGS = -w -pg
prof: pjass

pjass: lex.yy.o grammar.tab.o misc.o
	$(CC) $(CFLAGS) $^ -o $@

lex.yy.o: lex.yy.c grammar.tab.h

misc.o: misc.c misc.h grammar.tab.h
	$(CC) $(CFLAGS) -c -o $@ $< -DVERSIONSTR="\"git-$(VERSION)\""

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

lex.yy.c: token.l
	flex $<

%.tab.c %.tab.h: %.y
	bison -d $<

clean: clean-build-files clean-release-files clean-prof-files

clean-build-files:
	rm -f grammar.tab.h grammar.tab.c lex.yy.c \
          misc.o grammar.tab.o lex.yy.o \
          pjass.exe

clean-release-files:
	rm -f pjass-git-*.zip

clean-prof-files:
	rm -f tests/should-check/*-analysis.txt \
          tests/should-fail/*-analysis.txt \
          gmon.out

release: pjass-git-$(VERSION)-src.zip pjass-git-$(VERSION).zip

pjass-git-$(VERSION)-src.zip: grammar.y token.l misc.c misc.h Makefile notes.txt readme.txt
	zip -q pjass-git-$(VERSION)-src.zip $^

pjass-git-$(VERSION).zip: pjass
	strip pjass.exe
	upx --best --ultra-brute pjass.exe > /dev/null
	zip -q pjass-git-$(VERSION).zip pjass.exe


SHOULD_FAIL := $(wildcard tests/should-fail/*.j)
SHOULD_CHECK := $(wildcard tests/should-check/*.j)
MAP_SCRIPTS := $(wildcard tests/map-scripts/*.j)

.PHONY: test print-test should-check should-fail map-scripts
.PHONY: $(SHOULD_CHECK) $(SHOULD_FAIL) $(MAP_SCRIPTS)

$(MAP_SCRIPTS): pjass print-test
	@MAPSCRIPT=1 ./check.sh $@

$(SHOULD_CHECK): pjass print-test
	@./check.sh $@


$(SHOULD_FAIL): pjass print-test
	@./fail.sh $@

should-fail: $(SHOULD_FAIL)
should-check: $(SHOULD_CHECK)
map-scripts: $(MAP_SCRIPTS)

test: should-fail should-check map-scripts


print-test: pjass
	@echo 'Testing... '
