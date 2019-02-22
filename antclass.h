#ifndef _INC_ANTCLASS_H_
#define _INC_ANTCLASS_H_

#include <limits.h>
#include <stdbool.h>
#include "antypes.h"

#ifndef NDEBUG
	#define TC_VERBOSE
#endif

#define DEPTH_INF UINT_MAX

TimedClass_set *tclass_create(ProdLoc_set *, DBM_elem *, DBM_elem *, dpt_t);
TimedClass_set *tclass_create_nd(ProdLoc_set *, DBM_elem *, DBM_elem *);
TimedClass_set *tclass_create_base(ProdLoc_set *);
void tclass_print(TimedClass_set *);
void tclass_add(TimedClass_set *);
void tclass_del(TimedClass_set *);
ProdTrans_set *trans_get(ProdLoc_set *, ProdLoc_set *);
DBM_elem *tclass_readZoneDBM(TimedClass_set *);
DBM_elem *tclass_readCorDBM(TimedClass_set *);
DBM_elem *tclass_writeZoneDBM(TimedClass_set *);
DBM_elem *tclass_writeCorDBM(TimedClass_set *);
/*DBM_elem *tclass_consumeZoneDBM(TimedClass_set *);
DBM_elem *tclass_consumeCorDBM(TimedClass_set *);*/
void tclass_replaceCorDBM(TimedClass_set *, DBM_elem *);
void tclass_replaceZoneDBM(TimedClass_set *, DBM_elem *);
void tclass_depthUpdateInf(TimedClass_set *);
void tclass_depthUpdateIncr(TimedClass_set *, TimedClass_set *);
bool tclass_hasInit(TimedClass_set *);
bool tclass_is_in_ReachStable(TimedClass_set *);
void tclass_ReachStable_add(TimedClass_set *, TimedClassRS_set **, TimedClassRS_set **);
void tclass_ReachStable_addInit(TimedClass_set *, TimedClassRS_set **, TimedClassRS_set **);
void tclass_ReachStable_delete(TimedClass_set *, TimedClassRS_set **, TimedClassRS_set **);
void tclass_ReachStable_free(TimedClassRS_set **);
void tclass_mark_reach_unst(TimedClass_set *, TimedClassRS_set **, TimedClassRS_set **);
void tclass_rs_mark_stable(TimedClassRS_set *);
void tclass_mark_stable(TimedClass_set *);
void tclass_mark_unstable(TimedClass_set *);
void tclass_mark_preds_unstable(TimedClass_set *);
TimedClassRS_set *tclass_pickReachUnst(TimedClassRS_set *);
bool tclass_ReachStable_is_sorted(TimedClassRS_set *);
void tclass_ReachStable_print(TimedClassRS_set *);
void tclass_saveStable(TimedClass_set *, TimedClass_set *);
void tclass_forgetPreStability(TimedClass_set *);
void tclass_forgetSuccStability(TimedClass_set *);
void tclass_tcPtrDel(TimedClass_set *, TClassPtr_set **);
bool tclass_is_in_tcPtrs(TimedClass_set *, TClassPtr_set *);
bool tclass_isStableSucc(TimedClass_set *, TimedClass_set *);
bool tclass_isReallyStableSucc(TimedClass_set *, TimedClass_set *);
unsigned int tclass_count(ProdLoc_set *);

#endif /* !_INC_ANTCLASS_H_ */
