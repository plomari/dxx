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
 * Definitions for graphics lib.
 *
 */

#ifndef _GR_H
#define _GR_H

#include "pstypes.h"
#include "fix.h"

// some defines for transparency and blending
#define TRANSPARENCY_COLOR   255            // palette entry of transparency color -- 255 on the PC
#define GR_FADE_LEVELS       34
#define GR_FADE_OFF          GR_FADE_LEVELS // yes, max means OFF - don't screw that up
#define GR_BLEND_NORMAL      0              // normal blending
#define GR_BLEND_ADDITIVE_A  1              // additive alpha blending
#define GR_BLEND_ADDITIVE_C  2              // additive color blending

#define GWIDTH  grd_curcanv->cv_w
#define GHEIGHT grd_curcanv->cv_h
#define SWIDTH  (grd_curscreen->sc_w)
#define SHEIGHT (grd_curscreen->sc_h)

#define HIRESMODE (SWIDTH >= 640 && SHEIGHT >= 480 && GameArg.GfxHiresGFXAvailable)
#define MAX_BMP_SIZE(width, height) (4 + ((width) + 2) * (height))

#define SCRNS_DIR "screenshots/"

typedef struct _grs_point {
	fix x,y;
} grs_point;

//these are control characters that have special meaning in the font code

#define CC_COLOR        1   //next char is new foreground color
#define CC_LSPACING     2   //next char specifies line spacing
#define CC_UNDERLINE    3   //next char is underlined

//now have string versions of these control characters (can concat inside a string)

#define CC_COLOR_S      "\x1"   //next char is new foreground color
#define CC_LSPACING_S   "\x2"   //next char specifies line spacing
#define CC_UNDERLINE_S  "\x3"   //next char is underlined

#define BM_FLAG_TRANSPARENT         1
#define BM_FLAG_SUPER_TRANSPARENT   2
#define BM_FLAG_NO_LIGHTING         4
#define BM_FLAG_RLE                 8   // A run-length encoded bitmap.
#define BM_FLAG_PAGED_OUT           16  // This bitmap's data is paged out.
#define BM_FLAG_RLE_BIG             32  // for bitmaps that RLE to > 255 per row (i.e. cockpits)
// D2X-XL
#define BM_FLAG_SEE_THRU			64	// apparently means transparent for viewing, but no fly through
#define BM_FLAG_TGA					128
#define BM_FLAG_OPAQUE				256
// This software
#define BM_FLAG_ALPHA				512 // contains alpha values other than 0/1.0
#define BM_FLAG_NO_DOWNSCALE		1024 // no need to generate mipmaps

typedef struct _grs_bitmap {
	short   bm_x,bm_y;  // Offset from parent's origin
	short   bm_w,bm_h;  // width,height
	int     bm_flags;   // BM_FLAG_*
	short   bm_rowsize; // unsigned char offset to next row
	unsigned char *     bm_data;    // ptr to pixel data...
	                                //   Linear = *parent+(rowsize*y+x)
	                                //   ModeX = *parent+(rowsize*y+x/4)
	                                //   SVGA = *parent+(rowsize*y+x)
	unsigned short      bm_handle;  //for application.  initialized to 0
	ubyte   avg_color;  //  Average color of all pixels in texture map.
	fix avg_color_rgb[3]; // same as above but real rgb value to be used to textured objects that should emit light
	uint8_t				bm_depth;	// bytes per pixel (1 or 4)
	struct _grs_bitmap  *bm_parent;
	struct _ogl_texture *gltexture;
} grs_bitmap;

//font structure
typedef struct _grs_font {
	short       ft_w;           // Width in pixels
	short       ft_h;           // Height in pixels
	short       ft_flags;       // Proportional?
	short       ft_baseline;    //
	ubyte       ft_minchar;     // First char defined by this font
	ubyte       ft_maxchar;     // Last char defined by this font
	short       ft_bytewidth;   // Width in unsigned chars
	ubyte     * ft_data;        // Ptr to raw data.
	ubyte    ** ft_chars;       // Ptrs to data for each char (required for prop font)
	short     * ft_widths;      // Array of widths (required for prop font)
	ubyte     * ft_kerndata;    // Array of kerning triplet data
	// These fields do not participate in disk i/o!
	grs_bitmap *ft_bitmaps;
	grs_bitmap ft_parent_bitmap;
} grs_font;

#define GRS_FONT_SIZE 28    // how much space it takes up on disk

typedef struct _grs_canvas {
	short		cv_x, cv_y;		// offset into the root area (absolute screen pos)
	short		cv_w, cv_h;		// area size of this canvas
	short       cv_color;       // current color
	int         cv_fade_level;  // transparency level
	ubyte       cv_blend_func;  // blending function to use
	grs_font *  cv_font;        // the currently selected font
	short       cv_font_fg_color;   // current font foreground color (-1==Invisible)
	short       cv_font_bg_color;   // current font background color (-1==Invisible)
} grs_canvas;

typedef struct _grs_screen {    // This is a video screen
	grs_canvas  sc_canvas;  // Represents the entire screen
	short   sc_w, sc_h;     // Actual Width and Height
	fix     sc_aspect;      //aspect ratio (w/h) for this screen
} grs_screen;


//=========================================================================
// System functions:
// setup and set mode. this creates a grs_screen structure and sets
// grd_curscreen to point to it.  grs_curcanv points to this screen's
// canvas.  Saves the current VGA state and screen mode.

int gr_init(int mode);
void gr_create_window(void);
void gr_set_attributes(void);

extern void gr_pal_setblock( int start, int number, unsigned char * pal );
extern void gr_pal_getblock( int start, int number, unsigned char * pal );

//shut down the 2d.  Restore the screen mode.
void gr_close(void);

//=========================================================================
// Canvas functions:

// Initialize the specified sub canvas. no memory allocation is performed.

void gr_init_sub_canvas(grs_canvas *new,grs_canvas *src,int x,int y,int w, int h);

// Clear the current canvas to the specified color
void gr_clear_canvas(int color);

//=========================================================================
// Bitmap functions:

// these are the two workhorses, the others just use these
extern void gr_init_bitmap( grs_bitmap *bm, int x, int y, int w, int h, int bytesperline, unsigned char * data );
extern void gr_init_sub_bitmap (grs_bitmap *bm, grs_bitmap *bmParent, int x, int y, int w, int h );

extern void gr_init_bitmap_alloc( grs_bitmap *bm, int x, int y, int w, int h, int bytesperline);

// Allocate a bitmap and its pixel data buffer.
grs_bitmap *gr_create_bitmap(int w,int h);

// Allocated a bitmap and makes its data be raw_data that is already somewhere.
grs_bitmap *gr_create_bitmap_raw(int w, int h, unsigned char * raw_data );

// Creates a bitmap which is part of another bitmap
grs_bitmap *gr_create_sub_bitmap(grs_bitmap *bm,int x,int y,int w, int h);

// Free the bitmap and its pixel data
void gr_free_bitmap(grs_bitmap *bm);

// Free the bitmap's data
void gr_free_bitmap_data (grs_bitmap *bm);
void gr_init_bitmap_data (grs_bitmap *bm);

// Free the bitmap, but not the pixel data buffer
void gr_free_sub_bitmap(grs_bitmap *bm);

void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap *src, grs_canvas *dest);

void gr_set_bitmap_flags(grs_bitmap *pbm, int flags);
void gr_set_transparent(grs_bitmap *pbm, int bTransparent);
void gr_set_super_transparent(grs_bitmap *pbm, int bTransparent);
void gr_set_bitmap_data(grs_bitmap *bm, unsigned char *data);

//=========================================================================
// Color functions:

// When this function is called, the guns are set to gr_palette, and
// the palette stays the same until gr_close is called

void gr_use_palette_table(char * filename );
void gr_copy_palette(ubyte *gr_palette, ubyte *pal, int size);

//=========================================================================
// Drawing functions:

// For solid, XOR, or other fill modes.
int gr_set_drawmode(int mode);

// Sets the color in the current canvas.
void gr_setcolor(int color);
// Sets transparency and blending function
void gr_settransblend(int fade_level, ubyte blend_func);

// Draws a line into the current canvas in the current color and drawmode.
int gr_line(fix x0,fix y0,fix x1,fix y1);
int gr_uline(fix x0,fix y0,fix x1,fix y1);

void show_fullscr(grs_bitmap *bm);

void gr_bitmap_find_transparent_area(grs_bitmap *bm, int *minx, int *miny, int *maxx, int *maxy);

// bitmap function with transparency
void gr_bitmapm( int x, int y, grs_bitmap *bm );

// Draw a rectangle into the current canvas.
void gr_rect(int left,int top,int right,int bot);
void gr_urect(int left,int top,int right,int bot);

// Draw a filled circle
int gr_disk(fix x,fix y,fix r);

// Draw an outline circle
int gr_circle(fix x,fix y,fix r);
int gr_ucircle(fix x,fix y,fix r);

// Draw an unfilled rectangle into the current canvas
void gr_box(int left,int top,int right,int bot);
void gr_ubox(int left,int top,int right,int bot);

// Reads in a font file... current font set to this one.
grs_font * gr_init_font( const char * fontfile );
void gr_close_font( grs_font * font );

//remap a font, re-reading its data & palette
void gr_remap_font( grs_font *font, char * fontname, char *font_data );

//remap (by re-reading) all the color fonts
void gr_remap_color_fonts();
void gr_remap_mono_fonts();

// Writes a string using current font. Returns the next column after last char.
void gr_set_curfont( grs_font * new );
void gr_set_fontcolor( int fg_color, int bg_color );
int gr_string(int x, int y, const char *s );
int gr_ustring(int x, int y, const char *s );
int gr_printf( int x, int y, const char * format, ... );
int gr_uprintf( int x, int y, const char * format, ... );
void gr_get_string_size(const char *s, int *string_width, int *string_height, int *average_width );

typedef struct vms_vector vms_vector;
int gr_3d_string(vms_vector *at, vms_vector *dir_x, vms_vector *dir_y, char *s );

//===========================================================================
// Global variables
extern grs_canvas *grd_curcanv;             //active canvas
extern grs_screen *grd_curscreen;           //active screen
extern unsigned char Test_bitmap_data[64*64];

extern unsigned int FixDivide( unsigned int x, unsigned int y );

extern void gr_set_current_canvas( grs_canvas *canv );

//flags for fonts
#define FT_COLOR        1
#define FT_PROPORTIONAL 2
#define FT_KERNED       4

// Special effects
extern void gr_snow_out(int num_dots);

extern void test_rotate_bitmap(void);
extern void rotate_bitmap(grs_bitmap *bp, grs_point *vertbuf, int light_value);

extern ubyte gr_palette[256*3];
extern ubyte gr_fade_table[256*GR_FADE_LEVELS];
extern ubyte gr_inverse_table[32*32*32];

extern ushort gr_palette_selector;
extern ushort gr_inverse_table_selector;
extern ushort gr_fade_table_selector;

// Remaps a bitmap into the current palette. If transparent_color is
// between 0 and 255 then all occurances of that color are mapped to
// whatever color the 2d uses for transparency. This is normally used
// right after a call to iff_read_bitmap like this:
//		iff_error = iff_read_bitmap(filename,new,BM_LINEAR,newpal);
//		if (iff_error != IFF_NO_ERROR) Error("Can't load IFF file <%s>, error=%d",filename,iff_error);
//		if ( iff_has_transparency )
//			gr_remap_bitmap( new, newpal, iff_transparent_color );
//		else
//			gr_remap_bitmap( new, newpal, -1 );
extern void gr_remap_bitmap( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color );

// Same as above, but searches using gr_find_closest_color which uses
// 18-bit accurracy instead of 15bit when translating colors.
extern void gr_remap_bitmap_good( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color );

extern void build_colormap_good( ubyte * palette, ubyte * colormap, int * freq );

extern void gr_palette_step_up( int r, int g, int b );

extern void gr_bitmap_check_transparency( grs_bitmap * bmp );

// Allocates a selector that has a base address at 'address' and length 'size'.
// Returns 0 if successful... BE SURE TO CHECK the return value since there
// is a limited number of selectors available!!!
extern int get_selector( void * address, int size, unsigned int * selector );

#define BM_RGB(r,g,b) ( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )
#define BM_XRGB(r,g,b) gr_find_closest_color( (r)*2,(g)*2,(b)*2 )

// Given: r,g,b, each in range of 0-63, return the color index that
// best matches the input.
int gr_find_closest_color( int r, int g, int b );
int gr_find_closest_color_15bpp( int rgb );

void gr_prepare_frame(void);
extern void gr_flip(void);
extern void gr_set_draw_buffer(int buf);

/*
 * must return 0 if windowed, 1 if fullscreen
 */
int gr_check_fullscreen(void);

/*
 * returns state after toggling (ie, same as if you had called
 * check_fullscreen immediatly after)
 */
int gr_toggle_fullscreen(void);

void gr_enable_depth(int enable);

void gr_sdl_ogl_resize_window(int w, int h);

void gr_set_input_grab(bool val);

extern int gr_focus_lost;

#endif /* def _GR_H */
