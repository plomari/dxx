/* $Id: line.c,v 1.1.1.1 2006/03/17 19:52:07 zicodxx Exp $ */
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
 * Graphical routines for drawing lines.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>

#include "u_mem.h"

#include "gr.h"
#include "grdef.h"
#include "fix.h"

#include "clip.h"
#include "ogl_init.h"

//unclipped version just calls clipping version for now
int gr_uline(fix _a1, fix _b1, fix _a2, fix _b2)
{
	int a1,b1,a2,b2;
	a1 = f2i(_a1); b1 = f2i(_b1); a2 = f2i(_a2); b2 = f2i(_b2);
	switch(TYPE)
	{
	case BM_OGL:
		ogl_ulinec(a1,b1,a2,b2,COLOR);
		return 0;
	}
	return 2;
}

// Returns 0 if drawn with no clipping, 1 if drawn but clipped, and
// 2 if not drawn at all.

int gr_line(fix a1, fix b1, fix a2, fix b2)
{
	int x1, y1, x2, y2;
	int clipped=0;

	x1 = i2f(MINX);
	y1 = i2f(MINY);
	x2 = i2f(MAXX);
	y2 = i2f(MAXY);

	CLIPLINE(a1,b1,a2,b2,x1,y1,x2,y2,return 2,clipped=1, FSCALE );

	gr_uline( a1, b1, a2, b2 );

	return clipped;

}
