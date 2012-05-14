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
 * Routines to cache merged textures.
 *
 */

#include <stdio.h>

#include "gr.h"
#include "error.h"
#include "game.h"
#include "textures.h"
#include "rle.h"
#include "piggy.h"
#include "wall.h"
#include "timer.h"
#include "ogl_init.h"

#define MAX_NUM_CACHE_BITMAPS 200

//static grs_bitmap * cache_bitmaps[MAX_NUM_CACHE_BITMAPS];                     

typedef struct	{
	grs_bitmap * bitmap;
	grs_bitmap * bottom_bmp;
	grs_bitmap * top_bmp;
	int 		orient;
	fix		last_time_used;
} TEXTURE_CACHE;

static TEXTURE_CACHE Cache[MAX_NUM_CACHE_BITMAPS];

static int num_cache_entries = 0;

static int cache_hits = 0;
static int cache_misses = 0;

void texmerge_close();
static void do_merge_textures(int type, grs_bitmap *bottom_bmp, grs_bitmap *top_bmp, grs_bitmap *dest_bmp);

//----------------------------------------------------------------------

int texmerge_init(int num_cached_textures)
{
	int i;

	if ( num_cached_textures <= MAX_NUM_CACHE_BITMAPS )
		num_cache_entries = num_cached_textures;
	else
		num_cache_entries = MAX_NUM_CACHE_BITMAPS;
	
	for (i=0; i<num_cache_entries; i++ )	{
			// Make temp tmap for use when combining
		Cache[i].bitmap = gr_create_bitmap( 64, 64 );

		//if (get_selector( Cache[i].bitmap->bm_data, 64*64,  &Cache[i].bitmap->bm_selector))
		//	Error( "ERROR ALLOCATING CACHE BITMAP'S SELECTORS!!!!" );

		Cache[i].last_time_used = -1;
		Cache[i].top_bmp = NULL;
		Cache[i].bottom_bmp = NULL;
		Cache[i].orient = -1;
	}

	return 1;
}

void texmerge_flush()
{
	int i;

	for (i=0; i<num_cache_entries; i++ )	{
		Cache[i].last_time_used = -1;
		Cache[i].top_bmp = NULL;
		Cache[i].bottom_bmp = NULL;
		Cache[i].orient = -1;
	}
}


//-------------------------------------------------------------------------
void texmerge_close()
{
	int i;

	for (i=0; i<num_cache_entries; i++ )	{
		gr_free_bitmap( Cache[i].bitmap );
		Cache[i].bitmap = NULL;
	}
}

//--unused-- int info_printed = 0;

static void piggy_page_in_two(int tmap_bottom, int tmap_top)
{
	// Make sure the bitmaps are paged in...
	piggy_page_flushed = 0;

	PIGGY_PAGE_IN(Textures[tmap_top&0x3FFF]);
	PIGGY_PAGE_IN(Textures[tmap_bottom]);
	if (piggy_page_flushed)	{
		// If cache got flushed, re-read 'em.
		piggy_page_flushed = 0;
		PIGGY_PAGE_IN(Textures[tmap_top&0x3FFF]);
		PIGGY_PAGE_IN(Textures[tmap_bottom]);
	}
	Assert( piggy_page_flushed == 0 );
}

grs_bitmap * texmerge_get_cached_bitmap( int tmap_bottom, int tmap_top )
{
	grs_bitmap *bitmap_top, *bitmap_bottom;
	int i, orient;
	int lowest_time_used;
	int least_recently_used;

	bitmap_top = &GameBitmaps[Textures[tmap_top&0x3FFF].index];
	bitmap_bottom = &GameBitmaps[Textures[tmap_bottom].index];
	
	orient = ((tmap_top&0xC000)>>14) & 3;

	least_recently_used = 0;
	lowest_time_used = Cache[0].last_time_used;
	
	for (i=0; i<num_cache_entries; i++ )	{
		if ( (Cache[i].last_time_used > -1) && (Cache[i].top_bmp==bitmap_top) && (Cache[i].bottom_bmp==bitmap_bottom) && (Cache[i].orient==orient ))	{
			cache_hits++;
			Cache[i].last_time_used = timer_query();
			return Cache[i].bitmap;
		}	
		if ( Cache[i].last_time_used < lowest_time_used )	{
			lowest_time_used = Cache[i].last_time_used;
			least_recently_used = i;
		}
	}

	//---- Page out the LRU bitmap;
	cache_misses++;

	piggy_page_in_two(tmap_bottom, tmap_top);

	ogl_freebmtexture(Cache[least_recently_used].bitmap);

	// TODO: missing, this just does some broken bullshit
	// See door to the outside in 3rd part of Anthology.
	if ((bitmap_top->bm_flags & BM_FLAG_TGA) ||
		(bitmap_bottom->bm_flags & BM_FLAG_TGA))
		printf("merging TGAs! will produce corrupted texture\n");
	do_merge_textures(orient, bitmap_bottom, bitmap_top, Cache[least_recently_used].bitmap);

	Cache[least_recently_used].top_bmp = bitmap_top;
	Cache[least_recently_used].bottom_bmp = bitmap_bottom;
	Cache[least_recently_used].last_time_used = timer_query();
	Cache[least_recently_used].orient = orient;

	return Cache[least_recently_used].bitmap;
}

static void do_merge_textures(int type, grs_bitmap *bottom_bmp, grs_bitmap *top_bmp, grs_bitmap *dest_bmp)
{
	ubyte c;
	int x,y;

	ubyte * top_data, *bottom_data;

	if ( top_bmp->bm_flags & BM_FLAG_RLE )
		top_bmp = rle_expand_texture(top_bmp);

	if ( bottom_bmp->bm_flags & BM_FLAG_RLE )
		bottom_bmp = rle_expand_texture(bottom_bmp);

	top_data = top_bmp->bm_data;
	bottom_data = bottom_bmp->bm_data;
	ubyte *dest_data = dest_bmp->bm_data;

	int super_color = -1;
	if (top_bmp->bm_flags & BM_FLAG_SUPER_TRANSPARENT)
		super_color = 254;

	switch( type )	{
		case 0:
			// Normal
			for (y=0; y<64; y++ )
				for (x=0; x<64; x++ )	{
					c = top_data[ 64*y+x ];		
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ 64*y+x ];
					else if (c==super_color)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
		case 1:
			// 
			for (y=0; y<64; y++ )
				for (x=0; x<64; x++ )	{
					c = top_data[ 64*x+(63-y) ];		
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ 64*y+x ];
					else if (c==super_color)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
		case 2:
			// Normal
			for (y=0; y<64; y++ )
				for (x=0; x<64; x++ )	{
					c = top_data[ 64*(63-y)+(63-x) ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ 64*y+x ];
					else if (c==super_color)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
		case 3:
			// Normal
			for (y=0; y<64; y++ )
				for (x=0; x<64; x++ )	{
					c = top_data[ 64*(63-x)+y  ];
					if (c==TRANSPARENCY_COLOR)
						c = bottom_data[ 64*y+x ];
					else if (c==super_color)
						c = TRANSPARENCY_COLOR;
					*dest_data++ = c;
				}
			break;
	}

	if (top_bmp->bm_flags & BM_FLAG_SUPER_TRANSPARENT)	{
		dest_bmp->bm_flags = BM_FLAG_TRANSPARENT;
		dest_bmp->avg_color = top_bmp->avg_color;
	} else	{
		dest_bmp->bm_flags = bottom_bmp->bm_flags & (~BM_FLAG_RLE);
		dest_bmp->avg_color = bottom_bmp->avg_color;
	}
}

static int check_pixel_type(uint8_t *p, int bm_flags, int bm_depth)
{
	if (bm_depth <= 1) {
		uint8_t v = p[0];
		if (v == 254 && (bm_flags & BM_FLAG_SUPER_TRANSPARENT))
			return BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT;
		if (v == TRANSPARENCY_COLOR)
			return BM_FLAG_TRANSPARENT;
		return 0;
	} else if (bm_depth == 4) {
		// p[0,1,2,3] = r,g,b,a
		// Some sort of colorkey for super transparency (doors?)
		if (p[0] == 120 && p[1] == 88 && p[2] == 128)
			return BM_FLAG_SUPER_TRANSPARENT;
		if (p[3] <= 5)
			return BM_FLAG_TRANSPARENT;
		if (p[3] < 255)
			return BM_FLAG_SEE_THRU;
		return 0;
	}

	Assert(false);
	return 0;
}

int texmerge_test_pixel(int tmap_bottom, int tmap_top, fix u, fix v)
{
	int orient = ((tmap_top&0xC000)>>14) & 3;

	piggy_page_in_two(tmap_bottom, tmap_top);

	grs_bitmap *bm_bottom = &GameBitmaps[Textures[tmap_bottom].index];

	if (bm_bottom->bm_flags & BM_FLAG_RLE)
		bm_bottom = rle_expand_texture(bm_bottom);

	if (tmap_top) {
		grs_bitmap *bm_top = &GameBitmaps[Textures[tmap_top&0x3FFF].index];

		if (bm_top->bm_flags & BM_FLAG_RLE)
			bm_top = rle_expand_texture(bm_top);

		int w = bm_top->bm_w;
		int h = bm_top->bm_h;

		int bx = ((unsigned) f2i(u*w)) % w;
		int by = ((unsigned) f2i(v*h)) % h;

		// Reverse the transformation done on merging.
		int tx, ty;
		switch (orient)	{
		case 0:
			tx = bx;
			ty = by;
			break;
		case 1:
			tx = by;
			ty = w - bx - 1;
			break;
		case 2:
			tx = w - bx - 1;
			ty = h - by - 1;
			break;
		case 3:
			tx = h - by - 1;
			ty = bx;
			break;
		}

		size_t d_top = bm_top->bm_depth < 1 ? 1 : bm_top->bm_depth;
		int c_top = check_pixel_type(bm_top->bm_data + ty * w * d_top + tx * d_top,
									 bm_top->bm_flags, bm_top->bm_depth);
		if (c_top & BM_FLAG_SUPER_TRANSPARENT)
			return WID_RENDPAST_FLAG;
		if (!(c_top & BM_FLAG_TRANSPARENT))
			return 0;
	}

	int w = bm_bottom->bm_w;
	int h = bm_bottom->bm_h;

	int bx = ((unsigned) f2i(u*w)) % w;
	int by = ((unsigned) f2i(v*h)) % h;

	size_t d_bottom = bm_bottom->bm_depth < 1 ? 1 : bm_bottom->bm_depth;
	int c_bottom = check_pixel_type(bm_bottom->bm_data + by * w * d_bottom + bx * d_bottom,
									bm_bottom->bm_flags, bm_bottom->bm_depth);
	if (c_bottom & BM_FLAG_TRANSPARENT)
		return WID_RENDPAST_FLAG;

	return 0;
}

void gr_bitmap_check_transparency(grs_bitmap *bmp)
{
	bmp->bm_flags &= ~(BM_FLAG_TRANSPARENT |
					   BM_FLAG_SUPER_TRANSPARENT |
					   BM_FLAG_SEE_THRU);

	if (bmp->bm_depth == 4) {
		// D2X-XL computes these flags on loading in CTGA::SetProperties(). It's
		// a fucked up complicated heuristic, so I'm mostly _not_ duplicating it
		// here.
		// I have no idea why these flags aren't just set correctly when the
		// levels are authored.

		ubyte *data = bmp->bm_data;
		int flags = 0;

		for (int y=0; y<bmp->bm_h; y++ )	{
			for (int x=0; x<bmp->bm_w; x++ )	{
				int res = check_pixel_type(data, 0, 4);
				if (res & BM_FLAG_SUPER_TRANSPARENT)
					data[3] = 0; // fucked up
				data += 4;
				flags |= res;
			}
			data += bmp->bm_rowsize - bmp->bm_w * 4;
		}

		bmp->bm_flags |= flags;
	} else {
		abort(); // unused
	}
}
