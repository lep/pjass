/* Jass2 parser for bison/yacc */
/* by Rudi Cilibrasi */
/* Sun Jun  8 00:51:53 CEST 2003 */
/* thanks to Jeff Pang for the handy documentation that this was based */
/* on at http://jass.sourceforge.net */
/* Released under the BSD license */

%{
/* need this for the call to atof() below */
#undef yywrap
int yywrap()
{
	return 1;
}

#include "grammar.tab.h" 
#include "misc.h"

%}

COMMENT [/][/].*\r*\n
NEWLINE  (\r*\n)|\r*
OCTDIGIT [0-7]
DIGIT    [0-9]
HEXDIGIT [0-9a-fA-F]
ID       [a-zA-Z]([a-zA-Z0-9_]*[a-zA-Z0-9])?
SPACE    [ \t]
TABS     [\x01-\x09\x0B\x0C\x0E-\x1F]
CSTART  [/][*]
CEND    [*][/]
STRINGSTART ["]
STILLSTRINGA [^"\\\r]
STILLSTRINGB [\\][btrnf"\\]
STRINGDONE ["]
FSTRINGSTART ["]
FSTILLSTRINGA [^"\\]
FSTILLSTRINGB [\\].
FSTRINGDONE ["]
UNITTYPEINT (['][\x01-\x26\x29-\x5B\x5D-\x7F]([\x01-\x26\x29-\x5B\x5D-\x7F][\x01-\x26\x29-\x5B\x5D-\x7F][\x01-\x26\x29-\x5B\x5D-\x7F])?['])

%%


{STRINGSTART} {  int prevChar = 0;
                 int curChar = input();
                 islinebreak = 0;
                 for (;;) {
                   if (prevChar == '\r' && curChar != '\r') {
                     lineno++; isconstant = 0;
                   }
                   if (curChar == EOF)
                     break;
                   else if (prevChar == '\\') {
                     if (curChar != 'b' && curChar != 't' && curChar != 'r' && curChar != 'n' &&  curChar != 'f' && curChar != '\"' && curChar != '\\')
                       yyerrorline(3, lineno, "Invalid escape character sequence");
                     curChar = 0;
                   } else if (curChar == '\"') {
                     break;
                   }
                   prevChar = curChar;
                   curChar = input();
                 }
                 return STRINGLIT;
              }
{COMMENT} lineno++; islinebreak=1; isconstant=0; return NEWLINE;
{CSTART}.*{CEND} { }
{NEWLINE} lineno++; islinebreak=1; isconstant=0; return NEWLINE;

{DIGIT}*"."{DIGIT}*  return REALLIT;
"0"({OCTDIGIT}*("8"|"9"){OCTDIGIT}*)+ yyerrorline(3, lineno, "Invalid digit in octal integer notation"); return INTLIT;
({DIGIT}+)|(("0x"|"$"){HEXDIGIT}+)  return INTLIT;

"if" if (!islinebreak) yyerrorline(3, lineno, "Missing linebreak before if"); islinebreak=0; return IF;
"not" return NOT;
"then" return THEN;
"type" if (!islinebreak) { char ebuf[1024]; sprintf(ebuf, "Missing linebreak before type declaration"); yyerrorline(3, lineno, ebuf); } islinebreak=0; return TYPE;
"extends" return EXTENDS;
"handle" islinebreak=0; return HANDLE;
"globals" if (!islinebreak) { char ebuf[1024]; sprintf(ebuf, "Missing linebreak before globals block"); yyerrorline(3, lineno, ebuf); } islinebreak=0; inblock=1; return GLOBALS;
"endglobals" islinebreak=0; inblock=0; return ENDGLOBALS;
"constant" isconstant = islinebreak; islinebreak=0; return CONSTANT;
"native" if (!islinebreak && !isconstant) { char ebuf[1024]; sprintf(ebuf, "Missing linebreak before native declaration"); yyerrorline(3, lineno, ebuf); } islinebreak=0; return NATIVE;
"takes" return TAKES;
"returns" return RETURNS;
"function" if (!islinebreak && !isconstant && !inblock) { char ebuf[1024]; sprintf(ebuf, "Missing linebreak before function declaration"); yyerrorline(3, lineno, ebuf); } islinebreak=0; return FUNCTION;
"endfunction" islinebreak=0; return ENDFUNCTION;
"local" if (!islinebreak) { char ebuf[1024]; sprintf(ebuf, "Missing linebreak before local declaration"); yyerrorline(3, lineno, ebuf); } islinebreak=0; return LOCAL;
"array" return ARRAY;
"set" if (!islinebreak) yyerrorline(3, lineno, "Missing linebreak before assignment"); islinebreak=0; return SET;
"call" if (!islinebreak) yyerrorline(3, lineno, "Missing linebreak before function call"); islinebreak=0; return CALL;
"else" if (!islinebreak) yyerrorline(3, lineno, "Missing linebreak before else"); islinebreak=0; return ELSE;
"elseif" if (!islinebreak) yyerrorline(3, lineno, "Missing linebreak before elseif"); islinebreak=0; return ELSEIF;
"endif" if (!islinebreak) yyerrorline(3, lineno, "Missing linebreak before endif"); islinebreak=0; return ENDIF;
"loop" if (!islinebreak) yyerrorline(3, lineno, "Missing linebreak before loop"); islinebreak=0; return LOOP;
"exitwhen" if (!islinebreak) yyerrorline(3, lineno, "Missing linebreak before exitwhen"); islinebreak=0; return EXITWHEN;
"return" if (!islinebreak) yyerrorline(3, lineno, "Missing linebreak before return"); islinebreak=0; return RETURN;
"debug" return DEBUG;
"endloop" if (!islinebreak) yyerrorline(3, lineno, "Missing linebreak before endloop"); islinebreak=0; return ENDLOOP;
"null" return TNULL;
"true" return TTRUE;
"false" return TFALSE;
"code" islinebreak=0; return CODE;
"string" islinebreak=0; return STRING;
"integer" islinebreak=0; return INTEGER;
"real" islinebreak=0; return REAL;
"boolean" islinebreak=0; return BOOLEAN;
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

{ID}        islinebreak=0; return ID;
{UNITTYPEINT}        return UNITTYPEINT;

"<"|">"|"!"|"["|"]"|"("|")"|"+"|"-"|"*"|"/"|"."  return TNULL;

[ \t]+          /* eat up whitespace */
[\x01-\x09\x0B\x0C\x0E-\x1F]+    /* eat up tabs */

.            if (1) { char ebuf[1024]; sprintf(ebuf, "Unrecognized character %s (ASCII %d)", yytext, yytext[0] ); yyerrorline(3, lineno, ebuf); }

%%

