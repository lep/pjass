// Jass2 parser for bison/yacc
// by Rudi Cilibrasi
// Sun Jun  8 00:51:53 CEST 2003
// thanks to Jeff Pang for the handy documentation that this was based
// on at http://jass.sourceforge.net
// Released under the BSD license
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include "misc.h"

int pjass_flags;

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
int fnannotations;
int annotations;
int didparse;
int inloop;
int afterendglobals;
int *showerrorlevel;

struct hashtable functions;
struct hashtable globals;
struct hashtable locals;
struct hashtable params;
struct hashtable types;
struct hashtable initialized;

struct hashtable *curtab;

const struct typenode *retval;
const char *curfile;
struct typenode *gInteger, *gReal, *gBoolean, *gString, *gCode, *gHandle, *gNothing, *gNull, *gAny, *gNone, *gEmpty;
struct typenode *gCodeReturnsNoBoolean, *gCodeReturnsBoolean;
struct typenode *gEmpty;
struct funcdecl *fCurrent;
struct funcdecl *fFilter, *fCondition;


void yyerrorline (int errorlevel, int line, const char *s)
{
    //if (showerrorlevel[errorlevel]) {
    haderrors++;
    printf ("%s:%d: %s\n", curfile, line, s);
    //}
    //else
    //  ignorederrors++;
}

void yyerrorex (int errorlevel, const char *s)
{
    //if (showerrorlevel[errorlevel]) {
    haderrors++;
    printf ("%s:%d: %s\n", curfile, lineno, s);
    //}
    //else
    //  ignorederrors++;
}

void yyerror (const char *s)  /* Called by yyparse on error */
{
    yyerrorex(0, s);
}

void put(struct hashtable *h, const char *name, void *val){
    if( !ht_put(h, name, val) ){
        char ebuf[1024];
        snprintf(ebuf, 1024, "Symbol %s multiply defined", name);
        yyerrorline(3, islinebreak ? lineno - 1 : lineno, ebuf);
    }
}


#define min(a, b) (((a) < (b)) ? (a) : (b))

int abs(int i){
    if(i < 0)
        return -i;
    return i;
}

static int editdistance(const char *s, const char *t, int cutoff){
    if(!strcmp(s, t)) return 0;

    int a = strlen(s);
    int b = strlen(t);

    if(a==0) return b;
    if(b==0) return a;

    if(abs(a-b) > cutoff){
        return cutoff + 1;
    }

    int *v[3];
    for(int i = 0; i != 3; i++) {
        v[i] = malloc(sizeof(int) * (size_t)(b+1));
    }

    for(int i = 0; i != b+1; i++){
        v[0][i] = i;
    }

    int pcur;
    int ppcur;
    int cur = 1;
    for(int i = 0; i != a; i++){
        cur = (cur+1) % 3;
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
    for(int i = 0; i != 3; i++)
        free(v[i]);
    return d;
}

void getsuggestions(const char *name, char *buff, size_t buffsize, int nTables, ...)
{
    va_list ap;

    int len = strlen(name);
    int cutoff = (int)((len+2)/4.0);
    int count = 0;

    struct {int distance; const char *name;} suggestions[3];
    for(int i = 0; i != 3; i++){
        suggestions[i].distance = INT_MAX;
        suggestions[i].name = NULL;
    }

    va_start(ap, nTables);

    for(int i = 0; i != nTables; i++){
        struct hashtable *ht = va_arg(ap, struct hashtable*);

        for(size_t x = 0; x != ht->size; x++){
            if(ht->bucket[x].name){
                const struct typeandname *tan = ht->bucket[x].val;
                if(typeeq(tan->ty, gAny)){
                    continue;
                }

                int dist = editdistance(ht->bucket[x].name, name, cutoff);
                if(dist <= cutoff){
                    count++;
                    for(int j = 0; j != 3; j++){
                        if(suggestions[j].distance > dist){
                            if(i == 0){
                                suggestions[2] = suggestions[1];
                                suggestions[1] = suggestions[0];
                            }else if(i == 1){
                                suggestions[2] = suggestions[1];
                            }
                            suggestions[j].distance = dist;
                            suggestions[j].name = ht->bucket[x].name;

                            break;
                        }
                    }

                }
            }
        }

    }
    va_end(ap);

    char hbuff[1024];
    if(count == 1){
        snprintf(hbuff, 1024, ". Maybe you meant %s", suggestions[0].name);
        strncat(buff, hbuff, buffsize);
    }else if(count == 2){
        snprintf(hbuff, 1024, ". Maybe you meant %s or %s", suggestions[0].name, suggestions[1].name);
        strncat(buff, hbuff, buffsize);
    }else if(count >= 3){
        snprintf(hbuff, 1024, ". Maybe you meant %s, %s or %s", suggestions[0].name, suggestions[1].name, suggestions[2].name);
        strncat(buff, hbuff, buffsize);
    }
}


const struct typeandname *getVariable(const char *varname)
{
    char ebuf[1024];
    struct typeandname *result;

    result = ht_lookup(&locals, varname);
    if (result) return result;

    result = ht_lookup(&params, varname);
    if (result) return result;

    result = ht_lookup(&globals, varname);
    if (result) return result;

    snprintf(ebuf, 1024, "Undeclared variable %s", varname);
    getsuggestions(varname, ebuf, 1024, 3, &locals, &params, &globals);
    yyerrorline(2, islinebreak ? lineno - 1 : lineno, ebuf);

    // Store it as unidentified variable
    const struct typeandname *newtan = newtypeandname(gAny, varname);
    put(curtab, varname, (void*)newtan);
    if(infunction && !ht_lookup(&initialized, varname)){
        put(&initialized, varname, (void*)1);
    }
    return newtan;
}

void validateGlobalAssignment(const char *varname)
{
    char ebuf[1024];
    struct typeandname *result;
    result = ht_lookup(&globals, varname);
    if (result) {
        snprintf(ebuf, 1024, "Assignment to global variable %s in constant function", varname);
        yyerrorline(2, lineno - 1, ebuf);
    }
}


void checkParameters(const struct paramlist *func, const struct paramlist *inp, bool mustretbool)
{
    const struct typeandname *fi = func->head;
    const struct typeandname *pi = inp->head;
    while(true) {
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
        bool has_flag = (pjass_flags & flag_filter) || (fnannotations & flag_filter);
        if(has_flag && mustretbool && typeeq(pi->ty, gCodeReturnsNoBoolean)){
            yyerrorex(semanticerror, "Function passed to Filter or Condition must return a boolean");
            return;
        }
        pi = pi->next;
        fi = fi->next;
    }
}
const struct typenode *binop(const struct typenode *a, const struct typenode *b)
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

const struct typenode *combinetype(const struct typenode *n1, const struct typenode *n2)
{
    uint8_t ret = getTypeTag(n1) & getTypeTag(n2);
    if ((typeeq(n1, gNone)) || (typeeq(n2, gNone)))
        return mkretty(gNone, ret);
    if (typeeq(n1, n2))
        return mkretty(n1, ret);
    if (typeeq(n1, gNull))
        return mkretty(n2, ret);
    if (typeeq(n2, gNull))
        return mkretty(n1, ret);

    n1 = getPrimitiveAncestor(n1);
    n2 = getPrimitiveAncestor(n2);

    if (typeeq(n1, n2))
        return mkretty(n1, ret);
    if (typeeq(n1, gNull))
        return mkretty(n2, ret);
    if (typeeq(n2, gNull))
        return mkretty(n1, ret);
    if ((typeeq(n1, gInteger)) && (typeeq(n2, gReal)))
        return mkretty(gReal, ret);
    if ((typeeq(n1, gReal)) && (typeeq(n2, gInteger)))
        return mkretty(gInteger, ret);
    return mkretty(gNone, ret);
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
    if (getTypePtr(from)->typename == NULL || getTypePtr(to)->typename == NULL)
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

    snprintf(ebuf, 1024, "Cannot convert %s to %s", ufrom->typename, uto->typename);
    yyerrorline(3, lineno + linemod, ebuf);
    return 0;
}

// this is used for return statements only
void canconvertreturn(const struct typenode *ufrom, const struct typenode *uto, const int linemod)
{
    const struct typenode *from = ufrom, *to = uto;
    char ebuf[1024];
    if(typeeq(from, NULL) || typeeq(to, NULL))
        return; // garbage

    if (typeeq(from, gAny) || typeeq(to, gAny))
        return; // we don't care

    if (isDerivedFrom(from, to))
        return; // eg. from = unit, to = handle

    if (getTypePtr(from)->typename == NULL || getTypePtr(to)->typename == NULL)
        return; // garbage

    if (typeeq(from, gNone) || typeeq(to, gNone))
        return; // garbage


    from = getPrimitiveAncestor(from);
    to = getPrimitiveAncestor(to);
    if ((typeeq(to, gReal)) && (typeeq(from, gInteger))) {
        // can't return integer when it expects a real (added 9.5.2005)
        snprintf(ebuf, 1024, "Cannot convert returned value from %s to %s", getTypePtr(from)->typename, getTypePtr(to)->typename);
        yyerrorline(1, lineno + linemod, ebuf);
        return;
    }

    if ((typeeq(from, gNull)) && (!typeeq(to, gInteger)) && (!typeeq(to, gReal)) && (!typeeq(to, gBoolean)))
        return; // can't return null when it expects integer, real or boolean (added 9.5.2005)

    if (strict) {
        if (isDerivedFrom(ufrom, uto))
            return;
    } else if (typeeq(ufrom, uto)){
        return;
    }

    snprintf(ebuf, 1024, "Cannot convert returned value from %s to %s", getTypePtr(ufrom)->typename, getTypePtr(uto)->typename);
    yyerrorline(1, lineno + linemod, ebuf);
    return;
}

void isnumeric(const struct typenode *ty)
{
    ty = getPrimitiveAncestor(ty);
    if (!(ty == gInteger || ty == gReal || ty == gAny))
        yyerrorline(3, islinebreak ? lineno - 1 : lineno, "Cannot be converted to numeric type");
}

void checkcomparisonsimple(const struct typenode *a)
{
    const struct typenode *pa;
    pa = getPrimitiveAncestor(a);
    if (typeeq(pa, gString) || typeeq(pa, gHandle) || typeeq(pa, gCode) || typeeq(pa, gBoolean)) {
        yyerrorex(3, "Comparing the order/size of 2 variables only works on reals and integers");
        return;
    }
    if (typeeq(pa, gNull))
        yyerrorex(3, "Comparing null is not allowed");
}

void checkcomparison(const struct typenode *a, const struct typenode *b)
{
    const struct typenode *pa, *pb;
    pa = getPrimitiveAncestor(a);
    pb = getPrimitiveAncestor(b);
    if (typeeq(pa, gString) || typeeq(pa, gHandle) || typeeq(pa, gCode) || typeeq(pa, gBoolean) || typeeq(pb, gString) || typeeq(pb, gCode) || typeeq(pb, gHandle) || typeeq(pb, gBoolean)) {
        yyerrorex(3, "Comparing the order/size of 2 variables only works on reals and integers");
        return;
    }
    if (typeeq(pa, gNull) && typeeq(pb, gNull))
        yyerrorex(3, "Comparing null is not allowed");
}

void checkeqtest(const struct typenode *a, const struct typenode *b)
{
    const struct typenode *pa, *pb;
    pa = getPrimitiveAncestor(a);
    pb = getPrimitiveAncestor(b);
    if ((typeeq(pa, gInteger) || typeeq(pa, gReal)) && (typeeq(pb, gInteger) || typeeq(pb, gReal)))
        return;
    if (typeeq(pa, gNull) || typeeq(pb, gNull))
        return;
    if (!typeeq(pa, pb)) {
        yyerrorex(3, "Comparing two variables of different primitive types (except real and integer) is not allowed");
        return;
    }
}


int updateannotation(int cur, char *txt){
    char sep[] = " \t\n";
    char *ann;
    memset(txt, ' ', strlen("//#"));
    for(ann = strtok(txt, sep); ann; ann = strtok(NULL, sep)){
        char *name = ann+1;
        char sgn = ann[0];
        int flag = 0;

        if(! strcmp(name, "rb")){
            flag = flag_rb;
        } else if(! strcmp(name, "filter") ){
            flag = flag_filter;
        }

        if(sgn == '+') {
            cur |= flag;
        } else if(sgn == '-') {
            cur &= ~flag;
        }
    }
    return cur;
}
