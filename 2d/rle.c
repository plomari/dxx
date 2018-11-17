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
 * Routines to do run length encoding/decoding
 * on bitmaps.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "dxxerror.h"
#include "rle.h"
#include "byteswap.h"

#define RLE_CODE        0xE0
#define NOT_RLE_CODE    31
#define IS_RLE_CODE(x) (((x) & RLE_CODE) == RLE_CODE)
#define rle_stosb(_dest, _len, _color)	memset(_dest,_color,_len)

void gr_rle_decode( ubyte * src, ubyte * dest )
{
	int i;
	ubyte data, count = 0;

	while(1) {
		data = *src++;
		if ( ! IS_RLE_CODE(data) ) {
			*dest++ = data;
		} else {
			count = data & NOT_RLE_CODE;
			if (count == 0)
				return;
			data = *src++;
			for (i = 0; i < count; i++)
				*dest++ = data;
		}
	}
}

#define MAX_CACHE_BITMAPS 32

typedef struct rle_cache_element {
	grs_bitmap * rle_bitmap;
	ubyte * rle_data;
	grs_bitmap * expanded_bitmap;
	int last_used;
} rle_cache_element;

int rle_cache_initialized = 0;
int rle_counter = 0;
int rle_next = 0;
rle_cache_element rle_cache[MAX_CACHE_BITMAPS];

int rle_hits = 0;
int rle_misses = 0;

void rle_cache_close(void)
{
	if (rle_cache_initialized)	{
		int i;
		rle_cache_initialized = 0;
		for (i=0; i<MAX_CACHE_BITMAPS; i++ )	{
			gr_free_bitmap(rle_cache[i].expanded_bitmap);
		}
	}
}

void rle_cache_init()
{
	int i;
	for (i=0; i<MAX_CACHE_BITMAPS; i++ )	{
		rle_cache[i].rle_bitmap = NULL;
		rle_cache[i].expanded_bitmap = gr_create_bitmap( 64, 64 );
		rle_cache[i].last_used = 0;
		Assert( rle_cache[i].expanded_bitmap != NULL );
	}
	rle_cache_initialized = 1;
}

void rle_cache_flush()
{
	int i;
	for (i=0; i<MAX_CACHE_BITMAPS; i++ )	{
		rle_cache[i].rle_bitmap = NULL;
		rle_cache[i].last_used = 0;
	}
}

void rle_expand_texture_sub( grs_bitmap * bmp, grs_bitmap * rle_temp_bitmap_1 )
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i;

	sbits = &bmp->bm_data[4 + bmp->bm_h];
	dbits = rle_temp_bitmap_1->bm_data;

	rle_temp_bitmap_1->bm_flags = bmp->bm_flags & (~BM_FLAG_RLE);

	for (i=0; i < bmp->bm_h; i++ )    {
		gr_rle_decode( sbits, dbits );
		sbits += (int)bmp->bm_data[4+i];
		dbits += bmp->bm_w;
	}
}


grs_bitmap * rle_expand_texture( grs_bitmap * bmp )
{
	int i;
	int lowest_count, lc;
	int least_recently_used;

	if (!rle_cache_initialized) rle_cache_init();

	Assert( !(bmp->bm_flags & BM_FLAG_PAGED_OUT) );

	lc = rle_counter;
	rle_counter++;

	if (rle_counter < 0)
		rle_counter = 0;
	
	if ( rle_counter < lc )	{
		for (i=0; i<MAX_CACHE_BITMAPS; i++ )	{
			rle_cache[i].rle_bitmap = NULL;
			rle_cache[i].last_used = 0;
		}
	}

	lowest_count = rle_cache[rle_next].last_used;
	least_recently_used = rle_next;
	rle_next++;
	if ( rle_next >= MAX_CACHE_BITMAPS )
		rle_next = 0;

	for (i=0; i<MAX_CACHE_BITMAPS; i++ )	{
		if (rle_cache[i].rle_bitmap == bmp) 	{
			rle_hits++;
			rle_cache[i].last_used = rle_counter;
			return rle_cache[i].expanded_bitmap;
		}
		if ( rle_cache[i].last_used < lowest_count )	{
			lowest_count = rle_cache[i].last_used;
			least_recently_used = i;
		}
	}

	Assert(bmp->bm_w<=64 && bmp->bm_h<=64); //dest buffer is 64x64
	rle_misses++;
	rle_expand_texture_sub( bmp, rle_cache[least_recently_used].expanded_bitmap );
	rle_cache[least_recently_used].rle_bitmap = bmp;
	rle_cache[least_recently_used].last_used = rle_counter;
	return rle_cache[least_recently_used].expanded_bitmap;
}

/*
 * swaps entries 0 and 255 in an RLE bitmap without uncompressing it
 */
void rle_swap_0_255(grs_bitmap *bmp)
{
	int i, j, len, rle_big;
	unsigned char *ptr, *ptr2, *temp, *start;
	unsigned short line_size;

	rle_big = bmp->bm_flags & BM_FLAG_RLE_BIG;

	temp = d_malloc( MAX_BMP_SIZE(bmp->bm_w, bmp->bm_h) );

	if (rle_big) {                  // set ptrs to first lines
		ptr = bmp->bm_data + 4 + 2 * bmp->bm_h;
		ptr2 = temp + 4 + 2 * bmp->bm_h;
	} else {
		ptr = bmp->bm_data + 4 + bmp->bm_h;
		ptr2 = temp + 4 + bmp->bm_h;
	}
	for (i = 0; i < bmp->bm_h; i++) {
		start = ptr2;
		if (rle_big)
			line_size = INTEL_SHORT(*((unsigned short *)&bmp->bm_data[4 + 2 * i]));
		else
			line_size = bmp->bm_data[4 + i];
		for (j = 0; j < line_size; j++) {
			if ( ! IS_RLE_CODE(ptr[j]) ) {
				if (ptr[j] == 0) {
					*ptr2++ = RLE_CODE | 1;
					*ptr2++ = 255;
				} else
					*ptr2++ = ptr[j];
			} else {
				*ptr2++ = ptr[j];
				if ((ptr[j] & NOT_RLE_CODE) == 0)
					break;
				j++;
				if (ptr[j] == 0)
					*ptr2++ = 255;
				else if (ptr[j] == 255)
					*ptr2++ = 0;
				else
					*ptr2++ = ptr[j];
			}
		}
		if (rle_big)                // set line size
			*((unsigned short *)&temp[4 + 2 * i]) = INTEL_SHORT(ptr2 - start);
		else
			temp[4 + i] = ptr2 - start;
		ptr += line_size;           // go to next line
	}
	len = ptr2 - temp;
	*((int *)temp) = len;           // set total size
	memcpy(bmp->bm_data, temp, len);
	d_free(temp);
}

/*
 * remaps all entries using colormap in an RLE bitmap without uncompressing it
 */
void rle_remap(grs_bitmap *bmp, ubyte *colormap)
{
	int i, j, len, rle_big;
	unsigned char *ptr, *ptr2, *temp, *start;
	unsigned short line_size;

	rle_big = bmp->bm_flags & BM_FLAG_RLE_BIG;

	temp = d_malloc( MAX_BMP_SIZE(bmp->bm_w, bmp->bm_h) + 30000 );

	if (rle_big) {                  // set ptrs to first lines
		ptr = bmp->bm_data + 4 + 2 * bmp->bm_h;
		ptr2 = temp + 4 + 2 * bmp->bm_h;
	} else {
		ptr = bmp->bm_data + 4 + bmp->bm_h;
		ptr2 = temp + 4 + bmp->bm_h;
	}
	for (i = 0; i < bmp->bm_h; i++) {
		start = ptr2;
		if (rle_big)
			line_size = INTEL_SHORT(*((unsigned short *)&bmp->bm_data[4 + 2 * i]));
		else
			line_size = bmp->bm_data[4 + i];
		for (j = 0; j < line_size; j++) {
			if ( ! IS_RLE_CODE(ptr[j])) {
				if (IS_RLE_CODE(colormap[ptr[j]])) 
					*ptr2++ = RLE_CODE | 1; // add "escape sequence"
				*ptr2++ = colormap[ptr[j]]; // translate
			} else {
				*ptr2++ = ptr[j]; // just copy current rle code
				if ((ptr[j] & NOT_RLE_CODE) == 0)
					break;
				j++;
				*ptr2++ = colormap[ptr[j]]; // translate
			}
		}
		if (rle_big)                // set line size
			*((unsigned short *)&temp[4 + 2 * i]) = INTEL_SHORT(ptr2 - start);
		else
			temp[4 + i] = ptr2 - start;
		ptr += line_size;           // go to next line
	}
	len = ptr2 - temp;
	*((int *)temp) = len;           // set total size
	memcpy(bmp->bm_data, temp, len);
	d_free(temp);
}
