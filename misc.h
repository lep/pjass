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
  char *str;
  int ival;
  const struct typenode *ty;
  struct paramlist *pl;
  struct funcdecl *fd;
  struct typeandname *tan;
};

enum errortype {
    syntaxerror = 0,
    semanticerror,
    warning,
};


void yyerrorline (int errorlevel, int line, const char *s);
void yyerrorex (int errorlevel, const char *s);
void yyerror (const char *s);

void put(struct hashtable *h, const char *name, void *val);

void getsuggestions(const char*, char*, size_t, int, ...);

const struct typenode *binop(const struct typenode *a, const struct typenode *b);
const struct typenode *combinetype(const struct typenode *n1, const struct typenode *n2);
void checkParameters(const struct paramlist *func, const struct paramlist *inp);
const struct typeandname *getVariable(const char *varname);
int canconvert(const struct typenode *ufrom, const struct typenode *uto, const int linemod);
void canconvertreturn(const struct typenode *ufrom, const struct typenode *uto, const int linemod);

void validateGlobalAssignment(const char *varname);

void isnumeric(const struct typenode *ty);
void checkcomparison(const struct typenode *a, const struct typenode *b);
void checkcomparisonsimple(const struct typenode *a);
void checkeqtest(const struct typenode *a, const struct typenode *b);

extern int fno, lineno, totlines, islinebreak, isconstant, inblock, inconstant, infunction;
extern int haderrors;
extern int ignorederrors;
extern int didparse;
extern int inloop;
extern int strict;
extern int returnbug;
extern int fnhasrbannotation;
extern int rbannotated;
extern int afterendglobals;
extern int *showerrorlevel;
extern char *yytext;
extern const char *curfile;
extern int yydebug;
int *showerrorlevel;
extern struct hashtable functions, globals, locals, params, types, initialized, *curtab;
extern struct typenode *gInteger, *gReal, *gBoolean, *gString, *gCode, *gHandle, *gNothing, *gNull, *gAny, *gNone, *gEmpty;
extern struct funcdecl *fCurrent;
extern const struct typenode *retval;

#endif
