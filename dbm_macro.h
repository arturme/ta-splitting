#ifndef _INC_DBM_MACRO_H_
#define _INC_DBM_MACRO_H_

#include <limits.h>

/* 
 * Definicje dla czytelnosci, a nie dla elastycznosci.
 * Zmiana wplywa na porzadek zbioru.
 */
#define REL_LT 0 /* <  */
#define REL_LE 1 /* <= */
#define INF INT_MAX /* nieskonczonosc */

#define DBM_RAW_MAX (INT_MAX >> 1) /* co z nieskonczonoscia? */
#define DBM_RAW_MIN (INT_MIN >> 1) 
#define DBM_VAL_MAX (DBM_RAW_MAX >> 1)
/*
 * Jak abs(DBM_VAL_MIN) jest takie samo jak _MAX, to nie trzeba sprawdzac 
 * wartosci po zmianie znaku.
 */
#define DBM_VAL_MIN ((DBM_RAW_MIN+2) >> 1)

/* Z makrami jest o wieeeeele szybciej... */
#define DBM(a, b, c, d) a[((c)*(b)+(d))]
#define DBM_P(a, b, c, d) &a[((c)*(b)+(d))]
#define DBM_MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define DBM_SUM(a, b) ( ((a) == INF || (b) == INF) ? INF : ((a) + (b) - ( ( ((a) | (b)) & 1 ) )) )
#define DBM_ELEM(a, b) ( ((a) << 1) | (b) ) /* tworzenie elementu */
#define DBM_E_0LE 1  /* (0, <=)  */
#define DBM_E_0LT 0  /* (0, <)   */
#define DBM_E_NEG -1 /* (-1, <=) */
#define DBM_V(a) ( (a) >> 1 ) /* pobieranie wartosci z elementu */
#define DBM_R(a) ( (a) & 1 ) /* pobieranie rodzaju relacji */
#define DBM_INV_R(a) ( ((a) & 1) ^ 1 ) /* pobieranie odwrotnej relacji (<= daje <, a < daje <=) */
#define DBM_MARK_EMPTY(a) ( *(a) = DBM_E_NEG )

#define GET_ASIZE(a) ( ((a) + (sizeof(DBM_idx_flags)*8 - 1)) >> 5 ) /* sprytny pomysl z ">> 5" wziety z Uppaala */
#define GET_IDX(a) ( (((a) + (sizeof(DBM_idx_flags)*8)) >> 5) - 1 )
#define GET_BIT_IDX(a) ( (a) % (sizeof(DBM_idx_flags)*8) )
#define SET_FLAG(f, a) \
{ \
	DBM_idx_flags *t; \
	t = &f[GET_IDX(a)]; \
	*t = ( *t | (1 << GET_BIT_IDX(a)) ); \
}
#define IS_FLAGGED(f, a) ( (f[GET_IDX(a)] >> GET_BIT_IDX(a)) & 1 )

#define FERROR(s) \
{ \
	printf("%s, line %d: %s\n", __FILE__, __LINE__, s); \
	exit(1); \
}

#define DBM_VSC(a) dbm_sval((a), __FILE__, __LINE__) /* value sanity check */
#define DBM_LVSC(a) dbm_lsval((a), __FILE__, __LINE__) /* min. value sanity check */

/* Moze zabieg ze zwracaniem wartosci jest przesadzony...?
 * Moze lepiej przejsc na sprawdzanie "przed" uzyciem a nie "w momencie" uzycia?
 */
static inline DBM_elem dbm_sval(DBM_elem e, char *fname, int line)
{
	if (e < DBM_RAW_MIN || e > DBM_RAW_MAX) {
		if (e != INF) {
			printf("DBM value overflow, %s:%d\n", fname, line);
			exit(1);
		}
	}
	
	return e;
}

static inline DBM_elem dbm_lsval(DBM_elem e, char *fname, int line)
{
	if (e < DBM_RAW_MIN) {
			printf("DBM value overflow, %s:%d\n", fname, line);
			exit(1);
	}
	
	return e;
}

#endif /* !_INC_DBM_MACRO_H_ */
