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
#include "dxxerror.h"
#include "game.h"
#include "textures.h"
#include "rle.h"
#include "piggy.h"
#include "wall.h"
#include "timer.h"
#include "ogl.h"
#include "texmerge.h"

#define MAX_NUM_CACHE_BITMAPS 200

//static grs_bitmap * cache_bitmaps[MAX_NUM_CACHE_BITMAPS];                     

typedef struct	{
	grs_bitmap * bitmap;
	int 		top_idx, bot_idx;
	int 		orient;
	fix			last_time_used;
} TEXTURE_CACHE;

static TEXTURE_CACHE Cache[MAX_NUM_CACHE_BITMAPS];

static int num_cache_entries = 0;

void texmerge_close();
static bool do_merge_textures(int orient, grs_bitmap *bot, grs_bitmap *top,
							  grs_bitmap **dst);

//----------------------------------------------------------------------

int texmerge_init(int num_cached_textures)
{
	if ( num_cached_textures <= MAX_NUM_CACHE_BITMAPS )
		num_cache_entries = num_cached_textures;
	else
		num_cache_entries = MAX_NUM_CACHE_BITMAPS;
	
	texmerge_flush();

	return 1;
}

void texmerge_flush()
{
	int i;

	for (i=0; i<num_cache_entries; i++ )
		Cache[i].last_time_used = -1;
}


//-------------------------------------------------------------------------
void texmerge_close()
{
	texmerge_flush();

	for (int i=0; i<num_cache_entries; i++ )	{
		gr_free_bitmap( Cache[i].bitmap );
		Cache[i].bitmap = NULL;
	}
}

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
	int top_idx = Textures[tmap_top & 0x3FFF].index;
	int bot_idx = Textures[tmap_bottom].index;

	int orient = ((tmap_top&0xC000)>>14) & 3;

	int least_recently_used = 0;
	fix lowest_time_used = Cache[0].last_time_used;
	
	for (int i = 0; i < num_cache_entries; i++) {
		TEXTURE_CACHE *e = &Cache[i];

		if (e->last_time_used > -1 &&
			e->top_idx == top_idx &&
			e->bot_idx == bot_idx &&
			e->orient == orient &&
			e->bitmap)
		{
			e->last_time_used = timer_query();
			return e->bitmap;
		}

		if (e->last_time_used < lowest_time_used) {
			lowest_time_used = e->last_time_used;
			least_recently_used = i;
		}
	}

	//---- Page out the LRU bitmap;

	piggy_page_in_two(tmap_bottom, tmap_top);

	if (!tmap_top)
		return &GameBitmaps[bot_idx];

	TEXTURE_CACHE *e = &Cache[least_recently_used];

	ogl_freebmtexture(e->bitmap);

	e->top_idx = top_idx;
	e->bot_idx = bot_idx;
	e->orient = orient;
	e->last_time_used = timer_query();

	if (!do_merge_textures(orient, &GameBitmaps[bot_idx], &GameBitmaps[top_idx],
						   &e->bitmap))
		return NULL;

	return e->bitmap;
}

static int check_pixel_type(uint8_t *p, int bm_flags, int bm_depth)
{
	if (bm_depth <= 1) {
		uint8_t v = p[0];
		if (v == 254 && (bm_flags & BM_FLAG_SUPER_TRANSPARENT))
			return BM_FLAG_SUPER_TRANSPARENT;
		if (v == TRANSPARENCY_COLOR)
			return BM_FLAG_TRANSPARENT;
		return 0;
	} else if (bm_depth == 4) {
		// p[0,1,2,3] = r,g,b,a
		// Some sort of colorkey for super transparency (doors?)
		if (p[0] == 120 && p[1] == 88 && p[2] == 128)
			return BM_FLAG_SUPER_TRANSPARENT;
		if (p[3] <= 5)
			return BM_FLAG_TRANSPARENT | BM_FLAG_ALPHA;
		if (p[3] < 255)
			return BM_FLAG_SEE_THRU | BM_FLAG_ALPHA;
		return 0;
	}

	Assert(false);
	return 0;
}

// For x and y in range [0, src_s), return scaled *out_x and _y so that they're
// in the range [0, dst_s). (Accomplishes nearest scaling.)
static void scale_pixel(int src_s, int dst_s, int x, int y,
						int *out_x, int *out_y)
{
	*out_x = x  * dst_s / src_s;
	*out_y = y  * dst_s / src_s;
}

static void orient_transform(int orient, int s, int x, int y,
							 int *out_x, int *out_y)
{
	switch (orient & 3) {
	case 0:
		*out_x = x;
		*out_y = y;
		break;
	case 1:
		*out_x = s - y - 1;
		*out_y = x;
		break;
	case 2:
		*out_x = s - x - 1;
		*out_y = s - y - 1;
		break;
	case 3:
		*out_x = y;
		*out_y = s - x - 1;
		break;
	}
}

static void convert_pixel(uint8_t *dst_p, size_t dst_depth,
						  uint8_t *src_p, size_t src_depth,
						  int src_flags)
{
	if (dst_depth == src_depth) {
		memcpy(dst_p, src_p, dst_depth);
	} else if (dst_depth == 4 && src_depth == 1) {
		ogl_pal_to_rgba8(src_p, dst_p, src_flags, 1);
	} else {
		assert(0);
	}
}

static bool do_merge_textures(int orient, grs_bitmap *bot, grs_bitmap *top,
							  grs_bitmap **p_dst)
{
	if (top->bm_flags & BM_FLAG_RLE)
		top = rle_expand_texture(top);

	if (bot->bm_flags & BM_FLAG_RLE)
		bot = rle_expand_texture(bot);

	// Can merge square bitmaps only.
	if (WARN_ON(bot->bm_w != bot->bm_h || top->bm_w != top->bm_h))
		return false;

	int w = max(top->bm_w, bot->bm_w);
	int h = max(top->bm_h, bot->bm_h);

	size_t d_top = max(top->bm_depth, 1);
	size_t d_bot = max(bot->bm_depth, 1);

	Assert(d_top == 1 || d_top == 4);
	Assert(d_bot == 1 || d_bot == 4);

	size_t d_dst = d_top == 1 && d_bot == 1 ? 1 : 4;

	grs_bitmap *dst = *p_dst;
	if (!dst || dst->bm_w != w || dst->bm_h != h || dst->bm_depth != d_dst) {
		gr_free_bitmap(dst);
		dst = gr_new_bitmap(w, h, d_dst);
		*p_dst = dst;
	}

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint8_t *dst_p = dst->bm_data + dst->bm_rowsize * y + x * d_dst;

			int tx, ty;
			scale_pixel(w, top->bm_w, x, y, &tx, &ty);
			orient_transform(orient, top->bm_w, tx, ty, &tx, &ty);

			uint8_t *top_p = top->bm_data + ty * top->bm_rowsize + tx * d_top;

			int c_top = check_pixel_type(top_p, top->bm_flags, d_top);
			if (c_top & BM_FLAG_SUPER_TRANSPARENT) {
				convert_pixel(dst_p, d_dst, &(uint8_t){255}, 1, BM_FLAG_TRANSPARENT);
			} else if (!(c_top & BM_FLAG_TRANSPARENT)) {
				convert_pixel(dst_p, d_dst, top_p, d_top, top->bm_flags);
			} else {
				int bx, by;
				scale_pixel(w, bot->bm_w, x, y, &bx, &by);

				uint8_t *bot_p = bot->bm_data + by * bot->bm_rowsize + bx * d_bot;
				convert_pixel(dst_p, d_dst, bot_p, d_bot, bot->bm_flags);
			}
		}
	}

	if (top->bm_flags & BM_FLAG_SUPER_TRANSPARENT)	{
		dst->bm_flags = BM_FLAG_TRANSPARENT;
		dst->avg_color = top->avg_color;
	} else	{
		dst->bm_flags = bot->bm_flags & (~BM_FLAG_RLE);
		dst->avg_color = bot->avg_color;
	}

	return true;
}

// Return transparency-related BM_FLAG_* combination for this pixel.
int tmap_test_pixel(int tmap, fix u, fix v)
{
	int orient = ((tmap&0xC000)>>14) & 3;
	tmap = tmap&0x3FFF;

	PIGGY_PAGE_IN(Textures[tmap]);

	grs_bitmap *bm = &GameBitmaps[Textures[tmap].index];

	if (bm->bm_flags & BM_FLAG_RLE)
		bm = rle_expand_texture(bm);

	int w = bm->bm_w;
	int h = bm->bm_h;

	if (orient && WARN_ON(w != h))
		return 0;

	int bx = ((unsigned) f2i(u*w)) % w;
	int by = ((unsigned) f2i(v*h)) % h;

	int tx, ty;
	orient_transform(orient, w, bx, by, &tx, &ty);

	size_t depth = bm->bm_depth < 1 ? 1 : bm->bm_depth;
	return check_pixel_type(bm->bm_data + ty * w * depth + tx * depth,
							bm->bm_flags, bm->bm_depth);
}

int texmerge_test_pixel(int tmap_bottom, int tmap_top, fix u, fix v)
{
	piggy_page_in_two(tmap_bottom, tmap_top);

	if (tmap_top) {
		int c_top = tmap_test_pixel(tmap_top, u, v);

		if (c_top & BM_FLAG_SUPER_TRANSPARENT)
			return WID_RENDPAST_FLAG;
		if (!(c_top & BM_FLAG_TRANSPARENT))
			return 0;
	}

	int c_bottom = tmap_test_pixel(tmap_bottom, u, v);

	if (c_bottom & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))
		return WID_RENDPAST_FLAG;

	return 0;
}

void gr_bitmap_check_transparency(grs_bitmap *bmp)
{
	bmp->bm_flags &= ~(BM_FLAG_TRANSPARENT |
					   BM_FLAG_SUPER_TRANSPARENT |
					   BM_FLAG_SEE_THRU |
					   BM_FLAG_ALPHA);

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
