#include "token.yy.h"
#include "grammar.tab.h"

#include "misc.h"

#ifndef VERSIONSTR
#define VERSIONSTR "1.0-git"
#endif

static struct typenode* addPrimitiveType(const char *name)
{
    struct typenode *newty= newtypenode(name, NULL);
    put(&types, name, newty);
    return newty;
}


static void init()
{
    ht_init(&functions, 10009);
    ht_init(&globals, 8191);
    ht_init(&locals, 27);
    ht_init(&params, 23);
    ht_init(&types, 149);
    ht_init(&initialized, 2047);

    ht_init(&bad_natives_in_globals, 17);
    ht_init(&shadowed_variables, 17);
    
    ht_init(&uninitialized_globals, 2047);

    gHandle = addPrimitiveType("handle");
    gInteger = addPrimitiveType("integer");
    gReal = addPrimitiveType("real");
    gBoolean = addPrimitiveType("boolean");
    gString = addPrimitiveType("string");
    gCode = addPrimitiveType("code");

    gCodeReturnsBoolean = newtypenode("codereturnsboolean", gCode);
    gCodeReturnsNoBoolean = newtypenode("codereturnsboolean", gCode);

    gNothing = newtypenode("nothing", NULL);
    gNull = newtypenode("null", NULL);

    gAny = newtypenode("any", NULL);
    gNone = newtypenode("none", NULL);
    gEmpty = newtypenode("empty", NULL);

    curtab = &globals;

    pjass_flags = 0;

    fno = 0;
    fnannotations = 0;
    annotations = 0;
    haderrors = 0;
    ignorederrors = 0;
    islinebreak = 1;
    inblock = false;
    isconstant = false;
    inconstant = false;
    infunction = false;

    fCurrent = NULL;
    fFilter = NULL;
    fCondition = NULL;

    ht_init(&available_flags, 11);
    ht_put(&available_flags, "rb", (void*)flag_rb);
    ht_put(&available_flags, "shadow", (void*)flag_shadowing);
    ht_put(&available_flags, "filter", (void*)flag_filter);
    ht_put(&available_flags, "nosyntaxerror", (void*)flag_syntaxerror);
    ht_put(&available_flags, "nosemanticerror", (void*)flag_semanticerror);
    ht_put(&available_flags, "noruntimeerror", (void*)flag_runtimeerror);
    ht_put(&available_flags, "checkglobalsinit", (void*)flag_checkglobalsinit);

    ht_put(&bad_natives_in_globals, "OrderId", (void*)NullInGlobals);
    ht_put(&bad_natives_in_globals, "OrderId2String", (void*)NullInGlobals);
    ht_put(&bad_natives_in_globals, "UnitId2String", (void*)NullInGlobals);

    ht_put(&bad_natives_in_globals, "GetObjectName", (void*)CrashInGlobals);
    ht_put(&bad_natives_in_globals, "CreateQuest", (void*)CrashInGlobals);
    ht_put(&bad_natives_in_globals, "CreateMultiboard", (void*)CrashInGlobals);
    ht_put(&bad_natives_in_globals, "CreateLeaderboard", (void*)CrashInGlobals);
}

static void dofile(FILE *fp, const char *name)
{
    lineno = 1;
    islinebreak = 1;
    isconstant = false;
    inconstant = false;
    inblock = false;
    afterendglobals = false;
    inglobals = false;
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

static void printversion()
{
	printf("Pjass version %s by Rudi Cilibrasi, modified by AIAndy, PitzerMike, Deaod and lep\n", VERSIONSTR);
}

static void doparse(int argc, char **argv)
{
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
"pjass -h               Display this help\n"
"pjass -v               Display version information and exit\n"
"pjass +rb              Enable returnbug\n"
"pjass -rb              Disable returnbug\n"
"pjass +shadow          Enable error on variable shadowing\n"
"pjass -shadow          Disable error on variable shadowing\n"
"pjass +filter          Enable error on inappropriate code usage for Filter\n"
"pjass -filter          Disable error on inappropriate code usage for Filter\n"
"pjass +nosyntaxerror   Disable all syntax errors\n"
"pjass -nosyntaxerror   Enable syntax error reporting\n"
"pjass +nosemanticerror Disable all semantic errors\n"
"pjass -nosemanticerror Enable semantic error reporting\n"
"pjass +noruntimeerror  Disable all runtime errors\n"
"pjass -noruntimeerror  Enable runtime error reporting\n"
"pjass -                Read from standard input (may appear in a list)\n"
);
			exit(0);
		}
		if (strcmp(argv[i], "-v") == 0) {
			printf("%s version %s\n", argv[0], VERSIONSTR);
			exit(0);
		}
        if( isflag(argv[i], &available_flags)){
            pjass_flags = updateflag(pjass_flags, argv[i], &available_flags);
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
        if (ignorederrors) {
            printf("%d errors ignored\n", ignorederrors);
        }
        return 0;
    } else {
        if (haderrors) {
            printf("Parse failed: %d error%s total\n", haderrors, haderrors == 1 ? "" : "s");
        } else {
            printf("Parse failed\n");
        }
        if (ignorederrors) {
            printf("%d errors ignored\n", ignorederrors);
        }
        return 1;
    }
}
