/* Jass2 parser for bison/yacc */
/* by Rudi Cilibrasi */
/* Sun Jun  8 00:51:53 CEST 2003 */
/* thanks to Jeff Pang for the handy documentation that this was based */
/* on at http://jass.sourceforge.net */
/* Released under the BSD license */

%{

static int rawcodelen;
static int rawcodestartline;

#include "grammar.tab.h" 
#include "misc.h"

%}

%option noyywrap
%option nounput
%option noinput

%option outfile="token.yy.c"
%option header-file="token.yy.h"

COMMENTSTART [/][/]
NEWLINE  (\r?\n)
OCTDIGIT [0-7]
DIGIT    [0-9]
HEXDIGIT [0-9a-fA-F]
ID       [a-zA-Z]([a-zA-Z0-9_]*[a-zA-Z0-9])?


%x RAWCODE
%x INSTRING
%x xANNOTATION
%x xCOMMENT

%%

{COMMENTSTART}"#" {
    lineno++;
    islinebreak = 1;
    isconstant = 0;
    BEGIN(xANNOTATION);
}


{COMMENTSTART} {
    lineno++;
    islinebreak = 1;
    isconstant = 0;
    annotations = pjass_flags;
    BEGIN(xCOMMENT);
}

<xCOMMENT>[^\r\n]* { }
<xCOMMENT>{NEWLINE} { BEGIN(INITIAL); return NEWLINE; }
<xCOMMENT><<EOF>> { BEGIN(INITIAL); return NEWLINE; }

<xANNOTATION>[^\r\n]* { annotations = updateannotation(annotations, yytext, &available_flags); }
<xANNOTATION>{NEWLINE} { BEGIN(INITIAL); return NEWLINE; }
<xANNOTATION><<EOF>> { BEGIN(INITIAL); return NEWLINE; }


[']\\[btrnf\\]['] { return UNITTYPEINT; }
[']\\(.|[\r\n])['] {
        yyerrorline(syntaxerror, lineno, "Invalid escape character sequence");
        return UNITTYPEINT; 
    }

['] {
        BEGIN(RAWCODE);
        rawcodelen = 0;
        rawcodestartline = lineno;
    }
<RAWCODE>['] {
        if(rawcodelen != 4 && rawcodelen != 1){
            yyerrorline(syntaxerror, rawcodestartline, "Rawcodes must consist of 1 or 4 characters");
        }
        BEGIN(INITIAL);
        return UNITTYPEINT;
    }

<RAWCODE>\\[btrnf"\\] {
        rawcodelen++;
        yyerrorline(syntaxerror, rawcodestartline, "Escaped chars are only allowed if they are the only char in the rawcode.");
    }
<RAWCODE>\\(.|[\r\n]) {
        yyerrorline(syntaxerror, lineno, "Invalid escape character sequence");
        rawcodelen++;
    }

<RAWCODE>\r?\n { lineno++; isconstant = 0; rawcodelen += strlen(yytext); }
<RAWCODE>\r { lineno++; isconstant = 0; rawcodelen++; }
<RAWCODE>.  { rawcodelen++; }

["] {
    stringlit_length = 0;
    stringlit_buff[0] = 0;
    BEGIN(INSTRING);
}
<INSTRING>["] { BEGIN(INITIAL); return STRINGLIT; }
<INSTRING>\\[btrnf"\\] {
    str_append(stringlit_buff, yytext, stringlit_buffsize);
}
<INSTRING>\\(.|[\r\n]) { yyerrorline(syntaxerror, lineno, "Invalid escape character sequence"); }

<INSTRING>\r?\n {
    str_append(stringlit_buff, yytext, stringlit_buffsize);
    lineno++;
    isconstant = 0;
}
<INSTRING>\r {
    str_append(stringlit_buff, yytext, stringlit_buffsize);
    lineno++;
    isconstant = 0;
}
<INSTRING>. { str_append(stringlit_buff, yytext, stringlit_buffsize); }


{NEWLINE} {
    lineno++;
    islinebreak=1;
    isconstant=0;
    annotations = pjass_flags;
    return NEWLINE;
}

{DIGIT}+"."{DIGIT}+ return REALLIT;
"."{DIGIT}+  return REALLIT;
{DIGIT}+"." return REALLIT;
"0"({OCTDIGIT}*("8"|"9"){OCTDIGIT}*)+ yyerrorline(syntaxerror, lineno, "Invalid digit in octal integer notation"); return INTLIT;
({DIGIT}+)|(("0x"|"0X"|"$"){HEXDIGIT}+) return INTLIT;

"if" if (!islinebreak) yyerrorline(syntaxerror, lineno, "Missing linebreak before if"); islinebreak=0; return IF;
"not" return NOT;
"then" return THEN;
"type" if (!islinebreak) { char ebuf[1024]; snprintf(ebuf, 1024, "Missing linebreak before type declaration"); yyerrorline(syntaxerror, lineno, ebuf); } islinebreak=0; return TYPE;
"extends" return EXTENDS;
"globals" if (!islinebreak) { char ebuf[1024]; snprintf(ebuf, 1024, "Missing linebreak before globals block"); yyerrorline(syntaxerror, lineno, ebuf); } islinebreak=0; inblock=1; return GLOBALS;
"endglobals" islinebreak=0; inblock=0; return ENDGLOBALS;
"constant" isconstant = islinebreak; islinebreak=0; return CONSTANT;
"native" if (!islinebreak && !isconstant) { char ebuf[1024]; snprintf(ebuf, 1024, "Missing linebreak before native declaration"); yyerrorline(syntaxerror, lineno, ebuf); } islinebreak=0; return NATIVE;
"takes" return TAKES;
"returns" return RETURNS;
"function" if (!islinebreak && !isconstant && !inblock) { char ebuf[1024]; snprintf(ebuf, 1024, "Missing linebreak before function declaration"); yyerrorline(syntaxerror, lineno, ebuf); } islinebreak=0; return FUNCTION;
"endfunction" islinebreak=0; return ENDFUNCTION;
"local" if (!islinebreak) { char ebuf[1024]; snprintf(ebuf, 1024, "Missing linebreak before local declaration"); yyerrorline(syntaxerror, lineno, ebuf); } islinebreak=0; return LOCAL;
"array" return ARRAY;
"set" if (!islinebreak) yyerrorline(syntaxerror, lineno, "Missing linebreak before assignment"); islinebreak=0; return SET;
"call" if (!islinebreak) yyerrorline(syntaxerror, lineno, "Missing linebreak before function call"); islinebreak=0; return CALL;
"else" if (!islinebreak) yyerrorline(syntaxerror, lineno, "Missing linebreak before else"); islinebreak=0; return ELSE;
"elseif" if (!islinebreak) yyerrorline(syntaxerror, lineno, "Missing linebreak before elseif"); islinebreak=0; return ELSEIF;
"endif" if (!islinebreak) yyerrorline(syntaxerror, lineno, "Missing linebreak before endif"); islinebreak=0; return ENDIF;
"loop" if (!islinebreak) yyerrorline(syntaxerror, lineno, "Missing linebreak before loop"); islinebreak=0; return LOOP;
"exitwhen" if (!islinebreak) yyerrorline(syntaxerror, lineno, "Missing linebreak before exitwhen"); islinebreak=0; return EXITWHEN;
"return" if (!islinebreak) yyerrorline(syntaxerror, lineno, "Missing linebreak before return"); islinebreak=0; return RETURN;
"debug" return DEBUG;
"endloop" if (!islinebreak) yyerrorline(syntaxerror, lineno, "Missing linebreak before endloop"); islinebreak=0; return ENDLOOP;
"null" return TNULL;
"true" return TTRUE;
"false" return TFALSE;
"nothing" return NOTHING;
"and" return AND;
"or" return OR;
"alias" return ALIAS;
"," return COMMA;
"=" return EQUALS;
"*" return TIMES;
"/" return DIV;
"%" return MOD;
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


[ \t]+          /* eat up whitespace */
\xEF\xBB\xBF    /* utf8 bom */

\r {
    lineno++;
    char ebuf[1024];
    snprintf(ebuf, 1024, "MacOS style newline");
    yyerrorline(syntaxerror, lineno-1, ebuf);
}
.            { char ebuf[1024]; snprintf(ebuf, 1024, "Unrecognized character %s (ASCII %u)", yytext, (unsigned char)yytext[0] ); yyerrorline(syntaxerror, lineno, ebuf); }

%%

