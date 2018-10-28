/* $Id: box.c,v 1.1.1.1 2006/03/17 19:51:57 zicodxx Exp $ */
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "u_mem.h"


#include "gr.h"
#include "grdef.h"

static void gr_ubox12(int left,int top,int right,int bot)
{
	int i;

	for (i=top; i<=bot; i++ )
	{
		gr_upixel( left, i );
		gr_upixel( right, i );
	}

	for (i=left; i<=right; i++ )
	{
		gr_upixel( i, top );
		gr_upixel( i, bot );
	}
}

void gr_ubox(int left,int top,int right,int bot)
{
	gr_ubox12( left, top, right, bot );
}

void gr_box(int left,int top,int right,int bot)
{
	gr_ubox12( left, top, right, bot );
}
