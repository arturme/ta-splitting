#ifndef _INC_ANTYPES_H_
#define _INC_ANTYPES_H_

#include <stdbool.h>
#include "dbm.h"

typedef unsigned int idx_t;

typedef idx_t Aut_idx;
typedef idx_t Loc_idx;
typedef idx_t Act_idx;
typedef idx_t Constr_idx;

/* Clock_idx nie jest unsigned, bo ujemne wartosci uzywane jako oznaczenie konca listy */
typedef int Clock_idx;

typedef unsigned int dpt_t;

typedef unsigned int Constr_rel;
typedef char *Clock;
typedef unsigned int Loc_type;

typedef struct constr {
	Clock_idx l_clk;
	Clock_idx r_clk;
	Constr_rel rel;
	int	val;
} Constr;

typedef struct constr_set {
	Constr constr;
	struct constr_set *next;
} Constr_set;

typedef struct loc {
	char *name;
	Constr *inv;
	Loc_type type;
} Loc;

typedef struct loc_set {
	Loc loc;
	struct loc_set *next;
} Loc_set;

typedef unsigned int Act_type;

typedef struct act {
	char *name;
	Act_type type;
} Act;

/* zbior akcji */
typedef struct act_set {
	Act act;
	struct act_set *next;
} Act_set;

/* zbior zegarow */
typedef struct clock_set {
	Clock clock;
	struct clock_set *next;
} Clock_set;

/* zbior przejsc */
typedef struct transition_set {
	Act_idx act;
	Constr *guard;
	Clock_idx *clocks;
	Clock_idx nclocks;
	struct transition_set *next;
} Trans_set;

/* automat */
typedef struct automaton {
	char *name;			/* nazwa automatu */
	Clock_idx nclks;	/* liczba zegarow */
	Clock *clks;		/* zegary */
	Loc_idx nloc;		/* liczba lokacji */
	Loc *loc;			/* lokacje */
	Loc_idx init_loc;	/* lokacja poczatkowa */
	Trans_set **trans;	/* tranzycje */
	bool *uact;			/* akcje uzywane w automacie */
} Aut;

/* zbior automatow */
typedef struct automaton_set {
	char *name;			/* nazwa automatu */
	Clock_idx nclks;	/* liczba zegarow */
	Clock *clks;		/* zegary */
	Loc_idx nloc;		/* liczba lokacji */
	Loc *loc;			/* lokacje */
	Loc_idx init_loc;	/* lokacja poczatkowa */
	Trans_set **trans;	/* tranzycje */
	bool *uact;			/* akcje uzywane w automacie */
	struct automaton_set *next;
} Aut_set;

/* wlasnosci: konfiguracja lokacji */
typedef struct prod_conf_set {
	Aut_idx aut;
	Loc_idx loc;
	struct prod_conf_set *next;
} ProdConf_set;

/* wlasnosci: szukane konfiguracje lokacji */
typedef struct property_set {
	ProdConf_set *prop;
	char *name;
	struct property_set *next;
} Property_set;

/* siec automatow */
typedef struct autnet {
	Aut *aut;
	Aut_idx naut;
	Act *act;
	Act_idx nact;
	Property_set *ppts; /* wlasnosci sieci */
} AutNet;

/* zbior lokacji produktowych */
typedef struct prod_loc_set {
	Loc_idx *loc;
	DBM_elem *inv;
	struct src_ploc_set *src_locs;		/* lokacje z ktorych ta lokacja jest osiagalna (+przejscia) */
	struct src_ploc_set *last_src_loc;	/* zapisujemy ostatnio dodana lokacje zrodlowa */
	struct dst_ploc_set *dst_locs;		/* lokacje docelowe osiagalne przez przejscia z tej lokacji */
	bool trans_complete;
	struct timed_class_set *classes;
	struct timed_class_set *last_class;
	struct prod_loc_set *next;
} ProdLoc_set;

/* struktura do grupowania przejsc ze wzgledu na lokacje docelowa (wiazana z lokacja zrodlowa) */
typedef struct dst_ploc_set {
	struct prod_trans_set *trans;		/* struktura z odkrytymi przejsciami */
	struct prod_trans_set *last_trans;	/* ostatnio dodane przejscie */
	struct prod_loc_set *loc_set;		/* lokacja docelowa */
	struct dst_ploc_set *next;
} DstLoc_set;

/* 
 * struktura do zapisu lokacji z ktorych mozna sie dostac do biezacej; dodatkowo zawiera
 * wskaznik na pierwszy element listy przejsc do niej prowadzacych.
 */
typedef struct src_ploc_set {
	struct prod_loc_set *loc_set;		/* lokacja zrodlowa */
	struct prod_trans_set *trans;		/* lista przejsc (ten sam kawalek pamieci co w dst_ploc_set) */
	struct src_ploc_set *next;
} SrcLoc_set;

typedef struct clock_idx_set {
	Clock_idx idx;
	struct clock_idx_set *next;
} ClockIdx_set;

typedef struct prod_trans_set {
	Act_idx act;
	DBM_elem *guard;
	ClockIdx_set *clocks;	/* zegary do zresetowania */
	struct prod_trans_set *next;
} ProdTrans_set;

typedef struct prod_trans_loc_set {
	Act_idx act;
	DBM_elem *guard;
	ClockIdx_set *clocks;	/* zegary do zresetowania */
	Loc_idx *loc;			/* docelowa lokacja */
	struct prod_trans_loc_set *next;
} ProdTrLoc_set;

typedef struct trans_loc_set {
	Trans_set *trans;
	Loc_idx loc;
	struct trans_loc_set *next;
} TrLoc_set;

typedef struct prod_clock {
	Aut_idx par_aut;	/* automat-rodzic (skladowy) */
	Clock_idx par_idx;	/* index zegara u rodzica */
} ProdClock;

/* Zbior reachable-stable */
struct timed_class_rs_set {
	struct timed_class_set *tclass;
	bool stable; /* czy klasa nalezy do zbioru stable */
	struct timed_class_rs_set *next;
	struct timed_class_rs_set *prev;
};
typedef struct timed_class_rs_set TimedClassRS_set;

struct timed_class_set {
	DBM_elem *dbm;					/* strefa */
	DBM_elem *cor_dbm;				/* cor klasy */
	unsigned int depth;
	ProdLoc_set *loc_set;			/* wskaznik na element zbioru z lokacjami (wraz z niezmiennikami) */
	struct timed_class_rs_set *rs;
	struct timed_class_ptr_set *stable_pre;  /* poprzedniki ktore sa stabilne ze wzgledu na ta klase */
	struct timed_class_ptr_set *stable_succ; /* nastepniki ze wzgledu na ktore ta klasa jest stabilna */
	struct timed_class_set *next;
};
typedef struct timed_class_set TimedClass_set;

struct timed_class_ptr_set {
	struct timed_class_set *tclass;
	struct timed_class_ptr_set *next;
};
typedef struct timed_class_ptr_set TClassPtr_set;

#endif /* !_INC_ANTYPES_H_ */

