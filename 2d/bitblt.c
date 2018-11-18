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

#include <string.h>
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "rle.h"
#include "dxxerror.h"
#include "byteswap.h"
#include "ogl_init.h"

void gr_bitmap( int x, int y, grs_bitmap *bm )
{
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
	int sx=0, sy=0;

	if ((dx1 >= grd_curcanv->cv_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_w ) { dx2 = grd_curcanv->cv_w-1; }
	if ( dy2 >= grd_curcanv->cv_h ) { dy2 = grd_curcanv->cv_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	ogl_ubitblt(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm);

}

void gr_ubitmap( int x, int y, grs_bitmap *bm )
{
	ogl_ubitmapm_cs(x,y,-1,-1,bm,255,F1_0);
}


void gr_ubitmapm( int x, int y, grs_bitmap *bm )
{
	ogl_ubitmapm_cs(x,y,-1,-1,bm,255,F1_0);
}

void gr_bitmapm( int x, int y, grs_bitmap *bm )
{
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
	int sx=0, sy=0;

	if ((dx1 >= grd_curcanv->cv_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_w ) { dx2 = grd_curcanv->cv_w-1; }
	if ( dy2 >= grd_curcanv->cv_h ) { dy2 = grd_curcanv->cv_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	ogl_ubitblt(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm);

}

void show_fullscr(grs_bitmap *bm)
{
	ogl_ubitmapm_cs(0, 0, -1, -1, bm, -1, F1_0);
}

// Find transparent area in bitmap
void gr_bitblt_find_transparent_area(grs_bitmap *bm, int *minx, int *miny, int *maxx, int *maxy)
{
	ubyte c;
	int i = 0, x = 0, y = 0, count = 0;
	static unsigned char buf[1024*1024];

	if (!(bm->bm_flags&BM_FLAG_TRANSPARENT))
		return;

	memset(buf,0,1024*1024);

	*minx = bm->bm_w - 1;
	*maxx = 0;
	*miny = bm->bm_h - 1;
	*maxy = 0;

	// decode the bitmap
	if (bm->bm_flags & BM_FLAG_RLE){
		unsigned char * dbits;
		unsigned char * sbits;
		int i, data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		sbits = &bm->bm_data[4 + (bm->bm_h * data_offset)];
		dbits = buf;

		for (i=0; i < bm->bm_h; i++ )    {
			gr_rle_decode(sbits,dbits);
			if ( bm->bm_flags & BM_FLAG_RLE_BIG )
				sbits += (int)INTEL_SHORT(*((short *)&(bm->bm_data[4+(i*data_offset)])));
			else
				sbits += (int)bm->bm_data[4+i];
			dbits += bm->bm_w;
		}
	}
	else
	{
		memcpy(&buf, bm->bm_data, sizeof(unsigned char)*(bm->bm_w*bm->bm_h));
	}

	for (y = 0; y < bm->bm_h; y++) {
		for (x = 0; x < bm->bm_w; x++) {
			c = buf[i++];
			if (c == TRANSPARENCY_COLOR) {				// don't look for transparancy color here.
				count++;
				if (x < *minx) *minx = x;
				if (y < *miny) *miny = y;
				if (x > *maxx) *maxx = x;
				if (y > *maxy) *maxy = y;
			}
		}
	}
	Assert (count);
}
