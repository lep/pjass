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
    ht_init(&functions, 1 << 13);
    ht_init(&globals, 1 << 13);
    ht_init(&locals, 1 << 6);
    ht_init(&params, 1 << 5);
    ht_init(&types, 1 << 7);
    ht_init(&initialized, 1 << 11);

    ht_init(&bad_natives_in_globals, 1 << 4);
    ht_init(&shadowed_variables, 1 << 4);

    ht_init(&uninitialized_globals, 1 << 11);

    ht_init(&string_literals, 1 << 10);

    tree_init(&stringlit_hashes);

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
    fStringHash = NULL;

    ht_init(&available_flags, 16);
    ht_put(&available_flags, "rb", (void*)flag_rb);
    ht_put(&available_flags, "shadow", (void*)flag_shadowing);
    ht_put(&available_flags, "filter", (void*)flag_filter);
    ht_put(&available_flags, "nosyntaxerror", (void*)flag_syntaxerror);
    ht_put(&available_flags, "nosemanticerror", (void*)flag_semanticerror);
    ht_put(&available_flags, "noruntimeerror", (void*)flag_runtimeerror);
    ht_put(&available_flags, "checkglobalsinit", (void*)flag_checkglobalsinit);
    ht_put(&available_flags, "checkstringhash", (void*)flag_checkstringhash);

    ht_init(&flags_helpstring, 16);
    ht_put(&flags_helpstring, "rb", "Toggle returnbug checking");
    ht_put(&flags_helpstring, "shadow", "Toggle error on variable shadowing");
    ht_put(&flags_helpstring, "filter", "Toggle error on inappropriate code usage for Filter");
    ht_put(&flags_helpstring, "nosyntaxerror", "Toggle syntax error reporting");
    ht_put(&flags_helpstring, "nosemanticerror", "Toggle semantic error reporting");
    ht_put(&flags_helpstring, "noruntimeerror", "Toggle runtime error reporting");
    ht_put(&flags_helpstring, "checkglobalsinit", "Toggle a very bad checker for uninitialized globals usage");
    ht_put(&flags_helpstring, "checkstringhash", "Toggle StringHash collision checking");



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
    encoutered_first_function = false;
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

static void printhelp()
{
    printversion();
    printf("\n");
    printf(
        "To use this program, list the files you would like to parse in order\n"
        "If you would like to parse from standard input (the keyboard), then\n"
        "use - as an argument.  If you supply no arguments to pjass, it will\n"
        "parse the console standard input by default.\n"
        "The most common usage of pjass would be like this:\n"
        "pjass common.j Blizzard.j war3map.j\n"
        "\n"
        "pjass accepts some flags:\n"
        "%-20s print this help message and exit\n"
        "%-20s print pjass version and exit\n"
        "\n"
        "But pjass also allows to toggle some flags with either + or - in front of them.\n"
        "Once a flag is activated it will stay on until disabled and then it will stay disabled\n"
        "until potentially turned on again, and so on, and so forth.\n"
        "Usage could look like this:\n"
        "\n"
        "\tpjass +shadow file1.j +rb file2.j -rb file3.j\n"
        "\n"
        "Which would check all three files with shadow enabled and only file2 with rb enabled.\n"
        "Below you can see a list of all available options. They are all off by default.\n"
        "\n"
        , "-h", "-v"
    );

    int i;
    for(i = 0; i < available_flags.size; i++){
        struct hashnode hn = available_flags.bucket[i];
        if( hn.name ) {
            const char *helpstr = ht_lookup(&flags_helpstring, hn.name);
            if( helpstr ){
                printf("%-20s %s\n", hn.name, helpstr);
            }else{
                printf("%-20s\n", hn.name);
            }
        }
    }
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
            printhelp();
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
