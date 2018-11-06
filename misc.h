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

#include "hashtable.h"
#include "typeandname.h"
#include "paramlist.h"
#include "funcdecl.h"

#define BUFSIZE (16384)

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
};

enum {
    NullInGlobals = 1,
    CrashInGlobals,
};

void yyerrorline (enum errortype type, int line, const char *s);
void yyerrorex (enum errortype type, const char *s);
void yyerror (const char *s);

void put(struct hashtable *h, const char *name, void *val);

void getsuggestions(const char*, char*, size_t, int, ...);

const struct typenode *binop(const struct typenode *a, const struct typenode *b);
const struct typenode *combinetype(const struct typenode *n1, const struct typenode *n2);
void checkParameters(const struct funcdecl *fd, const struct paramlist *inp, bool mustretbool);
const struct typeandname *getVariable(const char *varname);

bool canconvertbuf(char *buf, size_t buflen, const struct typenode *ufrom, const struct typenode *uto);
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
extern int *showerrorlevel;
extern char *yytext;
extern const char *curfile;
extern int yydebug;
int *showerrorlevel;
extern struct hashtable functions, globals, locals, params, types, initialized, *curtab;
extern struct hashtable bad_natives_in_globals;
extern struct hashtable uninitialized_globals;
extern struct typenode *gInteger, *gReal, *gBoolean, *gString, *gCode, *gHandle, *gNothing, *gNull, *gAny, *gNone, *gEmpty;
extern struct typenode *gCodeReturnsNoBoolean, *gCodeReturnsBoolean;
extern struct funcdecl *fCurrent;
extern struct funcdecl *fFilter, *fCondition;
extern const struct typenode *retval;

extern struct hashtable available_flags;

extern struct hashtable shadowed_variables;

union node checkfunctionheader(const char *fnname, struct paramlist *pl, const struct typenode *retty);
union node checkfunccall(const char *fnname, struct paramlist *pl);
union node checkarraydecl(struct typeandname *tan);
union node checkvartypedecl(struct typeandname *tan);
void checkwrongshadowing(const struct typeandname *tan, int linemod);

void str_append(char *buf, const char *str, size_t buf_size);


#endif
