/* Console */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "pstypes.h"

/* Priority levels */
#define CON_CRITICAL -3
#define CON_URGENT   -2
#define CON_HUD      -1
#define CON_NORMAL    0
#define CON_VERBOSE   1
#define CON_DEBUG     2

void con_printf(int level, char *fmt, ...) PRINTF_FORMAT(2, 3);

#endif /* _CONSOLE_H_ */

