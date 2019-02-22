#ifndef _INC_ANPROC_H_
#define _INC_ANPROC_H_

#include <limits.h>
#include <stdbool.h>
#include "anread.h"
#include "antypes.h"

#ifndef NDEBUG
	#define VERBOSE
#endif

/** prototypy **/
void net_init(AutNet *);
void prod_clocks_mkmap(void);
void prod_clock_maps_free(void);
Clock_idx prod_clock_idx2pidx(Aut_idx, Clock_idx);
ProdClock prod_clock_pidx2idx(Clock_idx);
Loc_idx *prod_loc_initial(void);
Loc_idx *prod_loc_copy(Loc_idx *);
bool *prod_loc_acts(Loc_idx *);
void prod_loc_show(Loc_idx *prod_loc);
ProdLoc_set *prod_loc_set_add(Loc_idx *, DBM_elem *, ProdLoc_set **, ProdLoc_set **);
void prod_loc_set_raw_add(ProdLoc_set *, ProdLoc_set **, ProdLoc_set **);
bool prod_loc_set_is_in(Loc_idx *, ProdLoc_set *);
ProdLoc_set *prod_loc_set_getp(Loc_idx *, ProdLoc_set *);
ProdLoc_set *prod_loc_set_raw_pick(ProdLoc_set **);
void prod_loc_set_show(ProdLoc_set *);
void prod_loc_set_free(ProdLoc_set *);
void trans_add(ProdLoc_set *, ProdLoc_set *, ProdTrLoc_set *);
bool ploc_in_src_locs(ProdLoc_set *, SrcLoc_set *);
void trloc_set_free(TrLoc_set **);
bool constr_nonempty(Constr *);
void constr2dbm(Aut_idx, DBM_elem *, Constr *);
void clocks_collect(ClockIdx_set **, ClockIdx_set **, Aut_idx, Clock_idx *, Clock_idx);
void colclocks_free(ClockIdx_set *);
DBM_elem *prod_loc_inv(Loc_idx *);
DBM_elem *prod_loc_inv0(Loc_idx *);
ProdTrLoc_set *prod_loc_succ(Loc_idx *, Act_idx);
ProdTrLoc_set *prtl_free_getNext(ProdTrLoc_set *);
void prod_show(AutNet *);

extern Act_idx nact;
extern DBM_idx dbm_size;
extern DBM_elem *zeroDBM;
extern ProdLoc_set *init_loc_set;
extern Property_set *ppts;

#endif /* !_INC_ANPROC_H_ */
