/** anproc.c **/

/*
 * Przetwarzanie struktur stworzonych podczas wczytywania sieci automatow,
 * tworzenie produktu na podstawie sieci automatow.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "anproc.h"
#include "antypes.h"
#include "anmacro.h"
#include "macro.h"
#include "dbm.h"

static Aut *aut = NULL;
static Aut_idx naut = 0;

static Clock_idx pclocks_cnt = 0; /* zegary w automacie produktowym */
static ProdClock *pclocks_map;
static Clock_idx **rpclocks_map;

static Act *act = NULL;
Act_idx nact = 0;

DBM_idx dbm_size = 0;
DBM_elem *zeroDBM = NULL;
ProdLoc_set *init_loc_set = NULL;

Property_set *ppts = NULL;

/*
 * Inicjalizacja przetwarzania sieci
 *
 * F-cja ustawia wartosci zmiennych globalnych, zeby uniknac przekazywania niektorych argumentow
 */
void
net_init(AutNet *an)
{
	idx_t i;
	
	/* automaty */
	aut = an->aut;
	naut = an->naut;

	/* akcje */
	act = an->act;
	nact = an->nact;
	
	ppts = an->ppts;

	/* zliczamy zegary ze wszystkich automatow skladowych */
	for (i = 0; i < naut; i++)
	{
		pclocks_cnt += aut[i].nclks;
	}
	dbm_size = pclocks_cnt+1;
	
	zeroDBM = dbm_init_zero(dbm_size);
	
	prod_clocks_mkmap();
	
#ifdef VERBOSE
	printf("Liczba zegarow w automacie produktowym = %d\n", pclocks_cnt);
#endif
}

void
prod_clocks_mkmap(void)
{
	Clock_idx i, j, k;

	pclocks_map = smalloc(sizeof(ProdClock)*pclocks_cnt); /* produktowy -> skladowy */
	rpclocks_map = smalloc(sizeof(Clock_idx *)*naut); /* skladowy -> produktowy */

	k = 0;
	for (i = 0; i < naut; i++)
	{
		rpclocks_map[i] = smalloc(sizeof(Clock_idx)*aut[i].nclks);
		for (j = 0; j < aut[i].nclks; j++)
		{
			pclocks_map[k].par_aut = i;
			pclocks_map[k].par_idx = j+1; /* ze wzgledu na x_0 */
			rpclocks_map[i][j] = k+1;
			k++;
		}
	}
}

void
prod_clock_maps_free(void)
{
	idx_t i;
	
	free(pclocks_map);
	
	for (i = 0; i < naut; i++)
	{
		free(rpclocks_map[i]);
	}
	free(rpclocks_map);
}

/*
 * Konwersja indeksu zegara z automatu skladowego
 * do indeksu zegara z automatu produktoweg
 */
Clock_idx
prod_clock_idx2pidx(Aut_idx ai, Clock_idx ci)
{
	assert(ai < naut);
	assert(ci-1 < aut[ai].nclks);
	
	if (ci == 0)
		return 0;
	else
	{
		assert(rpclocks_map[ai][ci-1] < dbm_size);
		return rpclocks_map[ai][ci-1];
	}
}

/*
 * Indeks produktowego zegara -> indeks skladowego
 */
ProdClock
prod_clock_pidx2idx(Clock_idx ci)
{
	if (ci == 0)
	{
		ProdClock r;
		r.par_aut = 0;
		r.par_idx = 0;
		return r;
	}
	else
	{
		return pclocks_map[ci-1];
	}
}


/*
 * Okreslanie poczatkowej lokacji produktowej
 */
Loc_idx *
prod_loc_initial(void)
{
	idx_t i;
	Loc_idx *initial;
	
	initial = smalloc(sizeof(Loc_idx)*naut);
	
	for (i = 0; i < naut; i++)
		initial[i] = aut[i].init_loc;
	
	return initial;
}

/*
 * Kopiowanie okreslonej lokacji produktowej
 */
Loc_idx *
prod_loc_copy(Loc_idx *prod_loc)
{
	Loc_idx *copy;
	
	copy = smalloc(sizeof(Loc_idx)*naut);
	(void)memcpy(copy, prod_loc, sizeof(Loc_idx)*naut);
	
	return copy;
}

/*
 * Pobieranie listy akcji POTENCJALNIE wykonalnych z lokacji produktowej
 */
bool *
prod_loc_acts(Loc_idx *prod_loc)
{
	idx_t i, j;
	Trans_set *t;
	bool *used_acts;
	
	used_acts = smalloc_zero(sizeof(bool)*nact);
	
	for (i = 0; i < naut; i++)
	{
		for (j = 0; j < aut[i].nloc; j++)
		{
			for (t = TRANS(aut[i].trans, aut[i].nloc, prod_loc[i], j);
					t != NULL; 
					t = t->next)
			{
				used_acts[t->act] = true;
			}
		}
	}
	
	return used_acts;
}

/*
 * Wyswietlanie lokacji produktowej
 */
void
prod_loc_show(Loc_idx *prod_loc)
{
	idx_t i;
	
	printf("( ");
	for (i = 0; i < naut; i++)
		printf("%s ", get_loc_name(aut[i].loc, prod_loc[i]));
	printf(")");
}

/*
 * Dodawanie lokacji produktowej do zbioru
 */
ProdLoc_set *
prod_loc_set_add(Loc_idx *prod_loc, DBM_elem *inv, ProdLoc_set **root, ProdLoc_set **cur)
{
	ProdLoc_set *new;

	new = smalloc(sizeof(ProdLoc_set));
	new->loc = prod_loc;
	new->inv = inv;
	new->src_locs = NULL;
	new->dst_locs = NULL;
	new->classes = NULL;
	new->trans_complete = false;
	
	prod_loc_set_raw_add(new, root, cur);
	
	return new;
}

/*
 * Dodawanie lokacji produktowej do zbioru w postaci surowej
 */
void
prod_loc_set_raw_add(ProdLoc_set *new, ProdLoc_set **root, ProdLoc_set **cur)
{
	new->next = NULL; /* nie wiadomo co tam bylo, a dodajemy na koncu */

	if (!*root)
		*root = new;
	else
		(*cur)->next = new;
	*cur = new;
}

/*
 * Sprawdzanie zawierania lokacji produktowej przez zbior
 */
bool
prod_loc_set_is_in(Loc_idx *prod_loc, ProdLoc_set *root)
{
	ProdLoc_set *cur;
	
	for (cur = root; cur; cur = cur->next)
	{
		if (memcmp(cur->loc, prod_loc, sizeof(Loc_idx)*naut) == 0) return true; /* znaleziono */
	}
	
	return false; /* nie znaleziono */
}

/*
 * Sprawdzanie zawierania lokacji produktowej przez zbior
 */
ProdLoc_set *
prod_loc_set_getp(Loc_idx *prod_loc, ProdLoc_set *root)
{
	ProdLoc_set *cur;
	
	for (cur = root; cur; cur = cur->next)
	{
		if (memcmp(cur->loc, prod_loc, sizeof(Loc_idx)*naut) == 0) return cur;
	}
	
	return NULL;
}

/*
 * Wyciaganie (wybieranie i usuwanie) elementu ze zbioru
 */
ProdLoc_set *
prod_loc_set_raw_pick(ProdLoc_set **root)
{
	ProdLoc_set *picked;
	
	picked = *root; /* bierzemy korzen */
	
	/* jesli byl korzen, to mial ustawione pole next, ktore wskazuje na nowy korzen */
	if (*root)
		*root = picked->next;
	
	return picked;
}

/*
 * Wyswietlanie zbioru lokacji produktowych
 */
void
prod_loc_set_show(ProdLoc_set *root)
{
	ProdLoc_set *cur;

	for (cur = root; cur; cur = cur->next)
	{
		prod_loc_show(cur->loc);
		printf("\ninv:\n");
		dbm_print(cur->inv, dbm_size);
	}
	printf("\n");
}

/*
 * Pelne zwalnianie pamieci przydzielonej na zbior lokacji produktowych
 * (pelne - wraz z lokacjami produktowymi).
 */
void
prod_loc_set_free(ProdLoc_set *root)
{
	ProdLoc_set *cur;
	
	for (cur = root; cur; )
	{
		ProdLoc_set *tmp = cur;
		
		cur = cur->next;
		free(tmp->loc);
		dbm_destroy(tmp->inv);
		free(tmp);
	}
}

/*
 * Dodawanie przejscia do lokacji
 */
void
trans_add(ProdLoc_set *src_loc, ProdLoc_set *dst_loc, ProdTrLoc_set *prtl)
{
	ProdTrans_set *new_trans;
	DstLoc_set *cur_dst_loc;
	DstLoc_set *dst_group = NULL;
	
	new_trans = smalloc(sizeof(ProdTrans_set));
	new_trans->act = prtl->act;
	new_trans->guard = prtl->guard;
	new_trans->clocks = prtl->clocks;
	new_trans->next = NULL;
	
	/*  
		- zgrupowane wszystkie przejscia do jednej lokacji docelowej
		- dodajac nowe przejscie szukamy grupy do ktorej nalezy okreslone
		  przejscie dodac
		- jesli nie ma jeszcze takiej grupy, to ja tworzymy
		- z grupa zwiazany jest wskaznik na lokacje docelowa 
		  (dokladnie, element zbioru lokacji ja przetrzymujacy)
	*/
	
	/* szukamy grupy przejsc prowadzacych do dst_loc */
	cur_dst_loc = src_loc->dst_locs;
	while (cur_dst_loc)
	{
		/* sprawdzamy czy znalezlismy cur_dst_loc == dst_loc */
		if (cur_dst_loc->loc_set == dst_loc)
		{
			dst_group = cur_dst_loc;
			break;
		}
		
		if (!cur_dst_loc->next)
			break;
		else
			cur_dst_loc = cur_dst_loc->next;
	}
	
	/* 
	 * jesli dst_group jest nieustawione, to nie ma jeszcze takiej
	 * lokacji docelowej i trzeba ja dodac
	 */
	if (!dst_group)
	{
		SrcLoc_set *new_src_loc;
		
		dst_group = smalloc(sizeof(DstLoc_set));
		dst_group->loc_set = dst_loc;
		dst_group->trans = NULL;
		dst_group->last_trans = NULL;
		dst_group->next = NULL;

		/*
		 * naszym dst_locs_LAST jest cur_dst_loc (gdy dst_group == NULL)
		 */
		if (!src_loc->dst_locs)
			src_loc->dst_locs = dst_group;
		else
		{
			assert(cur_dst_loc != NULL);
			cur_dst_loc->next = dst_group; /* Segfault! */
		}

		new_src_loc = smalloc(sizeof(SrcLoc_set));
		new_src_loc->loc_set = src_loc;
		new_src_loc->trans = new_trans; /* zapisujemy tylko raz (i tylko pierwszy element) */
		new_src_loc->next = NULL;

		/*
	 	 * zakladamy ze jesli nie udalo sie znalezc istniejacej grupy to
		 * w lokacji docelowej tez nie ma informacji o tej lokacji zrodlowej
		 */
		if(!dst_loc->src_locs)
			dst_loc->src_locs = new_src_loc;
		else
			dst_loc->last_src_loc->next = new_src_loc;
		dst_loc->last_src_loc = new_src_loc;

	}
	
	if (!dst_group->trans)
		dst_group->trans = new_trans;
	else
		dst_group->last_trans->next = new_trans;
	dst_group->last_trans = new_trans;
	
}


/*
 * Pobieranie wskaznika na przejscia z lokacji src do lokacji dst
 */
ProdTrans_set *
trans_get(ProdLoc_set *src, ProdLoc_set *dst)
{
	DstLoc_set *cur_dstloc;
	
	for (cur_dstloc = src->dst_locs; cur_dstloc; cur_dstloc = cur_dstloc->next)
	{
		if (cur_dstloc->loc_set == dst)
		{
			return cur_dstloc->trans;
		}
	}
	
	return NULL;
}

/*
 * Sprawdzanie zawierania elementu zbioru lokacji produktowych
 * przez zbior lokacji zrodlowych
 */
bool
ploc_in_src_locs(ProdLoc_set *prod_loc, SrcLoc_set *root)
{
	SrcLoc_set *cur;
		
	for (cur = root; cur; cur = cur->next)
	{
		if (cur->loc_set == prod_loc) return true; /* znaleziono */
	}
	
	return false; /* nie znaleziono */
}

void
trloc_set_free(TrLoc_set **trloc)
{
	idx_t i;
	TrLoc_set *tmp;
	
	for (i = 0; i < naut; i++)
		for (tmp = trloc[i]; tmp != NULL; tmp = tmp->next)
			free(tmp);
	
	free(trloc);
}

/*
 * Sprawdzanie czy lista ograniczen zawiera jakies rzeczywiste ograniczenia
 */
bool
constr_nonempty(Constr *c)
{
	if (c)
		/* lista jest niepusta */
		if (CONSTR_IS_TERM(c)) 
			/* pierwsze ogr. jest terminujace, czyli nic nie ma */
			return false;
		else
			/* jest przynajmniej jedno ogr. */
			return true;
	else
		return false;
}

/*
 * Wprowadzanie ograniczen z listy c do DBM-a p
 */
void
constr2dbm(Aut_idx ai, DBM_elem *p, Constr *c)
{
	if (!c) return;

	for (; !CONSTR_IS_TERM(c); c++)
	{
		DBM_idx i, j;
			
		i = prod_clock_idx2pidx(ai, c->l_clk);
		j = prod_clock_idx2pidx(ai, c->r_clk);
			
		switch (c->rel)
		{
			case CONSTR_LE:
				dbm_constr_le(p, dbm_size, i, j, c->val);
				break;
				
			case CONSTR_LT:
				dbm_constr_lt(p, dbm_size, i, j, c->val);
				break;
				
			default:
				FERROR("Unknown relation");
				break;
		}
		
		dbm_canon1(p, dbm_size, i, j);
	}
}

/*
 * Zbieranie z automatow skladowych zegarow z indeksami automatu produktowego
 */
void
clocks_collect(ClockIdx_set **root, ClockIdx_set **cur, Aut_idx ai, Clock_idx *clks, Clock_idx nclks)
{
	idx_t i;
	
	for (i = 0; i < nclks; i++, clks++)
	{
		ClockIdx_set *n = smalloc(sizeof(ClockIdx_set));
	
		n->idx = prod_clock_idx2pidx(ai, *clks);
		assert(n->idx > 0);
		n->next = NULL;
	
		if (!*root)
			*root = n;
		else
			(*cur)->next = n;
		*cur = n;
	}
}

/*
 * Zwalnianie pamieci zaalokowanej przez clocks_collect
 */
void
colclocks_free(ClockIdx_set *elem)
{
	ClockIdx_set *next;
	
	while (elem)
	{
		next = elem->next;
		free(elem);
		elem = next;
	}
}

/*
 * Pobieranie niezmiennika dla okreslonej lokacji produktowej w post. DBM.
 */
DBM_elem *
prod_loc_inv(Loc_idx *ploc)
{
	DBM_elem *p = NULL;
	idx_t i;
	
	p = dbm_init(dbm_size);
	
	for (i = 0; i < naut; i++)
	{
		constr2dbm(i, p, aut[i].loc[ploc[i]].inv);
	}
	
	return p;
}

/*
 * Pobieranie niezmiennika dla okreslonej lokacji produktowej w post. DBM.
 * Jesli nie ma ograniczen, to DBM == NULL.
 */
DBM_elem *
prod_loc_inv0(Loc_idx *ploc)
{
	DBM_elem *p = NULL;
	idx_t i;
	
	for (i = 0; i < naut; i++)
	{
		Constr *cinv;
		
		cinv = aut[i].loc[ploc[i]].inv;

		/* DBM alokowany tylko jak jest co do niego wrzucic */
		if (!p && constr_nonempty(cinv)) p = dbm_init(dbm_size);

		constr2dbm(i, p, cinv);
	}
	
	return p; /* p == NULL jesli nie ma ograniczen */
}

/*
 * Znajdowanie przejsc z okreslona akcja mozliwych z danej lokacji
 * produktowej wraz z lokacjami produktowymi do ktorych prowadza.
 */
ProdTrLoc_set *
prod_loc_succ(Loc_idx *src_ploc, Act_idx a)
{
	idx_t i, j;
	Trans_set *t;
	
	TrLoc_set **trloc;	/* posrednia struktura do ktorej bedziemy zbierac przejscia i lokacje docelowe */
	idx_t *ntrloc;		/* liczniki odnalezionych przejsc w poszczegolnych automatach */

	trloc = smalloc_zero(sizeof(TrLoc_set *)*naut);
	ntrloc = smalloc_zero(sizeof(idx_t)*naut);
	
	for (i = 0; i < naut; i++) 
	{
		if (aut[i].uact[a]) /* jesli akcja jest w zbiorze akcji automatu, to idziemy dalej */
		{
		
			for (j = 0; j < aut[i].nloc; j++) 
			{
				for (t = TRANS(aut[i].trans, aut[i].nloc, src_ploc[i], j);
						t != NULL; t = t->next)
				{

					if (t->act == a) /* znalezlismy przejscie z poszukiwana akcja */
					{
						TrLoc_set *n;
						
						ntrloc[i]++; /* znaleziono przejscie */
						
						n = smalloc(sizeof(TrLoc_set));
						n->loc = j;
						n->trans = t;
						n->next = NULL;
						
						if (!trloc[i])
							trloc[i] = n;
						else
						{
							TrLoc_set *last;
							for (last = trloc[i]; last->next != NULL; last = last->next);
							last->next = n;
						}
						
					}
					
				}
			}
			
			if (!ntrloc[i])
			{
				/* akcja jest niewykonalna, poniewaz i-ty automat ma ta akcje
				 * w zbiorze swoich akcji, ale nie jest ona w nim umozliwiona.
				 */
				trloc_set_free(trloc);
				free(ntrloc);
				return NULL;
			}
		}
	}

	{
		ProdTrLoc_set *r = NULL, *r_cur = NULL;
		idx_t *idcs = smalloc(sizeof(idx_t)*naut);
		int k = 0;
		
		for (i = 0; i < naut; i++)
		{
			if (ntrloc[i] > 0) idcs[i] = 1;
			else idcs[i] = 0;
		}
		
		while (k >= 0)
		{
			Loc_idx *dst_ploc = prod_loc_copy(src_ploc);
			ProdTrLoc_set *n = smalloc(sizeof(ProdTrLoc_set));
			ClockIdx_set *clocks_cur = NULL;
			
			n->act = a;
			n->guard = dbm_init(dbm_size);
			n->clocks = NULL; /* musi byc, bo inaczej clocks_cur bedzie brany pod uwage w clocks_collect */
			
			for (i = 0; i < naut; i++)
			{
			
				int depth = 0;
				TrLoc_set *trloc_tmp;
				
				for (trloc_tmp = trloc[i]; trloc_tmp; trloc_tmp = trloc_tmp->next)
				{
					/* ten for wykonuje sie tylko jak cos sie zmienilo w i-tym automacie */

					depth++;
					if (depth == idcs[i])
					{
						dst_ploc[i] = trloc_tmp->loc;
						constr2dbm(i, n->guard, trloc_tmp->trans->guard); /* wystarczy tylko dla trloc[i]? */
						/* optymalniej byloby dokonac kanonizacji po wniesieniu zmian do DBM z uwzglednieniem
						 * ile zostalo zmodyfikowanych elementow (czasami dbm_canon1 nie oplaca sie), poniewaz
						 * obecnie constr2dbm oblicza za kazdym razem PK co na pewno nie jest oplacalne jesli
						 * modyfikacji jest tyle co dbm_size.
						 */
						clocks_collect(&(n->clocks), &clocks_cur, i, trloc_tmp->trans->clocks, trloc_tmp->trans->nclocks);
					}
				}
				
			}
			
			n->loc = dst_ploc;
			n->next = NULL;
			
			if (!r)
				r = n;
			else
				r_cur->next = n;
			r_cur = n;

			for (k = naut - 1; k >= 0 && idcs[k] == ntrloc[k]; k--)
			{
				if (ntrloc[k] > 0) idcs[k] = 1;
			}

			if (k >= 0) idcs[k]++;

		}

		free(idcs);
		trloc_set_free(trloc);
		free(ntrloc);

		return r;
	}
}

/*
 * Zwalniamy pamiec prtl i zwracamy kolejny element
 * (do wykorzystywania w petlach) 
 *  
 */
ProdTrLoc_set *
prtl_free_getNext(ProdTrLoc_set *prtl)
{
	ProdTrLoc_set *next;

	next = prtl->next;
	free(prtl);
	
	return next;
}

/*
 * Generowanie automatu pseudoproduktowego (do testow). Troche nasmiecone. Jesli ma zostac w kodzie, to
 * trzeba dokonac doglebnego przegladu.
 */
void
prod_show(AutNet *an)
{	
	ProdLoc_set *waiting_root = NULL, *waiting_cur = NULL; /* _cur nie musi byc NULL (ale kompilator...) */
	ProdLoc_set *visited_root = NULL, *visited_cur = NULL;
	
	Loc_idx *ploc_init;
	DBM_elem *ploc_init_inv;
	
	net_init(an);
	
	ploc_init = prod_loc_initial();
	ploc_init_inv = prod_loc_inv(ploc_init);
	
	prod_loc_set_add(ploc_init, ploc_init_inv, &waiting_root, &waiting_cur);
	
	while (waiting_root)
	{
		
		ProdLoc_set *processed;
		bool *actions;
		idx_t i;
		
		processed = prod_loc_set_raw_pick(&waiting_root);
		prod_loc_set_raw_add(processed, &visited_root, &visited_cur);

		actions = prod_loc_acts(processed->loc);
		
		printf("-------------------------------------------------------------\n");
		printf("Przejscia mozliwe z lokacji produktowej ");
		prod_loc_show(processed->loc); printf("\n");
		
		for (i = 0; i < nact; i++) 
		{
			if (actions[i]) 
			{
				ProdTrLoc_set *prtl, *prtl_tmp;
				prtl = prod_loc_succ(processed->loc, i);

				for (prtl_tmp = prtl; prtl_tmp;)
				{
					printf("\n");
					printf("Akcja %s\n", get_act_name(act, i));
					printf("Guard:\n");
					dbm_print(prtl_tmp->guard, dbm_size);
					printf("Zegary do zresetowania: ");
					{
						ClockIdx_set *cur;
						for (cur = prtl_tmp->clocks; cur; cur = cur->next)
						{
							ProdClock pc;
							pc = prod_clock_pidx2idx(cur->idx);
							printf("%s(%s,%d) ", aut[pc.par_aut].name, get_clock_name(aut[pc.par_aut].clks, pc.par_idx), cur->idx);
						}
						printf("\n");
					}
					printf("Do lokacji: ");
					prod_loc_show(prtl_tmp->loc); printf("\n");
					
					if (!prod_loc_set_is_in(prtl_tmp->loc, waiting_root) && !prod_loc_set_is_in(prtl_tmp->loc, visited_root))
					{
						/* lokacji nie ma jeszcze ani w WAITING ani w VISITED, wiec moze ja dodamy... */
						DBM_elem *inv_guard;
						
						inv_guard = dbm_intersection_cf(processed->inv, prtl_tmp->guard, dbm_size);
						if (!dbm_empty(inv_guard, dbm_size))
						{
							ClockIdx_set *cur;
							DBM_elem *prtl_tmp_locInv;
							
							for (cur = prtl_tmp->clocks; cur; cur = cur->next)
							{
								dbm_reset(inv_guard, dbm_size, cur->idx);
							}
							/* przeciecie inv_guard (po resetach) z niezmiennikiem lokacji docelowej */
							prtl_tmp_locInv = prod_loc_inv(prtl_tmp->loc);
							dbm_intersection_cf_ip(inv_guard, prtl_tmp_locInv, dbm_size);
							if (!dbm_empty(inv_guard, dbm_size))
							{
								prod_loc_set_add(prtl_tmp->loc, prtl_tmp_locInv, &waiting_root, &waiting_cur);
							}
							else
							{
								dbm_destroy(prtl_tmp_locInv);
							}
						}
					}
					else 
					{
						/* mamy juz ta lokacje, wiec zwalniamy pamiec */
						free(prtl_tmp->loc);
					}
					
					/* informacje z prod_loc_succ zostaly wykorzystane - zwalniamy pamiec */
					dbm_destroy(prtl_tmp->guard);
					colclocks_free(prtl_tmp->clocks);
					{
						ProdTrLoc_set *next;
						
						next = prtl_tmp->next;
						free(prtl_tmp);
						prtl_tmp = next;
					}
					
				}
			}

		}
		free(actions); /* moze lepiej byloby pomyslec nad czyms wielokrotnego uzytku...?
		albo zastapic jakas lepsza struktura danych? */
	}
	
	printf("------\n");
	prod_loc_set_show(visited_root);
	prod_loc_set_free(visited_root);
	
}

