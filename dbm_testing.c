#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "dbm.h"
#include "dbm_macro.h"

/*
 * Testowanie funkcji dbm_imm_time_predecessor
 */
void test_itp(void)
{
	DBM_elem *a, *b, *c, *c_exp;
	DBM_idx dim;
	
	/* przypadki 2-wymiarowe */
	dim = 3;
	
	/* (a) Z */
	a = dbm_init(dim);
	DBM(a, dim, 0, 1) = DBM_ELEM(0, REL_LE);
	DBM(a, dim, 0, 2) = DBM_ELEM(0, REL_LE);
	DBM(a, dim, 1, 0) = DBM_ELEM(5, REL_LT);
	DBM(a, dim, 1, 2) = DBM_ELEM(5, REL_LT);
	DBM(a, dim, 2, 0) = DBM_ELEM(5, REL_LT);
	DBM(a, dim, 2, 1) = DBM_ELEM(5, REL_LT);
	assert(dbm_is_canonical(a, dim));
	
	/* (b) Z' */
	b = dbm_init(dim);
	DBM(b, dim, 0, 1) = DBM_ELEM(-2, REL_LT);
	DBM(b, dim, 0, 2) = DBM_ELEM(-2, REL_LT);
	DBM(b, dim, 1, 0) = DBM_ELEM(6, REL_LT);
	DBM(b, dim, 1, 2) = DBM_ELEM(4, REL_LT);
	DBM(b, dim, 2, 0) = DBM_ELEM(7, REL_LT);
	DBM(b, dim, 2, 1) = DBM_ELEM(5, REL_LT);
	assert(dbm_is_canonical(b, dim));
	
	/* spodziewany wynik */
	c_exp = dbm_init(dim);
	DBM(c_exp, dim, 0, 1) = DBM_ELEM(0, REL_LE);
	DBM(c_exp, dim, 0, 2) = DBM_ELEM(0, REL_LE);
	DBM(c_exp, dim, 1, 0) = DBM_ELEM(5, REL_LT);
	DBM(c_exp, dim, 1, 2) = DBM_ELEM(3, REL_LT);
	DBM(c_exp, dim, 2, 0) = DBM_ELEM(5, REL_LT);
	DBM(c_exp, dim, 2, 1) = DBM_ELEM(3, REL_LT);
	assert(dbm_is_canonical(c_exp, dim));
	
	printf("Z = \n");
	dbm_print(a, dim); printf("\n");
	
	printf("Z' = \n");
	dbm_print(b, dim); printf("\n");
	
	printf("spodziewany Z /||\\ Z' = \n");
	dbm_print(c_exp, dim); printf("\n");
	
	c = dbm_imm_time_predecessor(a, b, dim);
	printf("otrzymany Z /||\\ Z' = \n");
	dbm_print(c, dim);
	
	dbm_destroy(a);
	dbm_destroy(b);
	dbm_destroy(c);
	dbm_destroy(c_exp);
	printf("---------------------------\n");
	
	return;
}

/*
 * Testowanie obliczania roznicy stref
 */
void test_diff(void)
{
	DBM_elem *a, *b;
	DBM_idx dim = 3;
	DBMset_elem *x;
	
	a = dbm_init(dim);
	DBM(a, dim, 0, 1) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 0, 2) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 1, 0) = DBM_ELEM(4, REL_LT);
	DBM(a, dim, 1, 2) = DBM_ELEM(2, REL_LT);
	DBM(a, dim, 2, 0) = DBM_ELEM(4, REL_LE);
	DBM(a, dim, 2, 1) = DBM_ELEM(2, REL_LE);

	b = dbm_init(dim);
	DBM(b, dim, 0, 1) = DBM_ELEM(-3, REL_LT);
	DBM(b, dim, 0, 2) = DBM_ELEM(-3, REL_LE);
	DBM(b, dim, 1, 0) = DBM_ELEM(5, REL_LE);
	DBM(b, dim, 1, 2) = DBM_ELEM(2, REL_LE);
	DBM(b, dim, 2, 0) = DBM_ELEM(5, REL_LE);
	DBM(b, dim, 2, 1) = DBM_ELEM(2, REL_LT);

	x = dbm_diff(a, b, dim);

	dbm_destroy(a);
	dbm_destroy(b);
	
	dbmset_print(x, dim);
	dbmset_destroy(x);
	
	return;
}

/*
 * Testowanie szybkosci obliczania roznicy stref
 */
void test_speed_diff(void)
{
	DBM_elem *a, *b;
	DBM_idx dim = 6;
	DBMset_elem *x;
	
	a = dbm_init(dim);
	DBM(a, dim, 0, 1) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 0, 2) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 1, 0) = DBM_ELEM(4, REL_LT);
	DBM(a, dim, 1, 2) = DBM_ELEM(2, REL_LT);
	DBM(a, dim, 2, 0) = DBM_ELEM(4, REL_LE);
	DBM(a, dim, 2, 1) = DBM_ELEM(2, REL_LE);

	b = dbm_init(dim);
	DBM(b, dim, 0, 1) = DBM_ELEM(-3, REL_LT);
	DBM(b, dim, 0, 2) = DBM_ELEM(-3, REL_LE);
	DBM(b, dim, 1, 0) = DBM_ELEM(5, REL_LE);
	DBM(b, dim, 1, 2) = DBM_ELEM(2, REL_LE);
	DBM(b, dim, 2, 0) = DBM_ELEM(5, REL_LE);
	DBM(b, dim, 2, 1) = DBM_ELEM(2, REL_LT);
	
	dbm_canonicalize(a, dim);
	dbm_canonicalize(b, dim);
	
	{
		unsigned int i;
				
		printf("\n");
		for (i = 0; i < 10000000; i++) {
			if ((i+1) % 10 == 0)
				printf("\r%d", i+1);
			fflush(stdout);

			x = dbm_diff(a, b, dim);
			dbmset_destroy(x);
	
		}
		printf("\n");
	}

	dbm_destroy(a);
	dbm_destroy(b);
	
	return;
}

/*
 * Testowanie obliczania przeciecia stref
 */
void test_intersection(void)
{
	DBM_elem *a, *b;
	DBM_idx dim = 3;
	DBM_elem *x1, *x2;
	
	a = dbm_init(dim);
	DBM(a, dim, 0, 1) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 0, 2) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 1, 0) = DBM_ELEM(4, REL_LT);
	DBM(a, dim, 1, 2) = DBM_ELEM(2, REL_LT);
	DBM(a, dim, 2, 0) = DBM_ELEM(4, REL_LE);
	DBM(a, dim, 2, 1) = DBM_ELEM(2, REL_LE);

	b = dbm_init(dim);
	DBM(b, dim, 0, 1) = DBM_ELEM(-3, REL_LT);
	DBM(b, dim, 0, 2) = DBM_ELEM(-3, REL_LE);
	DBM(b, dim, 1, 0) = DBM_ELEM(5, REL_LE);
	DBM(b, dim, 1, 2) = DBM_ELEM(2, REL_LE);
	DBM(b, dim, 2, 0) = DBM_ELEM(5, REL_LE);
	DBM(b, dim, 2, 1) = DBM_ELEM(2, REL_LT);

	x1 = dbm_intersection(a, b, dim);	
	dbm_print(x1, dim);
	
	x2 = dbm_intersection_cf(a, b, dim);
	dbm_print(x2, dim);
	
	if (dbm_equal(x1, x2, dim))
		printf("Oba identyczne\n");
	else
		printf("ROZNE, BLAD\n");
	
	dbm_destroy(x1);
	dbm_destroy(x2);

	dbm_destroy(a);
	dbm_destroy(b);
	
	return;
}

/*
void test_speed_isect(void)
{
	DBM_elem *a, *b;
	DBM_idx dim = 6;
	DBM_elem *x1, *x2;
	
	a = dbm_init(dim);
	DBM(a, dim, 0, 1) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 0, 2) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 1, 0) = DBM_ELEM(4, REL_LT);
	DBM(a, dim, 1, 2) = DBM_ELEM(2, REL_LT);
	DBM(a, dim, 2, 0) = DBM_ELEM(4, REL_LE);
	DBM(a, dim, 2, 1) = DBM_ELEM(2, REL_LE);

	b = dbm_init(dim);
	DBM(b, dim, 0, 1) = DBM_ELEM(-3, REL_LT);
	DBM(b, dim, 0, 2) = DBM_ELEM(-3, REL_LE);
	DBM(b, dim, 1, 0) = DBM_ELEM(5, REL_LE);
	DBM(b, dim, 1, 2) = DBM_ELEM(2, REL_LE);
	DBM(b, dim, 2, 0) = DBM_ELEM(5, REL_LE);
	DBM(b, dim, 2, 1) = DBM_ELEM(2, REL_LT);

	dbm_canonicalize(a, dim);
	dbm_canonicalize(b, dim);

	x1 = dbm_intersection(a, b, dim);
	
	{
		unsigned int i;
				
		printf("\n");
		for (i = 0; i < 10000000; i++) {
			if ((i+1) % 10 == 0)
				printf("\r%d", i+1);
			fflush(stdout);

			x2 = dbm_intersection_cf(a, b, dim);
			dbm_destroy(x2);
	
		}
		printf("\n");
	}
	
	x2 = dbm_intersection_cf2(a, b, dim);
	if (dbm_equal(x1, x2, dim))
		printf("Oba identyczne\n");
	else
		printf("ROZNE, BLAD\n");
	
	dbm_destroy(x1);
	dbm_destroy(x2);
	dbm_destroy(a);
	dbm_destroy(b);
	
	return;
}
*/

/*
 * Testowanie obliczania postaci kanonicznej
 */
void test_c14n(void)
{
	DBM_elem *a;
	
	DBM_idx dim = 3;

	a = dbm_init(dim);
	DBM(a, dim, 0, 1) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 0, 2) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 1, 0) = DBM_ELEM(4, REL_LE);
	DBM(a, dim, 1, 2) = DBM_ELEM(2, REL_LE);
	DBM(a, dim, 2, 0) = DBM_ELEM(4, REL_LE);
	DBM(a, dim, 2, 1) = DBM_ELEM(2, REL_LE);

	assert(dbm_is_canonical(a, dim));
	/* zaciesniamy x2 - x0 <= 4 do 3 */
	DBM(a, dim, 2, 0) = DBM_ELEM(3, REL_LE);
	/* obliczamy postac kanoniczna po ZACIESNIENIU ograniczenia */
	dbm_canon1(a, dim, 2, 0);
	assert(dbm_is_canonical(a, dim));
	
	dbm_destroy(a);

	return;
}

void test_closure_speed(void)
{
	DBM_elem *a;
	DBM_idx dim = 1000;
	
	a = dbm_init(dim);
	
	DBM(a, dim, 0, 1) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 0, 2) = DBM_ELEM(-2, REL_LE);
	DBM(a, dim, 1, 0) = DBM_ELEM(4, REL_LE);
	DBM(a, dim, 1, 2) = DBM_ELEM(2, REL_LE);
	DBM(a, dim, 2, 0) = DBM_ELEM(4, REL_LE);
	DBM(a, dim, 2, 1) = DBM_ELEM(2, REL_LE);
	
	dbm_canonicalize(a, dim);
	
	dbm_closure(a, dim);
		
	return;
}

void pbs(int n) {
	unsigned int i;
	i = 1<<(sizeof(n) * 8 - 1); while (i > 0) { if (n & i) printf("1"); else printf("0"); i >>= 1; }
	printf("\n");
} 


int
main(void)
{	
	
	assert(printf("assertions are enabled!\n"));
	
	/* Macierz od Bengtssona (w jego publikacji jest najprawdopodobniej blad)
	DBM(a, dim, 1, 0) = DBM_ELEM(20, REL_LT);
	DBM(a, dim, 1, 2) = DBM_ELEM(-10, REL_LE);
	DBM(a, dim, 2, 0) = DBM_ELEM(20, REL_LE);
	DBM(a, dim, 2, 1) = DBM_ELEM(10, REL_LE);
	*/
	/* Macierz z Clarke MC 
	DBM(a, dim, 0, 1) = DBM_ELEM(-1, REL_LE);
	DBM(a, dim, 0, 2) = DBM_ELEM(0, REL_LT);
	DBM(a, dim, 1, 0) = INF;
	DBM(a, dim, 1, 2) = DBM_ELEM(2, REL_LT);
	DBM(a, dim, 2, 0) = DBM_ELEM(2, REL_LE);
	DBM(a, dim, 2, 1) = INF;
	*/
	/* Testowanie Bengtssonowego down(D) z jego przykladem (w efekcie powinno byc DBM(0,1) = (-1, <=)):
	DBM(a, dim, 0, 1) = DBM_ELEM(-4, REL_LE);
	DBM(a, dim, 0, 2) = DBM_ELEM(-1, REL_LE);
	DBM(a, dim, 1, 0) = DBM_ELEM(5, REL_LE);
	DBM(a, dim, 1, 2) = DBM_ELEM(4, REL_LE);
	DBM(a, dim, 2, 0) = DBM_ELEM(2, REL_LE);
	DBM(a, dim, 2, 1) = DBM_ELEM(-1, REL_LE);
	*/
	/*dbm_print(a, dim);
	dbm_time_successor(a, dim);
	dbm_time_predecessor(a, dim);
	dbm_canonicalize(a, dim);
	*/
	/*
	{
	
		DBM(a, dim, 0, 2) = DBM_ELEM(-3, REL_LE);
		DBM(a, dim, 1, 0) = DBM_ELEM(4, REL_LE);
		DBM(a, dim, 1, 2) = DBM_ELEM(1, REL_LE);
		DBM(a, dim, 2, 0) = DBM_ELEM(7, REL_LE);
		DBM(a, dim, 2, 1) = DBM_ELEM(5, REL_LE);
	
		dbm_print(a, dim);
		dbm_reset(a, dim, 1);
		dbm_print(a, dim);
	}
	*/
	
	/*
	{
		unsigned int i;
		DBM_elem *c;
		
		printf("\n");
		for (i = 0; i < 1000; i++) {
			if ((i+1) % 10 == 0)
				printf("\r%d", i+1);
			fflush(stdout);
	
			c = dbm_intersection(a,b, dim);
			free(c);
	
		}	
		printf("\n");
	}
	*/

	/* odwrotny reset */
	/*{
		DBM_elem *a;
		DBM_idx dim;
		
		dim = 4;
		a = dbm_init(dim);
		
		assert(dbm_is_sane(a, dim));
		DBM(a, dim, 0, 1) = DBM_ELEM(0, REL_LE);
		DBM(a, dim, 0, 2) = DBM_ELEM(-2, REL_LE);
		DBM(a, dim, 1, 0) = DBM_ELEM(7, REL_LE);
		DBM(a, dim, 1, 2) = DBM_ELEM(1, REL_LE);
		DBM(a, dim, 2, 0) = DBM_ELEM(7, REL_LE);
		DBM(a, dim, 2, 1) = DBM_ELEM(5, REL_LE);
		DBM(a, dim, 0, 3) = DBM_ELEM(-1, REL_LE);
		DBM(a, dim, 3, 0) = DBM_ELEM(3, REL_LE);
		DBM(a, dim, 3, 2) = DBM_ELEM(-1, REL_LE);
		DBM(a, dim, 2, 3) = DBM_ELEM(1, REL_LE);
		DBM(a, dim, 1, 3) = DBM_ELEM(-1, REL_LE);
		DBM(a, dim, 3, 1) = DBM_ELEM(2, REL_LE);
		dbm_canonicalize(a, dim);
		
		dbm_print(a, dim);
		dbm_invreset(a, dim, 1);
		printf("\n");
		dbm_print(a, dim);

		dbm_destroy(a);
	}*/

	//test_intersection();
	//test_speed_isect();
	//test_diff();
	//test_speed_diff();
	//test_itp();
	//test_closure_speed();
	
	/*{
		int i;
		DBM_idx dim = 10;
		DBM_elem *a = dbm_init(dim);

		for (i = 0; i < 1000000; i++) {
			DBM(a, dim, 0, 1) = DBM_ELEM(0, REL_LE);
			DBM(a, dim, 0, 2) = DBM_ELEM(-2, REL_LE);
			DBM(a, dim, 1, 0) = DBM_ELEM(7, REL_LE);
			DBM(a, dim, 1, 2) = DBM_ELEM(1, REL_LE);
			DBM(a, dim, 2, 0) = DBM_ELEM(7, REL_LE);
			DBM(a, dim, 2, 1) = DBM_ELEM(5, REL_LE);
			DBM(a, dim, 0, 3) = DBM_ELEM(-1, REL_LE);
			DBM(a, dim, 3, 0) = DBM_ELEM(3, REL_LE);
			DBM(a, dim, 3, 2) = DBM_ELEM(-1, REL_LE);
			DBM(a, dim, 2, 3) = DBM_ELEM(1, REL_LE);
			DBM(a, dim, 1, 3) = DBM_ELEM(-1, REL_LE);
			DBM(a, dim, 3, 1) = DBM_ELEM(2, REL_LE);
			dbm_canonicalize(a, dim);
		}
	}*/
	
	// {
	// 	DBM_elem *a;
	// 	
	// 	DBM_idx dim = 2;
	// 	a = dbm_init(dim);
	// 	dbm_constr_lt(a, dim, 0, 1, -5);
	// 	dbm_constr_lt(a, dim, 1, 0, 5);
	// 	dbm_canonicalize(a, dim);
	// 	//pbs(DBM_VAL_MIN);
	// 	dbm_print(a, dim);
	// 
	// }
	
	{
		DBM_elem *a;
		DBM_idx dim = 3;
		a = dbm_init(dim);
		
		DBM(a, dim, 0, 1) = DBM_ELEM(-1, REL_LE);
		DBM(a, dim, 0, 2) = DBM_ELEM(0, REL_LT);
		DBM(a, dim, 1, 0) = INF;
		DBM(a, dim, 1, 2) = DBM_ELEM(2, REL_LT);
		DBM(a, dim, 2, 0) = DBM_ELEM(2, REL_LE);
		DBM(a, dim, 2, 1) = INF;
		dbm_canonicalize(a, dim);
		dbm_print(a, dim);
	}
	

	return 0;
}

