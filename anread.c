/** anread.c **/

/*
 * Konstruowanie struktury danych przetrzymujacej cala wczytana siec.
 * 
 * Nie okreslam ktore argumenty sa const, bo to juz nie te czasy. ;)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "anread.h"
#include "antypes.h"
#include "anmacro.h"
#include "macro.h"

/* lokacje */
static Loc_set *loc_root = NULL;
static Loc_set *loc_cur = NULL;
static Loc_idx loc_cnt = 0;
static Loc *loc_map = NULL;
static Loc_type loc_type = 0;

static Loc_idx init_loc;				/* indeks lok. poczatkowej */
static bool got_initial_loc = false;	/* czy juz mamy lok. poczatkowa */

/* akcje */
static Act_set *act_root = NULL;
static Act_set *act_cur = NULL;
static Act_idx act_cnt = 0;
static Act *act_map = NULL;
static Act_type act_type = 0;
static bool *uact = NULL;

/* zegary */
static Clock_set *clocks_root = NULL;	/* korzen zegarowego stosu */
static Clock_set *clocks_cur = NULL;	/* biezacy element (ostatni) zegarowego stosu */
static Clock_idx clocks_cnt = 0;		/* liczba aktualnie zapmietanych zegarow na zegarowym stosie */
static Clock *clocks_map = NULL;		/* mapowanie indeksow zegarow na ich nazwy */
static Clock_idx aut_clocks_cnt = 0;

/* ograniczenia */
static Constr_set *constr_root = NULL;
static Constr_set *constr_cur = NULL;
static Constr_idx constr_cnt = 0;

/* macierz przejsc */
static Trans_set **trs = NULL;

/* automaty */
static Aut_set *aut_root = NULL;
static Aut_set *aut_cur = NULL;
static Aut_idx aut_cnt = 0;

/* wlasnosci sieci */
static ProdConf_set *prodconf_root = NULL;
static ProdConf_set *prodconf_cur = NULL;
static Property_set *properties_root = NULL;
static Property_set *properties_cur = NULL;

/* siec automatow */
static AutNet autnet;

/**************/
/*** ZEGARY ***/
/**************/

/*
 * Dodawanie zegarow do listy
 */
void
clocks_append(Clock new_clock)
{
	Clock_set *n, *cur = clocks_root, *next;
	
	/* sprawdzamy czy dodawany zegar juz nie znajduje sie na liscie */
	while (cur)
	{
		next = cur->next;
		if(strcmp(cur->clock, new_clock) == 0)
		{
			printf("Duplicated clock %s. Not adding!\n", new_clock);
			return;
		}
		cur = next;
	}
	
	n = smalloc(sizeof(Clock_set));

	n->clock = new_clock;
	n->next = NULL;
		
	if (!clocks_cur)
		clocks_root = n;
	else
		clocks_cur->next = n;
	clocks_cur = n;
	
	clocks_cnt++;
}

/* 
 * Tworzenie mapy zegarow systemowych
*/
void 
clocks_mkmap(void)
{
	Clock_set *cur = clocks_root, *next;
	Clock *map = NULL;

	if (clocks_map) 
	{
		FERROR("Clocks already mapped");
	}

	aut_clocks_cnt = clocks_cnt;
	map = clocks_map = smalloc(sizeof(Clock) * aut_clocks_cnt);

	while (cur)
	{
		next = cur->next;
		*map = cur->clock;
		map++;
		free(cur);
		cur = next;
	}
	
	clocks_root = clocks_cur = NULL;
	clocks_cnt = 0;
}

/*
 * Konsumowanie zapamietanych zegarow i zwracanie
 * ich indeksow w postaci tablicy
 *
 * UWAGA: clocks_cnt jest resetowany, wiec 
 * ewentualnie trzeba wczesniej go zapamietac
 */
Clock_idx *
clocks_mkarr(void)
{
	Clock_set *cur = clocks_root, *next;
	Clock_idx *r, *arr;

	if (!clocks_root) return NULL;

	arr = r = smalloc(sizeof(Clock_idx) * clocks_cnt);
	
	while (cur)
	{
		next = cur->next;
		*(arr++) = get_cur_clock_idx(cur->clock); 
		free(cur->clock);
		free(cur);
		cur = next;
	}
	
	clocks_root = clocks_cur = NULL;
	clocks_cnt = 0;
	
	return r;
}

/*
 * Znajdowanie indeksu zegara na podstawie jego nazwy
 */
Clock_idx
get_clock_idx(Clock *cm, Clock_idx cc, char *name)
{
	int i;
	
	if (!cm) 
	{
		FERROR("Clocks map is empty!");
	}
	
	for (i = 0; i < cc; i++)
		if (!strcmp(*(cm++), name)) return i+1;

	printf("Clock %s undeclared. ", name);
	FERROR("Unknown clock!");
	/* return -1; */
} 

Clock_idx
get_cur_clock_idx(char *name)
{
	return get_clock_idx(clocks_map, aut_clocks_cnt, name);
}

char *
get_clock_name(Clock *cm, Clock_idx i)
{
	assert(i >= 0);

	if (!cm) 
	{
		FERROR("Clocks map is empty!");
	}

	if (i == 0) return CLOCK_0_NAME;
	else return cm[i-1];
}

char *
get_cur_clock_name(Clock_idx i)
{
	return get_clock_name(clocks_map, i);
}

void
clocks_show(Clock *cm, Clock_idx cc)
{
	int i;

	if (!cm) 
	{
		FERROR("Clocks map is empty!");
	}

	for (i = 0; i < cc+1; i++)
		printf("cm[%d] = %s\n", i, get_clock_name(cm, i));
}

/*****************************/
/*** OGRANICZENIA ZEGAROWE ***/
/*****************************/

void
constr_append(Clock_idx i, Clock_idx j, Constr_rel r, int val)
{
	Constr_set *n;

	n = smalloc(sizeof(Constr_set));
	
	n->constr.l_clk = i;
	n->constr.r_clk = j;
	n->constr.rel = r;
	n->constr.val = val;
	n->next = NULL;
	
	if (!constr_root)
		constr_root = n;
	else
		constr_cur->next = n;
	constr_cur = n;
	
	constr_cnt++;
}

Constr *
constrs_mkarr(void)
{
	Constr_set *cur = constr_root, *next;
	Constr *r, *arr;

	if (!constr_root) return NULL;

	r = smalloc(sizeof(Constr) * (constr_cnt+1));
	arr = r;
	
	while (cur)
	{
		next = cur->next;
		*(arr++) = cur->constr; 
		free(cur);
		cur = next;
	}
	
	CONSTR_MARK_TERM(arr); /* ostatnie ograniczenie terminuje tablice */
	
	constr_root = constr_cur = NULL;
	constr_cnt = 0;
	
	return r;
}

/*
 * Wyswietlanie ograniczen
 */
void
constrs_show(Constr *c, Clock *cm)
{
	if (!c) return;
	
	while (!CONSTR_IS_TERM(c))
	{
		printf("(%s - %s ", get_clock_name(cm, c->l_clk), get_clock_name(cm, c->r_clk));
		switch (c->rel)
		{
			case CONSTR_LE:
				printf("<= ");
				break;
			case CONSTR_LT:
				printf("< ");
				break;
		}
		printf("%d) ", c->val);
		c++;
	}
}

/***************/
/*** LOKACJE ***/
/***************/

void
locations_append(char *name)
{
	Loc_set *n, *cur = loc_root, *next;
	
	/* sprawdzamy czy dodawana lokacja juz nie znajduje sie na liscie */
	while (cur)
	{
		next = cur->next;
		if(strcmp(cur->loc.name, name) == 0)
		{
			printf("Duplicated location %s. Not adding!\n", name);
			return;
		}
		cur = next;
	}
	
	n = smalloc(sizeof(Loc_set));
	
	n->loc.name = name;
	n->loc.inv = constrs_mkarr();
	n->loc.type = loc_type; loc_type = 0;
	n->next = NULL;

	if (!loc_cur)
		loc_root = n;
	else
		loc_cur->next = n;
	loc_cur = n;
	
	loc_cnt++;
}

/*
 * Okresla typ aktualnej lokacji do dodania
 */
void
set_loc_type(Loc_type t)
{
	loc_type |= t;
}

/*
 * Tworzenie mapy lokacji 
 */
void
locations_mkmap(void)
{
	Loc_set *cur = loc_root, *next;
	Loc *map = NULL;
	int i;

	if (loc_map) 
	{
		FERROR("Locations already mapped");
	}

	loc_map = smalloc(sizeof(Loc)*loc_cnt);

	map = loc_map;
	
	for (i = 0; cur; i++)
	{
		next = cur->next;
		*map = cur->loc;
		if (LOC_IS_INITIAL(cur->loc))
		{
			if (got_initial_loc)
			{
				FERROR("Multiple initial locations not allowed.");
			}
			else
			{
				init_loc = i;
				got_initial_loc = true;
			}
		}
		map++;
		free(cur);
		cur = next;
	}
	
	loc_root = loc_cur = NULL;
}

Loc_idx
get_loc_idx(Loc *lm, Loc_idx lc, char *name)
{
	int i;
	
	if (!lm) 
	{
		FERROR("Locations map is empty!");
	}
	
	for (i = 0; i < lc; i++)
		if (!strcmp((lm++)->name, name)) return i;
	
	printf("Location %s undeclared\n", name);
	FERROR("Unknown location!");
	/* return -1; */
}

Loc_idx
get_cur_loc_idx(char *name)
{
	return get_loc_idx(loc_map, loc_cnt, name);
}

char *
get_loc_name(Loc *lm, Loc_idx i)
{
	return lm[i].name;
}

char *
get_cur_loc_name(Loc_idx i)
{
	return get_loc_name(loc_map, i);
}

void
locations_show(Loc *lm, Loc_idx lc, Clock *cm)
{
	int i;
	
	if (!lm) 
	{
		FERROR("Locations map is empty!");
	}
	
	printf("Locations:\n");
	
	for (i = 0; i < lc; i++)
	{
		printf("%d - %s ", i, lm[i].name);
		if (LOC_IS_INITIAL(lm[i])) printf("initial ");
		if (LOC_IS_URGENT(lm[i])) printf("urgent ");
		if (LOC_IS_COMMITED(lm[i])) printf("commited ");
		if (lm[i].inv)
		{
			printf("with invariant: ");
			constrs_show(lm[i].inv, cm);
		}
		printf("\n");
	}
}

/*************/
/*** AKCJE ***/
/*************/

void
actions_append(char *name)
{
	Act_set *n, *cur = act_root, *next;
	
	/* sprawdzamy czy dodawana akcja juz nie znajduje sie na liscie */
	while (cur)
	{
		next = cur->next;
		if(!strcmp(cur->act.name, name))
		{
			printf("Duplicated action %s. Not adding!\n", name);
			return;
		}
		cur = next;
	}
	
	n = smalloc(sizeof(Act_set));
	
	n->act.name = name;
	n->act.type = act_type; act_type = 0;
	n->next = NULL;

	if (!act_cur)
		act_root = n;
	else
		act_cur->next = n;
	act_cur = n;
	
	act_cnt++;
}

/*
 * Okresla typ aktualnej akcji do dodania
 */
void
set_act_type(Act_type t)
{
	act_type |= t;
}

/*
 * Tworzenie mapy akcji
 */
void
actions_mkmap(void)
{
	Act_set *cur = act_root, *next;
	Act *map = NULL;

	if (act_map)
	{
		FERROR("Actions already mapped");
	}

	act_map = smalloc(sizeof(Act)*act_cnt);

	map = act_map;
	
	while (cur)
	{
		next = cur->next;
		*map = cur->act;
		map++;
		free(cur);
		cur = next;
	}
	
	act_root = act_cur = NULL;
	
#ifdef VERBOSE
	printf("Got all actions\n");
	actions_show(act_map, act_cnt);
#endif
}

Act_idx
get_act_idx(Act *am, Act_idx ac, char *name)
{
	int i;
	
	if (!am) 
	{
		FERROR("Actions map is empty!"); 
		/* TODO: czy to faktycznie jest nie do przelkniecia? 
		   czy zakladamy ze jak jest automat to musi miec jakies akcje? */
	}
	
	for (i = 0; i < ac; i++)
		if (!strcmp((am++)->name, name)) return i;
	
	printf("Action %s undeclared. ", name);
	FERROR("Unknown action!");
	
	/* return -1; */
}

Loc_idx
get_cur_act_idx(char *name)
{
	return get_act_idx(act_map, act_cnt, name);
}

char *
get_act_name(Act *am, Act_idx i)
{
	return am[i].name;
}

char *
get_cur_act_name(Act_idx i)
{
	return get_act_name(act_map, i);
}

void
actions_show(Act *am, Act_idx ac)
{
	int i;
	
	if (!am)
	{
		FERROR("Actions map is empty!");
	}
	
	printf("Actions:\n");
	
	for (i = 0; i < ac; i++)
	{
		printf("%d - %s ", i, am[i].name);
		if (ACT_IS_URGENT(am[i])) printf("urgent ");
		printf("\n");
	}
}

void
act_mark_used(Act_idx i)
{
#ifdef VERBOSE
	printf("Marking action %d as used\n", i);
#endif
	if (!uact)
		uact = smalloc_zero(sizeof(bool)*act_cnt);

	uact[i] = true;
}

/*****************/
/*** PRZEJSCIA ***/
/*****************/

void
trans_append(Loc_idx src, Loc_idx dst, Act_idx action)
{
	Trans_set **ts;
	Trans_set *new_trans;
	Trans_set *cur;

	if (!trs) /* jesli nie ma tablicy tranzycji, to ja tworzymy */
		trs = smalloc_zero(sizeof(Trans_set *)*loc_cnt*loc_cnt);
	
	act_mark_used(action);
	
	new_trans = smalloc(sizeof(Trans_set));
	new_trans->act = action;
	new_trans->guard = constrs_mkarr();
	new_trans->nclocks = clocks_cnt; /* clocks_mkarr() zeruje clocks_cnt, dlatego najpierw zapisujemy _cnt */
	new_trans->clocks = clocks_mkarr();
	new_trans->next = NULL;
	
	ts = TRANS_P(trs, loc_cnt, src, dst); /* bierzemy odpowiedni wskaznik do wskaznika na pierwsza tranzycje */
	if (!*ts)
		*ts = new_trans;
	else
	{
		cur = *ts;
		while (cur->next) cur = cur->next;
		cur->next = new_trans;
	}

}

void
trans_show(Trans_set **t, Loc *lm, Loc_idx nl, Clock *cm, Act *am)
{
	Loc_idx i, j;
	Trans_set **ts;
	Trans_set *cur; /* mozna pozbyc sie tej zmiennej ;) */
	
	if (!t)
	{
		FERROR("No transitions recorded!");
	}
	
	printf("Transitions:\n");
	
	for (i = 0; i < nl; i++)
	{
		for (j = 0; j < nl; j++)
		{

			ts = TRANS_P(t, nl, i, j);

			if (*ts)
			{
				cur = *ts;
				do {
					printf("Action: %s ", get_act_name(am, cur->act));
					if (ACT_IS_URGENT(am[cur->act])) printf("(urgent)");
					printf("\n");
					printf("%s -> %s\n", get_loc_name(lm, i), get_loc_name(lm, j));
					printf("Guard: ");
					constrs_show(cur->guard, cm);
					printf("\n");
					printf("Reset clocks: ");
					trans_clocks_show(cur, cm);
					cur = cur->next;
				} while (cur);
				
			}
			
		}
	}
}

/* 
 * Wyswietlanie zegarow do zresetowania ze wskazana tranzycja
 */
void
trans_clocks_show(Trans_set *t, Clock *cm)
{		
	int i;
	Clock_idx *c = t->clocks;

	for (i = 0; i < t->nclocks; i++) {
		printf("%s ", get_clock_name(cm, c[i]));
	}
	printf("\n");
}

void
cur_trans_show(void)
{
	trans_show(trs, loc_map, loc_cnt, clocks_map, act_map);
}

/*****************/
/*** Wlasnosci ***/
/*****************/

/*
 * tutaj powinien byc tworzony element Property_set z powstalej
 * listy tymczasowej elementow ProdConf_set.
 */
void 
property_got_one(char *name)
{
	Property_set *new;
	
	new = smalloc(sizeof(Property_set));
	new->prop = prodconf_root; prodconf_root = NULL;
	new->name = name;
	new->next = NULL;
	
	if (!properties_root)
		properties_root = new;
	else
		properties_cur->next = new;
	properties_cur = new;
}

/*
 * tutaj powinien byc tworzony element ProdConf_set
 * i dodawany do listy tych elementow
 */
void
property_append_loc(char *loc, char *aut)
{
	Aut_idx ai;
	ProdConf_set *new;
	
	if (!autnet.aut) FERROR("Define network first!");

	ai = get_aut_idx(aut);
	
	new = smalloc(sizeof(ProdConf_set));
	new->aut = ai;
	new->loc = get_loc_idx(autnet.aut[ai].loc, autnet.aut[ai].nloc, loc);
	new->next = NULL;
	
	if (!prodconf_root)
		prodconf_root = new;
	else
		prodconf_cur->next = new;
	prodconf_cur = new;
	
	free(loc);
	free(aut);
}

/***************/
/*** AUTOMAT ***/
/***************/

Aut_idx
get_aut_idx(char *name)
{
	Aut_idx i;
	
	assert(autnet.aut != NULL);
	
	for (i = 0; i < autnet.naut; i++)
	{
		if (!strcmp(autnet.aut[i].name, name)) return i;
	}
	
	printf("Automaton %s not found\n", name);
	FERROR("Unknown automaton!");
}

void
complete_automaton(char *name)
{
	Aut_set *n;

	n = smalloc(sizeof(Aut_set));

	n->name = name;
	n->nclks = aut_clocks_cnt; aut_clocks_cnt = 0;
	n->clks = clocks_map; clocks_map = NULL;
	n->nloc = loc_cnt; loc_cnt = 0;
	n->loc = loc_map; loc_map = NULL;
	if (!got_initial_loc) /* brak lokacji poczatkowej! */
	{
		FERROR("Initial location not defined!");
	}
	else 
	{
		n->init_loc = init_loc;
		got_initial_loc = false;
	}
	n->trans = trs; trs = NULL;
	n->uact = uact; uact = NULL;
	n->next = NULL;
	
	if (!aut_cur)
		aut_root = n;
	else
		aut_cur->next = n;
	aut_cur = n;
	
	aut_cnt++;
		
#ifdef VERBOSE
	printf("Got complete automaton %s.\n", name);
	clocks_show(n->clks, n->nclks);
	locations_show(n->loc, n->nloc, n->clks);
	trans_show(n->trans, n->loc, n->nloc, n->clks, act_map);
#endif
}

void
complete_net(void)
{
	Aut_set *cur = aut_root, *next;
	Aut *map = NULL;

	autnet.aut = smalloc(sizeof(Aut) * aut_cnt);
	autnet.naut = aut_cnt;
	autnet.act = act_map; act_map = NULL;
	autnet.nact = act_cnt; act_cnt = 0;

	map = autnet.aut;	
	while (cur)
	{
		next = cur->next;
		map->name = cur->name;
		map->nclks = cur->nclks;
		map->clks = cur->clks;
		map->nloc = cur->nloc;
		map->loc = cur->loc;
		map->init_loc = cur->init_loc;
		map->trans = cur->trans;
		map->uact = cur->uact;
		map++;
		free(cur); /* TODO: co jeszcze zwalniac? */
		cur = next;
	}
	
	aut_root = aut_cur = NULL;
	
#ifdef VERBOSE	
	printf("Read complete network!\n");
#endif
}

AutNet
get_autnet(void)
{
	autnet.ppts = properties_root; /* zwracamy dodatkowo wlasnosci do zweryfikowania */
	return autnet;
}

