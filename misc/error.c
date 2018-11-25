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
 * Error handling/printing/exiting code
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "args.h"
#include "pstypes.h"
#include "console.h"
#include "dxxerror.h"
#include "inferno.h"

void con_printf(int level, char *fmt, ...)
{
	if (level > GameArg.DbgVerbose)
		return;

	const char *type = "?";
	switch (level) {
	case CON_CRITICAL:	type = "CRITICAL"; break;
	case CON_URGENT:	type = "URGENT"; break;
	case CON_HUD:		type = "HUD"; break;
	case CON_NORMAL:	type = NULL; break;
	case CON_VERBOSE:	type = "V"; break;
	case CON_DEBUG:		type = "DBG"; break;
	}

	if (type)
		fprintf(stderr, "%s: ", type);

	va_list arglist;
	va_start(arglist,fmt);
	vfprintf(stderr, fmt, arglist);
	va_end(arglist);
}

//terminates with error code 1, printing message
void Error(const char *fmt,...)
{
	fprintf(stderr, "Error: ");

	va_list arglist;
	va_start(arglist,fmt);
	vfprintf(stderr, fmt, arglist);
	va_end(arglist);

	fprintf(stderr, "\n");

	exit(1);
}

//print out warning message to user
void Warning(char *fmt,...)
{
	fprintf(stderr, "Warning: ");

	va_list arglist;
	va_start(arglist,fmt);
	vfprintf(stderr, fmt, arglist);
	va_end(arglist);

	fprintf(stderr, "\n");
}
