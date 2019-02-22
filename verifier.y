%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "anread.h"
#include "antypes.h"
#include "anmacro.h"
#include "anproc.h"
#include "anpsmod.h"

/*#define Y_VERBOSE*/

void yyerror(const char *str)
{
	fprintf(stderr, "error: %s\n", str);
}

int yywrap()
{
	return 1;
}

int main(void)
{
	AutNet an;

	printf("TA Verifier\n\n");
	yyparse();
	
	an = get_autnet();
	
	if (an.naut > 0) {
		// prod_show(&an);
		psmodel_builder(&an);
	} else {
		printf("Empty network!");
	}
	
	printf("\nDONE\n");
	return 0;
}
	
%}


%start fileElements

%token NETTOK AUTOMATONTOK LOCATIONSTOK ACTIONSTOK CLOCKSTOK TRANSTOK OBRACETOK REACHTOK 
%token EBRACETOK SEMICOLON COLON ATTOK INITIALTOK INVTOK COMMITEDTOK URGENTTOK 
%token RARRTOK CONJUNCTION CONSTANT
%token REL_LE REL_LT REL_GE REL_GT REL_EQ OP_DIFF

%union {
	int  number;
    char *string;
}

%token	<string>	WORD
%type	<string>	clock
%type	<number>	CONSTANT

%left CONJUNCTION

%%

fileElements:
	| fileElements fileElement
	;

fileElement:
	NETTOK OBRACETOK netElements EBRACETOK SEMICOLON {
		complete_net();
	}
	| REACHTOK OBRACETOK prodConfigurations EBRACETOK SEMICOLON
	;

netElements:
	| netElements netElement SEMICOLON
	;
	
netElement:
	| ACTIONSTOK OBRACETOK actions EBRACETOK {
		actions_mkmap();
	}
	| AUTOMATONTOK WORD OBRACETOK automatonElements EBRACETOK {
		complete_automaton($2);
	}
	;
	
automatonElements:
	| automatonElements automatonElement SEMICOLON
	;

automatonElement:
	CLOCKSTOK OBRACETOK clocks EBRACETOK {
		clocks_mkmap();
	}
	| LOCATIONSTOK OBRACETOK locations EBRACETOK {
		locations_mkmap();
	}
	| TRANSTOK OBRACETOK transitions EBRACETOK
	;

clocks:
	| clocks clock SEMICOLON {
		clocks_append($2);
	}
	;

clock:
	WORD {
		$$=$1;
	}
	;

locations:
	| locations location SEMICOLON
	;

location:
	WORD OBRACETOK locationParams EBRACETOK { /* lokacja z parametrami */
		locations_append($1);
	}
	| WORD { /* lokacja bez parametrow */
		locations_append($1);
	}
	;

locationParams:
	| locationParams locationParam SEMICOLON
	;

locationParam:
	INITIALTOK {
		set_loc_type(LOC_INITIAL);
	}
	| URGENTTOK {
		set_loc_type(LOC_URGENT);
	}
	| COMMITEDTOK {
		set_loc_type(LOC_COMMITED);
	}
	| INVTOK clockConstr
	;
	
actions:
	| actions action SEMICOLON
	;

action:
	WORD OBRACETOK actionParams EBRACETOK {
		actions_append($1);
	}
	| WORD {
		actions_append($1);
	}
	;

actionParams:
	| actionParams actionParam SEMICOLON
	;
	
actionParam:
	URGENTTOK {
		set_act_type(ACT_URGENT);
	}
	;
	
prodConfigurations:
	| prodConfigurations prodConfiguration SEMICOLON 
	;

prodConfiguration:
	WORD OBRACETOK prodConfigurationElements EBRACETOK {
		property_got_one($1);	
	}
	;

prodConfigurationElements:
	| prodConfigurationElements prodConfigurationElement
	;

prodConfigurationElement:
	WORD ATTOK WORD {
		property_append_loc($1, $3); /* pamietac o zwolnieniu pamieci $1 i $3 */
	}
	;
	
/* TODO: sprawdzic czy ograniczenia sa wlasciwie wprowadzane */
clockConstr:
	| clock REL_LE CONSTANT {
#ifdef Y_VERBOSE
		printf("%s <= %d --normalisation--> %s - 0 <= %d\n", $1, $3, $1, $3);
#endif
		constr_append(get_cur_clock_idx($1), 0, CONSTR_LE, $3);
		free($1); /* tylko $1 jest stringiem */
	}
	| clock REL_LT CONSTANT {
#ifdef Y_VERBOSE
		printf("%s < %d --normalisation--> %s - 0 < %d\n", $1, $3, $1, $3);
#endif
		constr_append(get_cur_clock_idx($1), 0, CONSTR_LT, $3);
		free($1);
	}
	| clock REL_GE CONSTANT {
#ifdef Y_VERBOSE
		printf("%s >= %d --normalisation--> 0 - %s <= -%d\n", $1, $3, $1, $3);
#endif
		constr_append(0, get_cur_clock_idx($1), CONSTR_LE, -$3);
		free($1);
	}
	| clock REL_GT CONSTANT {
#ifdef Y_VERBOSE
		printf("%s > %d --normalisation--> 0 - %s < -%d\n", $1, $3, $1, $3);
#endif
		constr_append(0, get_cur_clock_idx($1), CONSTR_LT, -$3);
		free($1);
	}
	| clock REL_EQ CONSTANT {
#ifdef Y_VERBOSE
		printf("%s = %d --normalisation--> %s - 0 <= %d && 0 - %s <= -%d\n", $1, $3, $1, $3, $1, $3);
#endif
		constr_append(get_cur_clock_idx($1), 0, CONSTR_LE, $3);
		constr_append(0, get_cur_clock_idx($1), CONSTR_LE, -$3);
		free($1);
	}
	| clock OP_DIFF clock REL_LE CONSTANT {
#ifdef Y_VERBOSE
		printf("%s - %s <= %d --normalised--\n", $1, $3, $5);
#endif
		constr_append(get_cur_clock_idx($1), get_cur_clock_idx($3), CONSTR_LE, $5);
		free($1); free($3);
	}
	| clock OP_DIFF clock REL_LT CONSTANT {
#ifdef Y_VERBOSE
		printf("%s - %s < %d --normalised--\n", $1, $3, $5);
#endif
		constr_append(get_cur_clock_idx($1), get_cur_clock_idx($3), CONSTR_LT, $5);
		free($1); free($3);
	}
	| clock OP_DIFF clock REL_GE CONSTANT {
#ifdef Y_VERBOSE
		printf("%s - %s >= %d --normalisation--> %s - %s <= -%d\n", $1, $3, $5, $3, $1, $5);
#endif
		constr_append(get_cur_clock_idx($1), get_cur_clock_idx($3), CONSTR_LE, -$5);
		free($1); free($3);
	}
	| clock OP_DIFF clock REL_GT CONSTANT {
#ifdef Y_VERBOSE
		printf("%s - %s > %d --normalisation--> %s - %s < -%d\n", $1, $3, $5, $3, $1, $5);
#endif
		constr_append(get_cur_clock_idx($1), get_cur_clock_idx($3), CONSTR_LT, -$5);
		free($1); free($3);
	}
	| clock OP_DIFF clock REL_EQ CONSTANT {
#ifdef Y_VERBOSE
		printf("%s - %s = %d --normalisation--> %s - %s <= %d && %s - %s <= -%d\n", $1, $3, $5, $1, $3, $5, $3, $1, $5);
#endif
		constr_append(get_cur_clock_idx($1), get_cur_clock_idx($3), CONSTR_LE, $5);
		constr_append(get_cur_clock_idx($3), get_cur_clock_idx($1), CONSTR_LE, -$5);
		free($1); free($3);
	}
	| clockConstr CONJUNCTION clockConstr
	;

transitions:
	| transitions transition SEMICOLON
	;

transition:
	WORD RARRTOK WORD COLON WORD COLON clockConstr COLON OBRACETOK clocks EBRACETOK {
		trans_append(get_cur_loc_idx($1), get_cur_loc_idx($3), get_cur_act_idx($5));
		free($1); free($3); free($5);
	}
	;

%%

