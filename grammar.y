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
#include "blocks.h"

#define YYMAXDEPTH 100000
#define YYDEBUG 1

%}

%define api.value.type {union node}

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
%token MOD
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
%token ALIAS

%right EQUALS
%left AND
%left OR
%left EQCOMP NEQ
%left LESS GREATER LEQ GEQ
%left NOT
%left MINUS PLUS
%left TIMES DIV MOD

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
         | globals newline vardecls endglobals
         | globals vardecls endglobals {yyerrorline(syntaxerror, lineno - 1, "Missing linebreak before global declaration");}
;

globals: GLOBALS { inglobals = 1; };
endglobals: ENDGLOBALS { inglobals = 0; };

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
      | stringexpr { $$.ty = $1.ty; }
      | boolexpr   { $$.ty = gBoolean; }
      | FUNCTION rid LPAREN exprlistcompl RPAREN {
            struct funcdecl *fd = ht_lookup(&functions, $2.str);
            if (fd == NULL) {
                char ebuf[1024];
                snprintf(ebuf, 1024, "Undefined function %s", $2.str);
                getsuggestions($2.str, ebuf, 1024, 1, &functions);
                yyerrorex(semanticerror, ebuf);
                $$.ty = gCode;
            } else {
                char ebuf[1024];
                if (fd->p->head != NULL) {
                    snprintf(ebuf, 1024, "Function %s must not take any arguments when used as code", $2.str);
                }else{
                    snprintf(ebuf, 1024, "Function must not take any arguments when used as code");
                }
                yyerrorex(semanticerror, ebuf);
                if( fd->ret == gBoolean) {
                    $$.ty = gCodeReturnsBoolean;
                } else {
                    $$.ty = gCodeReturnsNoBoolean;
                }
            }

      }
      | FUNCTION rid {
            struct funcdecl *fd = ht_lookup(&functions, $2.str);
            if (fd == NULL) {
                char ebuf[1024];
                snprintf(ebuf, 1024, "Undefined function %s", $2.str);
                getsuggestions($2.str, ebuf, 1024, 1, &functions);
                yyerrorex(semanticerror, ebuf);
                $$.ty = gCode;
            } else {
                if (fd->p->head != NULL) {
                    char ebuf[1024];
                    snprintf(ebuf, 1024, "Function %s must not take any arguments when used as code", $2.str);
                    yyerrorex(semanticerror, ebuf);
                }
                if( fd->isnative ) {
                    char ebuf[1024];
                    snprintf(ebuf, 1024, "Cannot use native '%s' as code", $2.str);
                    yyerrorline(runtimeerror, islinebreak ? lineno - 1 : lineno, ebuf);
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
      | expr MOD expr {
	    checkmodulo($1.ty, $3.ty);
	    $$.ty = gInteger;
	  }
      | expr MINUS expr { $$.ty = binop($1.ty, $3.ty); }
      | expr PLUS expr { 
            if ($1.ty == gString && $3.ty == gString)
                $$.ty = gString;
            else
                $$.ty = binop($1.ty, $3.ty);
        }
      | MINUS expr { isnumeric($2.ty); $$.ty = $2.ty; }
      | PLUS expr { isnumeric($2.ty); $$.ty = $2.ty; }
      | LPAREN expr RPAREN { $$.ty = $2.ty; }
      | funccall { $$.ty = $1.ty; }
      | rid LBRACKET expr RBRACKET {
          const struct typeandname *tan = getVariable($1.str);
          if (!typeeq(tan->ty, gAny)) {
	    checkarrayindex(tan->name, $3.ty, lineno);
            if (!tan->isarray) {
              char ebuf[1024];
              snprintf(ebuf, 1024, "%s not an array", $1.str);
              yyerrorex(semanticerror, ebuf);
            }
          }
          $$.ty = tan->ty;
       }
      | rid {
          const struct typeandname *tan = getVariable($1.str);
          if (tan->isarray) {
            char ebuf[1024];
            snprintf(ebuf, 1024, "Index missing for array variable %s", $1.str);
            yyerrorex(semanticerror, ebuf);
          }
          if( !tan->isarray && !ht_lookup(&initialized, $1.str) ){
            char buf[1024];
            
            if( ht_lookup(&locals, $1.str)){
                snprintf(buf, 1024, "Variable %s is uninitialized", $1.str);
                yyerrorline(semanticerror, lineno, buf);
            }else if(ht_lookup(&uninitialized_globals, $1.str)){
                if(infunction ){
                    if( flagenabled(flag_checkglobalsinit) ){
                        snprintf(buf, 1024, "Variable %s might be uninitalized", $1.str);
                        yyerrorline(semanticerror, lineno, buf);
                    }
                }else{
                    snprintf(buf, 1024, "Variable %s is uninitalized", $1.str);
                    yyerrorline(semanticerror, lineno, buf);
                }
            }

          
       }
       $$.ty = tan->ty;
      }
      | expr EQUALS expr {yyerrorex(syntaxerror, "Single = in expression, should probably be =="); checkeqtest($1.ty, $3.ty); $$.ty = gBoolean;}
      | LPAREN expr {yyerrorex(syntaxerror, "Mssing ')'"); $$.ty = $2.ty;}
      
      // incomplete expressions 
      | expr LEQ { checkcomparisonsimple($1.ty); yyerrorex(syntaxerror, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr GEQ { checkcomparisonsimple($1.ty); yyerrorex(syntaxerror, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr LESS { checkcomparisonsimple($1.ty); yyerrorex(syntaxerror, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr GREATER { checkcomparisonsimple($1.ty); yyerrorex(syntaxerror, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr EQCOMP { yyerrorex(syntaxerror, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr NEQ { yyerrorex(syntaxerror, "Missing expression for comparison"); $$.ty = gBoolean; }
      | expr AND { canconvert($1.ty, gBoolean, 0); yyerrorex(syntaxerror, "Missing expression for logical and"); $$.ty = gBoolean; }
      | expr OR { canconvert($1.ty, gBoolean, 0); yyerrorex(syntaxerror, "Missing expression for logical or"); $$.ty = gBoolean; }
      | NOT { yyerrorex(syntaxerror, "Missing expression for logical negation"); $$.ty = gBoolean; }
;

funccall: rid LPAREN exprlistcompl RPAREN {
        $$ = checkfunccall($1.str, $3.pl);    
    }
    |  rid LPAREN exprlistcompl newline {
        yyerrorex(syntaxerror, "Missing ')'");
        $$ = checkfunccall($1.str, $3.pl);
    }
;

exprlistcompl: /* empty */ { $$.pl = newparamlist(); }
       | exprlist { $$.pl = $1.pl; }
;

exprlist: expr         { $$.pl = newparamlist(); addParam($$.pl, newtypeandname($1.ty, "")); }
       |  expr COMMA exprlist { $$.pl = $3.pl; addParam($$.pl, newtypeandname($1.ty, "")); }
;


stringexpr: STRINGLIT {
    int len = strlen(stringlit_buff);
    if( len > 1023 ){
        yyerrorex(semanticerror, "String literals over 1023 chars long crash the game upon loading a saved game.");
    }
    if(flagenabled(flag_checkstringhash)){
        $$.ty = ht_lookup(&string_literals, stringlit_buff);
        if( $$.ty == NULL ) {
            $$.ty = newtypenode(stringlit_buff, gString);
            ht_put(&string_literals, $$.ty->typename, $$.ty);
        }
    }else{
        $$.ty = gString;
    }
};

realexpr: REALLIT {
    $$.ty = gReal;
    if(flagenabled(flag_checknumberliterals))
        checkreallit(yytext);
};

boolexpr: boollit { $$.ty = gBoolean; }
;

boollit: TTRUE
       | TFALSE
;

intexpr: INTLIT {
    $$.ty = gInteger;
    if(flagenabled(flag_checknumberliterals))
        checkintlit(yytext);
} | UNITTYPEINT { $$.ty = gInteger; }
;


funcdecl: nativefuncdecl { $$.fd = $1.fd; }
         | CONSTANT nativefuncdecl { $$.fd = $2.fd; }
         | funcdefncore { $$.fd = $1.fd; }
;

nativefuncdecl: NATIVE rid TAKES optparam_list RETURNS opttype
{
    infunction = 1;
    if(encoutered_first_function){
        yyerrorex(semanticerror, "Native declared after functions");
    }
    $$ = checkfunctionheader($2.str, $4.pl, $6.ty);
    $$.fd->isconst = isconstant;
    $$.fd->isnative = true;


    if( !strcmp("Filter", $$.fd->name) ){
        fFilter = $$.fd;
    }else if( !strcmp("Condition", $$.fd->name) ){
        fCondition = $$.fd;
    }else if( !strcmp("StringHash", $$.fd->name) ){
        fStringHash = $$.fd;
    }
    ht_clear(&locals);
    ht_clear(&initialized);
    infunction = 0;

    fCurrent = NULL;
}
;

funcdefn: newline
       | funcdefncore
       | statement { yyerrorex(syntaxerror, "Statement outside of function"); }
;

funcdefncore: funcbegin localblock codeblock funcend {
            if(retval != gNothing) {
                if(!getTypeTag($3.ty))
                    yyerrorline(semanticerror, lineno - 1, "Missing return");
                else if ( flagenabled(flag_rb) )
                    canconvertreturn($3.ty, retval, -1);
            }
            fnannotations = pjass_flags;
        }
       | funcbegin localblock codeblock {
       
            char msg[1024];
            block_missing_error(msg, 1024);
            yyerrorex(syntaxerror, msg);
            
            ht_clear(&locals);
            ht_clear(&initialized);
            fnannotations = pjass_flags;
        }
;

funcend: ENDFUNCTION {
        ht_clear(&locals);
        ht_clear(&initialized);
        inblock = 0;
        inconstant = 0;
        infunction = 0;
        
        char msg[1024];
        
        if(! block_pop(Function, msg, 1024)){
            yyerrorex(syntaxerror, msg);
        }
    }
;

returnorreturns: RETURNS
               | RETURN {yyerrorex(syntaxerror,"Expected \"returns\" instead of \"return\"");}
;

funcbegin: FUNCTION rid TAKES optparam_list returnorreturns opttype {
        inconstant = 0;
        infunction = 1;
        encoutered_first_function = 1;
        $$ = checkfunctionheader($2.str, $4.pl, $6.ty);
        $$.fd->isconst = 0;
        block_push(lineno, Function);
    }
    | CONSTANT FUNCTION rid TAKES optparam_list returnorreturns opttype {
        inconstant = 1;
        infunction = 1;
        encoutered_first_function = 1;
        $$ = checkfunctionheader($3.str, $5.pl, $7.ty);
        $$.fd->isconst = 1;
        block_push(lineno, Function);
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

statement:
      newline { $$.ty = gEmpty; }
    | CALL funccall newline { $$.ty = gAny; }
    /*1    2    3     4        5        6        7      8      9 */
    | ifstart expr THEN newline codeblock elsifseq elseseq ifend newline {
        canconvert($2.ty, gBoolean, -1);
        $$.ty = combinetype($5.ty, combinetype($6.ty, $7.ty));
    }
    | SET rid EQUALS expr newline {
        const struct typeandname *tan = getVariable($2.str);
        if (tan->isarray) {
            char ebuf[1024];
            snprintf(ebuf, 1024, "Index missing for array variable %s", $2.str);
            yyerrorline(semanticerror, lineno - 1,  ebuf);
        }
        canconvert($4.ty, tan->ty, -1);
        $$.ty = gAny;
        if (tan->isconst) {
            char ebuf[1024];
            snprintf(ebuf, 1024, "Cannot assign to constant %s", $2.str);
            yyerrorline(semanticerror, lineno - 1, ebuf);
        }
        if (inconstant)
            validateGlobalAssignment($2.str);
        if(infunction && !ht_lookup(&initialized, $2.str)){
            ht_put(&initialized, $2.str, (void*)1);
        }
        $$.ty = gAny;
    }
    | SET rid rid EQUALS expr newline {
        char ebuf[1024];
        if(ht_lookup(&types, $2.str) || ht_lookup(&builtin_types, $3.str) ){
            snprintf(ebuf, 1024, ">%s< %s is an error here. The type only needs to be stated at declartion time", $2.str, $3.str);
        }else if(ht_lookup(&types, $3.str) || ht_lookup(&builtin_types, $3.str) ){
            snprintf(ebuf, 1024, "%s >%s< is an error here. The type only needs to be stated at declartion time", $2.str, $3.str);
        }else{
            snprintf(ebuf, 1024, "Unexpected '%s'", $3.str);
        }
        yyerrorline(syntaxerror, lineno -1, ebuf);
        $$.ty = gAny;
    }
    | SET rid LBRACKET expr RBRACKET EQUALS expr newline{
        const struct typeandname *tan = getVariable($2.str);
        $$.ty = gAny;
        if (tan->ty != gAny) {
            checkarrayindex(tan->name, $4.ty, lineno -1);
            if (!tan->isarray) {
                char ebuf[1024];
                snprintf(ebuf, 1024, "%s is not an array", $2.str);
                yyerrorline(semanticerror, lineno - 1, ebuf);
            }
            canconvert($7.ty, tan->ty, -1);
            if (inconstant)
                validateGlobalAssignment($2.str);
        }
        $$.ty = gAny;
    }
    | loopstart newline codeblock loopend newline { $$.ty = $3.ty; }
    | loopstart newline codeblock {
        char msg[1024];
        block_missing_error(msg, 1024);
        yyerrorex(syntaxerror, msg);
        $$.ty = $3.ty;
     }
    | EXITWHEN expr newline {
        canconvert($2.ty, gBoolean, -1);
        if (!inloop)
            yyerrorline(syntaxerror, lineno - 1, "Exitwhen outside of loop");
        $$.ty = gAny;
    }
    | RETURN expr newline {
        if(retval == gNothing)
            yyerrorline(semanticerror, lineno - 1, "Cannot return value from function that returns nothing");
        else if (! flagenabled(flag_rb) )
            canconvertreturn($2.ty, retval, 0);
        $$.ty = mkretty($2.ty, 1);
     }
    | RETURN newline {
        if (retval != gNothing)
            yyerrorline(semanticerror, lineno - 1, "Return nothing in function that should return value");
        $$.ty = mkretty(gAny, 1);
    }
    | DEBUG statement { $$.ty = gAny; }
   /*1    2   3      4        5         6        7 */
    | ifstart expr THEN newline codeblock elsifseq elseseq {
        canconvert($2.ty, gBoolean, -1);
        $$.ty = combinetype($5.ty, combinetype($6.ty, $7.ty));
        
        char msg[1024];
        block_missing_error(msg, 1024);
        yyerrorex(syntaxerror, msg);
    }
    | ifstart expr newline {
        canconvert($2.ty, gBoolean, -1);
        $$.ty = gAny;
        yyerrorex(syntaxerror, "Missing then or non valid expression");
    }
    | SET funccall newline {
        $$.ty = gAny;
        yyerrorline(semanticerror, lineno - 1, "Call expected instead of set");
    }
    | lvardecl {
        $$.ty = gAny;
        yyerrorex(semanticerror, "Local declaration after first statement");
    }
    | funccall newline {
        yyerrorline(syntaxerror, lineno-1, "Missing 'call'");
        $$.ty = gAny;
    }
    | error { $$.ty = gAny; }
;

loopstart: LOOP {
    inloop++;
    block_push(lineno, Loop);
};

loopend: ENDLOOP {
    inloop--;
    
    char msg[1024];
    if(! block_pop(Loop, msg, 1024)){
        yyerrorex(syntaxerror, msg);
    }
};

ifstart: IF {
    block_push(lineno, If);
};

ifend: ENDIF {
    char msg[1024];
    if(! block_pop(If, msg, 1024)){
        yyerrorex(syntaxerror, msg);
    }
};

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

rid: ID {
        $$.str = strdup(yytext);
        checkidlength($$.str);
    }
    | ALIAS {
        $$.str = strdup("alias");
        yyerrorex(syntaxerror, "Invalid name \"alias\"");
    }
    | TYPE {
        $$.str = strdup("type");
        yyerrorex(syntaxerror, "Invalid name \"type\"");
    }
;

vartypedecl: type rid {
        struct typeandname *tan = newtypeandname($1.ty, $2.str);
        tan->lineno = lineno;
        $$ = checkvartypedecl(tan);
    }
    | CONSTANT type rid {
        if (infunction) {
            yyerrorex(semanticerror, "Local constants are not allowed");
        }
        struct typeandname *tan = newtypeandname($2.ty, $3.str);
        tan->isconst = 1;
        $$ = checkvartypedecl(tan);
    }
    | type ARRAY rid {
        struct typeandname *tan = newtypeandname($1.ty, $3.str);
        tan->isarray = 1;
        $$ = checkarraydecl(tan);
  
    }
;

localblock: endlocalsmarker
        | lvardecl localblock
        | newline localblock
;

endlocalsmarker: /* empty */ { fCurrent = NULL; }
;

lvardecl: LOCAL vardecl { }
        | vardecl { yyerrorex(syntaxerror, "Missing 'local'"); }
        | CONSTANT LOCAL vardecl { yyerrorex(syntaxerror, "Local variables can not be declared constant"); }
        | typedef { yyerrorex(syntaxerror,"Types can not be extended inside functions"); }
        | funccall newline {
            yyerrorline(syntaxerror, lineno-1, "Missing 'call'");
            $$.ty = gAny;
        }
;

vardecl: vartypedecl newline {
             const struct typeandname *tan = getVariable($1.str);
             if (tan->isconst) {
                 yyerrorline(syntaxerror, lineno - 1, "Constants must be initialized");
             }
             if(inglobals){ 
                ht_put(&uninitialized_globals, $1.str, (void*)1);
             }
             $$.ty = gNothing;
           }
        |  vartypedecl EQUALS expr newline {
             const struct typeandname *tan = getVariable($1.str);
             if (tan->isarray) {
               yyerrorex(syntaxerror, "Arrays cannot be directly initialized");
             }
             if(infunction ){
                ht_put(&initialized, tan->name, (void*)1);
             }
             canconvert($3.ty, tan->ty, -1);
             $$.ty = gNothing;
           }
        | error
;

// TODO: add some tests for this
typedef: TYPE rid EXTENDS type {
    check_name_allready_defined(&functions, $2.str, "%s already defined as function");
    check_name_allready_defined(&globals, $2.str, "%s already defined as global");
    check_name_allready_defined(&builtin_types, $2.str, "%s already defined as type");
    check_name_allready_defined(&types, $2.str, "%s already defined as type");
    ht_put(&types, $2.str, newtypenode($2.str, $4.ty));
}
;

typeandname: type rid { $$.tan = newtypeandname($1.ty, $2.str); }
;
  
type: rid {
    $$.ty = ht_lookup(&types, $1.str);
    if ($$.ty == NULL) {
        $$.ty = ht_lookup(&builtin_types, $1.str);
        if( $$.ty == NULL) {
        	char buf[1024];
        	snprintf(buf, 1024, "Undefined type %s", $1.str);
        	getsuggestions($1.str, buf, 1024, 1, &types);
        	yyerrorex(semanticerror, buf);
        	$$.ty = gAny;
        }
    }
}
;

newline: NEWLINE;
