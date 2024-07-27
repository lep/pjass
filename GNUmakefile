CFLAGS += -MMD -g -w
VERSION ?= $(shell git rev-parse --short HEAD || echo nix)

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

SRC := misc.c hashtable.c paramlist.c funcdecl.c typeandname.c blocks.c tree.c sstrhash.c

OBJS := $(SRC:.c=.o)
OBJS += main.o token.yy.o grammar.tab.o

.PHONY: all release clean prof
.PHONY: clean-release-files clean-prof-files clean-build-files
.PHONY: binary-release src-release
.PHONY: help

all: pjass

help:
	@awk -F ':|##' \
		'/^[^\t].+?:.*?##/ { \
			printf "\033[36m%-20s\033[0m %s\n", $$1, $$NF \
		}' $(MAKEFILE_LIST)

-include $(OBJS:.o=.d)

pjass: $(OBJS) ## Builds pjass
	$(CC) $(CFLAGS) $^ -o $@

test: should-fail should-check map-scripts ## Runs all tests

release: src-release binary-release ## Builds a pjass release with src- and bin-zipballs

clean: clean-build-files clean-release-files clean-prof-files ## Cleans the whole project


prof: CFLAGS = -w -pg
prof: pjass ## Builds pjass with profiling support. You can run all tests with profiling enabled via `make PROF=1 test`



main.o: main.c token.yy.h grammar.tab.h
	$(CC) $(CFLAGS) -c -o $@ $< -DVERSIONSTR="\"git-$(VERSION)\""

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<


token.yy.o: token.yy.c | grammar.tab.h
main.o: main.c | grammar.tab.h

# see token.l options block
%.yy.c %.yy.h: %.l
	flex $<

%.tab.c %.tab.h: %.y
	bison -d $<


clean-build-files: ## Cleans all build files
	rm -f grammar.tab.h grammar.tab.c token.yy.c token.yy.h \
		$(OBJS) $(OBJS:.o=.d) \
		pjass pjass.exe

clean-release-files: ## Cleans all release zipballs
	rm -f pjass-git-*.zip

clean-prof-files: ## Cleans all profiling files
	rm -f tests/should-check/*-analysis.txt \
          tests/should-fail/*-analysis.txt \
          tests/map-scripts/*-analysis.txt \
          gmon.out



pjass.exe: CFLAGS=-O3 -march=native
pjass.exe: CC=i686-w64-mingw32-gcc
pjass.exe: $(SRC) main.c token.yy.c grammar.tab.c ## Builds a windows executable using mingw
	find $^ | awk '{ print "#include \"" $$1 "\""}' | $(CC) $(CFLAGS) -x c -o $@ - -DVERSIONSTR="\"git-$(VERSION)\"" -DPJASS_AMALGATION

src-release: pjass-git-$(VERSION)-src.zip ## Builds the source zipball
binary-release: pjass.exe pjass-git-$(VERSION).zip ## Builds the exe zipball

pjass-git-$(VERSION)-src.zip: main.c grammar.y token.l GNUmakefile readme.md AUTHORS LICENSE $(SRC:.c=.h) $(SRC)
	zip -q -r pjass-git-$(VERSION)-src.zip $^ tests/should-check/ tests/should-fail/

pjass-git-$(VERSION).zip: pjass.exe
	i686-w64-mingw32-strip pjass.exe
	upx --best --ultra-brute pjass.exe > /dev/null
	zip -q pjass-git-$(VERSION).zip pjass.exe


SHOULD_FAIL := $(wildcard tests/should-fail/*.j)
SHOULD_FAIL += $(wildcard tests/should-fail/**/*.j)

SHOULD_CHECK := $(wildcard tests/should-check/*.j)
SHOULD_CHECK += $(wildcard tests/should-check/**/*.j)

MAP_SCRIPTS := $(wildcard tests/map-scripts/*.j)

.PHONY: test print-test should-check should-fail map-scripts
.PHONY: $(SHOULD_CHECK) $(SHOULD_FAIL) $(MAP_SCRIPTS)

$(MAP_SCRIPTS): pjass print-test
	@MAPSCRIPT=1 ./check.sh $@

$(SHOULD_CHECK): pjass print-test
	@./check.sh $@


$(SHOULD_FAIL): pjass print-test
	@./fail.sh $@


should-fail: $(SHOULD_FAIL) ## Tests that should fail
should-check: $(SHOULD_CHECK) ## Tests that should check
map-scripts: $(MAP_SCRIPTS) ## Tests which are run with common.j and Blizzard.j



print-test: pjass
	@echo 'Testing... '

