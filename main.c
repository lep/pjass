#include "token.yy.h"
#include "grammar.tab.h"

#include "misc.h"

#ifndef VERSIONSTR
#define VERSIONSTR "1.0-git"
#endif
#define ERRORLEVELNUM 4

static struct typenode* addPrimitiveType(const char *name) {
    struct typenode *new = newtypenode(name, NULL);
    put(&types, name, new);
    return new;
}


static void init() {
    ht_init(&functions, 8191);
    ht_init(&globals, 8191);
    ht_init(&locals, 23);
    ht_init(&params, 11);
    ht_init(&types, 511);
    ht_init(&initialized, 23);

    gHandle = addPrimitiveType("handle");
    gInteger = addPrimitiveType("integer");
    gReal = addPrimitiveType("real");
    gBoolean = addPrimitiveType("boolean");
    gString = addPrimitiveType("string");
    gCode = addPrimitiveType("code");

    gNothing = newtypenode("nothing", NULL);
    gNull = newtypenode("null", NULL);

    gAny = newtypenode("any", NULL);
    gNone = newtypenode("none", NULL);
    gEmpty = newtypenode("empty", NULL);

    curtab = &globals;
    fno = 0;
    strict = 0;
    returnbug = 0;
    fnhasrbannotation = 0;
    rbannotated = 0;
    haderrors = 0;
    ignorederrors = 0;
    islinebreak = 1;
    inblock = 0;
    isconstant = 0;
    inconstant = 0;
    infunction = 0;
    fCurrent = 0;
}

static void dofile(FILE *fp, const char *name) {
    lineno = 1;
    islinebreak = 1;
    isconstant = 0;
    inconstant = 0;
    inblock = 0;
    afterendglobals = 0;
    int olderrs = haderrors;
    yy_switch_to_buffer(yy_create_buffer(fp, BUFSIZE));
    curfile = name;

    while ( yyparse() ) ;

    if (olderrs == haderrors){
        printf("Parse successful: %8d lines: %s\n", lineno, curfile);
    }else{
        printf("%s failed with %d error%s\n", curfile, haderrors - olderrs,(haderrors == olderrs + 1) ? "" : "s");
    }
    totlines += lineno;
    fno++;
}

static void printversion() {
	printf("Pjass version %s by Rudi Cilibrasi, modified by AIAndy, PitzerMike, Deaod and lep\n", VERSIONSTR);
}

static void doparse(int argc, char **argv) {
	int i;
	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] == 0) {
			dofile(stdin, "<stdin>");
			didparse = 1;
			continue;
		}
		if (strcmp(argv[i], "-h") == 0) {
			printversion();
printf(
"To use this program, list the files you would like to parse in order.\n"
"If you would like to parse from standard input (the keyboard), then\n"
"use - as an argument.  If you supply no arguments to pjass, it will\n"
"parse the console standard input by default.\n"
"To test this program, go into your Scripts directory, and type:\n"
"pjass common.j common.ai Blizzard.j\n"
"pjass accepts some options:\n"
"pjass -h           Display this help\n"
"pjass -v           Display version information and exit\n"
"pjass -e1          Ignores error level 1\n"
"pjass +e2          Undo Ignore of error level 2\n"
"pjass +s           Enable strict downcast evaluation\n"
"pjass -s           Disable strict downcast evaluation\n"
"pjass +rb          Enable returnbug\n"
"pjass -rb          Disable returnbug\n"
"pjass -            Read from standard input (may appear in a list)\n"
);
			exit(0);
		}
		if (strcmp(argv[i], "-v") == 0) {
			printf("%s version %s\n", argv[0], VERSIONSTR);
			exit(0);
		}
		if (strcmp(argv[i], "+s") == 0) {
			strict = 1;
			continue;
		}
		if (strcmp(argv[i], "-s") == 0) {
			strict = 0;
			continue;
		}
		if (strcmp(argv[i], "+rb") == 0) {
			returnbug = 1;
			continue;
		}
		if (strcmp(argv[i], "-rb") == 0) {
			returnbug = 0;
			continue;
		}

		FILE *fp;
		fp = fopen(argv[i], "rb");
		if (fp == NULL) {
			printf("Error: Cannot open %s\n", argv[i]);
			haderrors++;
			continue;
		}

		dofile(fp, argv[i]);
		didparse = 1;
		fclose(fp);
	}
    if (argc == 1) {
        didparse = 1;
        dofile(stdin, "<stdin>");
    }
}
int main(int argc, char **argv)
{
  init();
  doparse(argc, argv);

  if (!haderrors && didparse) {
		printf("Parse successful: %8d lines: %s\n", totlines, "<total>");
    if (ignorederrors)
	printf("%d errors ignored", ignorederrors);

    return 0;
  }
  else {
		if (haderrors)
			printf("Parse failed: %d error%s total\n", haderrors, haderrors == 1 ? "" : "s");
		else
			printf("Parse failed\n");
		if (ignorederrors)
		  printf("%d errors ignored", ignorederrors);
    return 1;
	}
}
