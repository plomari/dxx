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
 * Graphical routines for manipulating grs_bitmaps.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "u_mem.h"


#include "gr.h"
#include "grdef.h"
#include "dxxerror.h"
#include "ogl.h"
#include "byteswap.h"
#include "rle.h"

void gr_set_bitmap_data (grs_bitmap *bm, unsigned char *data)
{
//	if (bm->bm_data!=data)
		ogl_freebmtexture(bm);
	bm->bm_data = data;
}

grs_bitmap *gr_new_bitmap(int w, int h, int depth)
{
	Assert(depth == 1 || depth == 4);

	grs_bitmap *new = malloc(sizeof(*new));
	if (WARN_ON(!new))
		abort();
	*new = (grs_bitmap){
		.bm_w = w,
		.bm_h = h,
		.bm_rowsize = w * depth,
		.bm_depth = depth,
	};
	new->bm_data = calloc(h, new->bm_rowsize);
	if (WARN_ON(!new->bm_data))
		abort();
	return new;
}

grs_bitmap *gr_create_bitmap(int w, int h )
{
	return gr_new_bitmap(w, h, 1);
}

void gr_init_bitmap( grs_bitmap *bm, int x, int y, int w, int h, int bytesperline, unsigned char * data ) // TODO: virtualize
{
	bm->bm_x = x;
	bm->bm_y = y;
	bm->bm_w = w;
	bm->bm_h = h;
	bm->bm_flags = 0;
	bm->bm_rowsize = bytesperline;
	bm->bm_depth = 0;

	bm->bm_data = NULL;
	bm->bm_parent=NULL;bm->gltexture=NULL;

//	if (data != 0)
		gr_set_bitmap_data (bm, data);
/*
	else
		gr_set_bitmap_data (bm, d_malloc( MAX_BMP_SIZE(w, h) ));
*/
}

void gr_init_bitmap_alloc( grs_bitmap *bm, int x, int y, int w, int h, int bytesperline)
{
	gr_init_bitmap(bm, x, y, w, h, bytesperline, 0);
	gr_set_bitmap_data(bm, d_malloc( MAX_BMP_SIZE(w, h) ));
}

void gr_init_bitmap_data (grs_bitmap *bm) // TODO: virtulize
{
	bm->bm_data = NULL;
	bm->bm_parent=NULL;bm->gltexture=NULL;
}

grs_bitmap *gr_create_sub_bitmap(grs_bitmap *bm, int x, int y, int w, int h )
{
	grs_bitmap *new;

	new = (grs_bitmap *)d_malloc( sizeof(grs_bitmap) );
	gr_init_sub_bitmap (new, bm, x, y, w, h);

	return new;
}

void gr_free_bitmap(grs_bitmap *bm )
{
	if (bm)
		gr_free_bitmap_data (bm);
	d_free(bm);
}

void gr_free_sub_bitmap(grs_bitmap *bm )
{
	if (bm!=NULL)
	{
		d_free(bm);
	}
}


void gr_free_bitmap_data (grs_bitmap *bm) // TODO: virtulize
{
	ogl_freebmtexture(bm);
	if (bm->bm_data != NULL)
		d_free (bm->bm_data);
	bm->bm_data = NULL;
}

void gr_init_sub_bitmap (grs_bitmap *bm, grs_bitmap *bmParent, int x, int y, int w, int h )	// TODO: virtualize
{
	bm->bm_x = x + bmParent->bm_x;
	bm->bm_y = y + bmParent->bm_y;
	bm->bm_w = w;
	bm->bm_h = h;
	bm->bm_flags = bmParent->bm_flags;
	bm->bm_rowsize = bmParent->bm_rowsize;

	bm->gltexture=bmParent->gltexture;
	bm->bm_parent=bmParent;
	bm->bm_data = bmParent->bm_data+(unsigned int)((y*bmParent->bm_rowsize)+x);
}

void decode_data(ubyte *data, int num_pixels, ubyte *colormap, int *count)
{
	int i;
	ubyte mapped;

	for (i = 0; i < num_pixels; i++) {
		count[*data]++;
		mapped = *data;
		*data = colormap[mapped];
		data++;
	}
}

void gr_set_bitmap_flags (grs_bitmap *pbm, int flags)
{
	pbm->bm_flags = flags;
}

void gr_set_transparent (grs_bitmap *pbm, int bTransparent)
{
	if (bTransparent)
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags | BM_FLAG_TRANSPARENT);
	}
	else
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags & ~BM_FLAG_TRANSPARENT);
	}
}

void gr_set_super_transparent (grs_bitmap *pbm, int bTransparent)
{
	if (bTransparent)
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags & ~BM_FLAG_SUPER_TRANSPARENT);
	}
	else
	{
		gr_set_bitmap_flags (pbm, pbm->bm_flags | BM_FLAG_SUPER_TRANSPARENT);
	}
}

void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq )
{
	int i, r, g, b;

	for (i=0; i<256; i++ )	{
		r = *palette++;
		g = *palette++;
		b = *palette++;
 		*colormap++ = gr_find_closest_color( r, g, b );
		*freq++ = 0;
	}
}

void gr_remap_bitmap_good( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color )
{
	ubyte colormap[256];
	int freq[256];
	build_colormap_good( palette, colormap, freq );

	Assert(bmp->bm_depth <= 1);

	if ( (super_transparent_color>=0) && (super_transparent_color<=255))
		colormap[super_transparent_color] = 254;

	if ( (transparent_color>=0) && (transparent_color<=255))
		colormap[transparent_color] = TRANSPARENCY_COLOR;

	if (bmp->bm_w == bmp->bm_rowsize)
		decode_data(bmp->bm_data, bmp->bm_w * bmp->bm_h, colormap, freq );
	else {
		int y;
		ubyte *p = bmp->bm_data;
		for (y=0;y<bmp->bm_h;y++,p+=bmp->bm_rowsize)
			decode_data(p, bmp->bm_w, colormap, freq );
	}

	if ( (transparent_color>=0) && (transparent_color<=255) && (freq[transparent_color]>0) )
		gr_set_transparent (bmp, 1);

	if ( (super_transparent_color>=0) && (super_transparent_color<=255) && (freq[super_transparent_color]>0) )
		gr_set_super_transparent (bmp, 1);
}

// Find transparent area in bitmap
void gr_bitmap_find_transparent_area(grs_bitmap *bm, int *minx, int *miny, int *maxx, int *maxy)
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
