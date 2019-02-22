%{
#include <stdio.h>
#include <string.h>
#include "y.tab.h"
%}

%%
\#.*					;
net						return NETTOK;
automaton				return AUTOMATONTOK;
locations				return LOCATIONSTOK;
actions					return ACTIONSTOK;
clocks					return CLOCKSTOK;
trans					return TRANSTOK;
init					return INITIALTOK;
commited				return COMMITEDTOK;
urgent					return URGENTTOK;
inv						return INVTOK;
reach					return REACHTOK;
[0-9]+					yylval.number=atoi(yytext); return CONSTANT;
[a-zA-Z_][a-zA-Z0-9_]*	yylval.string=strdup(yytext); return WORD;
\{						return OBRACETOK;
\}						return EBRACETOK;
;						return SEMICOLON;
,						return COLON;
->						return RARRTOK;
\&\&					return CONJUNCTION;
\<=						return REL_LE;
\<						return REL_LT;
\>=						return REL_GE;
\>						return REL_GT;
-						return OP_DIFF;
=						return REL_EQ;
@						return ATTOK;
\n						;
[ \t]+					;
%%
