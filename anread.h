#ifndef _INC_ANREAD_H_
#define _INC_ANREAD_H_

#include "antypes.h"
#include <stdbool.h>

// #define VERBOSE /* brzydko ;) */

/** prototypy **/

void clocks_append(char *);
void clocks_mkmap(void);
Clock_idx *clocks_mkarr(void);
/*void clocks_free(void);*/
void constr_append(Clock_idx, Clock_idx, Constr_rel, int);
Constr *constrs_mkarr(void);
Clock_idx get_clock_idx(Clock *, Clock_idx, char *);
Clock_idx get_cur_clock_idx(char *);
char *get_clock_name(Clock *, Clock_idx);
char *get_cur_clock_name(Clock_idx);
void clocks_show(Clock *, Clock_idx);
void constrs_show(Constr *, Clock *);
void trans_append(Loc_idx, Loc_idx, Act_idx);
void trans_show(Trans_set **, Loc *, Loc_idx, Clock *, Act *);
void trans_clocks_show(Trans_set *, Clock *);
void cur_trans_show(void);
Loc_idx get_loc_idx(Loc *, Loc_idx, char *);
Loc_idx get_cur_loc_idx(char *);
char *get_loc_name(Loc *, Loc_idx);
char *get_cur_loc_name(Loc_idx);
void locations_append(char *);
void set_loc_type(Loc_type);
void locations_mkmap(void);
void locations_show(Loc *, Loc_idx, Clock *);
void actions_append(char *);
void set_act_type(Act_type);
void actions_mkmap(void);
Act_idx get_act_idx(Act *, Act_idx, char *);
Loc_idx get_cur_act_idx(char *);
char *get_act_name(Act *, Act_idx);
char *get_cur_act_name(Act_idx);
void actions_show(Act *, Act_idx);
void act_mark_used(Act_idx);
void property_got_one(char *);
void property_append_loc(char *, char *);
Property_set *get_properties(void);
Aut_idx get_aut_idx(char *);
void complete_automaton(char *);
void complete_net(void);
AutNet get_autnet(void);

#endif /* !_INC_ANREAD_H_*/
