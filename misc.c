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

#include "hashtable.h"
#include "misc.h"
#include "typeandname.h"

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
bool inglobals;
bool encoutered_first_function;
int *showerrorlevel;

struct hashtable builtin_types;
struct hashtable functions;
struct hashtable globals;
struct hashtable locals;
struct hashtable types;
struct hashtable initialized;

struct hashtable bad_natives_in_globals;

struct hashtable shadowed_variables;

struct hashtable uninitialized_globals;
struct hashtable string_literals;

size_t stringlit_buffsize = 2048;
char stringlit_buff[2048] = {0};
size_t stringlit_length = 0;

struct tree stringlit_hashes;

const struct typenode *retval;
const char *curfile;
struct typenode *gInteger, *gReal, *gBoolean, *gString, *gCode, *gHandle, *gNothing, *gNull, *gAny, *gNone, *gEmpty;
struct typenode *gCodeReturnsNoBoolean, *gCodeReturnsBoolean;
struct typenode *gEmpty;
struct funcdecl *fCurrent;
struct funcdecl *fFilter, *fCondition, *fStringHash;

struct hashtable available_flags;
struct hashtable flags_helpstring;



void check_name_allready_defined(struct hashtable *ht, const char *name, const char *msg)
{
  char buf[1024];
  if( ht_lookup(ht, name ) )
  {
    snprintf(buf, 1024, msg, name);
    yyerrorex(semanticerror, buf);
  }

}

// Checks a newly created typeandname for potential name conflicts.
// Locals can shadow global variables and locals and parameters share
// a namespace. This all has changed over the years.
static void checkvarname(struct typeandname *tan)
{
  check_name_allready_defined(&builtin_types, tan->name, "Name %s allready defined as type");
  if( infunction ) {
    check_name_allready_defined(&locals, tan->ty->typename, "Type %s was shadowed previously");
  } else {
    // global names all share a namespace
    check_name_allready_defined(&types, tan->name, "Name %s allready defined as type");
    check_name_allready_defined(&functions, tan->name, "Name %s allready defined as function");
    check_name_allready_defined(&globals, tan->name, "Name %s allready defined as global");
    
  }
  
}

static void check_lawful_shadowing(struct hashtable *ht, const char *name, const char *msg)
{
  char buf[1024];
  if( ht_lookup(ht, name )) {
    snprintf(buf, 1024, msg, name);
    yyerror(buf);
  }
  
}


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

// Stores a typeandname as a name in either the local or global hashtable
// depending on the inglobals and infunction global variables.
// This doesn't check for allready defined names.
static void store_variable(const char *name, struct typeandname *tan)
{
    struct hashtable *ht;
    if( inglobals ){
        ht = &globals;
    } else {
        // This assertion can be false if the syntax is exceptionally broken,
        // like in the tests/should-fail/crashes files.
        // In any "normal" script though this assertion is sound.
        // assert(infunction);
        ht = &locals;
    }
    ht_put(ht, name, tan);
}

const struct typeandname *getVariable(const char *varname)
{
    char ebuf[1024];
    struct typeandname *result;

    result = ht_lookup(&locals, varname);
    if (result) return result;

    result = ht_lookup(&globals, varname);
    if (result) return result;

    struct funcdecl *fd = ht_lookup(&functions, varname);
    
    if( fd ) {
        snprintf(ebuf, 1024, "Cannot use function %s as variable", varname);
        yyerrorline(semanticerror, islinebreak ? lineno - 1 : lineno, ebuf);
    }else{
        snprintf(ebuf, 1024, "Undeclared variable %s", varname);
        getsuggestions(varname, ebuf, 1024, 2, &locals, &globals);
        yyerrorline(semanticerror, islinebreak ? lineno - 1 : lineno, ebuf);
    }

    // Store it as unidentified variable
    struct typeandname *newtan = newtypeandname(gAny, varname);
    store_variable(varname, newtan);
    if(infunction && !ht_lookup(&initialized, varname)){
        ht_put(&initialized, varname, (void*)1);
    }
    return newtan;
}

void validateGlobalAssignment(const char *varname)
{
    char ebuf[1024];
    if( ht_lookup(&globals, varname) && !ht_lookup(&locals, varname) ){
        snprintf(ebuf, 1024, "Assignment to global variable %s in constant function", varname);
        yyerrorline(semanticerror, lineno - 1, ebuf);
    }
}

static void check_too_many_params(int num_params, const struct typeandname *inp)
{
    // We use exact comparison to only report it once
    if(num_params == 32 && inp)
    {
            char buf[1024];
            snprintf(buf, 1024, "A function call can have at most 31 arguments");
            yyerrorex(runtimeerror, buf);
    }
}

static bool canconvertbuf(char *buf, size_t buflen, const struct typenode *ufrom, const struct typenode *uto)
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
    if (typeeq(from, gNull) && !typeeq(to, gBoolean))
        return true;
    if (typeeq(from, gInteger) && (typeeq(to, gReal) || typeeq(to, gInteger)))
        return true;
    if (typeeq(from, to) && (typeeq(from, gBoolean) || typeeq(from, gString) || typeeq(from, gReal) || typeeq(from, gInteger) || typeeq(from, gCode)))
        return true;

    snprintf(buf, buflen, "Cannot convert %s to %s", ufrom->typename, uto->typename);
    return false;
}

static void checkParameters(const struct funcdecl *fd, const struct paramlist *inp, bool mustretbool)
{
    const struct paramlist *func = fd->p;
    const struct typeandname *fi = func->head;
    const struct typeandname *pi = inp->head;

    int num_params = 1;
    while(true) {
        check_too_many_params(num_params, pi);
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
        num_params++;
    }
}


void checkarrayindex(const char *name, const struct typenode *ty, int lineno)
{
    char buf[1024];
    if(! canconvertbuf(buf, 1024, ty, gInteger)){
        str_append(buf, " as index for array ", 1024);
        str_append(buf, name, 1024);
        yyerrorline(semanticerror, lineno, buf);
    }
}

const struct typenode *binop(const struct typenode *a, const struct typenode *b)
{
    a = getPrimitiveAncestor(a);
    b = getPrimitiveAncestor(b);
    if (typeeq(a, gInteger) && typeeq(b, gInteger))
        return gInteger;
    if (typeeq(a, gString) && typeeq(b, gString))
        return gString;
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

    if ( typeeq(from, gNull) &&
	    ( !typeeq(to, gInteger) && !typeeq(to, gReal)
	    && !typeeq(to, gBoolean) && !typeeq(to, gCode) ) )
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

    if( flagenabled(flag_nomodulo) ){
        yyerrorex(warning, "Using modulo operator '%'");
    }

    if(! fst && ! snd){
        yyerrorex(semanticerror, "Both operands of the modulo-operator must be integers");
    }else if(! fst){
        yyerrorex(semanticerror, "First operand of the modulo-operator must be an integer");
    }else if(! snd){
        yyerrorex(semanticerror, "Second operand of the modulo-operator must be an integer");
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

    check_name_allready_defined(&functions, fnname, "%s already defined as function");
    check_name_allready_defined(&globals, fnname, "%s already defined as global");
    check_name_allready_defined(&types, fnname, "%s already defined as type");
    check_name_allready_defined(&builtin_types, fnname, "%s already defined as type");

    ret.fd = newfuncdecl(); 
    ret.fd->name = strdup(fnname);
    ret.fd->p = pl;
    ret.fd->ret = retty;

    fnannotations = annotations;
    ht_put(&functions, ret.fd->name, ret.fd);

    fCurrent = ht_lookup(&functions, fnname);

    struct typeandname *tan = pl->head;
    for (;tan; tan=tan->next) {
        tan->lineno = lineno;
        tan->fn = fno;

        checkvartypedecl(tan);
        ht_put(&initialized, tan->name, (void*)1);

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
                if( ! strcmp(fd->name, "CreateRegion")) {
                    snprintf(ebuf, 1024, "Call to %s in a globals block crashes the game upon saving", fd->name);
                    yyerrorex(runtimeerror, ebuf);
                } else {
                    snprintf(ebuf, 1024, "Call to %s in a globals block crashes the game", fd->name);
                    yyerrorex(runtimeerror, ebuf);
                }
            }else if(err == NullInGlobals){
                snprintf(ebuf, 1024, "Call to %s in a globals block always returns null", fd->name);
                yyerrorex(runtimeerror, ebuf);
            }
        }
        
        if( fd == fStringHash && pl->head && flagenabled(flag_checkstringhash) ){
            const struct typenode *a1 = pl->head->ty;
            if( ! typeeq(a1, gString) && isDerivedFrom(a1, gString) ){
                //printf("Got call to StringHash with argument %s\n", a1->typename);
                uint32_t strhash = SStrHash2(a1->typename);
                char *name = tree_lookup(&stringlit_hashes, strhash);
                if( name == NULL ){
                    tree_put(&stringlit_hashes, strhash, a1->typename);
                }else if( strcmp(name, a1->typename)){
                    char ebuf[1024];
                    snprintf(ebuf, 1024, "String %s produces the same hash as %s", name, a1->typename);
                    yyerrorex(semanticerror, ebuf);
                }
                
            }
        }

        checkParameters(fd, pl, fd == fFilter || fd == fCondition);
        ret.ty = fd->ret;
    }
    return ret;
}

// Checks a typeandname with respect to the shadow flag, puts its name into
// the current scope (either globals or locals) and returns a node for bison.
union node checkvartypedecl(struct typeandname *tan)
{
    const char *name = tan->name;
    union node ret;
    checkvarname(tan);

    ret.str = name;
    check_name_allready_defined(&locals, name, "%s already defined");
    store_variable(name, tan);


    // flag driven
    if(infunction && flagenabled(flag_shadowing)){
      check_name_allready_defined(&globals, tan->name, "%s shadows global variable");
      check_name_allready_defined(&types, tan->name, "%s shadows type");
      check_name_allready_defined(&functions, tan->name, "%s shadows function");
    }
    return ret;
}

union node checkarraydecl(struct typeandname *tan)
{
    const char *name = tan->name;
    union node ret;
    ret.str = name;

    if (getPrimitiveAncestor(tan->ty) == gCode)
        yyerrorex(semanticerror, "Code arrays are not allowed");

    checkvarname(tan);
    check_name_allready_defined(&locals, name, "%s already defined");
    store_variable(name, tan);

    return ret;
}

void checkidlength(char *name)
{
    
    int len;
    if( flagenabled(flag_verylongnames) && (len = strlen(name)) > MAX_IDENT_LENGTH ){
        char first_few[10] = {0};
        char last_few[10] = {0};
        char ebuf[1024] = {0};

        // We assume that MAX_IDENT_LENGTH is way bigger than 10.
        memcpy(first_few, yytext, 9);
        memcpy(last_few, yytext+len-10, 9);

        snprintf(ebuf, 1024, "Name '%s...%s' is too long (%d)", first_few, last_few, len);
        yyerror(ebuf);
        
    }
}

static bool validate_real_lit(char *lit)
{
  int32_t frac = 0, pow10 = 1;
  int nfrac = 0;
  int32_t result = 0;
  while( *lit ){
    char c = *lit++;
    if( c == '.')
      break;

    if( __builtin_mul_overflow(result, 10, &result))
      return false;

    if( __builtin_add_overflow(result, c - '0', &result)) 
      return false;
  }
    
  while( *lit ){
    char c = *lit++;
    nfrac++;
    if( __builtin_mul_overflow(frac, 10, &frac))
      return false;

    if( __builtin_add_overflow(frac, c - '0', &frac))
      return false;

    if( __builtin_mul_overflow(pow10, 10, &pow10)){
      if( frac != 0)
        return false;
      if( nfrac == 32 )
        return false;
    }
    
  }
  return true;
}

void checkreallit(char *lit){
    if( ! validate_real_lit(lit) ){
        char ebuf[2048];
        snprintf(ebuf, 2048, "real literal parsing overflow (%s)", lit);
        yyerrorex(warning, ebuf);
    }
}

static bool validate_int_lit(char *lit, int base)
{
    long long int result = strtoll(lit, NULL, base);
    return INT32_MIN <= result && result <= INT32_MAX;
}


void checkintlit(char *lit)
{
    bool ok = true;
    if( *lit == '0'){
        if( lit[1] == 'x' || lit[1] == 'X'){
            // hex
            ok = validate_int_lit(lit, 16);
        }else{
            // octal or just 0
            ok = validate_int_lit(lit, 8);
        }
    }else if( *lit == '$'){
        // hex
        ok = validate_int_lit(lit+1, 16);
    }else{
        // decimal
        ok = validate_int_lit(lit, 10);
    }

    if( ! ok) {
        char ebuf[2048];
        snprintf(ebuf, 2048, "integer literal parsing overflow (%s)", lit);
        yyerrorex(warning, ebuf);
    }
}
