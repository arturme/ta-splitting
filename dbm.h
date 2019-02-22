#ifndef _INC_DBM_H_
#define _INC_DBM_H_

#include <stdlib.h>
#include <stdbool.h> /* troche elegancji ;-) */
#include <assert.h>
/* 
 * Asercje miejscami sa mocno naciagane, ale moga nas zabezpieczac
 * przed samymi soba, wiec nie krepuje sie z ich stosowaniem.
 */

typedef int DBM_elem;
typedef int DBM_val;
typedef int DBM_rel;
typedef unsigned int DBM_idx;
typedef unsigned int DBM_idx_flags; /* unsigned! */

typedef struct dbmset_elem {
	DBM_elem *dbm;
	struct dbmset_elem *next;
} DBMset_elem;

/** prototypy **/
DBM_elem *dbm_init(DBM_idx);
DBM_elem *dbm_init_zero(DBM_idx);
DBM_elem *dbm_init_empty(DBM_idx);
DBM_elem *dbm_rawinit(DBM_idx);
DBM_elem *dbm_copy(DBM_elem *, DBM_idx);
void dbm_print_elem(DBM_elem);
void dbm_xprint_elem(DBM_elem *, DBM_idx, DBM_idx, DBM_idx);
void dbm_print(DBM_elem *, DBM_idx);
void dbm_xprint(DBM_elem *, DBM_idx);
void dbm_constr_le(DBM_elem *, DBM_idx, DBM_idx, DBM_idx, int);
void dbm_constr_lt(DBM_elem *, DBM_idx, DBM_idx, DBM_idx, int);
void dbm_rls_constr(DBM_elem *, DBM_idx, DBM_idx, DBM_idx);
void dbm_canonicalize(DBM_elem *, DBM_idx);
bool dbm_canon1(DBM_elem *, DBM_idx, DBM_idx, DBM_idx);
void dbm_scanon(DBM_elem *, DBM_idx, DBM_idx_flags *);
bool dbm_is_canonical(DBM_elem *, DBM_idx);
bool dbm_equal(DBM_elem *, DBM_elem *, DBM_idx);
DBM_elem *dbm_intersection(DBM_elem *, DBM_elem *, DBM_idx);
DBM_idx_flags *get_flags_array(DBM_idx);
void dbm_intersection_cfX_ip(DBM_elem *p, DBM_elem *q, DBM_idx dim);
void dbm_intersection_cf_ip(DBM_elem *p, DBM_elem *q, DBM_idx dim);
DBM_elem *dbm_intersection_cf(DBM_elem *, DBM_elem *, DBM_idx);
bool dbm_empty(DBM_elem *, DBM_idx);
void dbm_reset(DBM_elem *, DBM_idx, DBM_idx);
void dbm_invreset(DBM_elem *, DBM_idx, DBM_idx);
bool dbm_check_invreset(DBM_elem *, DBM_elem *, DBM_idx, DBM_idx);
void dbm_time_successor(DBM_elem *, DBM_idx);
void dbm_time_predecessor(DBM_elem *, DBM_idx);
void dbm_time_predecessor_nc(DBM_elem *, DBM_idx);
void dbm_closure(DBM_elem *, DBM_idx);
void dbm_fill(DBM_elem *, DBM_idx);
DBM_elem *dbm_border(DBM_elem *, DBM_elem *, DBM_idx);
DBM_elem *dbm_imm_time_predecessor(DBM_elem *, DBM_elem *, DBM_idx);
void dbmset_add(DBMset_elem **, DBMset_elem **, DBM_elem *);
void dbmset_destroy(DBMset_elem *);
void dbmset_scaffoldDestroy(DBMset_elem *);
void dbmset_print(DBMset_elem *, DBM_idx);
DBMset_elem *dbm_diff(DBM_elem *, DBM_elem *, DBM_idx);
bool dbm_valid_val(DBM_elem *, DBM_idx);
bool dbm_is_sane(DBM_elem *, DBM_idx);

/*
 * Zwalnianie pamieci przydzielonej na DBM
 */
static inline void
dbm_destroy(DBM_elem *p)
{
	free(p);
	
	return;
}

#endif /* !_INC_DBM_H_ */
