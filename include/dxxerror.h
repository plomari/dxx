/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header for error handling/printing/exiting code
 *
 */

#ifndef _ERROR_H
#define _ERROR_H

#include <stdio.h>
#include "pstypes.h"

// print out warning message to user
void Warning(char *fmt,...) PRINTF_FORMAT(1, 2);

// print message, call exit(1)
void Error(const char *fmt,...) NORETURN PRINTF_FORMAT(1, 2);

// Log unexpected situation. Non-fatal (used to act as break point in debuggers).
#define Int3() do { fprintf(stderr, "%s:%d(%s): Int3\n", __FILE__, __LINE__, __func__); } while(0)

// Log unexpected situation if !expr. Non-fatal. Use standard assert() for real asserts.
#define Assert(expr) ((expr)?(void)0:(void)fprintf(stderr,"%s:%d(%s): Assert(%s)\n",__FILE__,__LINE__,__func__,#expr))

// Log unexpected situation if !expr. Returns expr.
#define WARN_IF_FALSE(expr) (((expr)?(void)0:(void)fprintf(stderr,"%s:%d(%s): expected (%s)\n",__FILE__,__LINE__,__func__,#expr)), (expr))
#define WARN_ON(expr) (((!(expr))?(void)0:(void)fprintf(stderr,"%s:%d(%s): unexpected (%s)\n",__FILE__,__LINE__,__func__,#expr)), (expr))

#endif /* _ERROR_H */
