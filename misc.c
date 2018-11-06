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
bool isconstant;
bool inconstant;
bool infunction;
bool inblock;
int fnannotations;
int annotations;
int didparse;
int inloop;
bool afterendglobals;
bool inglobals;
int *showerrorlevel;

struct hashtable functions;
struct hashtable globals;
struct hashtable locals;
struct hashtable params;
struct hashtable types;
struct hashtable initialized;

struct hashtable bad_natives_in_globals;

struct hashtable shadowed_variables;

struct hashtable uninitialized_globals;


struct hashtable *curtab;

const struct typenode *retval;
const char *curfile;
struct typenode *gInteger, *gReal, *gBoolean, *gString, *gCode, *gHandle, *gNothing, *gNull, *gAny, *gNone, *gEmpty;
struct typenode *gCodeReturnsNoBoolean, *gCodeReturnsBoolean;
struct typenode *gEmpty;
struct funcdecl *fCurrent;
struct funcdecl *fFilter, *fCondition;

struct hashtable available_flags;

void yyerrorline (enum errortype type, int line, const char *s)
{
    if(flagenabled(flag_syntaxerror) && type == syntaxerror){
        ignorederrors++;
        return;
    }

    if(flagenabled(flag_semanticerror) && type == semanticerror){
        ignorederrors++;
        return;
    }

    if(flagenabled(flag_runtimeerror) && type == runtimeerror){
        ignorederrors++;
        return;
    }

    haderrors++;
    printf ("%s:%d: %s\n", curfile, line, s);
}

void yyerrorex (enum errortype type, const char *s)
{
    yyerrorline(type, lineno, s);
}

void yyerror (const char *s)  /* Called by yyparse on error */
{
    yyerrorex(syntaxerror, s);
}

void put(struct hashtable *h, const char *name, void *val){
    if( !ht_put(h, name, val) ){
        char ebuf[1024];
        snprintf(ebuf, 1024, "Symbol %s multiply defined", name);
        yyerrorline(semanticerror, islinebreak ? lineno - 1 : lineno, ebuf);
    }
}


#define min(a, b) (((a) < (b)) ? (a) : (b))

int abs(int i){
    if(i < 0)
        return -i;
    return i;
}

void str_append(char *buf, const char *str, size_t buf_size){
    size_t str_len = strlen(str);
    size_t buf_len = strlen(buf);
    size_t buf_freespace = buf_size - (buf_len+1); // +1 for zero byte at the end
    size_t to_copy;

    if(buf_freespace > str_len){
        to_copy = str_len;
    }else{
        to_copy = buf_freespace;
    }
    
    memmove(buf+buf_len, str, to_copy);
    buf[buf_len + to_copy] = 0;
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
    int i;
    for(i = 0; i != 3; i++) {
        v[i] = malloc(sizeof(int) * (size_t)(b+1));
    }

    for(i = 0; i != b+1; i++){
        v[0][i] = i;
    }

    int pcur;
    int ppcur;
    int cur = 1;
    for(i = 0; i != a; i++){
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
    for(i = 0; i != 3; i++)
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
    int i;
    for(i = 0; i != 3; i++){
        suggestions[i].distance = INT_MAX;
        suggestions[i].name = NULL;
    }

    va_start(ap, nTables);

    
    for(i = 0; i != nTables; i++){
        struct hashtable *ht = va_arg(ap, struct hashtable*);
        
        size_t x;
        for(x = 0; x != ht->size; x++){
            if(ht->bucket[x].name){
                const struct typeandname *tan = ht->bucket[x].val;
                if(typeeq(tan->ty, gAny)){
                    continue;
                }

                int dist = editdistance(ht->bucket[x].name, name, cutoff);
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
        str_append(buff, hbuff, buffsize);
    }else if(count == 2){
        snprintf(hbuff, 1024, ". Maybe you meant %s or %s", suggestions[0].name, suggestions[1].name);
        str_append(buff, hbuff, buffsize);
    }else if(count >= 3){
        snprintf(hbuff, 1024, ". Maybe you meant %s, %s or %s", suggestions[0].name, suggestions[1].name, suggestions[2].name);
        str_append(buff, hbuff, buffsize);
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
    yyerrorline(semanticerror, islinebreak ? lineno - 1 : lineno, ebuf);

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
        yyerrorline(semanticerror, lineno - 1, ebuf);
    }
}

void checkParameters(const struct funcdecl *fd, const struct paramlist *inp, bool mustretbool)
{
    const struct paramlist *func = fd->p;
    const struct typeandname *fi = func->head;
    const struct typeandname *pi = inp->head;
    while(true) {
        if (fi == NULL && pi == NULL)
            return;
        if (fi == NULL && pi != NULL) {
            char buf[1024];
            snprintf(buf, 1024, "Too many arguments passed to function %s. ", fd->name);
            yyerrorex(semanticerror, buf);
            return;
        }
        if (fi != NULL && pi == NULL) {
            char buf[1024];
            snprintf(buf, 1024, "Not enough arguments passed to function %s. ", fd->name);
            str_append(buf, "Still missing: ", 1024);
            bool addComma = false;
            for(; fi; fi = fi->next){
                if(addComma){
                    str_append(buf, ", ", 1024);
                }
                str_append(buf, fi->name, 1024);
                addComma = true;
            }
            yyerrorex(semanticerror, buf);
            return;
        }
        char buf[1024];
        if(! canconvertbuf(buf, 1024, pi->ty, fi->ty )){
            char pbuf[1024];
            snprintf(pbuf, 1024, " in parameter %s in call to %s", fi->name, fd->name);
            str_append(buf, pbuf, 1024);
            yyerrorex(semanticerror, buf);
        }
        if(flagenabled(flag_filter) && mustretbool && typeeq(pi->ty, gCodeReturnsNoBoolean)){
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
        yyerrorline(semanticerror, islinebreak ? lineno - 1 : lineno, "Bad types for binary operator");
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

bool canconvertbuf(char *buf, size_t buflen, const struct typenode *ufrom, const struct typenode *uto)
{
    const struct typenode *from = ufrom, *to = uto;
    if (from == NULL || to == NULL)
        return true;
    if (typeeq(from, gAny) || typeeq(to, gAny))
        return true;
    if (isDerivedFrom(from, to))
        return true;
    if (getTypePtr(from)->typename == NULL || getTypePtr(to)->typename == NULL)
        return true;
    if (typeeq(from, gNone) || typeeq(to, gNone))
        return true;
    from = getPrimitiveAncestor(from);
    to = getPrimitiveAncestor(to);
    if (typeeq(from, gNull) && !typeeq(to, gInteger) && !typeeq(to, gReal) && !typeeq(to, gBoolean))
        return true;
    if (typeeq(from, gInteger) && (typeeq(to, gReal) || typeeq(to, gInteger)))
        return true;
    if (typeeq(from, to) && (typeeq(from, gBoolean) || typeeq(from, gString) || typeeq(from, gReal) || typeeq(from, gInteger) || typeeq(from, gCode)))
        return true;

    snprintf(buf, buflen, "Cannot convert %s to %s", ufrom->typename, uto->typename);
    return false;
}

// this is used for reducing expressions in many places (if/exitwhen conditions, assignments etc.)
void canconvert(const struct typenode *ufrom, const struct typenode *uto, const int linemod)
{
    char buf[1024];
    if(! canconvertbuf(buf, 1024, ufrom, uto ) ){
        yyerrorline(semanticerror, lineno + linemod, buf);
    }
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
        yyerrorline(semanticerror, lineno + linemod, ebuf);
        return;
    }

    if ((typeeq(from, gNull)) && (!typeeq(to, gInteger)) && (!typeeq(to, gReal)) && (!typeeq(to, gBoolean)))
        return; // can't return null when it expects integer, real or boolean (added 9.5.2005)

    if (typeeq(ufrom, uto)){
        return;
    }

    snprintf(ebuf, 1024, "Cannot convert returned value from %s to %s", getTypePtr(ufrom)->typename, getTypePtr(uto)->typename);
    yyerrorline(semanticerror, lineno + linemod, ebuf);
    return;
}

void isnumeric(const struct typenode *ty)
{
    ty = getPrimitiveAncestor(ty);
    if (!(ty == gInteger || ty == gReal || ty == gAny))
        yyerrorline(semanticerror, islinebreak ? lineno - 1 : lineno, "Cannot be converted to numeric type");
}

void checkcomparisonsimple(const struct typenode *a)
{
    const struct typenode *pa;
    pa = getPrimitiveAncestor(a);
    if (typeeq(pa, gString) || typeeq(pa, gHandle) || typeeq(pa, gCode) || typeeq(pa, gBoolean)) {
        yyerrorex(semanticerror, "Comparing the order/size of 2 variables only works on reals and integers");
        return;
    }
    if (typeeq(pa, gNull))
        yyerrorex(semanticerror, "Comparing null is not allowed");
}

void checkcomparison(const struct typenode *a, const struct typenode *b)
{
    const struct typenode *pa, *pb;
    pa = getPrimitiveAncestor(a);
    pb = getPrimitiveAncestor(b);
    if (typeeq(pa, gString) || typeeq(pa, gHandle) || typeeq(pa, gCode) || typeeq(pa, gBoolean) || typeeq(pb, gString) || typeeq(pb, gCode) || typeeq(pb, gHandle) || typeeq(pb, gBoolean)) {
        yyerrorex(semanticerror, "Comparing the order/size of 2 variables only works on reals and integers");
        return;
    }
    if (typeeq(pa, gNull) && typeeq(pb, gNull))
        yyerrorex(semanticerror, "Comparing null is not allowed");
}

void checkmodulo(const struct typenode *a, const struct typenode *b)
{
    const struct typenode *pa, *pb;
    pa = getPrimitiveAncestor(a);
    pb = getPrimitiveAncestor(b);

    bool fst = typeeq(pa, gInteger);
    bool snd = typeeq(pb, gInteger);

    if(! fst && ! snd){
	yyerrorex(semanticerror, "Both arguments of the modulo-operator must be integers");
    }else if(! fst){
	yyerrorex(semanticerror, "First argument of the modulo-operator must be an integer");
    }else if(! snd){
	yyerrorex(semanticerror, "Second argument of the modulo-operator must be an integer");
    }


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
        yyerrorex(semanticerror, "Comparing two variables of different primitive types (except real and integer) is not allowed");
        return;
    }
}

int isflag(char *txt, struct hashtable *flags){
    txt++; // ignore +/- at the start
    void *flag = ht_lookup(flags, txt);
    return (int)flag;
}

int updateflag(int cur, char *txt, struct hashtable *flags){
    char sgn = txt[0];
    int flag = isflag(txt, flags);

    if( flag){
        if(sgn == '+') {
            cur |= flag;
        } else if(sgn == '-') {
            cur &= ~flag;
        }
    }
    return cur;
}

int updateannotation(int cur, char *txt, struct hashtable *flags){
    char sep[] = " \t\r\n";
    memset(txt, ' ', strlen("//#"));
    char *ann;
    for(ann = strtok(txt, sep); ann; ann = strtok(NULL, sep)){
        cur = updateflag(cur, ann, flags);
    }
    return cur;
}

bool flagenabled(int flag)
{
    if(infunction){
        return (fnannotations & flag);
    }else{
        return (pjass_flags & flag);
    }
}

union node checkfunctionheader(const char *fnname, struct paramlist *pl, const struct typenode *retty)
{
    union node ret;

    if (ht_lookup(&locals, fnname) || ht_lookup(&params, fnname) || ht_lookup(&globals, fnname)) {
        char buf[1024];
        snprintf(buf, 1024, "%s already defined as variable", fnname);
        yyerrorex(semanticerror, buf);
    } else if (ht_lookup(&types, fnname)) {
        char buf[1024];
        snprintf(buf, 1024, "%s already defined as type", fnname);
        yyerrorex(semanticerror, buf);
    }

    curtab = &locals;
    ret.fd = newfuncdecl(); 
    ret.fd->name = strdup(fnname);
    ret.fd->p = pl;
    ret.fd->ret = retty;

    put(&functions, ret.fd->name, ret.fd);

    fCurrent = ht_lookup(&functions, fnname);
    fnannotations = annotations;

    struct typeandname *tan = pl->head;
    for (;tan; tan=tan->next) {
        tan->lineno = lineno;
        tan->fn = fno;
        put(&params, strdup(tan->name), newtypeandname(tan->ty, tan->name));
        if (ht_lookup(&functions, tan->name)) {
            char buf[1024];
            snprintf(buf, 1024, "%s already defined as function", tan->name);
            yyerrorex(semanticerror, buf);
        } else if (ht_lookup(&types, tan->name)) {
            char buf[1024];
            snprintf(buf, 1024, "%s already defined as type", tan->name);
            yyerrorex(semanticerror, buf);
        }

        if( flagenabled(flag_shadowing) ){
            if( ht_lookup(&globals, tan->name) ){
                char buf[1024];
                snprintf(buf, 1024, "Parmeter %s shadows global variable", tan->name);
                yyerrorex(semanticerror, buf);
            }
        }

    }
    retval = ret.fd->ret;
    inblock = 1;
    inloop = 0;

    return ret;
}

union node checkfunccall(const char *fnname, struct paramlist *pl)
{
    union node ret;
    struct funcdecl *fd = ht_lookup(&functions, fnname);
    if (fd == NULL) {
        char ebuf[1024];
        snprintf(ebuf, 1024, "Undeclared function %s", fnname);
        getsuggestions(fnname, ebuf, 1024, 1, &functions);
        yyerrorex(semanticerror, ebuf);
        ret.ty = gAny;
    } else {
        if (inconstant && !(fd->isconst)) {
            char ebuf[1024];
            snprintf(ebuf, 1024, "Call to non-constant function %s in constant function", fnname);
            yyerrorex(semanticerror, ebuf);
        }

        if (fd == fCurrent && fCurrent)
            yyerrorex(semanticerror, "Recursive function calls are not permitted in local declarations");

        if( inglobals){
            char ebuf[1024];
            int err = (int)ht_lookup(&bad_natives_in_globals, fd->name);
            if(err == CrashInGlobals){
                snprintf(ebuf, 1024, "Call to %s in a globals block crashes the game", fd->name);
                yyerrorex(runtimeerror, ebuf);
            }else if(err == NullInGlobals){
                snprintf(ebuf, 1024, "Call to %s in a globals block always returns null", fd->name);
                yyerrorex(runtimeerror, ebuf);
            }
        }

        checkParameters(fd, pl, fd == fFilter || fd == fCondition);
        ret.ty = fd->ret;
    }
    return ret;
}

static void checkvarname(struct typeandname *tan, bool isarray)
{
    const char *name = tan->name;
    if (ht_lookup(&functions, name)) {
        char buf[1024];
        snprintf(buf, 1024, "Symbol %s already defined as function", name);
        yyerrorex(semanticerror, buf);
    } else if (ht_lookup(&types, name)) {
        char buf[1024];
        snprintf(buf, 1024, "Symbol %s already defined as type", name);
        yyerrorex(semanticerror, buf);
    }

    struct typeandname *existing = ht_lookup(&locals, name);

    if (!existing) {
        char buf[1024];
        existing = ht_lookup(&params, name);
        if ( isarray && infunction && existing) {
            snprintf(buf, 1024, "Symbol %s already defined as function parameter", name);
            yyerrorex(semanticerror, buf);
        }
        if (!existing) {
            existing = ht_lookup(&globals, name);
            if ( isarray && infunction && existing) {
                snprintf(buf, 1024, "Symbol %s already defined as global variable", name);
                yyerrorex(semanticerror, buf);
            }
        }
    }
    if (existing) {
        tan->lineno = existing->lineno;
        tan->fn = existing->fn;
    } else {
        tan->lineno = lineno;
        tan->fn = fno;
    }
}

void checkallshadowing(struct typeandname *tan){
    struct typeandname *global = ht_lookup(&globals, tan->name);
    char buf[1024];

    if( global ){

        // once a variable is shadowed with an incompatible type every usage
        // of the shadowed variable in the script file (sic) cannot be used
        // safely anymore. Usages of the shadowed variable in the script
        // above the shadowing still work fine.
        tan->lineno = lineno;
        ht_put(&shadowed_variables, tan->name, tan);

        if(flagenabled(flag_shadowing)){
            snprintf(buf, 1024, "Local variable %s shadows global variable", tan->name);
            yyerrorline(semanticerror, lineno, buf);
        }
    } else if( flagenabled(flag_shadowing) && ht_lookup(&params, tan->name)){
        snprintf(buf, 1024, "Local variable %s shadows parameter", tan->name);
        yyerrorline(semanticerror, lineno, buf);
    }
}

union node checkvartypedecl(struct typeandname *tan)
{
    const char *name = tan->name;
    union node ret;
    checkvarname(tan, false);

    ret.str = name;
    put(curtab, name, tan);


    if(infunction ){
        // always an error
        checkwrongshadowing(tan, 0);

        // flag driven
        checkallshadowing(tan);
    }
    return ret;
}

union node checkarraydecl(struct typeandname *tan)
{
    const char *name = tan->name;
    union node ret;

    if (getPrimitiveAncestor(tan->ty) == gCode)
        yyerrorex(semanticerror, "Code arrays are not allowed");

    checkvarname(tan, true);

    ret.str = name;
    put(curtab, name, tan);

    return ret;
}

void checkwrongshadowing(const struct typeandname *tan, int linemod){
    struct typeandname *global;
    char buf[1024];
    if( (global = ht_lookup(&shadowed_variables, tan->name)) ){
        if(! typeeq(getPrimitiveAncestor(global->ty), getPrimitiveAncestor(tan->ty))){
            snprintf(buf, 1024, "Global variable %s is used after it was shadowed with an incompatible type in line %d", tan->name, global->lineno);
            yyerrorline(semanticerror, lineno - linemod, buf);
        }
    }
}

