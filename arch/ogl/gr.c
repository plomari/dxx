/*
 *
 * OGL video functions. - Added 9/15/99 Matthew Mueller
 *
 */

#define DECLARE_VARS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#ifdef _MSC_VER
#include <windows.h>
#endif

#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#if !defined(macintosh)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <errno.h>
#include <SDL.h>
#include "hudmsg.h"
#include "game.h"
#include "text.h"
#include "gr.h"
#include "gamefont.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "inferno.h"
#include "screens.h"
#include "strutil.h"
#include "args.h"
#include "key.h"
#include "physfsx.h"
#include "internal.h"
#include "render.h"
#include "console.h"
#include "config.h"
#include "playsave.h"
#include "vers_id.h"
#include "game.h"

int gr_installed = 0;
int gl_initialized=0;
int ogl_fullscreen;
int gr_focus_lost;
static int curx=-1,cury=-1,curfull=0;
int linedotscale=1; // scalar of glLinewidth and glPointSize - only calculated once when resolution changes
static SDL_Window *sdl_window;
static bool user_grab;

static void ogl_get_verinfo(void);

static void gr_update_grab(void)
{
	if (sdl_window)
		SDL_SetWindowGrab(sdl_window, ogl_fullscreen && user_grab);
}

void ogl_swap_buffers_internal(void)
{
	if (sdl_window)
		SDL_GL_SwapWindow(sdl_window);
}

void gr_sdl_ogl_resize_window(int w, int h)
{
	if (curx == w && cury == h)
		return;

	curx=w;cury=h;

	if (!gl_initialized)
		return;

	linedotscale = ((w/640<h/480?w/640:h/480)<1?1:(w/640<h/480?w/640:h/480));

	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = F1_0;
	grd_curscreen->sc_canvas.cv_x = 0;
	grd_curscreen->sc_canvas.cv_y = 0;
	grd_curscreen->sc_canvas.cv_w = w;
	grd_curscreen->sc_canvas.cv_h = h;
	gr_set_current_canvas(NULL);

	/* select clearing (background) color   */
	glClearColor(0.0, 0.0, 0.0, 0.0);

	/* initialize viewing values */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();//clear matrix
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gr_palette_step_up(0,0,0);//in case its left over from in game

	gamefont_update_screen_size(w,h);
	gr_remap_color_fonts();
	gr_remap_mono_fonts();

	OGL_VIEWPORT(0,0,w,h);

	if (!gr_check_fullscreen()) {
		GameCfg.ResolutionX = w;
		GameCfg.ResolutionY = h;
	}
}

int ogl_init_window(int x, int y)
{
	if (gl_initialized && x==curx && y==cury && curfull==ogl_fullscreen)
		return 0;

	int fs_flags = ogl_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

	if (sdl_window) {
		SDL_SetWindowSize(sdl_window, x, y);
		SDL_SetWindowFullscreen(sdl_window, fs_flags);
	} else {
		sdl_window = SDL_CreateWindow(DESCENT_VERSION,
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             x, y,
                             SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | fs_flags);
		// (let the context leak)
		SDL_GL_CreateContext(sdl_window);
		ogl_get_verinfo();

		// Ignore system keys etc. if windows is grabbed.
		SDL_SetHint(SDL_HINT_GRAB_KEYBOARD, "1");

		// This is ridiculous. I think it's to combat OpenGL/WM integration
		// issues on shit OSes (Microsoft Windows). But on sane OSes, this just
		// makes things awkward.
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

		SDL_SetWindowIcon(sdl_window, SDL_LoadBMP("d2x-rebirth.bmp"));
	}

	gr_update_grab();

	SDL_ShowCursor(0);

	gl_initialized = 1;

	curfull = ogl_fullscreen;
	GameCfg.WindowMode = !ogl_fullscreen;

	SDL_GetWindowSize(sdl_window, &x, &y);
	gr_sdl_ogl_resize_window(x, y);

	return 0;
}

void gr_create_window(void)
{
	if (!gl_initialized)
		ogl_init_window(GameCfg.ResolutionX, GameCfg.ResolutionY);
}

void gr_set_input_grab(bool val)
{
	if (user_grab != val) {
		user_grab = val;
		gr_update_grab();
	}
}

int gr_check_fullscreen(void)
{
	return ogl_fullscreen;
}

void gr_do_fullscreen(int f)
{
	ogl_fullscreen=f;
	if (gl_initialized)
	{
		ogl_init_window(curx,cury);
	}
}

int gr_toggle_fullscreen(void)
{
	gr_do_fullscreen(!ogl_fullscreen);

	return ogl_fullscreen;
}

// Set the buffer to draw to. 0 is front, 1 is back
void gr_set_draw_buffer(int buf)
{
	glDrawBuffer((buf == 0) ? GL_FRONT : GL_BACK);
}

const char *gl_vendor, *gl_renderer, *gl_version, *gl_extensions;

static void ogl_get_verinfo(void)
{
	gl_vendor = (const char *) glGetString (GL_VENDOR);
	gl_renderer = (const char *) glGetString (GL_RENDERER);
	gl_version = (const char *) glGetString (GL_VERSION);
	gl_extensions = (const char *) glGetString (GL_EXTENSIONS);

	if (!gl_vendor) {
		fprintf(stderr, "GL_VENDOR is NULL, no GL initialized!\n");
		abort();
	}

	con_printf(CON_VERBOSE, "OpenGL: vendor: %s\nOpenGL: renderer: %s\nOpenGL: version: %s\n",gl_vendor,gl_renderer,gl_version);
	if (!stricmp(gl_extensions,"GL_EXT_texture_filter_anisotropic")==0)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &ogl_maxanisotropy);
		con_printf(CON_VERBOSE,"ogl_maxanisotropy:%f\n",ogl_maxanisotropy);
	}
	else if (GameCfg.TexFilt >= 3)
		GameCfg.TexFilt = 2;
}

#define GLstrcmptestr(a,b) if (stricmp(a,#b)==0 || stricmp(a,"GL_" #b)==0)return GL_ ## b;
int ogl_atotexfilti(char *a,int min)
{
	GLstrcmptestr(a,NEAREST);
	GLstrcmptestr(a,LINEAR);
	if (min)
	{//mipmaps are valid only for the min filter
		GLstrcmptestr(a,NEAREST_MIPMAP_NEAREST);
		GLstrcmptestr(a,NEAREST_MIPMAP_LINEAR);
		GLstrcmptestr(a,LINEAR_MIPMAP_NEAREST);
		GLstrcmptestr(a,LINEAR_MIPMAP_LINEAR);
	}
	Error("unknown/invalid texture filter %s\n",a);
}

void gr_set_attributes(void)
{
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
	SDL_GL_SetSwapInterval(GameCfg.VSync);
	if (GameCfg.Multisample)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	}
	ogl_smash_texture_list_internal();
	gr_remap_color_fonts();
	gr_remap_mono_fonts();
}

int gr_init(int mode)
{
 	// Only do this function once!
	if (gr_installed==1)
		return -1;

	if (!GameCfg.WindowMode && !GameArg.SysWindow)
 		gr_toggle_fullscreen();

	gr_set_attributes();

	ogl_init_texture_list_internal();

	MALLOC( grd_curscreen,grs_screen,1 );
	memset( grd_curscreen, 0, sizeof(grs_screen));

	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_fade_level = GR_FADE_OFF;
	grd_curscreen->sc_canvas.cv_blend_func = GR_BLEND_NORMAL;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	gr_installed = 1;

	return 0;
}

void gr_close()
{
	ogl_brightness_r = ogl_brightness_g = ogl_brightness_b = 0;

	if (gl_initialized)
	{
		ogl_smash_texture_list_internal();
		SDL_ShowCursor(1);
	}

	if (grd_curscreen)
	{
		d_free(grd_curscreen);
	}
}

extern int r_upixelc;
void ogl_upixelc(int x, int y, int c)
{
	GLfloat vertex_array[] = { (x+grd_curcanv->cv_x)/(float)last_width, 1.0-(y+grd_curcanv->cv_y)/(float)last_height };
	GLfloat color_array[] = { CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0, CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0, CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0, CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0 };

	r_upixelc++;
	OGL_DISABLE(TEXTURE_2D);
	glPointSize(linedotscale);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_POINTS, 0, 1);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void ogl_urect(int left,int top,int right,int bot)
{
	GLfloat xo, yo, xf, yf, color_r, color_g, color_b, color_a;
	GLfloat color_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	GLfloat vertex_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	int c=COLOR;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	xo=(left+grd_curcanv->cv_x)/(float)last_width;
	xf = (right + 1 + grd_curcanv->cv_x) / (float)last_width;
	yo=1.0-(top+grd_curcanv->cv_y)/(float)last_height;
	yf = 1.0 - (bot + 1 + grd_curcanv->cv_y) / (float)last_height;

	OGL_DISABLE(TEXTURE_2D);

	color_r = CPAL2Tr(c);
	color_g = CPAL2Tg(c);
	color_b = CPAL2Tb(c);

	if (grd_curcanv->cv_fade_level >= GR_FADE_OFF)
		color_a = 1.0;
	else
		color_a = 1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0);

	color_array[0] = color_array[4] = color_array[8] = color_array[12] = color_r;
	color_array[1] = color_array[5] = color_array[9] = color_array[13] = color_g;
	color_array[2] = color_array[6] = color_array[10] = color_array[14] = color_b;
	color_array[3] = color_array[7] = color_array[11] = color_array[15] = color_a;

	vertex_array[0] = xo;
	vertex_array[1] = yo;
	vertex_array[2] = xo;
	vertex_array[3] = yf;
	vertex_array[4] = xf;
	vertex_array[5] = yf;
	vertex_array[6] = xf;
	vertex_array[7] = yo;
	
	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);//replaced GL_QUADS
	glDisableClientState(GL_VERTEX_ARRAY);
}

void ogl_ulinec(int left,int top,int right,int bot,int c)
{
	GLfloat xo,yo,xf,yf;
	GLfloat color_array[] = { CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0), CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0), CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), 1.0, CPAL2Tr(c), CPAL2Tg(c), CPAL2Tb(c), (grd_curcanv->cv_fade_level >= GR_FADE_OFF)?1.0:1.0 - (float)grd_curcanv->cv_fade_level / ((float)GR_FADE_LEVELS - 1.0) };
	GLfloat vertex_array[] = { 0.0, 0.0, 0.0, 0.0 };

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	xo = (left + grd_curcanv->cv_x) / (float)last_width;
	xf = (right + grd_curcanv->cv_x ) / (float)last_width;
	yo = 1.0 - (top + grd_curcanv->cv_y + 0.5) / (float)last_height;
	yf = 1.0 - (bot + grd_curcanv->cv_y + 0.5) / (float)last_height;
 
	OGL_DISABLE(TEXTURE_2D);

	vertex_array[0] = xo;
	vertex_array[1] = yo;
	vertex_array[2] = xf;
	vertex_array[3] = yf;

	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_LINES, 0, 2);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}
	

GLfloat last_r=0, last_g=0, last_b=0;
int do_pal_step=0;

void ogl_do_palfx(void)
{
	GLfloat color_array[] = { last_r, last_g, last_b, 1.0, last_r, last_g, last_b, 1.0, last_r, last_g, last_b, 1.0, last_r, last_g, last_b, 1.0 };
	GLfloat vertex_array[] = { 0,0,0,1,1,1,1,0 };

	OGL_DISABLE(TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
 
	if (do_pal_step)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE,GL_ONE);
	}
	else
		return;
 
	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);//replaced GL_QUADS
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int ogl_brightness_ok = 0;
int ogl_brightness_r = 0, ogl_brightness_g = 0, ogl_brightness_b = 0;
static int old_b_r = 0, old_b_g = 0, old_b_b = 0;

void gr_palette_step_up(int r, int g, int b)
{
	old_b_r = ogl_brightness_r;
	old_b_g = ogl_brightness_g;
	old_b_b = ogl_brightness_b;

	ogl_brightness_r = max(r + gr_palette_gamma, 0);
	ogl_brightness_g = max(g + gr_palette_gamma, 0);
	ogl_brightness_b = max(b + gr_palette_gamma, 0);

	if (!ogl_brightness_ok)
	{
		last_r = ogl_brightness_r / 63.0;
		last_g = ogl_brightness_g / 63.0;
		last_b = ogl_brightness_b / 63.0;

		do_pal_step = (r || g || b || gr_palette_gamma);
	}
	else
	{
		do_pal_step = 0;
	}
}

#undef min
static inline int min(int x, int y) { return x < y ? x : y; }

void gr_palette_load( ubyte *pal )
{
	int i;

	for (i=0; i<768; i++ )
	{
		gr_current_pal[i] = pal[i];
		if (gr_current_pal[i] > 63)
			gr_current_pal[i] = 63;
	}

	gr_palette_step_up(0, 0, 0); // make ogl_setbrightness_internal get run so that menus get brightened too.
	init_computed_colors();
}

void gr_palette_read(ubyte * pal)
{
	int i;
	for (i=0; i<768; i++ )
	{
		pal[i]=gr_current_pal[i];
		if (pal[i] > 63)
			pal[i] = 63;
	}
}

#define GL_BGR_EXT 0x80E0

typedef struct
{
      unsigned char TGAheader[12];
      unsigned char header[6];
} TGA_header;

//writes out an uncompressed RGB .tga file
//if we got really spiffy, we could optionally link in libpng or something, and use that.
void write_bmp(char *savename,int w,int h,unsigned char *buf)
{
	PHYSFS_file* TGAFile;
	TGA_header TGA;
	GLbyte HeightH,HeightL,WidthH,WidthL;

	buf = (unsigned char*)calloc(w*h*3,sizeof(unsigned char));

	glReadPixels(0,0,w,h,GL_BGR_EXT,GL_UNSIGNED_BYTE,buf);

	if (!(TGAFile = PHYSFSX_openWriteBuffered(savename)))
	{
		con_printf(CON_URGENT,"Could not create TGA file to dump screenshot!");
		d_free(buf);
		return;
	}

	HeightH = (GLbyte)(h / 256);
	HeightL = (GLbyte)(h % 256);
	WidthH  = (GLbyte)(w / 256);
	WidthL  = (GLbyte)(w % 256);
	// Write TGA Header
	TGA.TGAheader[0] = 0;
	TGA.TGAheader[1] = 0;
	TGA.TGAheader[2] = 2;
	TGA.TGAheader[3] = 0;
	TGA.TGAheader[4] = 0;
	TGA.TGAheader[5] = 0;
	TGA.TGAheader[6] = 0;
	TGA.TGAheader[7] = 0;
	TGA.TGAheader[8] = 0;
	TGA.TGAheader[9] = 0;
	TGA.TGAheader[10] = 0;
	TGA.TGAheader[11] = 0;
	TGA.header[0] = (GLbyte) WidthL;
	TGA.header[1] = (GLbyte) WidthH;
	TGA.header[2] = (GLbyte) HeightL;
	TGA.header[3] = (GLbyte) HeightH;
	TGA.header[4] = (GLbyte) 24;
	TGA.header[5] = 0;
	PHYSFS_write(TGAFile,&TGA,sizeof(TGA_header),1);
	PHYSFS_write(TGAFile,buf,w*h*3*sizeof(unsigned char),1);
	PHYSFS_close(TGAFile);
	free(buf);
}

void save_screen_shot(int automap_flag)
{
	char message[100];
	static int savenum=0;
	char savename[13+sizeof(SCRNS_DIR)];
	unsigned char *buf;

	stop_time();

	if (!cfexist(SCRNS_DIR))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	do
	{
		sprintf(savename, "%sscrn%04d.tga",SCRNS_DIR, savenum++);
	} while (PHYSFS_exists(savename));

	sprintf( message, "%s 'scrn%04d.tga'", TXT_DUMPING_SCREEN, savenum-1 );

	if (!automap_flag)
		HUD_init_message(HM_DEFAULT, message);

	glReadBuffer(GL_FRONT);

	buf = d_malloc(grd_curscreen->sc_w*grd_curscreen->sc_h*3);
	write_bmp(savename,grd_curscreen->sc_w,grd_curscreen->sc_h,buf);
	d_free(buf);

	start_time();
}
