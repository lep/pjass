// Jass2 parser for bison/yacc
// by Rudi Cilibrasi
// Sun Jun  8 00:51:53 CEST 2003
// thanks to Jeff Pang for the handy documentation that this was based
// on at http://jass.sourceforge.net
// Released under the BSD license
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>
#include "grammar.tab.h"
#include "misc.h"

#ifndef VERSIONSTR
#define VERSIONSTR "1.0-git"
#endif
#define ERRORLEVELNUM 4

int fno;
int lineno;
int haderrors;
int ignorederrors;
int totlines;
int islinebreak;
int isconstant;
int inconstant;
int infunction;
int inblock;
int strict;
int returnbug;
int didparse;
int inloop;
int afterendglobals;
int *showerrorlevel;

int hashfunc(const char *name);
struct hashtable functions, globals, locals, params, types, initialized;
struct hashtable *curtab;
struct typenode *retval, *retcheck;
const char *curfile;
struct typenode *gInteger, *gReal, *gBoolean, *gString, *gCode, *gHandle, *gNothing, *gNull, *gAny, *gNone, *gEmpty;
struct typenode *gEmpty;
struct funcdecl *fCurrent;

void addPrimitiveType(const char *name, struct typenode **toSave)
{
  put(&types, name, *toSave = newtypenode(name, NULL));
}

void init(int argc, char **argv)
{
  int i;
  addPrimitiveType("handle", &gHandle);
  addPrimitiveType("integer", &gInteger);
  addPrimitiveType("real", &gReal);
  addPrimitiveType("boolean", &gBoolean);
  addPrimitiveType("string", &gString);
  addPrimitiveType("code", &gCode);
  gNothing = newtypenode("nothing", NULL);
  gNull = newtypenode("null", NULL);
  gAny = newtypenode("any", NULL);
  gNone = newtypenode("none", NULL);
  gEmpty = newtypenode("gempty", NULL);
  curtab = &globals;
  fno = 0;
  strict = 0;
  returnbug = 0;
  haderrors = 0;
  ignorederrors = 0;
  islinebreak = 1;
  inblock = 0;
  isconstant = 0;
  inconstant = 0;
  infunction = 0;
  fCurrent = 0;
  showerrorlevel = malloc(ERRORLEVELNUM*sizeof(int));
  for(i=0;i<ERRORLEVELNUM;i++)
    showerrorlevel[i] = 1;
  if (lookup(&functions, "ConvertRace") != NULL) {
     printf("Major error!!\n");
     exit(1);
  }
}

int min(int a, int b){
    if(a < b) return a;
    else return b;
}

int abs(int i){
    if(i < 0)
        return -i;
    return i;
}

int editdistance(const char *s, const char *t, int cutoff){
    if(!strcmp(s, t)) return 0;

    int a = strlen(s);
    int b = strlen(t);

    if(a==0) return b;
    if(b==0) return a;
    
    if(abs(a-b) > cutoff){
        return cutoff + 1;
    }

    int *v[3];
    int i;
    for(i = 0; i != 3; i++)
        v[i] = malloc(sizeof(int) * (b+1));

    for(i = 0; i != b+1; i++){
        v[0][i] = i;
    }

    int pcur;
    int ppcur;
    int cur = 1;
    for(i = 0; i != a; i++, cur = (cur+1) % 3){
        pcur = cur -1;
        if(pcur < 0) pcur += 3;
        ppcur = pcur -1;
        if(ppcur < 0) ppcur += 3;

        v[cur][0] = i + 1;
        
        int minDistance = INT_MAX;
        
        int j;
        for(j = 0; j != b; j++){
            int cost = (s[i] == t[j]) ? 0 : 1;

            v[cur][j+1] = min(v[cur][j] + 1, min(v[pcur][j+1] + 1, v[pcur][j] + cost));

            if(i > 0 && j > 0 && s[i] == t[j-1] && s[i-1] == t[j]){
                v[cur][j+1] = min(v[cur][j+1], v[ppcur][j-1] + cost);
            }
            
            if(v[cur][j+1] < minDistance){
                minDistance = v[cur][j+1];
            }
        }
        
        if(minDistance > cutoff){
            return cutoff + 1;
        }
    }
    pcur = cur -1;
    if(pcur < 0) pcur += 3;
    int d = v[pcur][b];
    for(i = 0; i != 3; i++)
        free(v[i]);
    return d;
}

void getsuggestions(const char *name, char *buff, int nTables, ...){
    int i;
    va_list ap;

    int len = strlen(name);
    int cutoff = (int)((len+2)/4.0);
    int count = 0;

    struct {int distance; char *name;} suggestions[3];
    for(i = 0; i != 3; i++){
        suggestions[i].distance = INT_MAX;
        suggestions[i].name = NULL;
    }

    va_start(ap, nTables);
    for(i = 0; i != nTables; i++){
        struct hashtable *ht = va_arg(ap, struct hashtable*);
        int x;
        for(x = 0; x != BUCKETS; x++){
            struct hashnode *hn;
            hn = ht->h[x];
            while (hn) {
                int dist = editdistance(hn->name, name, cutoff);
                if(dist <= cutoff){
                    count++;
                    int j;
                    for(j = 0; j != 3; j++){
                        if(suggestions[j].distance > dist){
                            if(i == 0){
                                suggestions[2] = suggestions[1];
                                suggestions[1] = suggestions[0];
                            }else if(i == 1){
                                suggestions[2] = suggestions[1];
                            }
                            suggestions[j].distance = dist;
                            suggestions[j].name = hn->name;

                            break;
                        }
                    }
                }
                hn = hn->next;
            }
        }
    }
    va_end(ap);

    if(count==0)
        return;
    else if(count == 1){
        char hbuff[1024];
        sprintf(hbuff, ". Maybe you meant %s", suggestions[0].name);
        strcat(buff, hbuff);
    }else{
        strcat(buff, ". Maybe you meant ");
        for(i=0; suggestions[i].name; i++){
            strcat(buff, suggestions[i].name);
            if(i!=2 && suggestions[i+1].name)
                strcat(buff, ", ");
        }
    }
}

const struct typeandname *getVariable(const char *varname)
{
  char ebuf[1024];
  struct typeandname *result;
  result = lookup(&locals, varname);
  if (result) return result;
  result = lookup(&params, varname);
  if (result) return result;
  result = lookup(&globals, varname);
  if (result) return result;
  sprintf(ebuf, "Undeclared variable %s", varname);
  getsuggestions(varname, ebuf, 3, &locals, &params, &globals);
  yyerrorline(2, islinebreak ? lineno - 1 : lineno, ebuf);
  // Store it as unidentified variable
  put(curtab, varname, newtypeandname(gAny, varname));
  if(infunction && lookup(curtab, varname) && !lookup(&initialized, varname)){
    put(&initialized, varname, (void*)1);
  }
  return getVariable(varname);
}

void validateGlobalAssignment(const char *varname) {
  char ebuf[1024];
  struct typeandname *result;
  result = lookup(&globals, varname);
  if (result) {
    sprintf(ebuf, "Assignment to global variable %s in constant function", varname);
    yyerrorline(2, lineno - 1, ebuf);
  }
}

struct typeandname *newtypeandname(const struct typenode *ty, const char *name)
{
  struct typeandname *tan = calloc(sizeof(struct typeandname), 1);
  tan->ty = ty;
  tan->name = strdup(name);
  tan->next = NULL;
  return tan;
}

struct typenode *newtypenode(const char *typename, const struct typenode *superclass)
{
  struct typenode *result;
#if defined __CYGWIN__
    result = memalign(8, sizeof(struct typenode));
#else
    result = _aligned_malloc(8, sizeof(struct typenode));
#endif
  result->typename = strdup(typename);
  result->superclass = superclass;
  return result;
}

struct paramlist *newparamlist()
{
  struct paramlist *tl = calloc(sizeof(struct paramlist), 1);
  tl->head = NULL;
  tl->tail = &tl->head;
  return tl;
}

void addParam(struct paramlist *pl, struct typeandname *tan)
{
  tan->next = *(pl->tail);
  *(pl->tail) = tan;
}

struct funcdecl *newfuncdecl()
{
  struct funcdecl *fd = calloc(sizeof(struct funcdecl), 1);
  fd->name = NULL;
  fd->p = NULL;
  fd->ret = NULL;
  return fd;
}

const struct typenode *getPrimitiveAncestor(const struct typenode *cur)
{
  while (cur->superclass)
    cur = cur->superclass;
  return cur;
}

int isDerivedFrom(const struct typenode *cur, const struct typenode *base)
{
  do {
    if (typeeq(cur, base)) return 1;
    cur = cur->superclass;
  } while (cur);
  return 0;
}

void showtypenode(const struct typenode *td)
{
  const char *tp = NULL;
  const char *tn = "";
  char *extends = "";
  char ebuf[1024];
  assert(td);
  assert(td->typename);
  /*
  if (td->superclass) {
    sprintf(ebuf, " extends %s", td->superclass->typename);
    extends = ebuf;
  }
  */
  printf("%s  %s \n", td->typename, extends);
}

void showfuncdecl(struct funcdecl *fd)
{
  struct typeandname *tan;
  printf("%s takes ", fd->name);
  if (fd->p->head == NULL)
    printf("nothing ");
  for (tan = fd->p->head; tan; tan = tan->next) {
    showtypenode(tan->ty);
    printf(" %s", tan->name);
    if (tan->next)
      printf(",");
    printf(" ");
  }
  printf("returns ");
  showtypenode(fd->ret);
  printf("\n");
}


int hashfunc(const char *name)
{
  int h = 0;
  const unsigned char *s;
  for (s = name; *s; ++s)
    h = ((811 * h + (*s)) % 19205861);
  return ((h % BUCKETS) + BUCKETS) % BUCKETS;
}

void *lookup(struct hashtable *h, const char *name)
{
  struct hashnode *hn;
  int hf = hashfunc(name);
  hn = h->h[hf];
  while (hn) {
    if (strcmp(hn->name, name) == 0)
      return hn->val;
    hn = hn->next;
  }
  return NULL;
}

void put(struct hashtable *h, const char *name, void *val)
{
  struct hashnode *hn;
  int hf;
  
  if (lookup(h, name) != NULL) {
    char ebuf[1024];
    sprintf(ebuf, "Symbol %s multiply defined", name);
    yyerrorline(3, islinebreak ? lineno - 1 : lineno, ebuf);
    return;
  }
  hf = hashfunc(name);
  hn = calloc(sizeof(struct hashnode), 1);
  hn->name = strdup(name);
  hn->val = val;
  hn->next = h->h[hf];
  h->h[hf] = hn;
}

void clear(struct hashtable *h)
{
  int i;
  struct hashnode *hn;
  for (i = 0; i < BUCKETS; ++i) {
    hn = h->h[i];
    while (hn) {
      struct hashnode *tofree = hn;
      hn = hn->next;
      free(tofree->name);
      free(tofree);
    }
    h->h[i] = NULL;
  }
}

struct typenode *binop(const struct typenode *a, const struct typenode *b)
{
  a = getPrimitiveAncestor(a);
  b = getPrimitiveAncestor(b);
  if (typeeq(a, gInteger) && typeeq(b, gInteger))
    return gInteger;
  if (typeeq(a, gAny))
    return b;
  if (typeeq(b, gAny))
    return a;
  if ((!typeeq(a, gInteger) && !typeeq(a, gReal)) || (!typeeq(b, gInteger) && !typeeq(b, gReal))) {
    yyerrorline(3, islinebreak ? lineno - 1 : lineno, "Bad types for binary operator");
  }
  return gReal;
}

// this is used for reducing expressions in many places (if/exitwhen conditions, assignments etc.)
int canconvert(const struct typenode *ufrom, const struct typenode *uto, const int linemod)
{
  const struct typenode *from = ufrom, *to = uto;
  char ebuf[1024];
  if (from == NULL || to == NULL)
    return 0;
  if (typeeq(from, gAny) || typeeq(to, gAny))
    return 1;
  if (isDerivedFrom(from, to))
    return 1;
  //if (isDerivedFrom(to, from))
  //  return 1; // blizzard bug allows downcasting erroneously, we don't support this though
  if (from->typename == NULL || to->typename == NULL)
    return 0;
  if (typeeq(from, gNone) || typeeq(to, gNone))
    return 0;
  from = getPrimitiveAncestor(from);
  to = getPrimitiveAncestor(to);
  if (typeeq(from, gNull) && !typeeq(to, gInteger) && !typeeq(to, gReal) && !typeeq(to, gBoolean))
    return 1;
  if (strict) {
    if (typeeq(ufrom, gInteger) && (typeeq(to, gReal) || typeeq(to, gInteger)))
      return 1;
    if (typeeq(ufrom, to) && (typeeq(ufrom, gBoolean) || typeeq(ufrom, gString) || typeeq(ufrom, gReal) || typeeq(ufrom, gInteger) || typeeq(ufrom, gCode)))
      return 1;
  } else {
    if (typeeq(from, gInteger) && (typeeq(to, gReal) || typeeq(to, gInteger)))
      return 1;
    if (typeeq(from, to) && (typeeq(from, gBoolean) || typeeq(from, gString) || typeeq(from, gReal) || typeeq(from, gInteger) || typeeq(from, gCode)))
      return 1;
  }

  sprintf(ebuf, "Cannot convert %s to %s", ufrom->typename, uto->typename);
  yyerrorline(3, lineno + linemod, ebuf);
  return 0;
}

// this is used for return statements only
int canconvertreturn(const struct typenode *ufrom, const struct typenode *uto, const int linemod)
{
  const struct typenode *from = ufrom, *to = uto;
  char ebuf[1024];
  if (from == NULL || to == NULL)
	  return 0; // garbage
  if (typeeq(from, gAny) || typeeq(to, gAny))
	  return 1; // we don't care
  if (isDerivedFrom(from, to))
    return 1; // eg. from = unit, to = handle
  //if (isDerivedFrom(to, from))
  //  return 1; // blizzard bug allows downcasting erroneously, we don't support this though
  if (from->typename == NULL || to->typename == NULL)
	  return 0; // garbage
  if (typeeq(from, gNone) || typeeq(to, gNone))
	  return 0; // garbage

  from = getPrimitiveAncestor(from);
  to = getPrimitiveAncestor(to);
  if ((typeeq(to, gReal)) && (typeeq(from, gInteger))) {
	// can't return integer when it expects a real (added 9.5.2005)
    sprintf(ebuf, "Cannot convert returned value from %s to %s", from->typename, to->typename);
    yyerrorline(1, lineno + linemod, ebuf);
    return 0;
  }
  if ((typeeq(from, gNull)) && (!typeeq(to, gInteger)) && (!typeeq(to, gReal)) && (!typeeq(to, gBoolean)))
    return 1; // can't return null when it expects integer, real or boolean (added 9.5.2005)
  
  if (strict) {
    if (isDerivedFrom(uto, ufrom))
      return 1;
  } else if (typeeq(from, to))
    return 1;
    
  sprintf(ebuf, "Cannot convert returned value from %s to %s", ufrom->typename, uto->typename);
  yyerrorline(1, lineno + linemod, ebuf);
  return 0;
}

struct typenode* mkretty(struct typenode *ty, int ret){
    uintptr_t tagMask = (8-1);
    uintptr_t pointerMask = ~tagMask;
    uintptr_t ptr = (uintptr_t)ty;
    ret = ret & tagMask;
    return (struct typenode*)((ptr & pointerMask) | ret);
}

int getTypeTag(struct typenode *ty){
    uintptr_t tagMask = (8-1);
    uintptr_t ptr = (uintptr_t)ty;
    return (int)(ptr & tagMask);
}

struct typenode* getTypePtr(struct typenode *ty){
    uintptr_t tagMask = (8-1);
    uintptr_t pointerMask = ~tagMask;
    uintptr_t ptr = (uintptr_t)ty;
    return (struct typenode*)(ptr & pointerMask);
}

int typeeq(struct typenode *a, struct typenode *b){
    return getTypePtr(a) == getTypePtr(b);
}

struct typenode *combinetype(struct typenode *n1, struct typenode *n2) {
  int ret = getTypeTag(n1) & getTypeTag(n2);
  if ((typeeq(n1, gNone)) || (typeeq(n2, gNone))) return mkretty(gNone, ret);
  if (typeeq(n1, n2)) return mkretty(n1, ret);
  if (typeeq(n1, gNull))
    return mkretty(n2, ret);
  if (typeeq(n2, gNull))
    return mkretty(n1, ret);
  n1 = getPrimitiveAncestor(n1);
  n2 = getPrimitiveAncestor(n2);
  if (typeeq(n1, n2)) return mkretty(n1, ret);
  if (typeeq(n1, gNull))
    return mkretty(n2, ret);
  if (typeeq(n2, gNull))
    return mkretty(n1, ret);
  if ((typeeq(n1, gInteger)) && (typeeq(n2, gReal)))
    return mkretty(gReal, ret);
  if ((typeeq(n1, gReal)) && (typeeq(n2, gInteger)))
    return mkretty(gInteger, ret);
  // printf("Cannot convert %s to %s", n1->typename, n2->typename);
  return mkretty(gNone, ret);
}

void checkParameters(const struct paramlist *func, const struct paramlist *inp)
{
  const struct typeandname *fi = func->head;
  const struct typeandname *pi = inp->head;
  for (;;) {
    if (fi == NULL && pi == NULL)
      return;
    if (fi == NULL && pi != NULL) {
      yyerrorex(3, "Too many arguments passed to function");
      return;
    }
    if (fi != NULL && pi == NULL) {
      yyerrorex(3, "Not enough arguments passed to function");
      return;
    }
    canconvert(pi->ty, fi->ty, 0);
    pi = pi->next;
    fi = fi->next;
  }
}

void isnumeric(const struct typenode *ty)
{
  ty = getPrimitiveAncestor(ty);
  if (!(ty == gInteger || ty == gReal || ty == gAny))
    yyerrorline(3, islinebreak ? lineno - 1 : lineno, "Cannot be converted to numeric type");
}

void checkcomparisonsimple(const struct typenode *a) {
  const struct typenode *pa;
  pa = getPrimitiveAncestor(a);
  if (pa == gString || pa == gHandle || pa == gCode || pa == gBoolean) {
    yyerrorex(3, "Comparing the order/size of 2 variables only works on reals and integers");
    return;
  }
  if (pa == gNull)
    yyerrorex(3, "Comparing null is not allowed");
}

void checkcomparison(const struct typenode *a, const struct typenode *b)
{
  const struct typenode *pa, *pb;
  pa = getPrimitiveAncestor(a);
  pb = getPrimitiveAncestor(b);
  if (pa == gString || pa == gHandle || pa == gCode || pa == gBoolean || pb == gString || pb == gCode || pb == gHandle || pb == gBoolean) {
    yyerrorex(3, "Comparing the order/size of 2 variables only works on reals and integers");
    return;
  }
  if (pa == gNull && pb == gNull)
    yyerrorex(3, "Comparing null is not allowed");
}

void checkeqtest(const struct typenode *a, const struct typenode *b)
{
  const struct typenode *pa, *pb;
  pa = getPrimitiveAncestor(a);
  pb = getPrimitiveAncestor(b);
  if ((pa == gInteger || pa == gReal) && (pb == gInteger || pb == gReal))
    return;
  if (pa == gNull || pb == gNull)
    return;
  if (!typeeq(pa, pb)) {
    yyerrorex(3, "Comparing two variables of different primitive types (except real and integer) is not allowed");
    return;
  }
}

void dofile(FILE *fp, const char *name)
{
  lineno = 1;
  islinebreak = 1;
  isconstant = 0;
  inconstant = 0;
  inblock = 0;
  afterendglobals = 0;
  int olderrs = haderrors;
  yy_switch_to_buffer(yy_create_buffer(fp, BUFSIZE));
  curfile = name;
	while (yyparse())
    ;
  if (olderrs == haderrors)
		printf("Parse successful: %8d lines: %s\n", lineno, curfile);
  else
		printf("%s failed with %d error%s\n", curfile, haderrors - olderrs,(haderrors == olderrs + 1) ? "" : "s");
  totlines += lineno;
  fno++;
}

void printversion()
{
	printf("Pjass version %s by Rudi Cilibrasi, modified by AIAndy, PitzerMike, Deaod and lep\n", VERSIONSTR);
}

void doparse(int argc, char **argv)
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
			continue;
		}
		if (strcmp(argv[i], "-v") == 0) {
			printf("%s version %s\n", argv[0], VERSIONSTR);
			exit(0);
			continue;
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
		if (argv[i][0] == '-' && argv[i][1] == 'e' && argv[i][2] >= '0' && argv[i][2] < ('0' + ERRORLEVELNUM)) {
			showerrorlevel[argv[i][2]-'0'] = 0;
			continue;
		}
		if (argv[i][0] == '+' && argv[i][1] == 'e' && argv[i][2] >= '0' && argv[i][2] < ('0' + ERRORLEVELNUM)) {
			showerrorlevel[argv[i][2]-'0'] = 1;
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
