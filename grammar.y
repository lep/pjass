// Jass2 parser for bison/yacc
// by Rudi Cilibrasi
// Sun Jun  8 00:51:53 CEST 2003
// thanks to Jeff Pang for the handy documentation that this was based
// on at http://jass.sourceforge.net
%{

#include <stdio.h>
#include <string.h>
#include "token.yy.h"
#include "misc.h"


#define YYSTYPE union node
#define YYMAXDEPTH 100000
#define YYDEBUG 1

%}

%token IF
%token THEN
%token TYPE
%token EXTENDS
%token HANDLE
%token NEWLINE
%token GLOBALS
%token ENDGLOBALS
%token CONSTANT
%token NATIVE
%token TAKES
%token RETURNS
%token FUNCTION
%token ENDFUNCTION
%token LOCAL
%token ARRAY
%token SET
%token CALL
%token ELSE
%token ELSEIF
%token ENDIF
%token LOOP
%token EXITWHEN
%token RETURN
%token DEBUG
%token ENDLOOP
%token NOT
%token TNULL
%token TTRUE
%token TFALSE
%token CODE
%token STRING
%token INTEGER
%token REAL
%token BOOLEAN
%token NOTHING
%token ID
%token COMMA
%token AND
%token OR
%token EQUALS
%token TIMES
%token DIV
%token PLUS
%token MINUS
%token LPAREN
%token RPAREN
%token LBRACKET
%token RBRACKET
%token LESS
%token GREATER
%token LEQ
%token GEQ
%token EQCOMP
%token NEQ
%token STRINGLIT
%token INTLIT
%token REALLIT
%token UNITTYPEINT
%token ANNOTATION

%right EQUALS
%left AND
%left OR
%left LESS GREATER EQCOMP NEQ LEQ GEQ
%left NOT
%left MINUS PLUS
%left TIMES DIV

%%



program: topscopes globdefs topscopes funcdefns
;

topscopes: topscope
       | topscopes topscope
;

topscope: typedefs  
       | funcdecls
;

funcdefns: /* empty */
       | funcdefns funcdefn
;

globdefs: /* empty */
         | GLOBALS newline vardecls ENDGLOBALS endglobalsmarker
         | GLOBALS vardecls ENDGLOBALS endglobalsmarker {yyerrorline(0, lineno - 1, "Missing linebreak before global declaration");}
;

endglobalsmarker: /* empty */  {afterendglobals = 1;}
;

vardecls: /* empty */
         | vd vardecls
;

vd:      newline
       | vardecl
;

funcdecls: /* empty */
         | fd funcdecls
;

fd:      newline
       | funcdecl
;

typedefs:  /* empty */
         | td typedefs
;

td:      newline
       | typedef
;

// Returns a typenode
expr: intexpr      { $$.ty = gInteger; }
      | realexpr   { $$.ty = gReal; }
      | stringexpr { $$.ty = gString; }
      | boolexpr   { $$.ty = gBoolean; }
      | FUNCTION rid { struct funcdecl *fd = ht_lookup(&functions, $2.str);
                       if (fd == NULL) {
                           char ebuf[1024];
                           snprintf(ebuf, 1024, "Undefined function %s", $2.str);
                           getsuggestions($2.str, ebuf, 1024, 1, &functions);
                           yyerrorex(3, ebuf);
                           $$.ty = gCode;
                       } else {
                           if (fd->p->head != NULL) {
                               char ebuf[1024];
                               snprintf(ebuf, 1024, "Function %s must not take any arguments when used as code", $2.str);
                               yyerrorex(3, ebuf);
                           }
                           if( fd->ret == gBoolean) {
                               $$.ty = gCodeReturnsBoolean;
                           } else {
                               $$.ty = gCodeReturnsNoBoolean;
                           }
                       }
                     }
      | TNULL { $$.ty = gNull; }
      | expr LEQ expr { checkcomparison($1.ty, $3.ty); $$.ty = gBoolean; }
      | expr GEQ expr { checkcomparison($1.ty, $3.ty); $$.ty = gBoolean; }
      | expr LESS expr { checkcomparison($1.ty, $3.ty); $$.ty = gBoolean; }
      | expr GREATER expr { checkcomparison($1.ty, $3.ty); $$.ty = gBoolean; }
      | expr EQCOMP expr { checkeqtest($1.ty, $3.ty); $$.ty = gBoolean; }
      | expr NEQ expr { checkeqtest($1.ty, $3.ty); $$.ty = gBoolean; }
      | expr AND expr { canconvert($1.ty, gBoolean, 0); canconvert($3.ty, gBoolean, 0); $$.ty = gBoolean; }
      | expr OR expr { canconvert($1.ty, gBoolean, 0); canconvert($3.ty, gBoolean, 0); $$.ty = gBoolean; }
      | NOT expr { canconvert($2.ty, gBoolean, 0); $$.ty = gBoolean; }
      | expr TIMES expr { $$.ty = binop($1.ty, $3.ty); }
      | expr DIV expr { $$.ty = binop($1.ty, $3.ty); }
      | expr MINUS expr { $$.ty = binop($1.ty, $3.ty); }
      | expr PLUS expr { 
                         if ($1.ty == gString && $3.ty == gString)
                           $$.ty = gString;
                         else
                           $$.ty = binop($1.ty, $3.ty); }
      | MINUS expr { isnumeric($2.ty); $$.ty = $2.ty; }
      | LPAREN expr RPAREN { $$.ty = $2.ty; }
      | funccall { $$.ty = $1.ty; }
      | rid LBRACKET expr RBRACKET {
          const struct typeandname *tan = getVariable($1.str);
          if (!typeeq(tan->ty, gAny)) {
            if (!tan->isarray) {
              char ebuf[1024];
              snprintf(ebuf, 1024, "%s not an array", $1.str);
              yyerrorex(3, ebuf);
            }
            else {
              canconvert($3.ty, gInteger, 0);
            }
          }
          $$.ty = tan->ty;
       }
      | rid {
          const struct typeandname *tan = getVariable($1.str);
          if (tan->lineno == lineno && tan->fn == fno) {
            char ebuf[1024];
            snprintf(ebuf, 1024, "Use of variable %s before its declaration", $1.str);
            yyerrorex(3, ebuf);
          } else if (islinebreak && tan->lineno == lineno - 1 && tan->fn == fno) {
            char ebuf[1024];
            snprintf(ebuf, 1024, "Use of variable %s before its declaration", $1.str);
            yyerrorline(3, lineno - 1, ebuf);
          } else if (tan->isarray) {
            char ebuf[1024];
            snprintf(ebuf, 1024, "Index missing for array variable %s", $1.str);
            yyerrorex(3, ebuf);
          }
          if(infunction && ht_lookup(curtab, $1.str) && !ht_lookup(&initialized, $1.str) ){
            char ebuf[1024];
            snprintf(ebuf, 1024, "Variable %s is uninitialized", $1.str);
            //yyerrorex(3, ebuf);
            yyerrorline(3, lineno - 1, ebuf);
          }
          $$.ty = tan->ty;
       }
      | expr EQUALS expr {yyerrorex(0, "Single = in expression, should probably be =="); checkeqtest($1.ty, $3.ty); $$.ty = gBoolean;}
      | LPAREN expr {yyerrorex(0, "Mssing ')'"); $$.ty = $2.ty;}
      
      // incomplete expressions 
      | expr LEQ { checkcomparisonsimple($1.ty); yyerrorex(3, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr GEQ { checkcomparisonsimple($1.ty); yyerrorex(3, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr LESS { checkcomparisonsimple($1.ty); yyerrorex(3, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr GREATER { checkcomparisonsimple($1.ty); yyerrorex(3, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr EQCOMP { yyerrorex(3, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr NEQ { yyerrorex(3, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr AND { canconvert($1.ty, gBoolean, 0); yyerrorex(3, "Missing expression for logical and"); $$.ty = gBoolean; }
      | expr OR { canconvert($1.ty, gBoolean, 0); yyerrorex(3, "Missing expression for logical or"); $$.ty = gBoolean; }
      | NOT { yyerrorex(3, "Missing expression for logical negation"); $$.ty = gBoolean; }
;

funccall: rid LPAREN exprlistcompl RPAREN {
          struct funcdecl *fd = ht_lookup(&functions, $1.str);
          if (fd == NULL) {
            char ebuf[1024];
            snprintf(ebuf, 1024, "Undeclared function %s", $1.str);
            getsuggestions($1.str, ebuf, 1024, 1, &functions);
            yyerrorex(3, ebuf);
            $$.ty = gAny;
          } else {
            if (inconstant && !(fd->isconst)) {
              char ebuf[1024];
              snprintf(ebuf, 1024, "Call to non-constant function %s in constant function", $1.str);
              yyerrorex(3, ebuf);
            }
            if (fd == fCurrent && fCurrent)
          		yyerrorex(3, "Recursive function calls are not permitted in local declarations");
            checkParameters(fd->p, $3.pl, fd == fFilter || fd == fCondition);
            $$.ty = fd->ret;
          }
       }
       |  rid LPAREN exprlistcompl newline {
          yyerrorex(0, "Missing ')'");
          struct funcdecl *fd = ht_lookup(&functions, $1.str);
          if (fd == NULL) {
            char ebuf[1024];
            snprintf(ebuf, 1024, "Undeclared function %s", $1.str);
            getsuggestions($1.str, ebuf, 1024, 1, &functions);
            yyerrorex(3, ebuf);
            $$.ty = gAny;
          } else if (inconstant && !(fd->isconst)) {
            char ebuf[1024];
            snprintf(ebuf, 1024, "Call to non-constant function %s in constant function", $1.str);
            yyerrorex(3, ebuf);
            $$.ty = gAny;
          } else {
          	if (fd == fCurrent && fCurrent)
          		yyerrorex(3, "Recursive function calls are not permitted in local declarations");
            checkParameters(fd->p, $3.pl, fd == fCondition || fd == fFilter);
            $$.ty = fd->ret;
          }
       }
;

exprlistcompl: /* empty */ { $$.pl = newparamlist(); }
       | exprlist { $$.pl = $1.pl; }
;

exprlist: expr         { $$.pl = newparamlist(); addParam($$.pl, newtypeandname($1.ty, "")); }
       |  expr COMMA exprlist { $$.pl = $3.pl; addParam($$.pl, newtypeandname($1.ty, "")); }
;


stringexpr: STRINGLIT { $$.ty = gString; }
;

realexpr: REALLIT { $$.ty = gReal; }
;

boolexpr: boollit { $$.ty = gBoolean; }
;

boollit: TTRUE
       | TFALSE
;

intexpr:   INTLIT { $$.ty = gInteger; }
         | UNITTYPEINT { $$.ty = gInteger; }
;


funcdecl: nativefuncdecl { $$.fd = $1.fd; }
         | CONSTANT nativefuncdecl { $$.fd = $2.fd; }
         | funcdefncore { $$.fd = $1.fd; }
;

nativefuncdecl: NATIVE rid TAKES optparam_list RETURNS opttype
{
    if (ht_lookup(&locals, $2.str) || ht_lookup(&params, $2.str) || ht_lookup(&globals, $2.str)) {
        char buf[1024];
        snprintf(buf, 1024, "%s already defined as variable", $2.str);
        yyerrorex(3, buf);
    } else if (ht_lookup(&types, $2.str)) {
        char buf[1024];
        snprintf(buf, 1024, "%s already defined as type", $2.str);
        yyerrorex(3, buf);
    }
    $$.fd = newfuncdecl(); 
    $$.fd->name = strdup($2.str);
    $$.fd->p = $4.pl;
    $$.fd->ret = $6.ty;
    $$.fd->isconst = isconstant;

    put(&functions, $$.fd->name, $$.fd);

    if( !strcmp("Filter", $$.fd->name) ){
        fFilter = $$.fd;
    }else if( !strcmp("Condition", $$.fd->name) ){
        fCondition = $$.fd;
    }

}
;

funcdefn: newline
       | funcdefncore
       | statement { yyerrorex(0, "Statement outside of function"); }
;

funcdefncore: funcbegin localblock codeblock funcend {
            if(retval != gNothing) {
                if(!getTypeTag($3.ty))
                    yyerrorline(1, lineno - 1, "Missing return");
                else if ((pjass_flags & flag_rb) || (fnannotations & flag_rb))
                    canconvertreturn($3.ty, retval, -1);
            }
            fnannotations = 0;
        }
       | funcbegin localblock codeblock {yyerrorex(0, "Missing endfunction"); ht_clear(&params); ht_clear(&locals); ht_clear(&initialized); curtab = &globals; fnannotations = 0;}
;

funcend: ENDFUNCTION { ht_clear(&params); ht_clear(&locals); ht_clear(&initialized); curtab = &globals; inblock = 0; inconstant = 0; infunction = 0; }
;

returnorreturns: RETURNS
               | RETURN {yyerrorex(3,"Expected \"returns\" instead of \"return\"");}
;

funcbegin: FUNCTION rid TAKES optparam_list returnorreturns opttype {
  if (ht_lookup(&locals, $2.str) || ht_lookup(&params, $2.str) || ht_lookup(&globals, $2.str)) {
    char buf[1024];
    snprintf(buf, 1024, "%s already defined as variable", $2.str);
    yyerrorex(3, buf);
  } else if (ht_lookup(&types, $2.str)) {
    char buf[1024];
    snprintf(buf, 1024, "%s already defined as type", $2.str);
    yyerrorex(3, buf);
  }
  inconstant = 0;
  infunction = 1;
  curtab = &locals;
  $$.fd = newfuncdecl(); 
  $$.fd->name = strdup($2.str);
  $$.fd->p = $4.pl;
  $$.fd->ret = $6.ty;
  $$.fd->isconst = 0;
  put(&functions, $$.fd->name, $$.fd);
  fCurrent = ht_lookup(&functions, $2.str);
  struct typeandname *tan = $4.pl->head;
  for (;tan; tan=tan->next) {
    tan->lineno = lineno;
    tan->fn = fno;
    put(&params, strdup(tan->name), newtypeandname(tan->ty, tan->name));
    if (ht_lookup(&functions, tan->name)) {
      char buf[1024];
      snprintf(buf, 1024, "%s already defined as function", tan->name);
      yyerrorex(3, buf);
    } else if (ht_lookup(&types, tan->name)) {
      char buf[1024];
      snprintf(buf, 1024, "%s already defined as type", tan->name);
      yyerrorex(3, buf);
    }
  }
  retval = $$.fd->ret;
  fnannotations = annotations;
  inblock = 1;
  inloop = 0;
  //showfuncdecl($$.fd);
}
       | CONSTANT FUNCTION rid TAKES optparam_list returnorreturns opttype {
  if (ht_lookup(&locals, $3.str) || ht_lookup(&params, $3.str) || ht_lookup(&globals, $3.str)) {
    char buf[1024];
    snprintf(buf, 1024, "%s already defined as variable", $3.str);
    yyerrorex(3, buf);
  } else if (ht_lookup(&types, $3.str)) {
    char buf[1024];
    snprintf(buf, 1024, "%s already defined as type", $3.str);
    yyerrorex(3, buf);
  }
  inconstant = 1;
  curtab = &locals;
  $$.fd = newfuncdecl(); 
  $$.fd->name = strdup($3.str);
  $$.fd->p = $5.pl;
  $$.fd->ret = $7.ty;
  $$.fd->isconst = 1;
  put(&functions, $$.fd->name, $$.fd);
  struct typeandname *tan = $5.pl->head;
  for (;tan; tan=tan->next) {
    tan->lineno = lineno;
    tan->fn = fno;
    put(&params, strdup(tan->name), newtypeandname(tan->ty, tan->name));
    if (ht_lookup(&functions, tan->name)) {
      char buf[1024];
      snprintf(buf, 1024, "%s already defined as function", tan->name);
      yyerrorex(3, buf);
    } else if (ht_lookup(&types, tan->name)) {
      char buf[1024];
      snprintf(buf, 1024, "%s already defined as type", tan->name);
      yyerrorex(3, buf);
    }
  }
  retval = $$.fd->ret;
  inblock = 1;
  inloop = 0;
  //showfuncdecl($$.fd);
}
;

codeblock: /* empty */ { $$.ty = gEmpty; }
       | statement codeblock {
            if(typeeq($2.ty, gEmpty))
                $$.ty = $1.ty;
            else
                $$.ty = mkretty($2.ty, getTypeTag($1.ty) || getTypeTag($2.ty) );
        }
;

statement:  newline { $$.ty = gEmpty; }
       | CALL funccall newline{ $$.ty = gAny;}
       /*1    2    3     4        5        6        7      8      9 */
       | IF expr THEN newline codeblock elsifseq elseseq ENDIF newline {
            canconvert($2.ty, gBoolean, -1);
            $$.ty = combinetype($5.ty, combinetype($6.ty, $7.ty));
       }
       | SET rid EQUALS expr newline { if (getVariable($2.str)->isarray) {
                                         char ebuf[1024];
                                         snprintf(ebuf, 1024, "Index missing for array variable %s", $2.str);
                                         yyerrorline(3, lineno - 1,  ebuf);
                                       }
                                       canconvert($4.ty, getVariable($2.str)->ty, -1);
                                       $$.ty = gAny;
                                       if (getVariable($2.str)->isconst) {
                                         char ebuf[1024];
                                         snprintf(ebuf, 1024, "Cannot assign to constant %s", $2.str);
                                         yyerrorline(3, lineno - 1, ebuf);
                                       }
                                       if (inconstant)
                                         validateGlobalAssignment($2.str);
                                       if(infunction && ht_lookup(curtab, $2.str) && !ht_lookup(&initialized, $2.str)){
                                         put(&initialized, $2.str, (void*)1);
                                       }
				    }
       | SET rid LBRACKET expr RBRACKET EQUALS expr newline{ 
           const struct typeandname *tan = getVariable($2.str);
           $$.ty = gAny;
           if (tan->ty != gAny) {
             canconvert($4.ty, gInteger, -1);
             if (!tan->isarray) {
               char ebuf[1024];
               snprintf(ebuf, 1024, "%s is not an array", $2.str);
               yyerrorline(3, lineno - 1, ebuf);
             }
             canconvert($7.ty, tan->ty, -1);
             if (inconstant)
               validateGlobalAssignment($2.str);
             }
           }
       | loopstart newline codeblock loopend newline {$$.ty = $3.ty;}
       | loopstart newline codeblock {$$.ty = $3.ty; yyerrorex(0, "Missing endloop");}
       | EXITWHEN expr newline { canconvert($2.ty, gBoolean, -1); if (!inloop) yyerrorline(0, lineno - 1, "Exitwhen outside of loop"); $$.ty = gAny;}
       | RETURN expr newline {
            $$.ty = mkretty($2.ty, 1);
            if(retval == gNothing)
                yyerrorline(1, lineno - 1, "Cannot return value from function that returns nothing");
            else if (!(pjass_flags & flag_rb) && !(fnannotations & flag_rb))
                canconvertreturn($2.ty, retval, 0);
         }
       | RETURN newline {
            if (retval != gNothing)
                yyerrorline(1, lineno - 1, "Return nothing in function that should return value");
                $$.ty = mkretty(gAny, 1);
            }
       | DEBUG statement {$$.ty = gAny;}
       /*1    2   3      4        5         6        7 */
       | IF expr THEN newline codeblock elsifseq elseseq {
            canconvert($2.ty, gBoolean, -1);
            $$.ty = combinetype($5.ty, combinetype($6.ty, $7.ty));
            yyerrorex(0, "Missing endif");
        }
       | IF expr newline {
            canconvert($2.ty, gBoolean, -1);
            $$.ty = gAny;
            yyerrorex(0, "Missing then or non valid expression");
        }
       | SET funccall newline{ $$.ty = gAny; yyerrorline(0, lineno - 1, "Call expected instead of set");}
       | lvardecl {
            $$.ty = gAny;
            yyerrorex(0, "Local declaration after first statement");
        }
       | error {$$.ty = gAny; }
;

loopstart: LOOP {inloop++;}
;

loopend: ENDLOOP {inloop--;}
;

elseseq: /* empty */ { $$.ty = gAny; }
        | ELSE newline codeblock {
            $$.ty = $3.ty;
        }
;

elsifseq: /* empty */ { $$.ty = mkretty(gEmpty, 1); }
        /*   1     2    3    4         5         6 */
        | ELSEIF expr THEN newline codeblock elsifseq {
            canconvert($2.ty, gBoolean, -1);
            
            if(typeeq($6.ty, gEmpty)){
                if(typeeq($5.ty, gEmpty)){
                    $$.ty = mkretty(gAny, 0);
                }else{
                    $$.ty = $5.ty;
                }
            }else{
                $$.ty = combinetype($5.ty, $6.ty);
            }
        }
;

optparam_list: param_list { $$.pl = $1.pl; }
               | NOTHING { $$.pl = newparamlist(); }
;

opttype: NOTHING { $$.ty = gNothing; }
         | type { $$.ty = $1.ty; }
;

param_list: typeandname { $$.pl = newparamlist(); addParam($$.pl, $1.tan); }
          | typeandname COMMA param_list { addParam($3.pl, $1.tan); $$.pl = $3.pl; }
;

rid: ID
{ $$.str = strdup(yytext); }
;

vartypedecl: type rid {
  if (ht_lookup(&functions, $2.str)) {
    char buf[1024];
    snprintf(buf, 1024, "Symbol %s already defined as function", $2.str);
    yyerrorex(3, buf);
  } else if (ht_lookup(&types, $2.str)) {
    char buf[1024];
    snprintf(buf, 1024, "Symbol %s already defined as type", $2.str);
    yyerrorex(3, buf);
  }
  struct typeandname *tan = newtypeandname($1.ty, $2.str);
  $$.str = $2.str;
  struct typeandname *existing = ht_lookup(&locals, $2.str);
  if (!existing) {
    existing = ht_lookup(&params, $2.str);
    if (!existing)
      existing = ht_lookup(&globals, $2.str);
  }
  if (existing) {
    tan->lineno = existing->lineno;
    tan->fn = existing->fn;
  } else {
    tan->lineno = lineno;
    tan->fn = fno;
  }
  put(curtab, $2.str, tan);  }
       | CONSTANT type rid {
  if (afterendglobals) {
    yyerrorex(3, "Local constants are not allowed");
  }
  if (ht_lookup(&functions, $3.str)) {
    char buf[1024];
    snprintf(buf, 1024, "Symbol %s already defined as function", $3.str);
    yyerrorex(3, buf);
  } else if (ht_lookup(&types, $3.str)) {
    char buf[1024];
    snprintf(buf, 1024, "Symbol %s already defined as type", $3.str);
    yyerrorex(3, buf);
  }
  struct typeandname *tan = newtypeandname($2.ty, $3.str);
  $$.str = $3.str;
  tan->isconst = 1;
  struct typeandname *existing = ht_lookup(&locals, $3.str);
  if (!existing) {
    existing = ht_lookup(&params, $3.str);
    if (!existing)
      existing = ht_lookup(&globals, $3.str);
  }
  if (existing) {
    tan->lineno = existing->lineno;
    tan->fn = existing->fn;
  } else {
    tan->lineno = lineno;
    tan->fn = fno;
  }
  put(curtab, $3.str, tan); }
       | type ARRAY rid {
  if (ht_lookup(&functions, $3.str)) {
    char buf[1024];
    snprintf(buf, 1024, "Symbol %s already defined as function", $3.str);
    yyerrorex(3, buf);
  } else if (ht_lookup(&types, $3.str)) {
    char buf[1024];
    snprintf(buf, 1024, "Symbol %s already defined as type", $3.str);
    yyerrorex(3, buf);
  }
  if (getPrimitiveAncestor($1.ty) == gCode)
    yyerrorex(3, "Code arrays are not allowed");
  struct typeandname *tan = newtypeandname($1.ty, $3.str);
  $$.str = $3.str;
  tan->isarray = 1;
  struct typeandname *existing = ht_lookup(&locals, $3.str);
  if (!existing) {
    char buf[1024];
    existing = ht_lookup(&params, $3.str);
    if (afterendglobals && existing) {
    	snprintf(buf, 1024, "Symbol %s already defined as function parameter", $3.str);
    	yyerrorex(3, buf);
    }
    if (!existing) {
      existing = ht_lookup(&globals, $3.str);
      if (afterendglobals && existing) {
      	snprintf(buf, 1024, "Symbol %s already defined as global variable", $3.str);
      	yyerrorex(3, buf);
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
  put(curtab, $3.str, tan); }
  
 // using "type" as variable name 
      | type TYPE {
  yyerrorex(3, "Invalid variable name \"type\"");
  struct typeandname *tan = newtypeandname($1.ty, "type");
  $$.str = "type";
  struct typeandname *existing = ht_lookup(&locals, "type");
  if (!existing) {
    existing = ht_lookup(&params, "type");
    if (!existing)
      existing = ht_lookup(&globals, "type");
  }
  if (existing) {
    tan->lineno = existing->lineno;
    tan->fn = existing->fn;
  } else {
    tan->lineno = lineno;
    tan->fn = fno;
  }
  put(curtab, "type", tan);  }
       | CONSTANT type TYPE {
  if (afterendglobals) {
    yyerrorex(3, "Local constants are not allowed");
  }
  yyerrorex(3, "Invalid variable name \"type\"");
  struct typeandname *tan = newtypeandname($2.ty, "type");
  $$.str = "type";
  tan->isconst = 1;
  struct typeandname *existing = ht_lookup(&locals, "type");
  if (!existing) {
    existing = ht_lookup(&params, "type");
    if (!existing)
      existing = ht_lookup(&globals, "type");
  }
  if (existing) {
    tan->lineno = existing->lineno;
    tan->fn = existing->fn;
  } else {
    tan->lineno = lineno;
    tan->fn = fno;
  }
  put(curtab, "type", tan); }
       | type ARRAY TYPE {
  yyerrorex(3, "Invalid variable name \"type\"");
  struct typeandname *tan = newtypeandname($1.ty, "type");
  $$.str = "type";
  tan->isarray = 1;
  struct typeandname *existing = ht_lookup(&locals, "type");
  if (!existing) {
    existing = ht_lookup(&params, "type");
    if (!existing)
      existing = ht_lookup(&globals, "type");
  }
  if (existing) {
    tan->lineno = existing->lineno;
    tan->fn = existing->fn;
  } else {
    tan->lineno = lineno;
    tan->fn = fno;
  }
  put(curtab, "type", tan); }
;

localblock: endlocalsmarker
        | lvardecl localblock
        | newline localblock
;

endlocalsmarker: /* empty */ { fCurrent = 0; }
;

lvardecl: LOCAL vardecl { }
        | CONSTANT LOCAL vardecl { yyerrorex(3, "Local variables can not be declared constant"); }
        | typedef { yyerrorex(3,"Types can not be extended inside functions"); }
;

vardecl: vartypedecl newline {
             const struct typeandname *tan = getVariable($1.str);
             if (tan->isconst) {
               yyerrorline(3, lineno - 1, "Constants must be initialized");
             }
             $$.ty = gNothing;
           }
        |  vartypedecl EQUALS expr newline {
             const struct typeandname *tan = getVariable($1.str);
             if (tan->isarray) {
               yyerrorex(3, "Arrays cannot be directly initialized");
             }
             if(infunction && !ht_lookup(&initialized, tan->name)){
               put(&initialized, tan->name, (void*)1);
             }
             canconvert($3.ty, tan->ty, -1);
             $$.ty = gNothing;
           }
        | error
;

typedef: TYPE rid EXTENDS type {
  if (ht_lookup(&types, $2.str)) {
     char buf[1024];
     snprintf(buf, 1024, "Multiply defined type %s", $2.str);
     yyerrorex(3, buf);
  } else if (ht_lookup(&functions, $2.str)) {
    char buf[1024];
    snprintf(buf, 1024, "%s already defined as function", $2.str);
    yyerrorex(3, buf);
  }
  else
    put(&types, $2.str, newtypenode($2.str, $4.ty));
}
;

typeandname: type rid { $$.tan = newtypeandname($1.ty, $2.str); }
;
  
type: primtype { $$.ty = $1.ty; }
  | rid {
   if (ht_lookup(&types, $1.str) == NULL) {
     char buf[1024];
     snprintf(buf, 1024, "Undefined type %s", $1.str);
     getsuggestions($1.str, buf, 1024, 1, &types);
     yyerrorex(3, buf);
     $$.ty = gAny;
   }
   else
     $$.ty = ht_lookup(&types, $1.str);
}
;

primtype: HANDLE  { $$.ty = ht_lookup(&types, yytext); }
 | INTEGER        { $$.ty = ht_lookup(&types, yytext); }
 | REAL           { $$.ty = ht_lookup(&types, yytext); }
 | BOOLEAN        { $$.ty = ht_lookup(&types, yytext); }
 | STRING         { $$.ty = ht_lookup(&types, yytext); }
 | CODE           { $$.ty = ht_lookup(&types, yytext); }
;

newline: NEWLINE { annotations = 0; }
       | ANNOTATION { annotations = updateannotation(annotations, yytext, &available_flags); }
;
