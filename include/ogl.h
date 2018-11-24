#ifndef OGL_H_
#define OGL_H_

#include <stddef.h>

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#define GL_GLEXT_PROTOTYPES
#  ifdef OGLES
#  include <GLES/gl.h>
#  else
#  include <GL/gl.h>
#  endif
#endif

#include "gr.h"
#include "palette.h"
#include "pstypes.h"

/* we need to export ogl_texture for 2d/font.c */
typedef struct _ogl_texture {
	GLuint handle;
	GLint internalformat;
	GLenum format;
	int w,h,tw,th,lw;
	int bytesu;
	int bytes;
	GLfloat u,v;
	GLfloat prio;
	int wrapstate;
	char wantmip;
} ogl_texture;

extern ogl_texture* ogl_get_free_texture();
void ogl_init_texture(ogl_texture* t, int w, int h);

extern int ogl_rgba_internalformat;
extern int ogl_rgb_internalformat;

void ogl_init_shared_palette(void);

extern int gl_initialized;

extern GLfloat ogl_maxanisotropy;

void ogl_loadbmtexture(grs_bitmap *bm);
void ogl_freebmtexture(grs_bitmap *bm);

void ogl_prepare_state_3d(void);
void ogl_prepare_state_2d(void);
void ogl_swap_buffers_internal(void);
void ogl_cache_level_textures(void);

void ogl_urect(int left, int top, int right, int bot);
bool ogl_ubitmapm_cs(int x, int y,int dw, int dh, grs_bitmap *bm,int c, int scale);
void ogl_ulinec(int left, int top, int right, int bot, int c);

bool ogl_ubitmapm_3d(vms_vector *p, vms_vector *dx, vms_vector *dy, grs_bitmap *bm,int c);

#include "3d.h"
bool g3_draw_tmap_2(int nv,const g3s_point **pointlist,g3s_uvl *uvl_list,g3s_lrgb *light_rgb, grs_bitmap *bmbot,grs_bitmap *bm, int orient);

void ogl_draw_vertex_reticle(int cross,int primary,int secondary,int color,int alpha,int size_offs);
void ogl_toggle_depth_test(int enable);
void ogl_set_blending();

/* I assume this ought to be >= MAX_BITMAP_FILES in piggy.h? */
#define OGL_TEXTURE_LIST_SIZE 20000

extern ogl_texture ogl_texture_list[OGL_TEXTURE_LIST_SIZE];

void ogl_init_texture_list_internal(void);
void ogl_smash_texture_list_internal(void);
void ogl_vivify_texture_list_internal(void);

extern int ogl_brightness_ok;
extern int ogl_brightness_r, ogl_brightness_g, ogl_brightness_b;
extern int ogl_fullscreen;

extern int GL_TEXTURE_2D_enabled;
#define OGL_ENABLE2(a,f) {if (a ## _enabled!=1) {f;a ## _enabled=1;}}
#define OGL_DISABLE2(a,f) {if (a ## _enabled!=0) {f;a ## _enabled=0;}}

#define OGL_ENABLE(a) OGL_ENABLE2(GL_ ## a,glEnable(GL_ ## a))
#define OGL_DISABLE(a) OGL_DISABLE2(GL_ ## a,glDisable(GL_ ## a))

void ogl_swap_buffers_internal(void);

extern unsigned char *ogl_pal;

#define CPAL2Tr(c) ((gr_current_pal[c*3])/63.0)
#define CPAL2Tg(c) ((gr_current_pal[c*3+1])/63.0)
#define CPAL2Tb(c) ((gr_current_pal[c*3+2])/63.0)
#define PAL2Tr(c) ((ogl_pal[c*3])/63.0)
#define PAL2Tg(c) ((ogl_pal[c*3+1])/63.0)
#define PAL2Tb(c) ((ogl_pal[c*3+2])/63.0)

#endif
