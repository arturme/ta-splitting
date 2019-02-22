#ifndef _INC_ANMACRO_H_
#define _INC_ANMACRO_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FERROR(s) \
{ \
	printf("%s (function %s), line %d: %s\n", __FILE__, __func__, __LINE__, s); \
	exit(1); \
}

#define CLOCK_0_NAME "0"

#define CONSTR_LT 0
#define CONSTR_LE 1 
#define CONSTR_MARK_TERM(a) ((a)->l_clk = -1) /* oznaczanie ograniczenia jako terminujace liste */
#define CONSTR_IS_TERM(a) ((a)->l_clk == -1) /* sprawdzanie czy org. jest terminujace */

#define LOC_INITIAL (1 << 0)
#define LOC_COMMITED (1 << 1)
#define LOC_URGENT (1 << 2)
#define LOC_IS_INITIAL(a) ((a).type & LOC_INITIAL)
#define LOC_IS_COMMITED(a) ((a).type & LOC_COMMITED)
#define LOC_IS_URGENT(a) ((a).type & LOC_URGENT)

#define ACT_URGENT (1 << 0)
#define ACT_IS_URGENT(a) ((a).type & ACT_URGENT)

#define TRANS_P(x, y, a, b) &(x)[((a)*(y)+(b))] /* pobieranie listy przejsc z lokacji a do b */
#define TRANS(x, y, a, b) (x)[((a)*(y)+(b))]

#endif /* !_INC_ANMACRO_H_ */
