/** dbm.c **/

/*
 * Operacje na macierzach ograniczen roznic
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dbm.h"
#include "dbm_macro.h"
#include "macro.h"

DBM_elem *
dbm_init(DBM_idx dim)
{
	DBM_elem *p;
	DBM_idx i, j;
	
	p = dbm_rawinit(dim);

	/* Poczatkowe wypelnianie macierzy */
	for (i = 0; i < dim; i++)
	{
		for (j = 0; j < dim; j++)
		{
			if (i == j || i == 0) /* pierwszy wiersz macierzy i przekatna: (0, <=) */
			{
				DBM(p, dim, i, j) = DBM_E_0LE;
			}
			else /* pozostale: nieograniczone */
			{
				DBM(p, dim, i, j) = INF;
			}
		}
	}

	assert(dbm_is_sane(p, dim));
	
	return p;	
}

DBM_elem *
dbm_init_zero(DBM_idx dim)
{
	DBM_elem *p;
	DBM_idx i, j;
	
	p = dbm_rawinit(dim);

	for (i = 0; i < dim; i++)
	{
		for (j = 0; j < dim; j++)
		{
			DBM(p, dim, i, j) = DBM_E_0LE;
		}
	}

	assert(dbm_is_sane(p, dim));
	
	return p;	
}

DBM_elem *
dbm_init_empty(DBM_idx dim)
{
	DBM_elem *p;
	
	p = dbm_init(dim);
	DBM_MARK_EMPTY(p);
	
	return p;
}

DBM_elem *
dbm_rawinit(DBM_idx dim)
{			
	return (DBM_elem *)smalloc((dim*dim)*sizeof(DBM_elem));
}

DBM_elem *
dbm_copy(DBM_elem *p, DBM_idx dim)
{
	DBM_elem *r;
	
	assert(p && dim);
	
	r = dbm_rawinit(dim);
	memcpy(r, p, sizeof(DBM_elem)*dim*dim);
	
	return r;
}

void
dbm_print_elem(DBM_elem e)
{
	if (e == INF)
		printf("inf ");
	else 
	{
		printf("(%d,", e >> 1);
		switch (e & 1)
		{
			case REL_LE: /* najmniej znaczacy bit - 1 */
				printf("<=) ");
				break;
			case REL_LT: /* najmniej znaczacy bit - 0 */
				printf(" <) ");
				break;
		}
	}

	return;
}


void
dbm_print(DBM_elem *p, DBM_idx dim)
{
	DBM_idx i, j;
	
	if (!p)
	{
		FERROR("dbm_print: p == NULL");
	}
	
	for (i = 0; i < dim; i++)
	{
		printf("[ ");
		for (j = 0; j < dim; j++)
		{
			dbm_print_elem(DBM(p, dim, i, j));
		}
		printf("]\n");
	}
	
	return;
}

/*
 * Wyswietlanie elementow w zhumanizowanej postaci ;)
 * 
 * Pisane na kolanie, wiec mozna byloby to przemyslec...
 */
void
dbm_xprint_elem(DBM_elem *p, DBM_idx dim, DBM_idx i, DBM_idx j)
{
	DBM_elem e;
	
	e = DBM(p, dim, i, j);
	
	if (j == 0) {
		printf("[x%d%s%d]", i, (e==INF?"inf":((e&1)==REL_LE?"<=":"<")), e >> 1);
		
	}
	else
	{
		if (i == 0)
		{
			printf("[x%d%s%d]", j, (e==INF?"inf":((e&1)==REL_LE?">=":">")), -(e >> 1));
		}
		else
		{
			printf("[x%d-x%d%s%d]", i, j, (e==INF?"inf":((e&1)==REL_LE?"<=":"<")), e >> 1);
		}
	}

	return;
}

/*
 * Wyswietlanie kanonicznych DBM-ow
 */
void
dbm_xprint(DBM_elem *p, DBM_idx dim)
{
	DBM_idx i, j;
	bool f = false;
	
	if (dbm_empty(p, dim))
		printf("Empty");
	
	for (i = 0; i < dim; i++)
	{
		for (j = 0; j < dim; j++)
		{
			if (i == j || i == 0)
			{
				if (DBM(p, dim, i, j) != DBM_E_0LE)
				{
					f = true;
					dbm_xprint_elem(p, dim, i, j);
				}
			}
			else
			{
				if (DBM(p, dim, i, j) != INF)
				{
					f = true;
					dbm_xprint_elem(p, dim, i, j);
				}
			}
		}
	}
	if (!f) printf("Default");
}

/* Ustawianie ograniczen */
void
dbm_constr_le(DBM_elem *p, DBM_idx dim, DBM_idx i, DBM_idx j, int val)
{
	assert(i < dim && j < dim);
	assert(i != j); /* nie wolno modyfikowac przekatnej */
	
	DBM(p, dim, i, j) = DBM_VSC(DBM_ELEM(val, REL_LE));
		
	return;
}

void
dbm_constr_lt(DBM_elem *p, DBM_idx dim, DBM_idx i, DBM_idx j, int val)
{
	assert(i < dim && j < dim);
	assert(i != j); /* nie wolno modyfikowac przekatnej */
		
	DBM(p, dim, i, j) = DBM_VSC(DBM_ELEM(val, REL_LT));

	return;
}

/* Uwalnianie ograniczenia */
void
dbm_rls_constr(DBM_elem *p, DBM_idx dim, DBM_idx i, DBM_idx j)
{
	assert(i < dim && j < dim);
	assert(i != j); /* nie wolno modyfikowac przekatnej */

	DBM(p, dim, i, j) = INF;

	assert(dbm_is_sane(p, dim));
	
	return;
}


/*
 * Sprowadzamy DBM do postaci kanonicznej (Floyd-Warshall)
 *
 * p = {0, 1, 2, ..., dim-1}
 *
 * for (k in p)
 *   for (i in p)
 *     for (j in p)
 *       if (a[i,j] > a[i,k] + a[k,j]) then
 *         a[i,j] = a[i,k] + a[k,j]
 *
 */
void
dbm_canonicalize(DBM_elem *p, DBM_idx dim)
{
	DBM_idx k, i, j;
	DBM_elem *e, sum;
	
	assert(p != NULL);
	assert(dbm_valid_val(p, dim));
				
	for (k = 0; k < dim; k++)
	{
		for (i = 0; i < dim; i++)
		{
			for (j = 0; j < dim; j++)
			{
				e = DBM_P(p, dim, i, j);
				sum = DBM_SUM(DBM(p, dim, i, k), DBM(p, dim, k, j));
				if (*e > sum)
				{
					*e = DBM_LVSC(sum);
				}
					
				if (i == j && *e < DBM_E_0LE)
				{
					/* 
					 * Nie obliczamy do konca post. kanonicznej jesli pojawia element ujemny na przekatnej,
					 * bo i tak mozna to robic w nieskonczonosc. Wtedy natychmiast oznaczamy strefe jako
					 * pusta. Dzieki temu strefy ktorych DBM-y sprowadzalismy do post. kanonicznej szybko
					 * sprawdza sie pod katem pustosci.
					 */
					DBM_MARK_EMPTY(p);
					return;
				}
				
			}
		}
	}
	
	assert(dbm_valid_val(p, dim));
	
	return;
}

/*
 * Specjalizowany Floyd-Warshal Rokickiego do obliczania
 * postaci kanonicznej DBM po wniesieniu jednej zmiany
 * do kanonicznego DBM, ktora zaciesnia ograniczenie.
 *
 * a[x,y] - zmodyfikowany element
 * p = {0, 1, 2, ..., dim-1}
 * 
 * for (j in p)
 *   if (a[x,j] > a[x,y] + a[y,j]) then
 *     a[x,j] = a[x,y] + a[y,j]
 * for (i in p)
 *   if (a[i,y] > a[i,x] + a[x,y]) then
 *     a[i,y] = a[i,x] + a[x,y]
 *     for (j in p)
 *       if (a[i,j] > a[i,y] + a[y,j]) then
 *         a[i,j] = a[i,y] + a[y,j]
 *
 * Odwolanie do pracy Rokickiego odnalezione w kodzie biblioteki DBM Uppaala.
 * Stad tez pomysl dobierania sposobu kanonizacji. Metoda przekazywania
 * zbioru indeksow elementow zmodyfikowanych jest silnie zainspirowana Uppaalem.
 *
 * Funkcja zwraca prawde jesli strefa nie jest pusta, w przeciwnym wypadku falsz.
 */
bool
dbm_canon1(DBM_elem *p, DBM_idx dim, DBM_idx x, DBM_idx y)
{
	DBM_idx i, j;
	DBM_elem *e, sum;
	
	assert(p != NULL);
	assert(dbm_valid_val(p, dim));
	
	for (j = 0; j < dim; j++)
	{
		e = DBM_P(p, dim, x, j);
		sum = DBM_SUM(DBM(p, dim, x, y), DBM(p, dim, y, j));
		if (*e > sum)
		{
			*e = DBM_LVSC(sum);
		}
	}
	if (DBM(p, dim, x, x) < DBM_E_0LE) /* strefa pusta? */
	{
		DBM_MARK_EMPTY(p);
		return false;
	}
	
	for (i = 0; i < dim; i++)
	{
		e = DBM_P(p, dim, i, y);
		sum = DBM_SUM(DBM(p,dim, i, x), DBM(p, dim, x, y));
		if (*e > sum)
		{
			*e = DBM_LVSC(sum);
			
			for (j = 0; j < dim; j++)
			{
				e = DBM_P(p, dim, i, j);
				sum = DBM_SUM(DBM(p,dim, i, y), DBM(p, dim, y, j));
				if (*e > sum) {
					*e = DBM_LVSC(sum);
				}
			}	
		}
		if (DBM(p, dim, i, i) < DBM_E_0LE) /* strefa pusta? */
		{
			DBM_MARK_EMPTY(p);
			return false;
		}
	}
	
	assert(dbm_valid_val(p, dim));
	assert(dbm_is_canonical(p, dim));
	
	return true;
}


/*
 * Specjalizowany Floyd-Warshall do obliczania postaci
 * kanonicznej po wniesieniu kliku zmian do kanonicznego DBM,
 * ktore zaciesniaja ograniczenia.
 *
 * p = {0, 1, 2, ..., dim-1}
 * f - zbior indeksow elementow zmodyfikowanych
 *
 * for (k in f)
 *   for (i in p)
 *     for (j in p)
 *       if (a[i,j] > a[i,k] + a[k,j]) then
 *         a[i,j] = a[i,k] + a[k,j]
 *
 */
void
dbm_scanon(DBM_elem *p, DBM_idx dim, DBM_idx_flags *f)
{
	DBM_idx i, j, k;
	DBM_elem *e, sum;
	
	assert(p != NULL);
	assert(dbm_valid_val(p, dim));
	
	for (k = 0; k < dim; k++) 
	{
		if (IS_FLAGGED(f, k))
		{

			for (i = 0; i < dim; i++)
			{
				for (j = 0; j < dim; j++)
				{
					
					e = DBM_P(p, dim, i, j);
		
					sum = DBM_SUM(DBM(p, dim, i, k), DBM(p, dim, k, j));
					if (*e > sum)
					{
						*e = DBM_LVSC(sum);
					}
						
					if (i == j && *e < DBM_E_0LE) /* strefa pusta? */
					{
						DBM_MARK_EMPTY(p);
						return;
					}
					
				}
			}

		}
	}

	assert(dbm_valid_val(p, dim));
	assert(dbm_is_canonical(p, dim));
	
	return;
}

bool
dbm_is_canonical(DBM_elem *p, DBM_idx dim)
{
	DBM_idx k, i, j;
	DBM_elem *e;	
	
	assert(p != NULL);
	/*
	 * Jesli strefa ma element ujemny na przekatnej, to uznajemy, ze mamy postac kanoniczna.
	 * W przeciwnym wypadku nie bylibysmy w stanie tego stwierdzic badajac czy algorytm
	 * obliczajacy postac kanoniczna nanosi jakies zmiany, poniewaz nanosilby je do
	 * nieskonczonosci (teoretycznie).
	 */
	for (i = 0; i < dim; i++) 
	{
		if (DBM(p, dim, i, i) < DBM_E_0LE)
			return true;
	}
	
	for (k = 0; k < dim; k++)
	{
		for (i = 0; i < dim; i++)
		{
			for (j = 0; j < dim; j++)
			{

				e = DBM_P(p, dim, i, j);
				
				/* sprawdzamy czy algorytm wprowadzilby jakas zmiane w DBM */				
				if (*e > DBM_SUM(DBM(p, dim, i, k), DBM(p, dim, k, j)))
					return false;

			}
		}
	}
	
	return true;
}

/*
 * Porownywanie stref w postaciach kanonicznych
 */
bool
dbm_equal(DBM_elem *p, DBM_elem *q, DBM_idx dim)
{
	assert(dbm_is_canonical(p, dim));
	assert(dbm_is_canonical(q, dim));

	if (dbm_empty(p, dim) && dbm_empty(q, dim))
		return true; /* obie strefy puste */
		/* Jesli chcemy pominac sprawdzenia dbm_empty, to trzeba byloby
		 * zmienic zalozenie o PK (komentarz w dbm_empty).
		 */
		
	if (memcmp(p, q, sizeof(DBM_elem)*dim*dim) == 0)
		return true;
	else
		return false;
}

/*
 * Sprawdzanie pustosci DBM w postaci kanonicznej
 */
bool
dbm_empty(DBM_elem *p, DBM_idx dim)
{
	int i;

	assert(p != NULL);
	assert(dbm_is_canonical(p, dim));

	if (*p < DBM_E_0LE) return true;
	
	/*
	 * Jesli dalej nie chcemy sprawdzac calej przekatnej, musielibysmy
	 * zmienic zalozenie dot. postaci kanonicznej. DBM bylby w postaci
	 * kanonicznej wtw gdy jesli strefa jest pusta, to DBM(0,0) < DBM_E_0LE
	 * (czyli jesli gdzies na przekatnej jest element < DBM_E_0LE, to
	 * wtedy rowniez DBM(0,0) < DBM_E_0LE).
	 */
	for (i = 1; i < dim; i++) 
	{
		
		if (DBM(p, dim, i, i) < DBM_E_0LE)
		{
			DBM_MARK_EMPTY(p); /* oznaczamy strefe aby kolejne sprawdzenia byly szybsze */
			return true;
		}
	
	}
	
	return false;
}

/*
 * Obliczanie przeciecia dwoch stref
 */
DBM_elem *
dbm_intersection(DBM_elem *p, DBM_elem *q, DBM_idx dim)
{
	DBM_elem *r, *e;
	DBM_idx i, j;
	
	assert(p != NULL && q != NULL);
	
	r = dbm_rawinit(dim);
	
	for (i = 0; i < dim; i++)
	{
		for (j = 0; j < dim; j++)
		{
			
			e = DBM_P(r, dim, i, j);
			*e = DBM_MIN(DBM(p, dim, i, j), DBM(q, dim, i, j));
			
			if (i == j && *e < DBM_E_0LE)
			{
				/* 
				 * Jesli gdzies na przekatnej pojawi sie liczba ujemna,
				 * wtedy strefe oznaczamy jako pusta i nic wiecej z nia
				 * nie robimy, bo i tak pozostanie pusta.
				 */
				DBM_MARK_EMPTY(r);
				return r;
			}
		}
	}
	
	dbm_canonicalize(r, dim);
	
	return r;
}

/*
 * Obliczanie liczby elementow tablicy mieszczacej bity odpowiadajace zegarom
 */
DBM_idx_flags *
get_flags_array(DBM_idx dim)
{
	return (DBM_idx_flags *)smalloc_zero(sizeof(DBM_idx_flags) * GET_ASIZE(dim));
}

/*
 * Obliczanie przeciecia dwoch stref (w miejscu, modyfikuje p)
 * 
 * p - DBM w postaci kanonicznej (z tym zalozeniem jest szybciej,
 *     a w praktyce nie jest ono trudne do spelnienia)
 */
void
dbm_intersection_cf_ip(DBM_elem *p, DBM_elem *q, DBM_idx dim)
{
	DBM_elem *e_p, *e_q;
	DBM_idx i, j;
	DBM_idx_flags *flags;
	DBM_idx chg_i = 0, chg_j = 0;
	unsigned int chg_cnt = 0;
	
	assert(p != NULL && q != NULL);
	assert(dbm_valid_val(p, dim));
	assert(dbm_valid_val(q, dim));
	assert(dbm_is_canonical(p, dim));
	
	e_p = p;
	e_q = q;
	
	/* tablica do flagowania zmodyfikowanych indeksow */
	flags = get_flags_array(dim);
	
	for (i = 0; i < dim; i++)
	{
		for (j = 0; j < dim; j++)
		{
			
			assert(e_p == DBM_P(p, dim, i, j)); /* czy na pewno biezacy element odpowiada temu, czego sie spodziewamy */
			assert(e_q == DBM_P(q, dim, i, j));
			
			if (*e_p > *e_q) 
			{
				
				*e_p = *e_q;
				
				SET_FLAG(flags, i); /* flagujemy indeksy modyfikacji */
				SET_FLAG(flags, j);
				
				/* jesli jest szansa, ze bedzie to jedyna modyfikacja, to zapisujemy co zmodyfikowano */
				if (chg_cnt < 1) 
				{
					chg_i = i;
					chg_j = j;
				}
				chg_cnt++; /* zwiekszamy licznik modyfikacji */
				
			}
			
			if (i == j && *e_p < DBM_E_0LE)
			{
				free(flags);
				DBM_MARK_EMPTY(p);
				return;
			}
			
			e_p++; /* przechodzimy do kolejnych elementow */
			e_q++;
			
		}
	}
	
	if (chg_cnt == 1)
	{
		dbm_canon1(p, dim, chg_i, chg_j);
	}
	else if (chg_cnt > 1)	
	{
		dbm_scanon(p, dim, flags);
	}
	
	free(flags); /* tablica flag juz jest zbedna */
	
	assert(dbm_is_canonical(p, dim));
	
	return;
}

/*
 * Obliczanie przeciecia dwoch stref, wejsciowy DBM p w postaci kanonicznej.
 *
 * W niektorych zastosowaniach jest to szybka metoda. Wystarczyloby udowodnic,
 * ze zachowywanie postaci kanonicznej po kazdej najmniejszej zmianie w niczym
 * nie przeszkadza.
 * 
 * W praktyce weryfikacji niekoniecznie ma sens ;)
 * 
 * (prototypowo)
 */
void
dbm_intersection_cfX_ip(DBM_elem *p, DBM_elem *q, DBM_idx dim)
{
	DBM_elem *e_p, *e_q;
	DBM_idx i, j;
	
	assert(dbm_valid_val(p, dim));
	assert(dbm_valid_val(q, dim));
	assert(dbm_is_canonical(p, dim));
	
	e_p = p;
	e_q = q;
	
	for (i = 0; i < dim; i++) {
		for (j = 0; j < dim; j++) {
			
			assert(e_p == DBM_P(p, dim, i, j)); /* czy na pewno biezacy element odpowiada temu, czego sie spodziewamy */
			assert(e_q == DBM_P(q, dim, i, j));
			
			if (*e_p > *e_q) {
				
				*e_p = *e_q;
				
				if(!dbm_canon1(p, dim, i, j)) {
					return; /* dbm_canon1 oznaczyl strefe jako pusta */
				}
								
			}
			
			/* Moze nie jest to oplacalne jednak?
			 * Jesli mamy p w PK i jesli nie jest oznaczona jako pusta to daje nam to tyle, ze
			 * zostanie oznaczona, ale wczesniej moze to zrobic tez dbm_canon1.
			 */
			if (i == j && *e_p < 0) {
				*p = DBM_E_NEG;
				return;
			}
			
			e_p++; /* przechodzimy do kolejnych elementow */
			e_q++;
			
		}
	}
	
	assert(dbm_is_canonical(p, dim));
	
	return;
}

/*
 * Obliczanie przeciecia dwoch stref
 * 
 * p - DBM w postaci kanonicznej (pozostaje nienaruszony)
 */
DBM_elem *
dbm_intersection_cf(DBM_elem *p, DBM_elem *q, DBM_idx dim)
{
	DBM_elem *x;
	
	assert(p != NULL && q != NULL);
	
	x = dbm_copy(p, dim);
	dbm_intersection_cf_ip(x, q, dim);
	
	return x;
}

/*
 * Z[x:=0]
 * Resetowanie zegara z zachowaniem postaci kanonicznej
 *
 *  c - resetowany zegar
 */
void
dbm_reset(DBM_elem *p, DBM_idx dim, DBM_idx c)
{
	DBM_idx i;

	assert(p != NULL);
	assert(dbm_is_canonical(p, dim));
	
	for (i = 0; i < dim; i++)
	{
		
		if (i != c) /* pomijamy DBM(i,i) */
		{
			DBM(p, dim, c, i) = DBM(p, dim, 0, i);
			DBM(p, dim, i, c) = DBM(p, dim, i, 0);
		}
		
	}
	
	assert(dbm_is_canonical(p, dim));
	
	return;
}

/*
 * [x:=0]Z
 *
 *  c - resetowany zegar
 */
void
dbm_invreset(DBM_elem *p, DBM_idx dim, DBM_idx c)
{
	DBM_idx i;
#ifndef NDEBUG
	DBM_elem *p_orig;			
#endif	
	
	assert(p != NULL);
	assert(dbm_is_canonical(p, dim));
	
#ifndef NDEBUG
	p_orig = dbm_copy(p, dim);
#endif
	
	if (DBM(p, dim, 0, c) == DBM_E_0LE)
	{
		/*
		 * Pomijamy: DBM(p, dim, c, 0) = DBM_E_0LE;
		 * bo zawiera sie to w nastepnym kroku.
		 *
		 * Zamiast obliczania postaci kanonicznej, 
		 * poprawiamy tylko to co trzeba:
		 */
		for (i = 1; i < dim; i++)
			DBM(p, dim, i, 0) = DBM(p, dim, i, c);
		
		for (i = 0; i < dim; i++)
			if (i != c) DBM(p, dim, c, i) = INF;
		
		dbm_canonicalize(p, dim);
		
	}
	else /* wynikiem operacji jest pusta strefa */
	{
		DBM_MARK_EMPTY(p);
	}
	
	assert(dbm_is_canonical(p, dim));

#ifndef NDEBUG
	assert(dbm_check_invreset(p_orig, p, dim, c));
	dbm_destroy(p_orig);
#endif
	
	return;
}

/*
 * Sprawdzanie poprawnosci obliczania [x:=0]Z wzgledem pierwotnego algorytmu z AinV
 *
 *  p - nienaruszony DBM (do obliczenia)
 *  q - wynikowy DBM do porownania
 */
bool
dbm_check_invreset(DBM_elem *p, DBM_elem *q, DBM_idx dim, DBM_idx c)
{
	DBM_idx i;
	
	assert(p != NULL && q != NULL);

	if (DBM(p, dim, 0, c) == DBM_E_0LE)
	{
		
		DBM(p, dim, c, 0) = DBM_E_0LE;
		dbm_canonicalize(p, dim);
		
		for (i = 0; i < dim; i++)
		{
			if (i != c) DBM(p, dim, c, i) = INF;
		}

		dbm_canonicalize(p, dim);
		
	} 
	else
	{
		DBM_MARK_EMPTY(p);
	}
	
	if (dbm_equal(p, q, dim))
	{
		return true;
	}
	
	printf("Inconsistency detected!\nExpected result:\n");
	dbm_print(p, dim);
	printf("\nIncorrect result:\n");
	dbm_print(q, dim);
	
	return false;
}

/* 
 * Obliczanie nastepnika czasowego (zachowuje postac kanoniczna)
 */
void
dbm_time_successor(DBM_elem *p, DBM_idx dim)
{
	int i;

	assert(p != NULL);
	assert(dbm_is_canonical(p, dim));
	
	for (i = 1; i < dim; i++)
	{
		DBM(p, dim, i, 0) = INF;
	}

	assert(dbm_is_canonical(p, dim));
	
	return;
}

/* 
 * Obliczanie poprzednika czasowego (zachowuje postac kanoniczna)
 */
void
dbm_time_predecessor(DBM_elem *p, DBM_idx dim)
{
	int i, j;
	DBM_elem *e1, *e2;
	
	assert(p != NULL);
	assert(dbm_is_canonical(p, dim));	

	for (j = 1; j < dim; j++)
	{
		e1 = DBM_P(p, dim, 0, j);
		*e1 = DBM_E_0LE; /* DBM(0,j) := (0, <=) */
		
		for (i = 1; i < dim; i++)
		{	
			e2 = DBM_P(p, dim, i, j);
		
			if (*e2 < *e1) /* DBM(i,j) < DBM(0,j) */
			{
				*e1 = *e2;
			}
		}

	}
		
	assert(dbm_is_canonical(p, dim));
	
	return;
}


/* 
 * Obliczanie poprzednika czasowego (nie zachowuje postaci kanonicznej)
 */
void
dbm_time_predecessor_nc(DBM_elem *p, DBM_idx dim)
{
	int j;
	
	assert(p != NULL);
	assert(dbm_is_canonical(p, dim)); /* wejsciowy DBM musi byc w PK */
	
	for (j = 1; j < dim; j++)
	{
		DBM(p, dim, 0, j) = DBM_E_0LE; /* DBM(0,j) := (0, <=) */
	}
	
	return;
}


/*
 * Makro do rozluzniania ograniczen (zamiana < na <=) 
 *
 * Dzialamy tylko jezeli DBM(i,j) != inf oraz DBM(i,j) z ograniczeniem <.
 * Mimo ze obecnosc tego warunku nie jest konieczna, jest on oplacalny.
 */
#define DBM_RELAX_REL(i, j) \
{ \
	DBM_elem *e; \
	e = DBM_P(p, dim, i, j); \
	if (*e != INF && (*e & 1) == REL_LT) *e |= 1; \
}

/*
 * Domkniecie strefy (nie mylic z postacia kanoniczna)
 */
void
dbm_closure(DBM_elem *p, DBM_idx dim)
{
	DBM_idx i, j;
	
	assert(p != NULL);
	assert(dbm_is_canonical(p, dim));
	
	for (i = 0; i < dim; i++)
	{
		for (j = 0; j < dim; j++)
		{
			DBM_RELAX_REL(i, j);
		}
	}
	
	assert(dbm_is_canonical(p, dim));
	
	return;
}

void
dbm_fill(DBM_elem *p, DBM_idx dim)
{
	DBM_idx i;
	
	assert(p != NULL);
	assert(dbm_is_canonical(p, dim));

	/* zaczynamy od 1, bo DBM(0,0) nie potrzeba poprawiac */
	for (i = 1; i < dim; i++)
	{
		DBM_RELAX_REL(i, 0);
		DBM_RELAX_REL(0, i);
	}
	
	assert(dbm_is_canonical(p, dim));
	
	return;
}

/* wynik w PK */
DBM_elem *
dbm_border(DBM_elem *p, DBM_elem *q, DBM_idx dim)
{
	DBM_elem *r;
	
	assert(p != NULL && q != NULL);
	assert(dbm_is_canonical(p, dim));
	assert(dbm_is_canonical(q, dim));
	
	r = dbm_intersection(p, q, dim);
	if (!dbm_empty(r, dim))
	{
		
		/* strefy nie sa rozlaczne; wynik: domkniecie przeciecia */
		/* dbm_closure(r, dim); */ /* DO SPRAWDZENIA: hm, w rzeczywistosci to wcale nie closure? */
		return r;
		
	} 
	else
	{
		
		DBM_idx i, j;
		DBM_elem pe, qe;
		
		dbm_destroy(r); /* nie potrzebujemy juz przeciecia stref */
		
		for (i = 1; i < dim; i++) 
		{
			/* (1) szukamy: DBMp(i,0) = (c, <) i DBMq(0,i) = (-c, <=) */
			pe = DBM(p, dim, i, 0);
			qe = DBM(q, dim, 0, i);
			if (DBM_V(pe) == -DBM_V(qe) && DBM_R(pe) == REL_LT && DBM_R(qe) == REL_LE)
			{
				
				for (j = 1; j < dim; j++)
				{
					/* (2) szukamy: DBMp(j,0) = (c, <=) i DBMq(0,j) = (-c, <), j != i*/
					pe = DBM(p, dim, j, 0);
					qe = DBM(q, dim, 0, j);
					if (j != i && DBM_V(pe) == -DBM_V(qe) && DBM_R(pe) == REL_LE && DBM_R(qe) == REL_LT)
					{
						/* wynik: pusta strefa */
						r = dbm_init(dim);
						DBM_MARK_EMPTY(r);
						return r;
					}
				} /* nie znalezlismy (2) */
				
				/* wynik: Fill(Z) n Z' */
				dbm_fill(p, dim);
				r = dbm_intersection(p, q, dim);
				return r;
			}
			
		} /* nie znalezlismy (1) */
		
		for (i = 1; i < dim; i++)
		{
			/* (3) szukamy: DBMp(i,0) = (c, <=) i DBMq(0,i) = (-c, <) */
			pe = DBM(p, dim, i, 0);
			qe = DBM(q, dim, 0, i);
			if (DBM_V(pe) == -DBM_V(qe) && DBM_R(pe) == REL_LE && DBM_R(qe) == REL_LT)
			{
				/* wynik: Z n Fill(Z') */
				dbm_fill(q, dim);
				r = dbm_intersection(p, q, dim);
				return r;
			}
		} /* nie znalezlismy (3) */
		
		/* wynik: pusta strefa */
		r = dbm_init(dim);
		DBM_MARK_EMPTY(r);
		return r;	
	}
	
}

/* Natychmiastowy poprzednik czasowy (w PK) */
DBM_elem *
dbm_imm_time_predecessor(DBM_elem *p, DBM_elem *q, DBM_idx dim)
{
	DBM_elem *r, *t;
	
	assert(p != NULL && q != NULL);
	assert(dbm_is_canonical(p, dim));
	assert(dbm_is_canonical(q, dim));
	
	t = dbm_border(p, q, dim);
	dbm_time_predecessor_nc(t, dim);
	
	r = dbm_intersection(p, t, dim);
		
	return r;
}

/*
 * Dodawanie kolejnych DBM-ow do zbioru
 */
void
dbmset_add(DBMset_elem **root, DBMset_elem **last, DBM_elem *dbm)
{
	DBMset_elem *new;
	
	new = (DBMset_elem *)smalloc(sizeof(DBMset_elem));
	
 	new->dbm = dbm;
	new->next = NULL;

	if (*root == NULL)
		*root = new;
	else
		(*last)->next = new;
	*last = new;
	
	return;
}

/*
 * Odalokowywanie zbioru DBM-ow (lacznie z elementami zbioru)
 */
void
dbmset_destroy(DBMset_elem *root)
{
	DBMset_elem *cur, *tmp;
	
	cur = root;
	while (cur != NULL)
	{
		assert(cur->dbm != NULL);
		free(cur->dbm);

		tmp = cur;
		cur = cur->next;
		free(tmp);
	}
	
	return;
}

/*
 * Odalokowywanie samej struktury zbioru DBM-ow
 */
void
dbmset_scaffoldDestroy(DBMset_elem *root)
{
	DBMset_elem *cur, *tmp;
	
	cur = root;
	while (cur != NULL)
	{
		tmp = cur;
		cur = cur->next;
		free(tmp);
	}
	
	return;
}

/*
 * Wyswietlanie DBM-ow nalezacych do zbioru
 */
void
dbmset_print(DBMset_elem *root, DBM_idx dim)
{
	DBMset_elem *cur;
	unsigned int i = 1;
	
	cur = root;
	while (cur != NULL)
	{
		printf("DBM %d:\n", i++);
		dbm_print(cur->dbm, dim);
		cur = cur->next;
	}
	
	return;
}

/*
 * Obliczanie roznicy stref
 */
DBMset_elem *
dbm_diff(DBM_elem *p_orig, DBM_elem *q, DBM_idx dim)
{
	DBM_idx i, j;
	DBMset_elem *r = NULL, *last = NULL; /* zbior stref */
	bool done = false, p_used = false;
	DBM_elem *p, *q_c, *t, *z, w;
	
	assert(p_orig != NULL && q != NULL);
	assert(dbm_is_canonical(p_orig, dim));
	assert(dbm_is_canonical(q, dim));
	
	p = dbm_copy(p_orig, dim); /* p bedziemy modyfikowac */

	q_c = dbm_copy(q, dim);
	dbm_closure(q_c, dim);
	
	for (i = 0; i < dim && !done; i++)
	{
		for (j = 0; j < dim && !done; j++)
		{
			 /* dodatkowe sprawdzanie != INF (dodane na szybko dla pewnosci, TODO: upewnic sie czy potrzebne) */
			if (i != j && DBM(q, dim, i, j) != INF)
			{
				
				/* sprawdzamy przeciecie obu stref */
				t = dbm_intersection_cf(p, q, dim); /* t jest w PK */
				
				if (dbm_empty(t, dim))
				{ /* aktualne Z i Z' sa rozlaczne */
					
					dbm_destroy(t);
					
					assert(dbm_is_canonical(p, dim));
					if (!dbm_empty(p, dim)) /* nie dodajemy pustych DBM-ow */
					{
						dbmset_add(&r, &last, p);
						p_used = true; /* p zostalo wykorzystane */
						assert(r != NULL);
					}
					/* nie trzeba zwalniac, bo !p_used (zwolni sie dalej) */
					
					done = true;
					
				} 
				else if (dbm_equal(t, p, dim)) /* Z n Z' jest rowne aktualnemu Z */
				{

					dbm_destroy(t);
					done = true;
					
				}
				else
				{
					
					dbm_destroy(t);
					
					t = dbm_copy(q_c, dim);
					w = DBM_V(DBM(q, dim, i, j));
					DBM(t, dim, i, j) = DBM_ELEM(w, REL_LE);
					DBM(t, dim, j, i) = DBM_ELEM(-w, REL_LE);
					
					z = dbm_intersection_cf(p, t, dim); /* z jest w PK */
					dbm_destroy(t);
					
					if (!dbm_empty(z, dim))
					{
						
						t = dbm_copy(p, dim);
						w = DBM(q, dim, i, j);
						DBM(t, dim, j, i) = DBM_ELEM(-DBM_V(w), DBM_INV_R(w));
						dbm_canonicalize(t, dim); /* przed dodaniem obliczamy PK, TODO: zastanowic sie czy
												   * nie mozna zoptymalizowac (to jest poprawka na szybko)
												   */
						if (!dbm_empty(t, dim)) /* nie dodajemy pustych DBM-ow */
						{
							dbmset_add(&r, &last, t); /* potem nie zwalniamy t, bo juz zostalo "oddane" (inaczej bysmy je utracili) */
							assert(r != NULL);
						}
						else
						{
							dbm_destroy(t);
						}
						assert(w <= DBM(p, dim, i, j)); /* zawezanie ograniczenia */
						DBM(p, dim, i, j) = w;
						dbm_canon1(p, dim, i, j); /* postac kanoniczna p potrzebna jest na wejsciu dbm_equal() */
						
					}

					dbm_destroy(z);
					
				}
				
			}
		}
	}
	
	dbm_destroy(q_c);
	if (!p_used) dbm_destroy(p); /* zwalniamy p tylko jesli nie jest uzywane: TODO moze lepiej operowac na 'if (p != NULL)'? */
	
	return r;
}

/*
 * Sprawdzanie czy wartosci elementow zawieraja sie w dopuszczalnym przedziale.
 */
bool
dbm_valid_val(DBM_elem *p, DBM_idx dim)
{	
	assert(p != NULL);
	
	while (dim-- != 0)
	{
		if ((*p < DBM_RAW_MIN || *p > DBM_RAW_MAX) && *p != INF)
		{
			return false;
		}
		p++;
	}
	
	return true;
}

/*
 * Sprawdzanie czy DBM jest "zdrowy".
 */
bool
dbm_is_sane(DBM_elem *p, DBM_idx dim)
{
	DBM_idx i;
	
	assert(p != NULL);
	
	for (i = 0; i < dim; i++)
	{
		
		/* Element > (0, <=) w pierwszym wierszu macierzy */
		if (DBM(p, dim, 0, i) > DBM_E_0LE)
		{
			printf("Negative clock valuation allowed!\n");
			return false;
		}

		/* czy (0, <=) na calej przekatnej */
		if (DBM(p, dim, i, i) != DBM_E_0LE)
		{
			printf("Invalid element on a diagonal!\n");
			return false;
		}
		
	}
	
	if (dbm_valid_val(p, dim))
	{
		return true;
	}
	else
	{
		printf("Not allowed value found (dbm_valid_val check failed)!\n");
		return false;
	}
	
	return true;
}
