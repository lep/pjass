// Jass2 parser for bison/yacc
// by Rudi Cilibrasi
// Sun Jun  8 00:51:53 CEST 2003
// thanks to Jeff Pang for the handy documentation that this was based
// on at http://jass.sourceforge.net
// Released under the BSD license
#include <stdio.h>
#define YYDEBUG 1

#define BUFSIZE 8192


struct typenode {
  char *typename;
  const struct typenode *superclass;
};

struct typeandname {
  const struct typenode *ty;
  const char *name;
  int isarray, isconst;
  struct typeandname *next;
};

struct paramlist {
  struct typeandname *head;
  struct typeandname **tail;
};

struct funcdecl {
  char *name;
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

void *lookup(struct hashtable *h, const char *name);
void put(struct hashtable *h, const char *name, void *val);
void clear(struct hashtable *h);
void init();

struct typenode *newtypenode(const char *typename, const struct typenode *superclass);
struct paramlist *newparamlist();
struct typeandname *newtypeandname(const struct typenode *ty, const char *name);
const struct typenode *getPrimitiveAncestor(const struct typenode *cur);
int isDerivedFrom(const struct typenode *cur, const struct typenode *base);
void addParam(struct paramlist *tl, struct typeandname *tan);
struct funcdecl *newfuncdecl();
void showfuncdecl(struct funcdecl *fd);
struct typenode *binop(const struct typenode *a, const struct typenode *b);
int canconvert(const struct typenode *from, const struct typenode *to);
void checkParameters(const struct paramlist *func, const struct paramlist *inp);

extern int lineno, totlines;
extern int haderrors;
extern int didparse;
extern char *yytext, *curfile;
extern int yydebug;
extern struct hashtable functions, globals, locals, params, types, *curtab;
extern struct typenode *gInteger, *gReal, *gBoolean, *gString, *gCode, *gHandle, *gNothing, *gNull;
extern struct typenode *retval;
const struct typeandname *getVariable(const char *varname);
void isnumeric(const struct typenode *ty);
void checkcomparison(const struct typenode *a, const struct typenode *b);
void checkeqtest(const struct typenode *a, const struct typenode *b);
void init(int argc, char **argv);
void doparse(int argc, char **argv);
