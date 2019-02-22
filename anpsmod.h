#ifndef _INC_ANPSMOD_H_
#define _INC_ANPSMOD_H_

#include <stdbool.h>
#include "antypes.h"

#ifndef NDEBUG
	#define PSM_VERBOSE
#endif

/* wylaczanie cache'u stabilnosci */
/*
#define NO_STCACHE
*/

DBM_elem *psmodel_pre(ProdTrans_set *, ProdLoc_set *, ProdLoc_set *, DBM_elem *, DBM_elem *);
bool psmodel_pre_eq(DBM_elem *, ProdTrans_set *, ProdLoc_set *, ProdLoc_set *, DBM_elem *, DBM_elem *);
bool psmodel_pre_empty(ProdTrans_set *, ProdLoc_set *, ProdLoc_set *, DBM_elem *, DBM_elem *);
DBM_elem *psmodel_post(ProdTrans_set *, ProdLoc_set *, DBM_elem *, DBM_elem *);
bool psmodel_post_empty(ProdTrans_set *, ProdLoc_set *, DBM_elem *, DBM_elem *);

void psmodel_split(TimedClass_set *, TimedClassRS_set **, TimedClassRS_set **);
bool psmodel_stabilityCheck(TimedClass_set *, TimedClassRS_set *);
void psmodel_stableHandler(TimedClass_set *, TimedClassRS_set **, TimedClassRS_set **);
bool psmodel_realSplit_isDefined(TimedClass_set *, TimedClass_set *, ProdTrans_set *);
void psmodel_realSplit(TimedClass_set *, TimedClass_set *, ProdTrans_set *, TimedClassRS_set **, TimedClassRS_set **);

void psmodel_init(ProdLoc_set **, ProdLoc_set **, TimedClassRS_set **, TimedClassRS_set **);
void psmodel_builder(AutNet *);

bool ppty_check(Loc_idx *);


#endif /* !_INC_ANPSMOD_H_ */
