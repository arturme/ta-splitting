/** anpsmod.c **/

/*
 * Tutaj jest splitting. Zaczyna sie od psmodel_builder.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "anpsmod.h"
#include "antclass.h"
#include "anproc.h"
#include "antypes.h"
#include "anmacro.h"
#include "macro.h"
#include "dbm.h"

#ifdef PSM_VERBOSE
static unsigned int stTests;
static unsigned int skip_stTests;
#endif

/*
 * Obliczanie pre_e.
 * 
 * (nie ma sprawdzania zgodnosci przejscia miedzy lokacjami, bo to powinno byc wiadomo
 * zanim zostanie wywolana funkcja)
 *
 * Z n time_pre( [Y:=0]( time_pre(Z') n I(s') ) n guard(e) n I(s) )
 */
DBM_elem *
psmodel_pre(ProdTrans_set *trans, ProdLoc_set *loc_src, ProdLoc_set *loc_dst, DBM_elem *z_src, DBM_elem *z_dst)
{
	DBM_elem *guard_inv, *tmp_dbm;

	guard_inv = dbm_intersection_cf(trans->guard, loc_src->inv, dbm_size);
	if (dbm_empty(guard_inv, dbm_size))
	{
		tmp_dbm = guard_inv;
	}
	else
	{
		tmp_dbm = dbm_copy(z_dst, dbm_size);
		dbm_time_predecessor(tmp_dbm, dbm_size);
		dbm_intersection_cf_ip(tmp_dbm, loc_dst->inv, dbm_size); /* modyfikacja w miejscu tmp_dbm */
		if (!dbm_empty(tmp_dbm, dbm_size))
		{
			/* tmp_dbm := [reset(trans) := 0]tmp_dbm */
			ClockIdx_set *cur_clock;

			for (cur_clock = trans->clocks; cur_clock; cur_clock = cur_clock->next)
				dbm_invreset(tmp_dbm, dbm_size, cur_clock->idx);

			dbm_intersection_cf_ip(tmp_dbm, guard_inv, dbm_size);
			if (!dbm_empty(tmp_dbm, dbm_size))
			{
				dbm_time_predecessor(tmp_dbm, dbm_size);				
				dbm_intersection_cf_ip(tmp_dbm, z_src, dbm_size);
			}
		}
		dbm_destroy(guard_inv);
	}
	
	return tmp_dbm;
}

/*
 * Porownywanie wyniku pre_e z konkretna strefa
 */
bool
psmodel_pre_eq(DBM_elem *z_cmp, ProdTrans_set *trans, ProdLoc_set *loc_src, ProdLoc_set *loc_dst,
               DBM_elem *z_src, DBM_elem *z_dst)
{
	bool are_equal;
	DBM_elem *rlt; 
	
	rlt = psmodel_pre(trans, loc_src, loc_dst, z_src, z_dst);
	
	are_equal = dbm_equal(rlt, z_cmp, dbm_size);
	dbm_destroy(rlt);
	
	return are_equal;
}

/*
 * Sprawdzanie pustosci wyniku operacji pre_e
 */
bool
psmodel_pre_empty(ProdTrans_set *trans, ProdLoc_set *loc_src, ProdLoc_set *loc_dst, 
				  DBM_elem *z_src, DBM_elem *z_dst)
{
	bool is_empty;
	DBM_elem *rlt;
	
	rlt = psmodel_pre(trans, loc_src, loc_dst, z_src, z_dst);
	
	is_empty = dbm_empty(rlt, dbm_size);
	dbm_destroy(rlt);
	
	return is_empty;
}

/*
 * Funkcja wykorzystywana przy Post
 */
DBM_elem *
psmodel_post(ProdTrans_set *trans, ProdLoc_set *loc_src, DBM_elem *z_src, DBM_elem *z_dst)
{
	DBM_elem *tmp_dbm;
	
	tmp_dbm = dbm_copy(z_src, dbm_size);
	dbm_time_successor(tmp_dbm, dbm_size);
	dbm_intersection_cf_ip(tmp_dbm, loc_src->inv, dbm_size);
	if (!dbm_empty(tmp_dbm, dbm_size))
	{
		dbm_intersection_cf_ip(tmp_dbm, trans->guard, dbm_size);
		if (!dbm_empty(tmp_dbm, dbm_size))
		{
			ClockIdx_set *cur_clock;
			
			for (cur_clock = trans->clocks; cur_clock; cur_clock = cur_clock->next)
				dbm_reset(tmp_dbm, dbm_size, cur_clock->idx);
			
			dbm_time_successor(tmp_dbm, dbm_size);
			dbm_intersection_cf_ip(tmp_dbm, z_dst, dbm_size);
		}
	}
	
	return tmp_dbm;
}

bool
psmodel_post_empty(ProdTrans_set *trans, ProdLoc_set *loc_src, DBM_elem *z_src, DBM_elem *z_dst)
{
	bool is_empty;
	DBM_elem *rlt;
	
	rlt = psmodel_post(trans, loc_src, z_src, z_dst);
	
	is_empty = dbm_empty(rlt, dbm_size);
	dbm_destroy(rlt);
	
	return is_empty;
}

/* 
 * Sprawdzanie stabilnosci danej klasy ze wzgledu na mozliwe nastepniki,
 * a nastepnie ewentualna stabilizacja.
 */
void
psmodel_split(TimedClass_set *tcX, TimedClassRS_set **reach_stable, TimedClassRS_set **reach_stable_cur)
{
	TimedClass_set *tcY;
	DstLoc_set *tcY_dstloc;
	ProdLoc_set *tcX_prodloc;
	DBM_elem *tcX_dbm, *tcY_dbm;
	ProdTrans_set *trans_e;
		
	tcX_prodloc = tcX->loc_set;
	tcX_dbm = tclass_readZoneDBM(tcX);
	
	assert(tcX->depth != DEPTH_INF);
	
	/*
	 * Biore wszystkie klasy Y t.z. da sie do nich przejsc z X (potencjalnie), a nastepnie
	 * szukam przejsc e \in E t.z. pre_e(X,Y) != 0. 
	 * 
	 * Dla przyspieszenia bierzemy pod uwage potencjalna mozliwosc przejscia. Wystepuje ona dla
	 * klas zwiazanych z lokacja do ktorych prowadzi przejscie z lokacji zwiazanej z aktualnie
	 * przetwarzana klasa.
	 */

	/** SZUKAMY WSZYSTKICH MOZLIWYCH NASTEPNIKOW KLASY X **/
	/* Przechodzimy przez wszystkie lokacje (docelowe - dst_locs) z location(X) */
	for (tcY_dstloc = tcX_prodloc->dst_locs; tcY_dstloc; tcY_dstloc = tcY_dstloc->next)
	{		
		/*
		 * Bierzemy po kolei wszystkie klasy zwiazane z location(Y),
		 * ktore sa potencjalnymi klasami docelowymi
		 */
		for (tcY = tcY_dstloc->loc_set->classes; tcY; tcY = tcY->next)
		{	
			tcY_dbm = tclass_readZoneDBM(tcY);
			
			/* Bierzemy wszystkie przejscia mozliwe DO location(Y)... */
			for (trans_e = tcY_dstloc->trans; trans_e; trans_e = trans_e->next)
			{
				/* ...i dla kazdego e (= trans_e) sprawdzamy czy pre_e(X,Y) != 0 */
				if (!psmodel_pre_empty(trans_e, tcX_prodloc, tcY_dstloc->loc_set, tcX_dbm, tcY_dbm))
				{
#ifdef PSM_VERBOSE
					printf("\nSprawdzam stabilnosc klasy (%p) ", tcX); tclass_print(tcX);
					printf("\n\tze wzgledu na klase "); tclass_print(tcY); printf("\n\n");
#endif
					assert(tcX->depth != DEPTH_INF);

					if (!psmodel_stabilityCheck(tcY, *reach_stable))
					{
						assert(tcY->depth >= tcX->depth+1);

						/* stabilizujemy klase tcX ze wzgl. na tcY oraz przejscie trans_e */
						psmodel_realSplit(tcX, tcY, trans_e, reach_stable, reach_stable_cur);

						return;						
					}
				}
			}

		}
	}			
	
	/* jak jestesmy tutaj, to stabilne (Split (X,Pi) = {X}): */
	psmodel_stableHandler(tcX, reach_stable, reach_stable_cur);
}

/*
 * Sprawdzanie stabilnosci wzgledem klasy Y
 * 
 * szukamy klasy X1 o minimalnej glebokosci z ktorej istnieje przejscie do klasy Y
 *
 * z klasy X1 do Y istnieje przejscie jak istnieje przejscie e miedzy location(X1) i location(Y)
 * oraz pre_e(X1,Y) != 0
 *  
 * location(X1) moze byc ktoras z lokacji nalezacych do zbioru lokacji z ktorych 
 * osiagalna jest location(Y)
 * 
 * jak znajdziemy takie przejscie to sprawdzamy stabilnosc
 */
bool
psmodel_stabilityCheck(TimedClass_set *tcY, TimedClassRS_set *reach_stable)
{
	TimedClassRS_set *curRS;
	dpt_t min_depth = DEPTH_INF;
	DBM_elem *tcY_dbm = NULL;

	assert(tclass_ReachStable_is_sorted(reach_stable)); /* polegamy na tym ze reach_stable jest posortowane */

#ifdef PSM_VERBOSE
	stTests++;
#endif

	/* zakladamy ze klasa ([q0], {q0}, 0) ma petle wlasna, wiec bedzie stabilna */
	if (tcY->depth == 0 && tclass_hasInit(tcY))
	{
		return true;
	}

	/* Bierzemy kolejne klasy X1 do sprawdzenia */
	for (curRS = reach_stable; curRS; curRS = curRS->next)
	{
		TimedClass_set *tcX1 = curRS->tclass;
		/* do sprawdzenia para lokacji i wszystkie przejscia miedzy nimi:  */
	
		if (tcX1->depth > min_depth)
		{
			return false; /* nie ma co dalej szukac, bo juz nie ma "minimalnych" */
		}
		
		/* Jesli znalazlo sie stabilne przejscie z wybranej klasy, to:
		 * - istnieje przejscie (z jakim h \in E)
		 * - zachodzi AE na rdzeniach
		 * - klasa ma minimalna glebokosc bo wybralismy ja z RS (sortowanie)
		 */
#ifndef NO_STCACHE
		if (tclass_isStableSucc(tcX1, tcY))
		{
#ifdef PSM_VERBOSE
			printf("Pominiety test stabilnosci\n");
			skip_stTests++;
#endif
			return true;
		}
#endif
		
		if (ploc_in_src_locs(tcX1->loc_set, tcY->loc_set->src_locs))
		{
			ProdTrans_set *trans_h;

			if (!tcY_dbm) tcY_dbm = tclass_readZoneDBM(tcY);
		
			/* Bierzemy kazde mozliwe przejscie z location(X1) do location(Y) */
			for (trans_h = trans_get(tcX1->loc_set, tcY->loc_set); trans_h; trans_h = trans_h->next)
			{
				if (!psmodel_pre_empty(trans_h, tcX1->loc_set, tcY->loc_set,
					tclass_readZoneDBM(tcX1), tcY_dbm))
				{
					/* znaleziono mozliwe przejscie z X1 do Y */
					DBM_elem *tcX1_corDBM;
				
					tcX1_corDBM = tclass_readCorDBM(tcX1);
				
					/* aktualizujemy min_depth, bo to bedzie nasze minimum */
					if (min_depth != tcX1->depth) 
						min_depth = tcX1->depth;
				
					/* Znalazlo sie przejscie! */
#ifdef PSM_VERBOSE
					printf("Mamy przejscie z klasy (%p) ", tcX1); tclass_print(tcX1);
					printf("\n\tdo klasy "); tclass_print(tcY);
#endif
					if (psmodel_pre_eq(tcX1_corDBM, trans_h, tcX1->loc_set, tcY->loc_set, 
						tcX1_corDBM, tclass_readCorDBM(tcY)))
					{
#ifdef PSM_VERBOSE
						printf("stabilna\n");
#endif

#ifndef NO_STCACHE
						/* zapamietujemy stabilnosc */
						tclass_saveStable(tcX1, tcY);
#endif
						return true;
					}
				}

			}
		
		}

	}
	
	return false;
}

/*
 * Obsluga przypadku, gdy split nie mial co robic
 */
void
psmodel_stableHandler(TimedClass_set *tcX, TimedClassRS_set **reach_stable, TimedClassRS_set **reach_stable_cur)
{
	DstLoc_set *dst_loc;
	TimedClass_set *tcY;
	ProdTrans_set *trans_e;
	
	/* oznaczanie klasy tcX jako stabilna */
	tclass_mark_stable(tcX);
	
	/* znajdowanie wszystkich nastepnikow klasy tcX */
	for (dst_loc = tcX->loc_set->dst_locs; dst_loc; dst_loc = dst_loc->next)
	{
		for (tcY = dst_loc->loc_set->classes; tcY; tcY = tcY->next)
		{
			for (trans_e = dst_loc->trans; trans_e; trans_e = trans_e->next)
			{
				if (!psmodel_post_empty(trans_e, tcX->loc_set, tclass_readZoneDBM(tcX), tclass_readZoneDBM(tcY)))
				{
					/* ustawianie dpt dla kazdego odnalezionego nastepnika */
					tclass_depthUpdateIncr(tcY, tcX);
				
					/* dodawanie do reachable */
					tclass_ReachStable_add(tcY, reach_stable, reach_stable_cur);
#ifdef PSM_VERBOSE
					printf("dodaje klase do reachable %p [%d]: ", trans_e, trans_e->act); tclass_print(tcY); printf("\n");
#endif
					/* TODO: tutaj testowanie spelniania wlasnosci - to sie zrobi ;) */
					if (ppty_check(tcY->loc_set->loc))
					{
						tclass_ReachStable_print(*reach_stable);
						exit(0);
					}
				}
			}
		}
	}
}

/*
 * Sprawdzenie warunku funkcji Sp na potrzeby asercji
 * 
 * Powinno zachodzic pre_e(X,Y) != 0 oraz pre_e(Xcor, Ycor) != Xcor
 */
bool
psmodel_realSplit_isDefined(TimedClass_set *tcX, TimedClass_set *tcY, ProdTrans_set *trans_e)
{
	if (psmodel_pre_empty(trans_e, tcX->loc_set, tcY->loc_set, tclass_readZoneDBM(tcX), tclass_readZoneDBM(tcY)))
	{
		printf("pre_e(X,Y) = 0\n");
		return false;
	}
	if (psmodel_pre_eq(tclass_readCorDBM(tcX), trans_e, tcX->loc_set, tcY->loc_set, tclass_readCorDBM(tcX), tclass_readCorDBM(tcY)))
	{
		printf("pre_e(Xcor, Ycor) = Xcor\n");
		return false;
	}
	
	return true;
}

/*
 * Split z rozpatrywaniem 4 przypadkow niestabilnosci
 */
void
psmodel_realSplit(TimedClass_set *tcX, TimedClass_set *tcY, ProdTrans_set *trans_e, 
				  TimedClassRS_set **reach_stable, TimedClassRS_set **reach_stable_cur)
{
	DBM_elem *pre_e_Xcor_Ycor;	
	DBM_elem *tcX_corDBM, *tcY_corDBM;
	
	assert(psmodel_realSplit_isDefined(tcX, tcY, trans_e)); /* bardzo wdzieczna assercja */
	
	tcX_corDBM = tclass_readCorDBM(tcX);
	tcY_corDBM = tclass_readCorDBM(tcY);
	
	pre_e_Xcor_Ycor = psmodel_pre(trans_e, tcX->loc_set, tcY->loc_set, tcX_corDBM, tcY_corDBM);
	
	if (!dbm_empty(pre_e_Xcor_Ycor, dbm_size))
	{
		/* jesli pre_e(Xcor,Ycor) != 0 to pseudo e-stabilny */
#ifdef PSM_VERBOSE
		printf("pseudo stable\n");
#endif	
		/*
		 * Przed aktualizacja stable aktualizujemy reachable, bo dzieki temu
		 * przeszukiwac bedziemy mniejszy zbior (stabilnosc oznaczamy na elementach
		 * reachable). Pi nie aktualizujemy, bo podmieniamy tylko cor klasy.
		 */
		tclass_ReachStable_delete(tcX, reach_stable, reach_stable_cur); /* usuwanie tcX z reachable i ze stable */
		tclass_mark_preds_unstable(tcX);
		/* tutaj tclass_mark_unstable(tcX, *reach_stable) nie mialoby sensu bo tcX juz jest wyrzucone */

#ifndef NO_STCACHE
		tclass_forgetPreStability(tcX);
#endif
		tclass_replaceCorDBM(tcX, pre_e_Xcor_Ycor); /* nowa klasa */
		tclass_depthUpdateInf(tcX); /* jesli nie zawiera q0 ustawiamy dpt(tcX) := inf */
		if (tclass_hasInit(tcX)) /* jesli zawiera q0, to dodajemy do reachable */
		{
			tclass_ReachStable_addInit(tcX, reach_stable, reach_stable_cur);
		}
		/* Nie zwalniamy pre_e_Xcor_Ycor - wykorzystane jako cor! */
	}
	else
	{
		/* pre_e(Xcor,Ycor) = 0 */
		DBM_elem *pre_e_X_Ycor, *tcX_zoneDBM;
		TimedClass_set *tcN;
		
		tcX_zoneDBM = tclass_readZoneDBM(tcX);
		pre_e_X_Ycor = psmodel_pre(trans_e, tcX->loc_set, tcY->loc_set, tcX_zoneDBM, tcY_corDBM);
		if (!dbm_empty(pre_e_X_Ycor, dbm_size))
		{
			DBMset_elem *dbms, *dbms_elem;
			
			/* jesli pre_e(X,Ycor) != 0 to pseudo e-niestabilny */
#ifdef PSM_VERBOSE
			printf("pseudo unstable\n");
#endif
			/* na podstawie zbioru tworzymy klasy Zone = X\Xcor, Cor = pre_e_X_Ycor */
			dbms = dbm_diff(tcX_zoneDBM, tcX_corDBM, dbm_size);
			for (dbms_elem = dbms; dbms_elem; dbms_elem = dbms_elem->next)
			{
				DBM_elem *new_cor;
				
				new_cor = dbm_intersection_cf(pre_e_X_Ycor, dbms_elem->dbm, dbm_size);
				if (dbm_empty(new_cor, dbm_size))
				{
					/*
					 * Jesli cor (pre_e_X_Ycor) nie zawiera sie w zone to ustawiamy,
					 * ze cor = zone (jak cor = NULL, to cor = zone).
					 */
					dbm_destroy(new_cor);
					new_cor = NULL;
				}

				tcN = tclass_create_nd(tcX->loc_set, dbms_elem->dbm, new_cor);
				tclass_depthUpdateInf(tcN);
				if (tclass_hasInit(tcN))
				{
					tclass_ReachStable_addInit(tcN, reach_stable, reach_stable_cur);
				}
				tclass_add(tcN);
			}
			dbmset_scaffoldDestroy(dbms);
			
			tclass_ReachStable_delete(tcX, reach_stable, reach_stable_cur);
			tclass_mark_preds_unstable(tcX);		
#ifndef NO_STCACHE
			tclass_forgetPreStability(tcX);
#endif
			tclass_replaceZoneDBM(tcX, tcX_corDBM); /* nowa klasa; cor rowny calej nowej klasie, tzn. cor == NULL */
			tclass_depthUpdateInf(tcX);
			if (tclass_hasInit(tcX))
			{
				tclass_ReachStable_addInit(tcX, reach_stable, reach_stable_cur);
			}
		}
		else
		{
			/* pre_e(X,Ycor) = 0 */
			DBM_elem *pre_e_Xcor_Y, *tcY_zoneDBM;
			
			tcY_zoneDBM = tclass_readZoneDBM(tcY);
			pre_e_Xcor_Y = psmodel_pre(trans_e, tcX->loc_set, tcY->loc_set, tcX_corDBM, tcY_zoneDBM);
			if (!dbm_empty(pre_e_Xcor_Y, dbm_size))
			{
				DBMset_elem *dbms, *dbms_elem;
				
				/* jesli pre_e(Xcor,Y) != 0 to semi e-niestabilny */
#ifdef PSM_VERBOSE
				printf("semi unstable\n");
#endif
				/* (X, pre_e_Xcor_Y, dpt), robimy podmiane i aktualizujemy */
				tclass_ReachStable_delete(tcX, reach_stable, reach_stable_cur);
				tclass_mark_preds_unstable(tcX);
#ifndef NO_STCACHE
				tclass_forgetPreStability(tcX);
#endif
				tclass_replaceCorDBM(tcX, pre_e_Xcor_Y); /* dalej zwolnic pre_e_Xcor_Y */
				tclass_depthUpdateInf(tcX);
				if (tclass_hasInit(tcX))
				{
					tclass_ReachStable_addInit(tcX, reach_stable, reach_stable_cur);
				}

				/* (Y\Ycor, Y\Ycor, dpt) */
				dbms = dbm_diff(tcY_zoneDBM, tcY_corDBM, dbm_size);
				for (dbms_elem = dbms; dbms_elem; dbms_elem = dbms_elem->next)
				{
					tcN = tclass_create_nd(tcY->loc_set, dbms_elem->dbm, NULL); /* NULL, bo cor = zone */
					tclass_depthUpdateInf(tcN);
					if (tclass_hasInit(tcN))
					{
						tclass_ReachStable_addInit(tcN, reach_stable, reach_stable_cur);
					}
					tclass_add(tcN);
				}
				dbmset_scaffoldDestroy(dbms);
				
				/* (Ycor, Ycor, dpt) */
				tclass_ReachStable_delete(tcY, reach_stable, reach_stable_cur);
				tclass_mark_preds_unstable(tcY);
#ifndef NO_STCACHE
				tclass_forgetPreStability(tcY);
#endif				
				tclass_replaceZoneDBM(tcY, tcY_corDBM); /* cor domyslnie rowny calej klasie */
				tclass_depthUpdateInf(tcY);
				if (tclass_hasInit(tcY))
				{
					tclass_ReachStable_addInit(tcY, reach_stable, reach_stable_cur);
				}
			}
			else
			{
				DBM_elem *pre_e_X_Y;
				DBMset_elem *dbms, *dbms_elem;
				/* pre_e(Xcor,Y) = 0 */
				
#ifdef PSM_VERBOSE
				printf("unstable\n");
#endif
				/* TODO: czasem niestety to obliczane jest dwa razy (wczesniej przy spr. mozliwosci przejscia) */
				pre_e_X_Y = psmodel_pre(trans_e, tcX->loc_set, tcY->loc_set, tcX_zoneDBM, tcY_zoneDBM);

				/* (X\pre_e_X_Y, Xcor, dpt) - uwaga podwojne wykorzystanie pre_e_X_Y - napisac komentarz! */
				dbms = dbm_diff(tcX_zoneDBM, pre_e_X_Y, dbm_size);
				for (dbms_elem = dbms; dbms_elem; dbms_elem = dbms_elem->next)
				{
					DBM_elem *new_cor;
					
					new_cor = dbm_intersection_cf(tcX_corDBM, dbms->dbm, dbm_size);
					if (dbm_empty(new_cor, dbm_size))
					{
						dbm_destroy(new_cor);
						new_cor = NULL;
					}
					tcN = tclass_create_nd(tcX->loc_set, dbms_elem->dbm, new_cor);
					tclass_depthUpdateInf(tcN);
					if (tclass_hasInit(tcN))
					{
						tclass_ReachStable_addInit(tcN, reach_stable, reach_stable_cur);
					}
					tclass_add(tcN);
				}
				dbmset_scaffoldDestroy(dbms);

				/* (pre_e_X_Y, pre_e_X_Y, dpt) */
				tclass_ReachStable_delete(tcX, reach_stable, reach_stable_cur);
				tclass_mark_preds_unstable(tcX);	
#ifndef NO_STCACHE
				tclass_forgetSuccStability(tcX);				
				tclass_forgetPreStability(tcX);
#endif
				tclass_replaceZoneDBM(tcX, pre_e_X_Y); /* nowa klasa; cor zostaje rowny zone, tzn. cor == NULL */
				dbm_destroy(pre_e_X_Y);
				tclass_depthUpdateInf(tcX);
				if (tclass_hasInit(tcX))
				{
					tclass_ReachStable_addInit(tcX, reach_stable, reach_stable_cur);
				}
				
				/* (Y\Ycor, Y\Ycor, dpt) */
				dbms = dbm_diff(tcY_zoneDBM, tcY_corDBM, dbm_size);
				for (dbms_elem = dbms; dbms_elem; dbms_elem = dbms_elem->next)
				{
					tcN = tclass_create_nd(tcY->loc_set, dbms_elem->dbm, NULL); /* NULL, bo cor = zone */
					tclass_depthUpdateInf(tcN);
					if (tclass_hasInit(tcN))
					{
						tclass_ReachStable_addInit(tcN, reach_stable, reach_stable_cur);
					}
					tclass_add(tcN);
				}
				dbmset_scaffoldDestroy(dbms);			
				
				/* (Ycor, Ycor, dpt) */
				tclass_ReachStable_delete(tcY, reach_stable, reach_stable_cur);
				tclass_mark_preds_unstable(tcY);
#ifndef NO_STCACHE
				tclass_forgetPreStability(tcY);
#endif
				tclass_replaceZoneDBM(tcY, tcY_corDBM); /* nowy cor domyslnie rowny calej klasie */
				tclass_depthUpdateInf(tcY);
				if (tclass_hasInit(tcY))
				{
					tclass_ReachStable_addInit(tcY, reach_stable, reach_stable_cur);
				}
			}
			dbm_destroy(pre_e_Xcor_Y);
		}
		dbm_destroy(pre_e_X_Ycor);
	}
	dbm_destroy(pre_e_Xcor_Ycor);
	
}

/*
 * Inicjalizacja potrzebnych struktur danych do zbudowania ps-modelu:
 *
 * - poczatkowy podzial
 * - zbior lokacji odwiedzonych
 * - zbior klas osiagalnych (reachable), zbior klas stabilnych (stable)
 */
void
psmodel_init(ProdLoc_set **visited_root, ProdLoc_set **visited_cur,
	TimedClassRS_set **reach_stable_root, TimedClassRS_set **reach_stable_cur)
{
	Loc_idx *ploc_init; /* lokacja poczatkowa */
	DBM_elem *ploc_init_inv; /* jej niezmiennik */
	TimedClass_set *init_tclass;
	
	/* zbieramy informacje o lokacji poczatkowej */
	ploc_init = prod_loc_initial(); /* lokacje skladowe */
	ploc_init_inv = prod_loc_inv(ploc_init); /* niezmiennik (koniunkcja niezm. skladowych) */

	init_loc_set = prod_loc_set_add(ploc_init, ploc_init_inv, visited_root, visited_cur);

	init_tclass = tclass_create(init_loc_set, NULL, dbm_copy(zeroDBM, dbm_size), 0);
	tclass_add(init_tclass);
	tclass_mark_reach_unst(init_tclass, reach_stable_root, reach_stable_cur);
}

/*
 * Konstruowanie pseudosymulacyjnego modelu abstrakcyjnego
 */
void
psmodel_builder(AutNet *an)
{
	ProdLoc_set *visited_locs_root = NULL, *visited_locs_cur;

	TimedClassRS_set *reach_stable_root = NULL, *reach_stable_cur, *picked_tclass_rs;
	
	net_init(an); /* inicjalizujemy siec */
	
	psmodel_init(&visited_locs_root, &visited_locs_cur, &reach_stable_root, &reach_stable_cur);

#ifdef PSM_VERBOSE
	stTests = 0;
	skip_stTests = 0;
#endif

	/* 
	 * Na razie stosujemy zbior zawierajacy klasy osiagalne i oznaczamy te ktore sa w stable,
	 * opierajac sie na obserwacji, ze stable zawsze zawiera sie w reachable.
	 * W przyszlosci mozna sprobowac rozbic reachable na reachable_stable (stable) i reachable_unstable.
	 * Dzieki temu uniknie sie przegladania zbioru w poszukiwaniu pierwszej niestabilnej klasy.
	 */
	
	while ((picked_tclass_rs = tclass_pickReachUnst(reach_stable_root)) != NULL)
	{
		TimedClass_set *tc; /* rozwazana klasa Y */
		bool *actions;
		idx_t i;
		ProdLoc_set *cur_loc;

		tc = picked_tclass_rs->tclass;
		cur_loc = tc->loc_set;

#ifdef PSM_VERBOSE
		printf("Przetwarzana klasa "); tclass_print(tc); printf("\n");
#endif

		if (!cur_loc->trans_complete)
		{
			/**** okreslamy new_locs, visited_locs i rozszerzamy zbior Pi ****/
			actions = prod_loc_acts(tc->loc_set->loc); /* akcje wykonalne z lokacji zwiazanej z biezaca klasa */
			for (i = 0; i < nact; i++)
			{
				if (actions[i])
				{
					ProdTrLoc_set *prtl, *prtl_tmp;
					prtl = prod_loc_succ(cur_loc->loc, i); /* pobieramy mozliwe przejscia dla akcji i */
				
					for (prtl_tmp = prtl; prtl_tmp; prtl_tmp = prtl_free_getNext(prtl_tmp))
					{
						DBM_elem *inv_guard;
						bool transition_added = false;
					
						/* 
						 * UWAGA:
						 * Zanim dodamy lokacje lub przejscie do niej prowadzace, sprawdzamy warunek
						 * na guardy i niezmienniki. Przejscia rowniez nie sa zapisywane jesli nie jest
						 * spelniony ten warunek (linia 6 i 7).
						 */
					
						inv_guard = dbm_intersection_cf(cur_loc->inv, prtl_tmp->guard, dbm_size);
						if (!dbm_empty(inv_guard, dbm_size))
						{
							/* niepuste przeciecie niezmiennika lokacji zrodlowej z guardem */
							ClockIdx_set *cur_clock;
							DBM_elem *dst_locInv;
							ProdLoc_set *dst_loc;
						
							/* inv_guard := inv_guard[reset(prtl_tmp) := 0] */
							for (cur_clock = prtl_tmp->clocks; cur_clock; cur_clock = cur_clock->next)
								dbm_reset(inv_guard, dbm_size, cur_clock->idx);
							
							/* pobieramy adres struktury z lokacja docelowa dla przetwarzanego przejscia
							 * jesli nie ma, to dst_loc == NULL
							 *
							 * dst_loc jest wykorzystywane jako warunek czy lokacja jest w visited_locs
							 */
							dst_loc = prod_loc_set_getp(prtl_tmp->loc, visited_locs_root);

							/* niezmiennik produktowy obliczamy tylko gdy jeszcze nie zrobilismy tego wczesniej */
							if (!dst_loc)
								dst_locInv = prod_loc_inv(prtl_tmp->loc);
							else
								dst_locInv = dst_loc->inv;
						
							dbm_intersection_cf_ip(inv_guard, dst_locInv, dbm_size);
							if (!dbm_empty(inv_guard, dbm_size))
							{
								/* dodajemy lokacje do visited_locs? */
								if (!dst_loc)
								{
									/* dst_loc == NULL */
									dst_loc = prod_loc_set_add(prtl_tmp->loc, dst_locInv, &visited_locs_root, &visited_locs_cur);
									tclass_add(tclass_create_base(dst_loc)); /* dodajemy nowa klase do Pi */
								}
								else
								{
									/* juz mamy ta lokacje, wiec zwalniamy pamiec (juz znamy ta konfiguracje lokacji skladowych) */
									free(prtl_tmp->loc);
									/* dst_locInv: niezmiennika nigdy nie zwalniamy, bo albo jest pobrany
									 *             istniejacy albo dodajemy nowa lokacje
									 */
								}
							
								/* dopisujemy odkryte przejscie do zbioru przejsc lokacji zrodlowej (aktualnie przetwarzana) */
								trans_add(cur_loc, dst_loc, prtl_tmp);
								transition_added = true;
							}
						}
					
						dbm_destroy(inv_guard);
					
						/* DBM z guardem i liste zegarow (reset) zwalniamy tylko jesli nie dodalismy tranzycji */
						if (!transition_added)
						{
							dbm_destroy(prtl_tmp->guard);
							colclocks_free(prtl_tmp->clocks);
						}

					}
				}
			}
			free(actions);		
			cur_loc->trans_complete = true; /* przejscia z tej lokacji zostaly skompletowane */
		}
		psmodel_split(tc, &reach_stable_root, &reach_stable_cur);
		
	}
	
	tclass_ReachStable_print(reach_stable_root);
#ifdef PSM_VERBOSE	
	printf("\nPominietych testow stabilnosci (cache): %u/%u\n", skip_stTests, stTests);
	printf("Utworzonych rzeczywistych klas: %u\n", tclass_count(visited_locs_root));
#endif

}

bool
ppty_check(Loc_idx *prodloc)
{
	Property_set *ppty;

	for (ppty = ppts; ppty; ppty = ppty->next)
	{
		ProdConf_set *w;
		bool sat = false;

		for (w = ppty->prop; w; w = w->next)
		{
			if (prodloc[w->aut] == w->loc)
			{
				sat = true;
			}
			else
			{
				sat = false;
				break;
			}
		}
		
		if (sat)
		{
			printf("Property \"%s\" satisfied!\n", ppty->name);
			return true;	
		}
	}
	
	return false;
}
