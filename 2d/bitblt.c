/* $Id: bitblt.c,v 1.1.1.1 2006/03/17 19:51:56 zicodxx Exp $ */
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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Routines for bitblt's.
 *
 */ 

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "error.h"
#include "ogl_init.h"

// Clipped bitmap ...

void gr_bitmap( int x, int y, grs_bitmap *bm )
{
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
	int sx=0, sy=0;

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_bitmap.bm_w ) { dx2 = grd_curcanv->cv_bitmap.bm_w-1; }
	if ( dy2 >= grd_curcanv->cv_bitmap.bm_h ) { dy2 = grd_curcanv->cv_bitmap.bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	gr_bm_ubitblt(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );

}

void gr_bm_ubitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	register int x1, y1;

	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_OGL ))
	{
		ogl_ubitblt(w, h, dx, dy, sx, sy, src, dest);
		return;
	}

	Assert(false);

	for (y1=0; y1 < h; y1++ )    {
		for (x1=0; x1 < w; x1++ )    {
			gr_bm_pixel( dest, dx+x1, dy+y1, gr_gpixel(src,sx+x1,sy+y1) );
		}
	}
}

void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int dx1=dx, dx2=dx+dest->bm_w-1;
	int dy1=dy, dy2=dy+dest->bm_h-1;

	int sx1=sx, sx2=sx+src->bm_w-1;
	int sy1=sy, sy2=sy+src->bm_h-1;

	if ((dx1 >= dest->bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= dest->bm_h ) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx1 += -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy1 += -dy1; dy1 = 0; }
	if ( dx2 >= dest->bm_w ) { dx2 = dest->bm_w-1; }
	if ( dy2 >= dest->bm_h ) { dy2 = dest->bm_h-1; }

	if ((sx1 >= src->bm_w ) || (sx2 < 0)) return;
	if ((sy1 >= src->bm_h ) || (sy2 < 0)) return;
	if ( sx1 < 0 ) { dx1 += -sx1; sx1 = 0; }
	if ( sy1 < 0 ) { dy1 += -sy1; sy1 = 0; }
	if ( sx2 >= src->bm_w ) { sx2 = src->bm_w-1; }
	if ( sy2 >= src->bm_h ) { sy2 = src->bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
	if ( dx2-dx1+1 < w )
		w = dx2-dx1+1;
	if ( dy2-dy1+1 < h )
		h = dy2-dy1+1;
	if ( sx2-sx1+1 < w )
		w = sx2-sx1+1;
	if ( sy2-sy1+1 < h )
		h = sy2-sy1+1;

	gr_bm_ubitblt(w,h, dx1, dy1, sx1, sy1, src, dest );
}

void gr_ubitmap( int x, int y, grs_bitmap *bm )
{
	int source, dest;

	source = bm->bm_type;
	dest = TYPE;

	if (source == BM_LINEAR && dest == BM_OGL) {
		ogl_ubitmapm(x,y,bm);
		return;
	}

	Assert(false);
}


void gr_ubitmapm( int x, int y, grs_bitmap *bm )
{
	int source, dest;

	source = bm->bm_type;
	dest = TYPE;

	if (source == BM_LINEAR && dest == BM_OGL) {
		ogl_ubitmapm(x,y,bm);
		return;
	}

	Assert(false);
}


void gr_bitmapm( int x, int y, grs_bitmap *bm )
{
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
	int sx=0, sy=0;

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_bitmap.bm_w ) { dx2 = grd_curcanv->cv_bitmap.bm_w-1; }
	if ( dy2 >= grd_curcanv->cv_bitmap.bm_h ) { dy2 = grd_curcanv->cv_bitmap.bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	gr_bm_ubitbltm(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );

}

void gr_bm_ubitbltm(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_OGL ))
	{
		ogl_ubitblt(w, h, dx, dy, sx, sy, src, dest);
		return;
	}

	Assert(false);
}

void show_fullscr(grs_bitmap *bm)
{
	grs_bitmap * const scr = &grd_curcanv->cv_bitmap;

	if(bm->bm_type == BM_LINEAR && scr->bm_type == BM_OGL) {
		ogl_ubitmapm_cs(0,0,-1,-1,bm,-1,F1_0);//use opengl to scale, faster and saves ram. -MPM
		return;
	}

	Assert(0);
}
