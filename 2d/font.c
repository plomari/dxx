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
 * Graphical routines for drawing fonts.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "dxxerror.h"
#include "cfile.h"
#include "byteswap.h"
#include "bitmap.h"
#include "makesig.h"
#include "gamefont.h"
#include "console.h"
#include "config.h"
#include "inferno.h"
#include "ogl.h"

#define FONTSCALE_X(x) ((float)(x)*(FNTScaleX))
#define FONTSCALE_Y(x) ((float)(x)*(FNTScaleY))

#define MAX_OPEN_FONTS	50

typedef struct openfont {
	char filename[FILENAME_LEN];
	grs_font *ptr;
	char *dataptr;
} openfont;

//list of open fonts, for use (for now) for palette remapping
openfont open_font[MAX_OPEN_FONTS];

#define BITS_TO_BYTES(x)    (((x)+7)>>3)

ubyte *find_kern_entry(grs_font *font,ubyte first,ubyte second)
{
	ubyte *p=font->ft_kerndata;

	while (*p!=255)
		if (p[0]==first && p[1]==second)
			return p;
		else p+=3;

	return NULL;

}

//takes the character AFTER being offset into font
#define INFONT(_c) ((_c >= 0) && (_c <= grd_curcanv->cv_font->ft_maxchar-grd_curcanv->cv_font->ft_minchar))

//takes the character BEFORE being offset into current font
void get_char_width(ubyte c,ubyte c2,int *width,int *spacing)
{
	int letter;

	letter = c-grd_curcanv->cv_font->ft_minchar;

	if (!INFONT(letter)) {				//not in font, draw as space
		*width=0;
		if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
			*spacing = FONTSCALE_X(grd_curcanv->cv_font->ft_w)/2;
		else
			*spacing = grd_curcanv->cv_font->ft_w;
		return;
	}

	if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
		*width = FONTSCALE_X(grd_curcanv->cv_font->ft_widths[letter]);
	else
		*width = grd_curcanv->cv_font->ft_w;

	*spacing = *width;

	if (grd_curcanv->cv_font->ft_flags & FT_KERNED)  {
		ubyte *p;

		if (!(c2==0 || c2=='\n')) {
			int letter2 = c2-grd_curcanv->cv_font->ft_minchar;

			if (INFONT(letter2)) {

				p = find_kern_entry(grd_curcanv->cv_font,(ubyte)letter,letter2);

				if (p)
					*spacing = FONTSCALE_X(p[2]);
			}
		}
	}
}

// Same as above but works with floats, which is better for string-size measurement while being bad for string composition of course
void get_char_width_f(ubyte c,ubyte c2,float *width,float *spacing)
{
	int letter;

	letter = c-grd_curcanv->cv_font->ft_minchar;

	if (!INFONT(letter)) {				//not in font, draw as space
		*width=0;
		if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
			*spacing = FONTSCALE_X(grd_curcanv->cv_font->ft_w)/2;
		else
			*spacing = grd_curcanv->cv_font->ft_w;
		return;
	}

	if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
		*width = FONTSCALE_X(grd_curcanv->cv_font->ft_widths[letter]);
	else
		*width = grd_curcanv->cv_font->ft_w;

	*spacing = *width;

	if (grd_curcanv->cv_font->ft_flags & FT_KERNED)  {
		ubyte *p;

		if (!(c2==0 || c2=='\n')) {
			int letter2 = c2-grd_curcanv->cv_font->ft_minchar;

			if (INFONT(letter2)) {

				p = find_kern_entry(grd_curcanv->cv_font,(ubyte)letter,letter2);

				if (p)
					*spacing = FONTSCALE_X(p[2]);
			}
		}
	}
}

int get_centered_x(const char *s)
{
	float w,w2,s2;

	for (w=0;*s!=0 && *s!='\n';s++) {
		if (*s<=0x06) {
			if (*s<=0x03)
				s++;
			continue;//skip color codes.
		}
		get_char_width_f(s[0],s[1],&w2,&s2);
		w += s2;
	}

	return ((grd_curcanv->cv_w - w) / 2);
}

//hack to allow color codes to be embedded in strings -MPM
//note we subtract one from color, since 255 is "transparent" so it'll never be used, and 0 would otherwise end the string.
//function must already have orig_color var set (or they could be passed as args...)
//perhaps some sort of recursive orig_color type thing would be better, but that would be way too much trouble for little gain
int gr_message_color_level=1;
#define CHECK_EMBEDDED_COLORS() if ((*text_ptr >= 0x01) && (*text_ptr <= 0x02)) { \
		text_ptr++; \
		if (*text_ptr){ \
			if (gr_message_color_level >= *(text_ptr-1)) \
				grd_curcanv->cv_font_fg_color = (unsigned char)*text_ptr; \
			text_ptr++; \
		} \
	} \
	else if (*text_ptr == 0x03) \
	{ \
		underline = 1; \
		text_ptr++; \
	} \
	else if ((*text_ptr >= 0x04) && (*text_ptr <= 0x06)){ \
		if (gr_message_color_level >= *text_ptr - 3) \
			grd_curcanv->cv_font_fg_color=(unsigned char)orig_color; \
		text_ptr++; \
	}

int pow2ize(int x);//from ogl.c

int get_font_total_width(grs_font * font){
	if (font->ft_flags & FT_PROPORTIONAL){
		int i,w=0,c=font->ft_minchar;
		for (i=0;c<=font->ft_maxchar;i++,c++){
			if (font->ft_widths[i]<0)
				Error("heh?\n");
			w+=font->ft_widths[i];
		}
		return w;
	}else{
		return font->ft_w*(font->ft_maxchar-font->ft_minchar+1);
	}
}
void ogl_font_choose_size(grs_font * font,int gap,int *rw,int *rh){
	int	nchars = font->ft_maxchar-font->ft_minchar+1;
	int r,x,y,nc=0,smallest=999999,smallr=-1,tries;
	int smallprop=10000;
	int h,w;
	for (h=32;h<=256;h*=2){
//		h=pow2ize(font->ft_h*rows+gap*(rows-1));
		if (font->ft_h>h)continue;
		r=(h/(font->ft_h+gap));
		w=pow2ize((get_font_total_width(font)+(nchars-r)*gap)/r);
		tries=0;
		do {
			if (tries)
				w=pow2ize(w+1);
			if(tries>3){
				break;
			}
			nc=0;
			y=0;
			while(y+font->ft_h<=h){
				x=0;
				while (x<w){
					if (nc==nchars)
						break;
					if (font->ft_flags & FT_PROPORTIONAL){
						if (x+font->ft_widths[nc]+gap>w)break;
						x+=font->ft_widths[nc++]+gap;
					}else{
						if (x+font->ft_w+gap>w)break;
						x+=font->ft_w+gap;
						nc++;
					}
				}
				if (nc==nchars)
					break;
				y+=font->ft_h+gap;
			}
			
			tries++;
		}while(nc!=nchars);
		if (nc!=nchars)
			continue;

		if (w*h==smallest){//this gives squarer sizes priority (ie, 128x128 would be better than 512*32)
			if (w>=h){
				if (w/h<smallprop){
					smallprop=w/h;
					smallest++;//hack
				}
			}else{
				if (h/w<smallprop){
					smallprop=h/w;
					smallest++;//hack
				}
			}
		}
		if (w*h<smallest){
			smallr=1;
			smallest=w*h;
			*rw=w;
			*rh=h;
		}
	}
	if (smallr<=0)
		Error("couldn't fit font?\n");
}

void ogl_init_font(grs_font * font)
{
	int	nchars = font->ft_maxchar-font->ft_minchar+1;
	int i,w,h,tw,th,x,y,curx=0,cury=0;
	unsigned char *fp;
	ubyte *data;
	int gap=1; // x/y offset between the chars so we can filter

	th = tw = 0xffff;

	ogl_font_choose_size(font,gap,&tw,&th);
	data=d_malloc(tw*th);
	memset(data, TRANSPARENCY_COLOR, tw * th); // map the whole data with transparency so we won't have borders if using gap
	gr_init_bitmap(&font->ft_parent_bitmap,0,0,tw,th,tw,data);
	gr_set_transparent(&font->ft_parent_bitmap, 1);

	font->ft_parent_bitmap.gltexture = ogl_get_free_texture();
	ogl_init_texture(font->ft_parent_bitmap.gltexture, tw, th);

	font->ft_bitmaps=(grs_bitmap*)d_malloc( nchars * sizeof(grs_bitmap));
	h=font->ft_h;

	for(i=0;i<nchars;i++)
	{
		if (font->ft_flags & FT_PROPORTIONAL)
			w=font->ft_widths[i];
		else
			w=font->ft_w;

		if (w<1 || w>256)
			continue;

		if (curx+w+gap>tw)
		{
			cury+=h+gap;
			curx=0;
		}

		if (cury+h>th)
			Error("font doesn't really fit (%i/%i)?\n",i,nchars);

		if (font->ft_flags & FT_COLOR)
		{
			if (font->ft_flags & FT_PROPORTIONAL)
				fp = font->ft_chars[i];
			else
				fp = font->ft_data + i * w*h;
			for (y=0;y<h;y++)
			{
				for (x=0;x<w;x++)
				{
					font->ft_parent_bitmap.bm_data[curx+x+(cury+y)*tw]=fp[x+y*w];
					// Let's call this a HACK:
					// If we filter the fonts, the sliders will be messed up as the border pixels will have an
					// alpha value while filtering. So the slider bitmaps will not look "connected".
					// To prevent this, duplicate the first/last pixel-row with a 1-pixel offset.
					if (gap && i >= 99 && i <= 102)
					{
						// See which bitmaps need left/right shifts:
						// 99  = SLIDER_LEFT - shift RIGHT
						// 100 = SLIDER_RIGHT - shift LEFT
						// 101 = SLIDER_MIDDLE - shift LEFT+RIGHT
						// 102 = SLIDER_MARKER - shift RIGHT

						// shift left border
						if (x==0 && i != 99 && i != 102)
							font->ft_parent_bitmap.bm_data[(curx+x+(cury+y)*tw)-1]=fp[x+y*w];

						// shift right border
						if (x==w-1 && i != 100)
							font->ft_parent_bitmap.bm_data[(curx+x+(cury+y)*tw)+1]=fp[x+y*w];
					}
				}
			}
		}
		else
		{
			int BitMask,bits=0,white=gr_find_closest_color(63,63,63);
			if (font->ft_flags & FT_PROPORTIONAL)
				fp = font->ft_chars[i];
			else
				fp = font->ft_data + i * BITS_TO_BYTES(w)*h;
			for (y=0;y<h;y++){
				BitMask=0;
				for (x=0; x< w; x++ )
				{
					if (BitMask==0) {
						bits = *fp++;
						BitMask = 0x80;
					}

					if (bits & BitMask)
						font->ft_parent_bitmap.bm_data[curx+x+(cury+y)*tw]=white;
					else
						font->ft_parent_bitmap.bm_data[curx+x+(cury+y)*tw]=255;
					BitMask >>= 1;
				}
			}
		}
		gr_init_sub_bitmap(&font->ft_bitmaps[i],&font->ft_parent_bitmap,curx,cury,w,h);
		curx+=w+gap;
	}
	ogl_loadbmtexture(&font->ft_parent_bitmap);
}

int ogl_internal_string(int x, int y, const char *s )
{
	const char * text_ptr, * next_row, * text_ptr1;
	int width, spacing,letter;
	int xx,yy;
	int orig_color=grd_curcanv->cv_font_fg_color;//to allow easy reseting to default string color with colored strings -MPM
	int underline;

	next_row = s;

	yy = y;

	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		text_ptr = text_ptr1;

		xx = x;

		if (xx==0x8000)			//centered
			xx = get_centered_x(text_ptr);

		while (*text_ptr)
		{
			int ft_w;

			if (*text_ptr == '\n' )
			{
				next_row = &text_ptr[1];
				yy += FONTSCALE_Y(grd_curcanv->cv_font->ft_h)+FSPACY(1);
				break;
			}

			letter = (unsigned char)*text_ptr - grd_curcanv->cv_font->ft_minchar;

			get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

			underline = 0;
			if (!INFONT(letter) || (unsigned char)*text_ptr <= 0x06) //not in font, draw as space
			{
				CHECK_EMBEDDED_COLORS() else{
					xx += spacing;
					text_ptr++;
				}
				
				if (underline)
				{
					ubyte save_c = (unsigned char) COLOR;
					
					gr_setcolor(grd_curcanv->cv_font_fg_color);
					gr_rect(xx, yy + grd_curcanv->cv_font->ft_baseline + 2, xx + grd_curcanv->cv_font->ft_w, yy + grd_curcanv->cv_font->ft_baseline + 3);
					gr_setcolor(save_c);
				}

				continue;
			}
			
			if (grd_curcanv->cv_font->ft_flags & FT_PROPORTIONAL)
				ft_w = grd_curcanv->cv_font->ft_widths[letter];
			else
				ft_w = grd_curcanv->cv_font->ft_w;

			if (grd_curcanv->cv_font->ft_flags&FT_COLOR)
				ogl_ubitmapm_cs(xx,yy,FONTSCALE_X(ft_w),FONTSCALE_Y(grd_curcanv->cv_font->ft_h),&grd_curcanv->cv_font->ft_bitmaps[letter],-1,F1_0);
			else{
				ogl_ubitmapm_cs(xx,yy,ft_w*(FONTSCALE_X(grd_curcanv->cv_font->ft_w)/grd_curcanv->cv_font->ft_w),FONTSCALE_Y(grd_curcanv->cv_font->ft_h),&grd_curcanv->cv_font->ft_bitmaps[letter],grd_curcanv->cv_font_fg_color,F1_0);
			}

			xx += spacing;

			text_ptr++;
		}

	}
	return 0;
}

// Draw the text at the point given by at.
// dir_x/dir_y define orientation and scaling.
// The length of dir_x and dir_y defines 1 pixel size (scaled to target).
int gr_3d_string(vms_vector *at, vms_vector *dir_x, vms_vector *dir_y, char *s )
{
	char * text_ptr, * next_row, * text_ptr1;
	int width, spacing,letter, underline;
	int orig_color=grd_curcanv->cv_font_fg_color;//to allow easy reseting to default string color with colored strings -MPM
	float x = 0, y = 0;

	next_row = s;

	while (next_row != NULL)
	{
		text_ptr1 = next_row;
		next_row = NULL;

		text_ptr = text_ptr1;

		x = 0;

		while (*text_ptr)
		{
			if (*text_ptr == '\n' )
			{
				next_row = &text_ptr[1];
				y += FONTSCALE_Y(grd_curcanv->cv_font->ft_h)+FSPACY(1);
				break;
			}

			letter = (unsigned char)*text_ptr - grd_curcanv->cv_font->ft_minchar;

			get_char_width(text_ptr[0],text_ptr[1],&width,&spacing);

			if (!INFONT(letter) || (unsigned char)*text_ptr <= 0x06)
			{   //not in font, draw as space
				CHECK_EMBEDDED_COLORS() else{
					x += spacing;
					text_ptr++;
				}
				continue;
			}

			// So, the "2D" text stuff also does this, but it appears not to make
			// any sense?
			//if (grd_curcanv->cv_bitmap.bm_type != BM_OGL)
 				//Error("ogl_internal_string: non-color string to non-ogl dest\n");
			//	return -1;

			vms_vector cur_at = *at;
			vm_vec_scale_add2(&cur_at, dir_x, fl2f(x));
			vm_vec_scale_add2(&cur_at, dir_y, fl2f(y));

			vms_vector cur_dx, cur_dy;
			vm_vec_copy_scale(&cur_dx, dir_x,
				fl2f(FONTSCALE_X(grd_curcanv->cv_font->ft_widths[letter])));
			vm_vec_copy_scale(&cur_dy, dir_y,
				fl2f(FONTSCALE_Y(grd_curcanv->cv_font->ft_h)));

			ogl_ubitmapm_3d(&cur_at, &cur_dx, &cur_dy,
							&grd_curcanv->cv_font->ft_bitmaps[letter],
							grd_curcanv->cv_font->ft_flags&FT_COLOR ? grd_curcanv->cv_font_fg_color : -1);

			x += spacing;

			text_ptr++;
		}

	}
	return 0;
}

int gr_string(int x, int y, const char *s )
{
	int w, h, aw;
	int clipped=0;

	Assert(grd_curcanv->cv_font != NULL);

	if ( x == 0x8000 )	{
		if ( y<0 ) clipped |= 1;
		gr_get_string_size(s, &w, &h, &aw );
		// for x, since this will be centered, only look at
		// width.
		if ( w > grd_curcanv->cv_w ) clipped |= 1;
		if ( (y+h) > grd_curcanv->cv_h ) clipped |= 1;

		if ( (y+h) < 0 ) clipped |= 2;
		if ( y > grd_curcanv->cv_h ) clipped |= 2;

	} else {
		if ( (x<0) || (y<0) ) clipped |= 1;
		gr_get_string_size(s, &w, &h, &aw );
		if ( (x+w) > grd_curcanv->cv_w ) clipped |= 1;
		if ( (y+h) > grd_curcanv->cv_h ) clipped |= 1;
		if ( (x+w) < 0 ) clipped |= 2;
		if ( (y+h) < 0 ) clipped |= 2;
		if ( x > grd_curcanv->cv_w ) clipped |= 2;
		if ( y > grd_curcanv->cv_h ) clipped |= 2;
	}

	if ( !clipped )
		return gr_ustring(x, y, s );

	if ( clipped & 2 )	{
		// Completely clipped...
		return 0;
	}

	// Partially clipped...

	return ogl_internal_string(x,y,s);
}

int gr_ustring(int x, int y, const char *s )
{
	return ogl_internal_string(x,y,s);
}


void gr_get_string_size(const char *s, int *string_width, int *string_height, int *average_width )
{
	int i = 0;
	float width=0.0,spacing=0.0,longest_width=0.0,string_width_f=0.0,string_height_f=0.0;

	string_height_f = FONTSCALE_Y(grd_curcanv->cv_font->ft_h);
	string_width_f = 0;
	*average_width = grd_curcanv->cv_font->ft_w;

	if (s != NULL )
	{
		string_width_f = 0;
		while (*s)
		{
//			if (*s == '&')
//				s++;
			while (*s == '\n')
			{
				s++;
				string_height_f += FONTSCALE_Y(grd_curcanv->cv_font->ft_h)+FSPACY(1);
				string_width_f = 0;
			}

			if (*s == 0) break;

			get_char_width_f(s[0],s[1],&width,&spacing);

			string_width_f += spacing;

			if (string_width_f > longest_width)
				longest_width = string_width_f;

			i++;
			s++;
		}
	}
	string_width_f = longest_width;
	*string_width = string_width_f;
	*string_height = string_height_f;
}

int gr_printf( int x, int y, const char * format, ... )
{
	char buffer[1000];
	va_list args;

	va_start(args, format );
	vsnprintf(buffer,sizeof(buffer),format,args);
	va_end(args);
	return gr_string( x, y, buffer );
}

void gr_close_font( grs_font * font )
{
	if (font)
	{
		int fontnum;
		char * font_data;

		//find font in list
		for (fontnum=0;fontnum<MAX_OPEN_FONTS && open_font[fontnum].ptr!=font;fontnum++);
		Assert(fontnum<MAX_OPEN_FONTS);	//did we find slot?

		font_data = open_font[fontnum].dataptr;
		d_free( font_data );

		open_font[fontnum].ptr = NULL;
		open_font[fontnum].dataptr = NULL;

		if ( font->ft_chars )
			d_free( font->ft_chars );
		if (font->ft_bitmaps)
			d_free( font->ft_bitmaps );
		gr_free_bitmap_data(&font->ft_parent_bitmap);
		d_free( font );
	}
}

//remap (by re-reading) all the color fonts
void gr_remap_color_fonts()
{
	int fontnum;

	for (fontnum=0;fontnum<MAX_OPEN_FONTS;fontnum++) {
		grs_font *font;

		font = open_font[fontnum].ptr;

		if (font && (font->ft_flags & FT_COLOR))
			gr_remap_font(font, open_font[fontnum].filename, open_font[fontnum].dataptr);
	}
}

void gr_remap_mono_fonts()
{
	int fontnum;
	con_printf (CON_DEBUG, "gr_remap_mono_fonts ()\n");
	for (fontnum=0;fontnum<MAX_OPEN_FONTS;fontnum++) {
		grs_font *font;
		font = open_font[fontnum].ptr;
		if (font && !(font->ft_flags & FT_COLOR))
			gr_remap_font(font, open_font[fontnum].filename, open_font[fontnum].dataptr);
	}
}

/*
 * reads a grs_font structure from a CFILE
 */
void grs_font_read(grs_font *gf, CFILE *fp)
{
	gf->ft_w = cfile_read_short(fp);
	gf->ft_h = cfile_read_short(fp);
	gf->ft_flags = cfile_read_short(fp);
	gf->ft_baseline = cfile_read_short(fp);
	gf->ft_minchar = cfile_read_byte(fp);
	gf->ft_maxchar = cfile_read_byte(fp);
	gf->ft_bytewidth = cfile_read_short(fp);
	gf->ft_data = (ubyte *)(size_t)cfile_read_int(fp);
	gf->ft_chars = (ubyte **)(size_t)cfile_read_int(fp);
	gf->ft_widths = (short *)(size_t)cfile_read_int(fp);
	gf->ft_kerndata = (ubyte *)(size_t)cfile_read_int(fp);
}

grs_font * gr_init_font( const char * fontname )
{
	static int first_time=1;
	grs_font *font;
	char *font_data;
	int i,fontnum;
	unsigned char * ptr;
	int nchars;
	CFILE *fontfile;
	char file_id[4];
	int datasize;	//size up to (but not including) palette

	if (first_time) {
		int i;
		for (i=0;i<MAX_OPEN_FONTS;i++)
		{
			open_font[i].ptr = NULL;
			open_font[i].dataptr = NULL;
		}
		first_time=0;
	}

	//find free font slot
	for (fontnum=0;fontnum<MAX_OPEN_FONTS && open_font[fontnum].ptr!=NULL;fontnum++);
	Assert(fontnum<MAX_OPEN_FONTS);	//did we find one?

	strncpy(open_font[fontnum].filename,fontname,FILENAME_LEN);

	fontfile = cfopen(fontname, "rb");

	if (!fontfile) {
		con_printf(CON_VERBOSE, "Can't open font file %s\n", fontname);
		return NULL;
	}

	cfread(file_id, 4, 1, fontfile);
	if ( !strncmp( file_id, "NFSP", 4 ) ) {
		con_printf(CON_NORMAL, "File %s is not a font file\n", fontname);
		return NULL;
	}

	datasize = cfile_read_int(fontfile);
	datasize -= GRS_FONT_SIZE; // subtract the size of the header.

	MALLOC(font, grs_font, 1);
	grs_font_read(font, fontfile);

	MALLOC(font_data, char, datasize);
	cfread(font_data, 1, datasize, fontfile);

	open_font[fontnum].ptr = font;
	open_font[fontnum].dataptr = font_data;

	// make these offsets relative to font_data
	font->ft_data = (ubyte *)((size_t)font->ft_data - GRS_FONT_SIZE);
	font->ft_widths = (short *)((size_t)font->ft_widths - GRS_FONT_SIZE);
	font->ft_kerndata = (ubyte *)((size_t)font->ft_kerndata - GRS_FONT_SIZE);

	nchars = font->ft_maxchar - font->ft_minchar + 1;

	if (font->ft_flags & FT_PROPORTIONAL) {

		font->ft_widths = (short *) &font_data[(size_t)font->ft_widths];
		font->ft_data = (unsigned char *) &font_data[(size_t)font->ft_data];
		font->ft_chars = (unsigned char **)d_malloc( nchars * sizeof(unsigned char *));

		ptr = font->ft_data;

		for (i=0; i< nchars; i++ ) {
			font->ft_widths[i] = INTEL_SHORT(font->ft_widths[i]);
			font->ft_chars[i] = ptr;
			if (font->ft_flags & FT_COLOR)
				ptr += font->ft_widths[i] * font->ft_h;
			else
				ptr += BITS_TO_BYTES(font->ft_widths[i]) * font->ft_h;
		}

	} else  {

		font->ft_data   = (unsigned char *) font_data;
		font->ft_chars  = NULL;
		font->ft_widths = NULL;

		ptr = font->ft_data + (nchars * font->ft_w * font->ft_h);
	}

	if (font->ft_flags & FT_KERNED)
		font->ft_kerndata = (unsigned char *) &font_data[(size_t)font->ft_kerndata];

	if (font->ft_flags & FT_COLOR) {		//remap palette
		ubyte palette[256*3];
		ubyte colormap[256];
		int freq[256];

		cfread(palette,3,256,fontfile);		//read the palette

		build_colormap_good( (ubyte *)&palette, colormap, freq );

		colormap[TRANSPARENCY_COLOR] = TRANSPARENCY_COLOR;              // changed from colormap[255] = 255 to this for macintosh

		decode_data(font->ft_data, ptr - font->ft_data, colormap, freq );
	}

	cfclose(fontfile);

	//set curcanv vars

	grd_curcanv->cv_font        = font;
	grd_curcanv->cv_font_fg_color    = 0;
	grd_curcanv->cv_font_bg_color    = 0;

	ogl_init_font(font);

	return font;
}

//remap a font by re-reading its data & palette
void gr_remap_font( grs_font *font, char * fontname, char *font_data )
{
	int i;
	int nchars;
	CFILE *fontfile;
	char file_id[4];
	int datasize;        //size up to (but not including) palette
	unsigned char *ptr;

	fontfile = cfopen(fontname, "rb");

	if (!fontfile)
		Error( "Can't open font file %s", fontname );

	cfread(file_id, 4, 1, fontfile);
	if ( !strncmp( file_id, "NFSP", 4 ) )
		Error( "File %s is not a font file", fontname );

	datasize = cfile_read_int(fontfile);
	datasize -= GRS_FONT_SIZE; // subtract the size of the header.

	d_free(font->ft_chars);
	grs_font_read(font, fontfile); // have to reread in case mission hogfile overrides font.

	cfread(font_data, 1, datasize, fontfile);  //read raw data

	// make these offsets relative to font_data
	font->ft_data = (ubyte *)((size_t)font->ft_data - GRS_FONT_SIZE);
	font->ft_widths = (short *)((size_t)font->ft_widths - GRS_FONT_SIZE);
	font->ft_kerndata = (ubyte *)((size_t)font->ft_kerndata - GRS_FONT_SIZE);

	nchars = font->ft_maxchar - font->ft_minchar + 1;

	if (font->ft_flags & FT_PROPORTIONAL) {

		font->ft_widths = (short *) &font_data[(size_t)font->ft_widths];
		font->ft_data = (unsigned char *) &font_data[(size_t)font->ft_data];
		font->ft_chars = (unsigned char **)d_malloc( nchars * sizeof(unsigned char *));

		ptr = font->ft_data;

		for (i=0; i< nchars; i++ ) {
			font->ft_widths[i] = INTEL_SHORT(font->ft_widths[i]);
			font->ft_chars[i] = ptr;
			if (font->ft_flags & FT_COLOR)
				ptr += font->ft_widths[i] * font->ft_h;
			else
				ptr += BITS_TO_BYTES(font->ft_widths[i]) * font->ft_h;
		}

	} else  {

		font->ft_data   = (unsigned char *) font_data;
		font->ft_chars  = NULL;
		font->ft_widths = NULL;

		ptr = font->ft_data + (nchars * font->ft_w * font->ft_h);
	}

	if (font->ft_flags & FT_KERNED)
		font->ft_kerndata = (unsigned char *) &font_data[(size_t)font->ft_kerndata];

	if (font->ft_flags & FT_COLOR) {		//remap palette
		ubyte palette[256*3];
		ubyte colormap[256];
		int freq[256];

		cfread(palette,3,256,fontfile);		//read the palette

		build_colormap_good( (ubyte *)&palette, colormap, freq );

		colormap[TRANSPARENCY_COLOR] = TRANSPARENCY_COLOR;              // changed from colormap[255] = 255 to this for macintosh

		decode_data(font->ft_data, ptr - font->ft_data, colormap, freq );

	}

	cfclose(fontfile);

	if (font->ft_bitmaps)
		d_free( font->ft_bitmaps );
	gr_free_bitmap_data(&font->ft_parent_bitmap);

	ogl_init_font(font);
}

void gr_set_curfont( grs_font * new )
{
	grd_curcanv->cv_font = new;
}

void gr_set_fontcolor( int fg_color, int bg_color )
{
	grd_curcanv->cv_font_fg_color    = fg_color;
	grd_curcanv->cv_font_bg_color    = bg_color;
}
