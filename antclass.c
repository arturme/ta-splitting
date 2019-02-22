/** antclass.c **/

/*
 * Manipulacje klasami abstrakcyjnymi, tworzenie nowych,
 * modyfikowanie ich parametrow, sprawdzanie, etc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "antclass.h"
#include "anpsmod.h"
#include "anproc.h"
#include "antypes.h"
#include "anmacro.h"
#include "macro.h"
#include "dbm.h"

/*
 * Tworzenie klasy
 */
TimedClass_set *
tclass_create(ProdLoc_set *loc, DBM_elem *zone, DBM_elem *cor, dpt_t depth)
{
	TimedClass_set *new_tclass;
	
	new_tclass = tclass_create_nd(loc, zone, cor);
	new_tclass->depth = depth;
	
	return new_tclass;
}
 
/*
 * Tworzenie klasy bez okreslania glebokosci
 */
TimedClass_set *
tclass_create_nd(ProdLoc_set *loc, DBM_elem *zone, DBM_elem *cor)
{
	TimedClass_set *new_tclass;
	
	new_tclass = smalloc(sizeof(TimedClass_set));
	new_tclass->dbm = zone;
	new_tclass->cor_dbm = cor;
	/*new_tclass->depth = depth;*/
	new_tclass->loc_set = loc;
	new_tclass->rs = NULL;
	new_tclass->stable_pre = NULL;
	new_tclass->stable_succ = NULL;
	new_tclass->next = NULL;
	
	return new_tclass;
}

/*
 * Tworzenie domyslnej klasy wyjsciowej dla odkrytej lokacji  
 */
TimedClass_set *
tclass_create_base(ProdLoc_set *loc)
{
	/* Nowa klasa rowna niezmiennikowi (z cor rownym rowniez niezmiennikowi) */
	return tclass_create(loc, NULL, NULL, DEPTH_INF);
}

/*
 * Dodawanie klasy do zbioru klas (przyczepianie do odpowiedniej lokacji)
 */
void
tclass_add(TimedClass_set *tclass)
{
	ProdLoc_set *loc;

#ifdef TC_VERBOSE
	printf("tclass_add (%p): ", tclass); tclass_print(tclass); printf("\n");
#endif
	
	loc = tclass->loc_set;
	
	if (!loc->classes)
		loc->classes = tclass;
	else
		loc->last_class->next = tclass;
	loc->last_class = tclass;
}

/*
 * Usuwanie klasy ze zbioru wszystkich klas nalezacych do podzialu
 * 
 * przez koniecznosc szukania poprzedniego elementu listy moze byc 
 * nieco bolesne, ale oszczedzamy troche pamieci ;)
 * 
 * W praktyce na razie funkcja okazuje sie raczej nieprzydatna, bo
 * wykorzystujemy istniejace klasy i zmieniamy w nich strefy.
 * 
 * UWAGA: brakuje usuwania informacji o klasach stabilnych (trzeba
 * zwolnic pamiec).
 */
// void
// tclass_del(TimedClass_set *tclass)
// {
// 	ProdLoc_set *loc;
// 	TimedClass_set *tmp, *prev = NULL;
// 	
// 	loc = tclass->loc_set;
// 	
// 	for (tmp = loc->classes; tmp; prev = tmp, tmp = tmp->next)
// 	{
// 		if (tmp == tclass)
// 		{
// 			if (prev) /* usuwamy dalej niz z poczatku */
// 			{
// 				prev->next = tmp->next;
// 				
// 				/* usuwamy z konca */
// 				if (tmp == loc->last_class)
// 					loc->last_class = prev;
// 			}
// 			else /* usuwamy z poczatku */
// 			{
// 				loc->classes = tmp->next;
// 				/* tutaj nie aktualizujemy last_class, bo jak korzen 
// 				 * bedzie NULL, to nie patrzymy na last_class.
// 				 */
// 			}
// 			
// 			if (tmp->dbm) dbm_destroy(tmp->dbm);
// 			if (tmp->cor_dbm) dbm_destroy(tmp->cor_dbm);
// 			free(tmp);
// 			
// 			return;
// 		}
// 	}
// 	
// 	assert(0); /* jesli jestesmy tutaj, to nie znalezlismy klasy do usuniecia */
// }

void
tclass_print(TimedClass_set *tc)
{
	printf("{"); prod_loc_show(tc->loc_set->loc);
	printf(" Z=");
	dbm_xprint(tclass_readZoneDBM(tc), dbm_size);
	if (!tc->dbm) printf("=(inv)");
	printf(" Cor=");
	if (tc->cor_dbm)
		dbm_xprint(tc->cor_dbm, dbm_size);
	else
		printf("(zone)");
	printf(" dpt=");
	if (tc->depth != DEPTH_INF)
		printf("%u", tc->depth);	
	else
		printf("inf");
	printf("} ");
}

/* 
 * Funkcja zwracajaca adres DBM ze strefa klasy w celu jego ODCZYTANIA
 * UWAGA: nie jest tworzona kopia, uwazac zeby nie zmodyfikowac!
 */
DBM_elem *
tclass_readZoneDBM(TimedClass_set *tclass)
{
	if (!tclass->dbm) return tclass->loc_set->inv;
	else return tclass->dbm;
}

/* 
 * Funkcja zwracajaca adres DBM z cor klasy w celu jego ODCZYTANIA
 * UWAGA: nie jest tworzona kopia, uwazac zeby nie zmodyfikowac!
 */
DBM_elem *
tclass_readCorDBM(TimedClass_set *tclass)
{
	 /* jesli cor == NULL, to jest taki sam jak zone */
	if (!tclass->cor_dbm) return tclass_readZoneDBM(tclass);
	else return tclass->cor_dbm; /* cor ustawiony jawnie (niedomyslny) */
}

/*
 * Funkcja zwracajaca strefe klasy gotowa do modyfikowania
 */
DBM_elem *
tclass_writeZoneDBM(TimedClass_set *tclass)
{
	if (!tclass->dbm) tclass->dbm = dbm_copy(tclass->loc_set->inv, dbm_size);
	
	return tclass->dbm;
}

/*
 * Funkcja zwracajaca cor klasy gotowy do modyfikowania
 */
DBM_elem *
tclass_writeCorDBM(TimedClass_set *tclass)
{
	if (!tclass->cor_dbm) tclass->cor_dbm = dbm_copy(tclass_readZoneDBM(tclass), dbm_size);
	
	return tclass->cor_dbm;
}

/*
 * Funkcje pomocnicze do przenoszenia DBM-ow do nowych klas
 * 		(unikanie kopiowania)
 * 
 * Po uzyciu ktorejs z nich klasa jest zdegenerowana,
 * tzn. cor i zone moga nie odpowiadac wlasciwej klasie.
 */
/*
DBM_elem *
tclass_consumeZoneDBM(TimedClass_set *tclass)
{
	DBM_elem *rlt;
	
	rlt = tclass_writeZoneDBM(tclass);
	tclass->dbm = NULL;
	
	return rlt;
}

DBM_elem *
tclass_consumeCorDBM(TimedClass_set *tclass)
{
	DBM_elem *rlt;
	
	rlt = tclass_writeCorDBM(tclass);
	tclass->cor_dbm = NULL;
	
	return rlt;
}
*/

/*
 * Podmienianie strefy klasy, cor domyslnie taki sam jak calosc klasy
 */
void
tclass_replaceZoneDBM(TimedClass_set *tclass, DBM_elem *new_dbm)
{
	DBM_elem *tmp;
	
	assert(!dbm_empty(new_dbm, dbm_size));
	
#ifdef TC_VERBOSE
	printf("tclass_replaceZoneDBM: nowy zone dla klasy (%p) ", tclass); tclass_print(tclass); 
	printf("; nowy zone: "); dbm_xprint(new_dbm, dbm_size); printf("\n");
#endif
	
	tmp = tclass->dbm;
	tclass->dbm = dbm_copy(new_dbm, dbm_size);
	if (tmp) dbm_destroy(tmp);

	if (tclass->cor_dbm) dbm_destroy(tclass->cor_dbm);
	tclass->cor_dbm = NULL;
}

/*
 * Podmienianie cor klasy
 */
void
tclass_replaceCorDBM(TimedClass_set *tclass, DBM_elem *new_cor)
{
	DBM_elem *tmp;
	
	assert(!dbm_empty(new_cor, dbm_size));

	tmp = tclass->cor_dbm;
	tclass->cor_dbm = dbm_copy(new_cor, dbm_size);
	
	if (tmp) dbm_destroy(tmp);
}

void
tclass_depthUpdateIncr(TimedClass_set *tclass, TimedClass_set *pre_tclass)
{
	dpt_t new_depth;
	
	if (pre_tclass->depth < DEPTH_INF-1)
	{
		new_depth = pre_tclass->depth+1;
		if (new_depth < tclass->depth)
		{
			tclass->depth = new_depth;
		}
	}
	else
	{
		FERROR("Depth overflow!");
	}
}

/*
 * Ustawianie glebokosci klasy na inf jesli nie zawiera
 * ona stanu poczatkowego q0
 */
void
tclass_depthUpdateInf(TimedClass_set *tclass)
{
	if (!tclass_hasInit(tclass))
	{
		assert(!tclass->rs); /* jak inf, to nie moze byc w RS */
		tclass->depth = DEPTH_INF;
	}
}

/*
 * Sprawdzanie czy klasa zawiera stan poczatkowy q0
 */
bool
tclass_hasInit(TimedClass_set *tclass)
{
	DBM_elem *tmp;
	bool has_init = false;
	
	if (tclass->loc_set == init_loc_set) /* zgodnosc lokacji */
	{
		/*dbm_print(tclass_readZoneDBM(tclass), dbm_size);*/
		
		assert(dbm_is_canonical(tclass_readZoneDBM(tclass), dbm_size));
		
		tmp = dbm_intersection_cf(tclass_readZoneDBM(tclass), zeroDBM, dbm_size);
		if (!dbm_empty(tmp, dbm_size)) has_init = true;
		dbm_destroy(tmp);
	}
	
	return has_init;
}

/*
 * Do poprawy. Trzeba troche zmienic struktury danych, bo to szukanie jest mordercze.
 */
bool
tclass_is_in_ReachStable(TimedClass_set *tclass)
{
	if (tclass->rs) return true;
	else return false;
}

/*
 * Dodawanie klasy do zbioru reachable+stable klas
 */
void
tclass_ReachStable_add(TimedClass_set *new_class, TimedClassRS_set **root, TimedClassRS_set **cur)
{
	assert(new_class->depth != DEPTH_INF);
	assert(tclass_ReachStable_is_sorted(*root));
	
	if (!tclass_is_in_ReachStable(new_class))
	{
		TimedClassRS_set *new, *pred;	
		
		new = smalloc(sizeof(TimedClassRS_set));
		new->tclass = new_class;
		new->stable = false;
		
		new_class->rs = new;
	
		if (!*root)
		{
			new->prev = NULL;
			new->next = NULL;
			*root = *cur = new;
		}
		else
		{
			/* cofamy sie do elementu o glebokosci nie wiekszej od naszej (po ktorym chcemy dodac nowy element) > */
			for (pred = *cur; pred->tclass->depth > new_class->depth; pred = pred->prev);

			new->next = pred->next;
			new->prev = pred; /* poprzedni element to ostatnio dodany */
			if (pred->next) {
				pred->next->prev = new;
			}
			pred->next = new;
			
			if (pred == *cur) *cur = new;
		}
		
	}
	
	assert(tclass_ReachStable_is_sorted(*root));
}

/*
 * Dodawanie klasy zawierajacej stan q0 do zbioru
 */
void
tclass_ReachStable_addInit(TimedClass_set *new_class, TimedClassRS_set **root, TimedClassRS_set **cur)
{
	TimedClassRS_set *new;
	
	if (!tclass_is_in_ReachStable(new_class))
	{	
		assert(new_class->depth != DEPTH_INF);
	
		new = smalloc(sizeof(TimedClassRS_set));
		new->tclass = new_class;
		new->stable = false;
		new->next = *root;	/* pod nowy element podczepiamy korzen */
		new->prev = NULL;	/* bo dodajemy na poczatku */

		new_class->rs = new;

		if (!*root) /* jesli lista byla pusta, to trzeba zaktualizowac cur, bo nowy bedzie ostatni */
		{
			*cur = new;
		}
		else
		{
			assert(*root != NULL);
			(*root)->prev = new;
		}
		*root = new; /* lista zaczyna sie od nowego elementu */
	
		assert(tclass_ReachStable_is_sorted(*root));
	}
}

/*
 * Usuwanie klasy ze zbioru klas
 */
void
tclass_ReachStable_delete(TimedClass_set *del_class, TimedClassRS_set **root, TimedClassRS_set **cur)
{	
#ifdef TC_VERBOSE
	printf("Usuwam klase (%p) ze zbioru ReachStable: ", del_class);
	tclass_print(del_class);
#endif

	if (tclass_is_in_ReachStable(del_class))
	{
		TimedClassRS_set *tmpRS;
		
		tmpRS = del_class->rs;

		if (tmpRS->next)
			tmpRS->next->prev = tmpRS->prev;
		if (tmpRS->prev)
			tmpRS->prev->next = tmpRS->next;

		if (tmpRS == *root)
			*root = NULL;
		else
			if (tmpRS == *cur) *cur = tmpRS->prev;

		free(tmpRS);
		del_class->rs = NULL;
		
#ifdef TC_VERBOSE
		printf("faktycznie usuwana\n");
		tclass_ReachStable_print(*root);
#endif	

	}
#ifdef TC_VERBOSE
	else
	{
		printf("nie bylo co usuwac\n");
	}
#endif
}

/*
 * Zwalnianie pamieci przydzielonej na strukture zbioru (bez faktycznych klas)
 */
/*void
tclass_ReachStable_free(TimedClassRS_set **root)
{
}*/

/*
 * Oznaczanie klasy jako osiagalna i niestabilna (czyli dodawanie do zbioru)
 */
void
tclass_mark_reach_unst(TimedClass_set *tclass, TimedClassRS_set **root, TimedClassRS_set **cur)
{
	tclass_ReachStable_add(tclass, root, cur);
}

/*
 * Oznaczanie klasy jako osiagalna (dodawanie klasy do zbioru reachable)
 */
void
tclass_rs_mark_stable(TimedClassRS_set *tclass_rs)
{
	tclass_rs->stable = true;
}

/*
 * Znajdowanie klasy tclass i oznaczanie jako stabilna
 */
void
tclass_mark_stable(TimedClass_set *tclass)
{
	assert(tclass->rs != NULL);
	
	tclass->rs->stable = true;
}

/*
 * Znajdowanie klasy tclass i oznaczanie jako niestabilna
 */
void
tclass_mark_unstable(TimedClass_set *tclass)
{
	if (tclass->rs)	
		tclass->rs->stable = false;
}

/*
 * Oznaczanie wszystkich poprzednikow danej klasy jako niestabilne 
 */
void
tclass_mark_preds_unstable(TimedClass_set *tclass)
{
	SrcLoc_set *src_loc;
	TimedClass_set *tclass_src;
	ProdTrans_set *trans_e;

	for (src_loc = tclass->loc_set->src_locs; src_loc; src_loc = src_loc->next)
	{
		for (tclass_src = src_loc->loc_set->classes; tclass_src; tclass_src = tclass_src->next)
		{
			for (trans_e = src_loc->trans; trans_e; trans_e = trans_e->next)
			{
				if (!psmodel_pre_empty(trans_e, src_loc->loc_set, tclass->loc_set,
					tclass_readZoneDBM(tclass_src), tclass_readZoneDBM(tclass)))
				{
					tclass_mark_unstable(tclass_src);
				}
			}
		}
	}
}

TimedClassRS_set *
tclass_pickReachUnst(TimedClassRS_set *root)
{
	TimedClassRS_set *curRS;
	
	assert(tclass_ReachStable_is_sorted(root));
	
	/* szukamy pierwszego niestabilnego elementu (od korzenia) */
	for (curRS = root; curRS; curRS = curRS->next)
	{
		if (curRS->stable == false)
		{
			return curRS;
		}
	}

	return NULL;
}

/*
 * Sprawdzanie czy lista ReachStable jest posortowana
 */
bool
tclass_ReachStable_is_sorted(TimedClassRS_set *root)
{
	TimedClassRS_set *curRS;
	dpt_t min_depth = 0;
	
	for (curRS = root; curRS; curRS = curRS->next)
	{
		if (curRS->tclass->depth < min_depth)
			return false;
		
		min_depth = curRS->tclass->depth;
	}
	
	return true;
}

/*
 * Wyswietlanie wszystkich stabilnych klas nalezacych do zbioru RS
 */
void
tclass_ReachStable_print(TimedClassRS_set *root)
{
	TimedClassRS_set *curRS;
	idx_t i;
	
	for (curRS = root, i = 1; curRS; curRS = curRS->next, i++)
	{
		printf("(%u) (%p) ", i, curRS->tclass); tclass_print(curRS->tclass);
		if (!curRS->stable) printf("UNSTABLE!!!");
		printf("\n");		
	}
}

/*
 * Zapamietywanie stabilnego nastepnika
 */
void
tclass_saveStable(TimedClass_set *tcX, TimedClass_set *tcY)
{
	TClassPtr_set *new;
	
	assert(tclass_isReallyStableSucc(tcX, tcY));
	
	if (!tclass_is_in_tcPtrs(tcY, tcX->stable_succ))
	{
		new = smalloc(sizeof(TClassPtr_set));
		new->tclass = tcY;
		new->next = tcX->stable_succ;
		tcX->stable_succ = new;
	}
	
	if (!tclass_is_in_tcPtrs(tcX, tcY->stable_pre))
	{
		new = smalloc(sizeof(TClassPtr_set));
		new->tclass = tcX;
		new->next = tcY->stable_pre;
		tcY->stable_pre = new;
	}
}

/*
 * Usuwanie informacji o poprzednikach stabilnych
 * wzgledem danej klasy tcY.
 */
void
tclass_forgetPreStability(TimedClass_set *tcY)
{
	TClassPtr_set *tcptr;

	if (tcY->stable_pre) 
	{	
		for (tcptr = tcY->stable_pre; tcptr; )
		{
			TClassPtr_set *tmp;
				
			/* zagladamy do kazdego poprzednika i usuwamy sie z jego stable_succ */
			tclass_tcPtrDel(tcY, &tcptr->tclass->stable_succ);
		
			tmp = tcptr;
			tcptr = tcptr->next;
			free(tmp);
		}
		tcY->stable_pre = NULL;
	}
}

/*
 * Usuwanie informacji o nastepnikach wzgledem
 * ktorych jest stabilna dana klasa tcX.
 */
void
tclass_forgetSuccStability(TimedClass_set *tcX)
{
	TClassPtr_set *tcptr;
	
	if (tcX->stable_succ)
	{
		for (tcptr = tcX->stable_succ; tcptr; )
		{
			TClassPtr_set *tmp;
	
			/* zagladamy do kazdego nastepnika i usuwamy sie z jego stable_pre */
			tclass_tcPtrDel(tcX, &tcptr->tclass->stable_pre);

			tmp = tcptr;		
			tcptr = tcptr->next;
			free(tmp);
		}
		tcX->stable_succ = NULL;
	}
}

void
tclass_tcPtrDel(TimedClass_set *tc, TClassPtr_set **tcptr)
{
	TClassPtr_set *tmp, *prev = NULL;
	
	for (tmp = *tcptr; tmp; prev = tmp, tmp = tmp->next)
	{
		if (tmp->tclass == tc)
		{
			if (prev)
				prev->next = tmp->next;
			else
				*tcptr = tmp->next;
		
			free(tmp);
		
			return;
		}
	}
}

bool
tclass_is_in_tcPtrs(TimedClass_set *tc, TClassPtr_set *tcptr)
{
	TClassPtr_set *tmp;
	
	for (tmp = tcptr; tmp; tmp = tmp->next)
		if (tmp->tclass == tc) return true;
		
	return false;
}

/*
 * Sprawdzanie czy klasa tcY jest stabilnym nastepnikiem tcX
 */
bool
tclass_isStableSucc(TimedClass_set *tcX, TimedClass_set *tcY)
{
	if(tclass_is_in_tcPtrs(tcY, tcX->stable_succ))
	{
		assert(tclass_isReallyStableSucc(tcX, tcY));
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * Sprawdzanie czy faktycznie mamy wlasciwie zapamietana stabilnosc
 */
bool
tclass_isReallyStableSucc(TimedClass_set *tcX, TimedClass_set *tcY)
{
	ProdTrans_set *trans_h;

	/* W rzeczywistosci interesuje nas tylko istnienie jakiegos przejscia dla ktorego jest AE */
	for (trans_h = trans_get(tcX->loc_set, tcY->loc_set); trans_h; trans_h = trans_h->next)
	{
		if (psmodel_pre_eq(tclass_readCorDBM(tcX), trans_h, tcX->loc_set, tcY->loc_set,
							tclass_readCorDBM(tcX), tclass_readCorDBM(tcY)))
			return true;
	}
	
	return true;
}

unsigned int
tclass_count(ProdLoc_set *locs)
{
	ProdLoc_set *loc;
	unsigned int nclasses = 0;
	
	for (loc = locs; loc; loc = loc->next)
	{
		TimedClass_set *tclass;

		for (tclass = loc->classes; tclass; tclass = tclass->next)
		{
			nclasses++;
		}
	}
	
	return nclasses;
}
