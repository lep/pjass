// Jass2 parser for bison/yacc
// by Rudi Cilibrasi
// Sun Jun  8 00:51:53 CEST 2003
// thanks to Jeff Pang for the handy documentation that this was based
// on at http://jass.sourceforge.net
// Released under the BSD license
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#define YYDEBUG 1

#define BUFSIZE 8192
#define MAYBE 0
#define NO 1
#define YES 2

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
  struct typenode *ret;
};

union node {
  char *str;
  int ival;
  struct typenode *ty;
  struct paramlist *pl;
  struct funcdecl *fd;
  struct typeandname *tan;
};

#define BUCKETS 6841

struct hashnode {
  char *name;
  void *val;
  struct hashnode *next;
};

struct hashtable {
  struct hashnode *h[BUCKETS];
};

//void getsuggestions(const char*, char*, int, ...);
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
struct typenode *binop(const struct typenode *a, const struct typenode *b);
int canconvert(const struct typenode *from, const struct typenode *to, const int linemod);
int canconvertreturn(const struct typenode *from, const struct typenode *to, const int linemod);
struct typenode* mkretty(struct typenode *ty, int ret);
struct typenode* getTypePtr(struct typenode *ty);
getTypeTag(struct typenode *ty);
int typeeq(struct typenode*, struct typenode*);
struct typenode *combinetype(struct typenode *n1, struct typenode *n2);
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
extern int afterendglobals;
extern char *yytext;
extern const char *curfile;
extern int yydebug;
int *showerrorlevel;
extern struct hashtable functions, globals, locals, params, types, initialized, *curtab;
extern struct typenode *gInteger, *gReal, *gBoolean, *gString, *gCode, *gHandle, *gNothing, *gNull, *gAny, *gNone, *gEmpty;
extern struct funcdecl *fCurrent;
extern struct typenode *retval;
const struct typeandname *getVariable(const char *varname);
void isnumeric(const struct typenode *ty);
void checkcomparison(const struct typenode *a, const struct typenode *b);
void checkeqtest(const struct typenode *a, const struct typenode *b);
void init(int argc, char **argv);
void doparse(int argc, char **argv);
