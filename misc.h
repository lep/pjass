// Jass2 parser for bison/yacc
// by Rudi Cilibrasi
// Sun Jun  8 00:51:53 CEST 2003
// thanks to Jeff Pang for the handy documentation that this was based
// on at http://jass.sourceforge.net
// Released under the BSD license

#ifndef MISC_H
#define MISC_H

#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "hashtable.h"
#include "tree.h"
#include "typeandname.h"
#include "paramlist.h"
#include "funcdecl.h"

#include "sstrhash.h"

#define BUFSIZE (16384)

#define MAX_IDENT_LENGTH (3958)

/*
For some reason flex produces the exact same #ifndef block in the .h and
the .c file except for these three defines which works out in the normal
build but not in the amalagamation.
We don't even need this ifdef as it builds w/o errors but this seems a
bit cleaner.
*/
#if defined(PJASS_AMALGATION)	  \
    && !defined(YY_BUFFER_NEW)	  \
    && !defined(YY_BUFFER_NORMAL) \
    && !defined(YY_BUFFER_EOF_PENDING)

#define YY_BUFFER_NEW 0
#define YY_BUFFER_NORMAL 1
#define YY_BUFFER_EOF_PENDING 2

#endif

union node {
  const char *str;
  int ival;
  const struct typenode *ty;
  struct paramlist *pl;
  struct funcdecl *fd;
  struct typeandname *tan;
};

enum errortype {
    syntaxerror = 0,
    semanticerror,
    runtimeerror,
    warning,
};

enum {
    flag_rb = 1 << 0,
    flag_filter = 1 << 1,
    flag_shadowing = 1 << 2,
    flag_syntaxerror = 1 << 3,
    flag_semanticerror = 1 << 4,
    flag_runtimeerror = 1 << 5,
    flag_checkglobalsinit = 1 << 6,
    flag_checkstringhash = 1 << 7,
    flag_nomodulo = 1 << 8,
    flag_verylongnames = 1 << 9,
    flag_checknumberliterals = 1 << 10,
};

enum {
    NullInGlobals = 1,
    CrashInGlobals,
};

void yyerrorline (enum errortype type, int line, const char *s);
void yyerrorex (enum errortype type, const char *s);
void yyerror (const char *s);

void getsuggestions(const char*, char*, size_t, int, ...);

const struct typenode *binop(const struct typenode *a, const struct typenode *b);
const struct typenode *combinetype(const struct typenode *n1, const struct typenode *n2);
const struct typeandname *getVariable(const char *varname);

void canconvert(const struct typenode *ufrom, const struct typenode *uto, const int linemod);
void canconvertreturn(const struct typenode *ufrom, const struct typenode *uto, const int linemod);

void validateGlobalAssignment(const char *varname);

void isnumeric(const struct typenode *ty);
void checkcomparison(const struct typenode *a, const struct typenode *b);
void checkcomparisonsimple(const struct typenode *a);
void checkeqtest(const struct typenode *a, const struct typenode *b);

int isflag(char *txt, struct hashtable *flags);
int updateflag(int cur, char *txt, struct hashtable *flags);
int updateannotation(int cur, char *txt, struct hashtable *flags);
bool flagenabled(int flag);

extern int pjass_flags;

extern int fno, lineno, totlines, islinebreak;
extern bool isconstant, inconstant, infunction, inblock;
extern int haderrors;
extern int ignorederrors;
extern int didparse;
extern int inloop;
extern int fnannotations;
extern int annotations;
extern bool afterendglobals;
extern bool inglobals;
extern bool encoutered_first_function;
extern int *showerrorlevel;
extern char *yytext;
extern const char *curfile;
extern int yydebug;
extern struct hashtable builtin_types, functions, globals, locals, params, types, initialized;
extern struct hashtable bad_natives_in_globals;
extern struct hashtable uninitialized_globals;
extern struct hashtable string_literals;
extern struct typenode *gInteger, *gReal, *gBoolean, *gString, *gCode, *gHandle, *gNothing, *gNull, *gAny, *gNone, *gEmpty;
extern struct typenode *gCodeReturnsNoBoolean, *gCodeReturnsBoolean;
extern struct funcdecl *fCurrent;
extern struct funcdecl *fFilter, *fCondition, *fStringHash;
extern const struct typenode *retval;

extern struct hashtable available_flags;
extern struct hashtable flags_helpstring;

extern struct hashtable shadowed_variables;

extern struct tree stringlit_hashes;

extern size_t stringlit_buffsize;
extern char stringlit_buff[2048];
extern size_t stringlit_length;

union node checkfunctionheader(const char *fnname, struct paramlist *pl, const struct typenode *retty);
union node checkfunccall(const char *fnname, struct paramlist *pl);
union node checkarraydecl(struct typeandname *tan);
union node checkvartypedecl(struct typeandname *tan);
void checkwrongshadowing(const struct typeandname *tan, int linemod);
void checkmodulo(const struct typenode *a, const struct typenode *b);
void checkarrayindex(const char *name, const struct typenode *ty, int lineno);

void str_append(char *buf, const char *str, size_t buf_size);

void checkidlength(char *name);
void check_name_allready_defined(struct hashtable *ht, const char *name, const char *msg);

void checkreallit(char *lit);
void checkintlit(char *lit);

#endif
