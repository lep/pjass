// Jass2 parser for bison/yacc
// by Rudi Cilibrasi
// Sun Jun  8 00:51:53 CEST 2003
// thanks to Jeff Pang for the handy documentation that this was based
// on at http://jass.sourceforge.net
// Released under the BSD license
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>

#define BUFSIZE (16384)

struct typenode {
  char *typename;
  const struct typenode *superclass;
};

struct typeandname {
  const struct typenode *ty;
  const char *name;
  int isarray, isconst, lineno, fn;
  struct typeandname *next;
};

struct paramlist {
  struct typeandname *head;
  struct typeandname **tail;
};

struct funcdecl {
  char *name;
  int isconst;
  struct paramlist *p;
  const struct typenode *ret;
};

union node {
  char *str;
  int ival;
  const struct typenode *ty;
  struct paramlist *pl;
  struct funcdecl *fd;
  struct typeandname *tan;
};

struct hashnode {
  const char *name;
  void *val;
};

struct hashtable {
  size_t size;
  size_t count;
  struct hashnode *bucket;
};

void yyerrorline (int errorlevel, int line, char *s);
void yyerrorex (int errorlevel, char *s);
void yyerror (char *s);

void getsuggestions(const char*, char*, int, int, ...);
void *lookup(struct hashtable *h, const char *name);
void put(struct hashtable *h, const char *name, void *val);
void clear(struct hashtable *h);
void init();

struct typenode *newtypenode(const char *typename, const struct typenode *superclass);
struct paramlist *newparamlist();
struct typeandname *newtypeandname(const struct typenode *ty, const char *name);
struct typeandname *newtypeandnamewithreturn(const struct typenode *ty, const char *name, int retbool);
const struct typenode *getPrimitiveAncestor(const struct typenode *cur);
int isDerivedFrom(const struct typenode *cur, const struct typenode *base);
void addParam(struct paramlist *tl, struct typeandname *tan);
struct funcdecl *newfuncdecl();
void showfuncdecl(struct funcdecl *fd);
const struct typenode *binop(const struct typenode *a, const struct typenode *b);
int canconvert(const struct typenode *from, const struct typenode *to, const int linemod);
void canconvertreturn(const struct typenode *from, const struct typenode *to, const int linemod);
struct typenode* mkretty(const struct typenode *ty, int ret);
struct typenode* getTypePtr(const struct typenode *ty);
int getTypeTag(const struct typenode *ty);
int typeeq(const struct typenode*, const struct typenode*);
const struct typenode *combinetype(const struct typenode *n1, const struct typenode *n2);
void checkParameters(const struct paramlist *func, const struct paramlist *inp);
void validateGlobalAssignment(const char *varname);
void checkcomparisonsimple(const struct typenode *a);
	
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
extern char *yytext;
extern const char *curfile;
extern int yydebug;
int *showerrorlevel;
extern struct hashtable functions, globals, locals, params, types, initialized, *curtab;
extern struct typenode *gInteger, *gReal, *gBoolean, *gString, *gCode, *gHandle, *gNothing, *gNull, *gAny, *gNone, *gEmpty;
extern struct funcdecl *fCurrent;
extern const struct typenode *retval;
const struct typeandname *getVariable(const char *varname);
void isnumeric(const struct typenode *ty);
void checkcomparison(const struct typenode *a, const struct typenode *b);
void checkeqtest(const struct typenode *a, const struct typenode *b);
void init();
void doparse(int argc, char **argv);
