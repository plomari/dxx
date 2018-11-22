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

#include <stdlib.h>
#include <stdio.h>

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#ifdef OGL
#include "ogl.h"
#endif

grs_canvas * grd_curcanv;    //active canvas
grs_screen * grd_curscreen;  //active screen

void gr_init_sub_canvas(grs_canvas *new, grs_canvas *src, int x, int y, int w, int h)
{
	*new = *src;

	new->cv_x += x;
	new->cv_y += y;
	new->cv_w = w;
	new->cv_h = h;
}

void gr_set_current_canvas( grs_canvas *canv )
{
	if (canv==NULL)
		grd_curcanv = &(grd_curscreen->sc_canvas);
	else
		grd_curcanv = canv;
}

void gr_clear_canvas(int color)
{
	gr_setcolor(color);
	gr_rect(0,0,GWIDTH-1,GHEIGHT-1);
}

void gr_setcolor(int color)
{
	grd_curcanv->cv_color=color;
}

void gr_settransblend(int fade_level, ubyte blend_func)
{
	grd_curcanv->cv_fade_level=fade_level;
	grd_curcanv->cv_blend_func=blend_func;
#ifdef OGL
	ogl_set_blending();
#endif
}
