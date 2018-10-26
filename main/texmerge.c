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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>

#include "gr.h"
#include "error.h"
#include "game.h"
#include "textures.h"
#include "rle.h"
#include "piggy.h"
#include "wall.h"

#ifdef OGL
#include "ogl_init.h"
#define MAX_NUM_CACHE_BITMAPS 200
#else
#define MAX_NUM_CACHE_BITMAPS 50
#endif

//static grs_bitmap * cache_bitmaps[MAX_NUM_CACHE_BITMAPS];                     

typedef struct	{
	grs_bitmap * bitmap;
	grs_bitmap * bottom_bmp;
	grs_bitmap * top_bmp;
	int 		orient;
	int		last_frame_used;
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

		Cache[i].last_frame_used = -1;
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
		Cache[i].last_frame_used = -1;
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
	int lowest_frame_count;
	int least_recently_used;

	bitmap_top = &GameBitmaps[Textures[tmap_top&0x3FFF].index];
	bitmap_bottom = &GameBitmaps[Textures[tmap_bottom].index];
	
	orient = ((tmap_top&0xC000)>>14) & 3;

	least_recently_used = 0;
	lowest_frame_count = Cache[0].last_frame_used;
	
	for (i=0; i<num_cache_entries; i++ )	{
		if ( (Cache[i].last_frame_used > -1) && (Cache[i].top_bmp==bitmap_top) && (Cache[i].bottom_bmp==bitmap_bottom) && (Cache[i].orient==orient ))	{
			cache_hits++;
			Cache[i].last_frame_used = FrameCount;
			return Cache[i].bitmap;
		}	
		if ( Cache[i].last_frame_used < lowest_frame_count )	{
			lowest_frame_count = Cache[i].last_frame_used;
			least_recently_used = i;
		}
	}

	//---- Page out the LRU bitmap;
	cache_misses++;

	piggy_page_in_two(tmap_bottom, tmap_top);

#ifdef OGL
        ogl_freebmtexture(Cache[least_recently_used].bitmap);
#endif

	do_merge_textures(orient, bitmap_bottom, bitmap_top, Cache[least_recently_used].bitmap);

	Cache[least_recently_used].top_bmp = bitmap_top;
	Cache[least_recently_used].bottom_bmp = bitmap_bottom;
	Cache[least_recently_used].last_frame_used = FrameCount;
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

int texmerge_test_pixel(int tmap_bottom, int tmap_top, fix u, fix v)
{
	int orient = ((tmap_top&0xC000)>>14) & 3;

	piggy_page_in_two(tmap_bottom, tmap_top);

	grs_bitmap *bm_bottom = &GameBitmaps[Textures[tmap_bottom].index];

	if (bm_bottom->bm_flags & BM_FLAG_RLE)
		bm_bottom = rle_expand_texture(bm_bottom);

	int w = bm_bottom->bm_w;
	int h = bm_bottom->bm_h;

	int bx = ((unsigned) f2i(u*w)) % w;
	int by = ((unsigned) f2i(v*h)) % h;

	if (tmap_top) {
		grs_bitmap *bm_top = &GameBitmaps[Textures[tmap_top&0x3FFF].index];

		if (bm_top->bm_flags & BM_FLAG_RLE)
			bm_top = rle_expand_texture(bm_top);

		Assert(bm_top->bm_w == w && bm_top->bm_h == h);

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

		int c_top = bm_top->bm_data[ty * w + tx];
		if (c_top == 254 && (bm_top->bm_flags & BM_FLAG_SUPER_TRANSPARENT))
			return WID_RENDPAST_FLAG;
		if (c_top != TRANSPARENCY_COLOR)
			return 0;
	}

	int c_bottom = bm_bottom->bm_data[by * w + bx];
	if (c_bottom == TRANSPARENCY_COLOR)
		return WID_RENDPAST_FLAG;

	return 0;
}
