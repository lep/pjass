/* scanner for a toy Pascal-like language */

%{
/* need this for the call to atof() below */
#include "grammar.tab.h" 
#include "misc.h"

%}

COMMENT [/][/].*\r?\n
NEWLINE  \r?\n
DIGIT    [0-9]
ID       [a-zA-Z_][a-zA-Z0-9_]*
SPACE    [ \t]
CSTART  [/][*]
CEND    [*][/]
STRINGSTART ["]
STILLSTRINGA [^"\\]
STILLSTRINGB [\\]["]
STILLSTRINGC [\\][\\]
STRINGDONE ["]
UNITTYPEINT ['][a-zA-Z0-9][a-zA-Z0-9][a-zA-Z0-9][a-zA-Z0-9][']

%%

{STRINGSTART}({STILLSTRINGA}|{STILLSTRINGB}|{STILLSTRINGC})*{STRINGDONE} { return STRINGLIT; }
{COMMENT} lineno++; return NEWLINE;
{CSTART}.*{CEND} { }
{NEWLINE} lineno++; return NEWLINE;

{DIGIT}+"."{DIGIT}*  return REALLIT;

{DIGIT}+  return INTLIT;

"if"  return IF;
"not" return NOT;
"then" return THEN;
"type" return TYPE;
"extends" return EXTENDS;
"handle" return HANDLE;
"globals" return GLOBALS;
"endglobals" return ENDGLOBALS;
"constant" return CONSTANT;
"native" return NATIVE;
"takes" return TAKES;
"returns" return RETURNS;
"function" return FUNCTION;
"endfunction" return ENDFUNCTION;
"local" return LOCAL;
"array" return ARRAY;
"set" return SET;
"call" return CALL;
"else" return ELSE;
"elseif" return ELSEIF;
"endif" return ENDIF;
"loop" return LOOP;
"exitwhen" return EXITWHEN;
"return" return RETURN;
"debug" return DEBUG;
"endloop" return ENDLOOP;
"null" return TNULL;
"true" return TTRUE;
"false" return TFALSE;
"code" return CODE;
"string" return STRING;
"integer" return INTEGER;
"real" return REAL;
"boolean" return BOOLEAN;
"nothing" return NOTHING;
"and" return AND;
"or" return OR;
"," return COMMA;
"=" return EQUALS;
"*" return TIMES;
"/" return DIV;
"+" return PLUS;
"-" return MINUS;
"(" return LPAREN;
")" return RPAREN;
"[" return LBRACKET;
"]" return RBRACKET;
"<" return LESS;
">" return GREATER;
"==" return EQCOMP;
"<=" return LEQ;
">=" return GEQ;
"!=" return NEQ;

{ID}        return ID;
{UNITTYPEINT}        return UNITTYPEINT;

"<"|">"|"!"|"["|"]"|"("|")"|"+"|"-"|"*"|"/"|"."  return DEBUG;

[ \t]+          /* eat up whitespace */

.           printf( "Unrecognized character: %s\n", yytext );

%%

