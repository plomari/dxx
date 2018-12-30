/*
 *
 * Graphics support functions for OpenGL.
 *
 */

//#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif
#include "internal.h"
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "3d.h"
#include "piggy.h"
#include "../../3d/globvars.h"
#include "error.h"
#include "texmap.h"
#include "palette.h"
#include "rle.h"
#include "console.h"
#include "u_mem.h"
#ifdef HAVE_LIBPNG
#include "pngfile.h"
#endif

#include "segment.h"
#include "textures.h"
#include "texmerge.h"
#include "effects.h"
#include "weapon.h"
#include "powerup.h"
#include "laser.h"
#include "player.h"
#include "polyobj.h"
#include "gamefont.h"
#include "byteswap.h"
#include "internal.h"
#include "gauges.h"
#include "playsave.h"

//change to 1 for lots of spew.
#if 0
#define glmprintf(0,a) con_printf(CON_DEBUG, a)
#else
#define glmprintf(a)
#endif

#ifndef M_PI
#define M_PI 3.14159
#endif

#if defined(_WIN32) || (defined(__APPLE__) && defined(__MACH__)) || defined(__sun__) || defined(macintosh)
#define cosf(a) cos(a)
#define sinf(a) sin(a)
#endif

#define BRIGHT_G        0.125, 1.0, 0.125, 1.0
#define DARK_G          0.125, 0.54, 0.125, 0.6

unsigned char *ogl_pal=gr_palette;

int last_width=-1,last_height=-1;
int GL_TEXTURE_2D_enabled=-1;
int GL_texclamp_enabled=-1;

int r_texcount = 0, r_cachedtexcount = 0;
GLfloat dark_lca[16 * 4] = {
        DARK_G, DARK_G, DARK_G, DARK_G,
        DARK_G, DARK_G, DARK_G, DARK_G,
        DARK_G, DARK_G, DARK_G, DARK_G,
        DARK_G, DARK_G, DARK_G, DARK_G
};
GLfloat bright_lca[16 * 4] = {
        BRIGHT_G, BRIGHT_G, BRIGHT_G, BRIGHT_G,
        BRIGHT_G, BRIGHT_G, BRIGHT_G, BRIGHT_G,
        BRIGHT_G, BRIGHT_G, BRIGHT_G, BRIGHT_G,
        BRIGHT_G, BRIGHT_G, BRIGHT_G, BRIGHT_G
};
GLfloat *sphere_va = NULL;
GLfloat cross_lca[8 * 4] = {
	DARK_G, BRIGHT_G,
	DARK_G, BRIGHT_G,
	DARK_G, BRIGHT_G,
	DARK_G, BRIGHT_G
};
GLfloat cross_lva[8 * 2] = {
	-4.0, 2.0, -2.0, 0,
	-3.0, -4.0, -2.0, -3.0,
	4.0, 2.0, 2.0, 0,
	3.0, -4.0, 2.0, -3.0,
};
GLfloat primary_lca[2][4 * 4] = {
	{BRIGHT_G, BRIGHT_G, DARK_G, DARK_G},
	{DARK_G, DARK_G, BRIGHT_G, BRIGHT_G}
};
GLfloat primary_lva[4][4 * 2] = {
	{
		-5.5, -5.0,
		-6.5, -7.5,
		-10.0, -7.0,
		-10.0, -8.7
	},
	{
		-10.0, -7.0,
		-10.0, -8.7,
		-15.0, -8.5,
		-15.0, -9.5
	},
	{
		5.5, -5.0,
		6.5, -7.5,
		10.0, -7.0,
		10.0, -8.7
	},
	{
		10.0, -7.0,
		10.0, -8.7,
		15.0, -8.5,
		15.0, -9.5
	}
};
GLfloat *secondary_lva[3]={NULL, NULL, NULL};
int r_polyc,r_tpolyc,r_bitmapc,r_ubitbltc,r_upixelc;
extern int linedotscale;
#define f2glf(x) (f2fl(x))

#define OGL_BINDTEXTURE(a) glBindTexture(GL_TEXTURE_2D, a);


ogl_texture ogl_texture_list[OGL_TEXTURE_LIST_SIZE];
int ogl_texture_list_cur;

/* some function prototypes */

extern GLubyte *pixels;
extern GLubyte *texbuf;
void ogl_filltexbuf(unsigned char *data, GLubyte *texp, int truewidth, int width, int height, int dxo, int dyo, int twidth, int theight, int type, int bm_flags, int data_format);
void ogl_loadbmtexture(grs_bitmap *bm);
static int ogl_loadtexture(unsigned char *data, int dxo, int dyo, ogl_texture *tex, int bm_flags, int data_format, grs_bitmap *bm);
void ogl_freetexture(ogl_texture *gltexture);
void ogl_do_palfx(void);

// Replacement for gluPerspective
void perspective(double fovy, double aspect, double zNear, double zFar)
{
	double xmin, xmax, ymin, ymax;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
	glMatrixMode(GL_MODELVIEW);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	
	glDepthMask(GL_TRUE);
}

void ogl_init_texture(ogl_texture* t, int w, int h, int flags)
{
	t->handle = 0;
	t->internalformat = GL_RGBA8;
	t->format = GL_RGBA;
	t->wrapstate = -1;
	t->lw = t->w = w;
	t->h = h;
	t->wantmip = flags & OGL_FLAG_MIPMAP;
}

void ogl_reset_texture(ogl_texture* t)
{
	ogl_init_texture(t, 0, 0, 0);
}

void ogl_init_texture_list_internal(void){
	int i;
	ogl_texture_list_cur=0;
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++)
		ogl_reset_texture(&ogl_texture_list[i]);
}

void ogl_smash_texture_list_internal(void){
	int i;
	if (sphere_va != NULL)
	{
		d_free(sphere_va);
		sphere_va = NULL;
	}
	for(i = 0; i < 3; i++) {
		if (secondary_lva[i] != NULL)
		{
			d_free(secondary_lva[i]);
			secondary_lva[i] = NULL;
		}
	}
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++){
		if (ogl_texture_list[i].handle>0){
			glDeleteTextures( 1, &ogl_texture_list[i].handle );
			ogl_texture_list[i].handle=0;
		}
		ogl_texture_list[i].wrapstate = -1;
	}
}

ogl_texture* ogl_get_free_texture(void){
	int i;
	for (i=0;i<OGL_TEXTURE_LIST_SIZE;i++){
		if (ogl_texture_list[ogl_texture_list_cur].handle<=0 && ogl_texture_list[ogl_texture_list_cur].w==0)
			return &ogl_texture_list[ogl_texture_list_cur];
		if (++ogl_texture_list_cur>=OGL_TEXTURE_LIST_SIZE)
			ogl_texture_list_cur=0;
	}
	Error("OGL: texture list full!\n");
}

void ogl_bindbmtex(grs_bitmap *bm){
	if (bm->gltexture==NULL || bm->gltexture->handle<=0)
		ogl_loadbmtexture(bm);
	OGL_BINDTEXTURE(bm->gltexture->handle);
}

//gltexture MUST be bound first
void ogl_texwrap(ogl_texture *gltexture,int state)
{
	if (gltexture->wrapstate != state)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state);
		gltexture->wrapstate = state;
	}
}

//crude texture precaching
//handles: powerups, walls, weapons, polymodels, etc.
//it is done with the horrid do_special_effects kludge so that sides that have to be texmerged and have animated textures will be correctly cached.
//similarly, with the objects(esp weapons), we could just go through and cache em all instead, but that would get ones that might not even be on the level
//TODO: doors

void ogl_cache_polymodel_textures(int model_num)
{
	polymodel *po;
	int i;

	if (model_num < 0)
		return;
	po = &Polygon_models[model_num];
	for (i=0;i<po->n_textures;i++)  {
		ogl_loadbmtexture(&GameBitmaps[ObjBitmaps[ObjBitmapPtrs[po->first_texture+i]].index]);
	}
}

void ogl_cache_vclip_textures(vclip *vc){
	int i;
	for (i=0;i<vc->num_frames;i++){
		PIGGY_PAGE_IN(vc->frames[i]);
		ogl_loadbmtexture(&GameBitmaps[vc->frames[i].index]);
	}
}

void ogl_cache_vclipn_textures(int i)
{
	if (i >= 0 && i < VCLIP_MAXNUM)
		ogl_cache_vclip_textures(&Vclip[i]);
}

void ogl_cache_weapon_textures(int weapon_type)
{
	weapon_info *w;

	if (weapon_type < 0)
		return;
	w = &Weapon_info[weapon_type];
	ogl_cache_vclipn_textures(w->flash_vclip);
	ogl_cache_vclipn_textures(w->robot_hit_vclip);
	ogl_cache_vclipn_textures(w->wall_hit_vclip);
	if (w->render_type==WEAPON_RENDER_VCLIP)
		ogl_cache_vclipn_textures(w->weapon_vclip);
	else if (w->render_type == WEAPON_RENDER_POLYMODEL)
	{
		ogl_cache_polymodel_textures(w->model_num);
		ogl_cache_polymodel_textures(w->model_num_inner);
	}
}

void ogl_cache_level_textures(void)
{
	int seg,side,i;
	eclip *ec;
	short tmap1,tmap2;
	grs_bitmap *bm,*bm2;
	struct side *sidep;
	int max_efx=0,ef;
	
	for (i=0,ec=Effects;i<Num_effects;i++,ec++) {
		ogl_cache_vclipn_textures(Effects[i].dest_vclip);
		if ((Effects[i].changing_wall_texture == -1) && (Effects[i].changing_object_texture==-1) )
			continue;
		if (ec->vc.num_frames>max_efx)
			max_efx=ec->vc.num_frames;
	}
	glmprintf((0,"max_efx:%i\n",max_efx));
	for (ef=0;ef<max_efx;ef++){
		for (i=0,ec=Effects;i<Num_effects;i++,ec++) {
			if ((Effects[i].changing_wall_texture == -1) && (Effects[i].changing_object_texture==-1) )
				continue;
			ec->time_left=-1;
		}
		do_special_effects();

		for (seg=0;seg<Num_segments;seg++){
			for (side=0;side<MAX_SIDES_PER_SEGMENT;side++){
				sidep=&Segments[seg].sides[side];
				tmap1=sidep->tmap_num;
				tmap2=sidep->tmap_num2;
				if (tmap1<0 || tmap1>=NumTextures){
					glmprintf((0,"ogl_cache_level_textures %i %i %i %i\n",seg,side,tmap1,NumTextures));
					//				tmap1=0;
					continue;
				}
				PIGGY_PAGE_IN(Textures[tmap1]);
				bm = &GameBitmaps[Textures[tmap1].index];
				if (tmap2 != 0){
					PIGGY_PAGE_IN(Textures[tmap2&0x3FFF]);
					bm2 = &GameBitmaps[Textures[tmap2&0x3FFF].index];
					if (GameArg.DbgAltTexMerge == 0 || (bm2->bm_flags & BM_FLAG_SUPER_TRANSPARENT))
						bm = texmerge_get_cached_bitmap( tmap1, tmap2 );
					else {
						ogl_loadbmtexture(bm2);
					}
				}
				ogl_loadbmtexture(bm);
			}
		}
		glmprintf((0,"finished ef:%i\n",ef));
	}
	reset_special_effects();
	init_special_effects();
	{
		// always have lasers, concs, flares.  Always shows player appearance, and at least concs are always available to disappear.
		ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[LASER_INDEX]);
		ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[CONCUSSION_INDEX]);
		ogl_cache_weapon_textures(FLARE_ID);
		ogl_cache_vclipn_textures(VCLIP_PLAYER_APPEARANCE);
		ogl_cache_vclipn_textures(VCLIP_POWERUP_DISAPPEARANCE);
		ogl_cache_polymodel_textures(Player_ship->model_num);
		ogl_cache_vclipn_textures(Player_ship->expl_vclip_num);

		for (i=0;i<Highest_object_index;i++){
			if(Objects[i].render_type==RT_POWERUP){
				ogl_cache_vclipn_textures(Objects[i].rtype.vclip_info.vclip_num);
				switch (Objects[i].id){
					case POW_VULCAN_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[VULCAN_INDEX]);
						break;
					case POW_SPREADFIRE_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[SPREADFIRE_INDEX]);
						break;
					case POW_PLASMA_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[PLASMA_INDEX]);
						break;
					case POW_FUSION_WEAPON:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[FUSION_INDEX]);
						break;
					case POW_PROXIMITY_WEAPON:
						ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[PROXIMITY_INDEX]);
						break;
					case POW_HOMING_AMMO_1:
					case POW_HOMING_AMMO_4:
						ogl_cache_weapon_textures(Primary_weapon_to_weapon_info[HOMING_INDEX]);
						break;
					case POW_SMARTBOMB_WEAPON:
						ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[SMART_INDEX]);
						break;
					case POW_MEGA_WEAPON:
						ogl_cache_weapon_textures(Secondary_weapon_to_weapon_info[MEGA_INDEX]);
						break;
				}
			}
			else if(Objects[i].render_type==RT_POLYOBJ){
				if (Objects[i].type == OBJ_ROBOT)
				{
					ogl_cache_vclipn_textures(Robot_info[Objects[i].id].exp1_vclip_num);
					ogl_cache_vclipn_textures(Robot_info[Objects[i].id].exp2_vclip_num);
					ogl_cache_weapon_textures(Robot_info[Objects[i].id].weapon_type);
				}
				if (Objects[i].rtype.pobj_info.tmap_override != -1)
					ogl_loadbmtexture(&GameBitmaps[Textures[Objects[i].rtype.pobj_info.tmap_override].index]);
				else
					ogl_cache_polymodel_textures(Objects[i].rtype.pobj_info.model_num);
			}
		}
	}
	glmprintf((0,"finished caching\n"));
	r_cachedtexcount = r_texcount;
}

bool g3_draw_line(g3s_point *p0,g3s_point *p1)
{
	int c;
	GLfloat color_r, color_g, color_b;
	GLfloat color_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	GLfloat vertex_array[] = { f2glf(p0->p3_vec.x),f2glf(p0->p3_vec.y),-f2glf(p0->p3_vec.z), f2glf(p1->p3_vec.x),f2glf(p1->p3_vec.y),-f2glf(p1->p3_vec.z) };
  
	c=grd_curcanv->cv_color;
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	OGL_DISABLE(TEXTURE_2D);
	color_r = PAL2Tr(c);
	color_g = PAL2Tg(c);
	color_b = PAL2Tb(c);
	color_array[0] = color_array[4] = color_r;
	color_array[1] = color_array[5] = color_g;
	color_array[2] = color_array[6] = color_b;
	color_array[3] = color_array[7] = 1.0;
	glVertexPointer(3, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_LINES, 0, 2);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	return 1;
}

void ogl_drawcircle(int nsides, int type, GLfloat *vertex_array)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glDrawArrays(type, 0, nsides);
	glDisableClientState(GL_VERTEX_ARRAY);
}

GLfloat *circle_array_init(int nsides)
{
	int i;
	float ang;
	GLfloat *vertex_array = (GLfloat *) d_malloc(sizeof(GLfloat) * nsides * 2);
	
	for(i = 0; i < nsides; i++) {
		ang = 2.0 * M_PI * i / nsides;
		vertex_array[i * 2] = cosf(ang);
        	vertex_array[i * 2 + 1] = sinf(ang);
	}
	
	return vertex_array;
}
 
GLfloat *circle_array_init_2(int nsides, float xsc, float xo, float ysc, float yo)
{
 	int i;
 	float ang;
	GLfloat *vertex_array = (GLfloat *) d_malloc(sizeof(GLfloat) * nsides * 2);
	
	for(i = 0; i < nsides; i++) {
		ang = 2.0 * M_PI * i / nsides;
		vertex_array[i * 2] = cosf(ang) * xsc + xo;
		vertex_array[i * 2 + 1] = sinf(ang) * ysc + yo;
	}
	
	return vertex_array;
}

void ogl_draw_reticle(int cross,int primary,int secondary)
{
	fix size=270;
	float scale = ((float)SWIDTH/SHEIGHT);

	glPushMatrix();
	glTranslatef((grd_curcanv->cv_bitmap.bm_w/2+grd_curcanv->cv_bitmap.bm_x)/(float)last_width,1.0-(grd_curcanv->cv_bitmap.bm_h/2+grd_curcanv->cv_bitmap.bm_y)/(float)last_height,0);

	if (scale >= 1)
	{
		size/=scale;
		glScalef(f2glf(size),f2glf(size*scale),f2glf(size));
	}
	else
	{
		size*=scale;
		glScalef(f2glf(size/scale),f2glf(size),f2glf(size));
	}

	glLineWidth(linedotscale*2);
	OGL_DISABLE(TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	//cross
	if(cross)
		glColorPointer(4, GL_FLOAT, 0, cross_lca);
	else
		glColorPointer(4, GL_FLOAT, 0, dark_lca);
	glVertexPointer(2, GL_FLOAT, 0, cross_lva);
	glDrawArrays(GL_LINES, 0, 8);
	
	//left primary bar
	if(primary == 0)
		glColorPointer(4, GL_FLOAT, 0, dark_lca);
	else
		glColorPointer(4, GL_FLOAT, 0, primary_lca[0]);
	glVertexPointer(2, GL_FLOAT, 0, primary_lva[0]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	if(primary != 2)
		glColorPointer(4, GL_FLOAT, 0, dark_lca);
	else
		glColorPointer(4, GL_FLOAT, 0, primary_lca[1]);
	glVertexPointer(2, GL_FLOAT, 0, primary_lva[1]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	//right primary bar
	if(primary == 0)
		glColorPointer(4, GL_FLOAT, 0, dark_lca);
	else
		glColorPointer(4, GL_FLOAT, 0, primary_lca[0]);
	glVertexPointer(2, GL_FLOAT, 0, primary_lva[2]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	if(primary != 2)
		glColorPointer(4, GL_FLOAT, 0, dark_lca);
	else
		glColorPointer(4, GL_FLOAT, 0, primary_lca[1]);
	glVertexPointer(2, GL_FLOAT, 0, primary_lva[3]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	if (secondary<=2){
		//left secondary
		if (secondary != 1)
			glColorPointer(4, GL_FLOAT, 0, dark_lca);
		else
			glColorPointer(4, GL_FLOAT, 0, bright_lca);
		if(!secondary_lva[0])
			secondary_lva[0] = circle_array_init_2(16, 2.0, -10.0, 2.0, -2.0);
		ogl_drawcircle(16, GL_LINE_LOOP, secondary_lva[0]);
		//right secondary
		if (secondary != 2)
			glColorPointer(4, GL_FLOAT, 0, dark_lca);
		else
			glColorPointer(4, GL_FLOAT, 0, bright_lca);
		if(!secondary_lva[1])
			secondary_lva[1] = circle_array_init_2(16, 2.0, 10.0, 2.0, -2.0);
		ogl_drawcircle(16, GL_LINE_LOOP, secondary_lva[1]);
	}
	else {
		//bottom/middle secondary
		if (secondary != 4)
			glColorPointer(4, GL_FLOAT, 0, dark_lca);
		else
			glColorPointer(4, GL_FLOAT, 0, bright_lca);
		if(!secondary_lva[2])
			secondary_lva[2] = circle_array_init_2(16, 2.0, 0.0, 2.0, -8.0);
		ogl_drawcircle(16, GL_LINE_LOOP, secondary_lva[2]);
	}
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glPopMatrix();
}

void gr_enable_depth(int enable)
{
	abort();
    if (enable) {
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    } else {
		glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
    }
    //glEnable(GL_CULL_FACE);
}

/*
 * Stars on heaven in exit sequence, automap objects
 */
int g3_draw_sphere(g3s_point *pnt,fix rad){
	int c=grd_curcanv->cv_color, i;
	float scale = ((float)grd_curcanv->cv_bitmap.bm_w/grd_curcanv->cv_bitmap.bm_h);
	GLfloat color_array[20*4];
	
	for (i = 0; i < 20*4; i += 4)
	{
		color_array[i] = CPAL2Tr(c);
		color_array[i+1] = CPAL2Tg(c);
		color_array[i+2] = CPAL2Tb(c);
		color_array[i+3] = 1.0;
	}
	OGL_DISABLE(TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glPushMatrix();
	glTranslatef(f2glf(pnt->p3_vec.x),f2glf(pnt->p3_vec.y),-f2glf(pnt->p3_vec.z));
	if (scale >= 1)
	{
		rad/=scale;
		glScalef(f2glf(rad),f2glf(rad*scale),f2glf(rad));
	}
	else
	{
		rad*=scale;
		glScalef(f2glf(rad/scale),f2glf(rad),f2glf(rad));
	}
	if(!sphere_va)
		sphere_va = circle_array_init(20);
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	ogl_drawcircle(20, GL_TRIANGLE_FAN, sphere_va);
	glDisableClientState(GL_COLOR_ARRAY);
	glPopMatrix();
	//glEnable(GL_CULL_FACE); // TODO: they forgot it?
	return 0;
}

int gr_ucircle(fix xc1, fix yc1, fix r1)
{
	int c, nsides;
	c=grd_curcanv->cv_color;
	OGL_DISABLE(TEXTURE_2D);
	glColor4f(CPAL2Tr(c),CPAL2Tg(c),CPAL2Tb(c),1.0);
	glPushMatrix();
	glTranslatef(
	             (f2fl(xc1) + grd_curcanv->cv_bitmap.bm_x + 0.5) / (float)last_width,
	             1.0 - (f2fl(yc1) + grd_curcanv->cv_bitmap.bm_y + 0.5) / (float)last_height,0);
	glScalef(f2fl(r1) / last_width, f2fl(r1) / last_height, 1.0);
	nsides = 10 + 2 * (int)(M_PI * f2fl(r1) / 19);
	ogl_drawcircle(nsides, GL_LINE_LOOP, circle_array_init(nsides));
	glPopMatrix();
	return 0;
}

int gr_circle(fix xc1,fix yc1,fix r1){
	return gr_ucircle(xc1,yc1,r1);
}

/*
 * Draw flat-shaded Polygon (Lasers, Drone-arms, Driller-ears)
 */
bool g3_draw_poly(int nv,g3s_point **pointlist)
{
	int c, index3, index4;
	float color_r, color_g, color_b, color_a;
	r_polyc++;
	GLfloat vertex_array[nv*3];
	GLfloat color_array[nv*4];

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	c = grd_curcanv->cv_color;
	OGL_DISABLE(TEXTURE_2D);
	color_r = PAL2Tr(c);
	color_g = PAL2Tg(c);
	color_b = PAL2Tb(c);

	if (Gr_scanline_darkening_level >= GR_FADE_LEVELS) {
		color_a = 1.0;
	} else {
		glDepthMask(GL_FALSE);
		color_a = 1.0 - (float)Gr_scanline_darkening_level / ((float)GR_FADE_LEVELS - 1.0);
	}

	for (c=0; c<nv; c++){
		index3 = c * 3;
		index4 = c * 4;
		color_array[index4]    = color_r;
		color_array[index4+1]  = color_g;
		color_array[index4+2]  = color_b;
		color_array[index4+3]  = color_a;
		vertex_array[index3]   = f2glf(pointlist[c]->p3_vec.x);
		vertex_array[index3+1] = f2glf(pointlist[c]->p3_vec.y);
		vertex_array[index3+2] = -f2glf(pointlist[c]->p3_vec.z);
	}

	glVertexPointer(3, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glDrawArrays(GL_TRIANGLE_FAN, 0, nv);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDepthMask(GL_TRUE);
	return 0;
}

float Gr_color[3] = {1,1,1};
static bool tmap_flat;

void g3_set_flat_shading(bool enable)
{
	tmap_flat = enable;
}

bool g3_draw_tmap(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,grs_bitmap *bm)
{
	int c, index2, index3, index4;
	GLfloat vertex_array[nv*3];
	GLfloat color_array[nv*4];
	GLfloat texcoord_array[nv*2];
	GLfloat color_alpha = 1.0;
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	if (!tmap_flat) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		OGL_ENABLE(TEXTURE_2D);
		ogl_bindbmtex(bm);
		ogl_texwrap(bm->gltexture, GL_REPEAT);
		r_tpolyc++;
	} else {
		OGL_DISABLE(TEXTURE_2D);
		/* for cloaked state faces */
		color_alpha = 1.0 - (Gr_scanline_darkening_level/(GLfloat)NUM_LIGHTING_LEVELS);
	}
	
	for (c=0; c<nv; c++) {
		index2 = c * 2;
		index3 = c * 3;
		index4 = c * 4;
		
		vertex_array[index3]     = f2glf(pointlist[c]->p3_vec.x);
		vertex_array[index3+1]   = f2glf(pointlist[c]->p3_vec.y);
		vertex_array[index3+2]   = -f2glf(pointlist[c]->p3_vec.z);
		if (tmap_flat) {
			color_array[index4]      = 0;
		} else { 
			color_array[index4]    = bm->bm_flags & BM_FLAG_NO_LIGHTING ? 1.0 : f2glf(uvl_list[c].l);
		}
		color_array[index4+1]    = color_array[index4];
		color_array[index4+2]    = color_array[index4];
		color_array[index4+3]  = color_alpha;
		texcoord_array[index2]   = f2glf(uvl_list[c].u);
		texcoord_array[index2+1] = f2glf(uvl_list[c].v);
	}
	
	glVertexPointer(3, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	if (!tmap_flat) {
		glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array);  
	}
	
	glDrawArrays(GL_TRIANGLE_FAN, 0, nv);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	return 0;
}

/*
 * Everything texturemapped with secondary texture (walls with secondary texture)
 */
bool g3_draw_tmap_2(int nv, g3s_point **pointlist, g3s_uvl *uvl_list, grs_bitmap *bmbot, grs_bitmap *bm, int orient)
{
	int c, index2, index3, index4;;
	GLfloat vertex_array[nv*3], color_array[nv*4], texcoord_array[nv*2];
	
	g3_draw_tmap(nv,pointlist,uvl_list,bmbot);//draw the bottom texture first.. could be optimized with multitexturing..
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	r_tpolyc++;
	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_REPEAT);
	
	for (c=0; c<nv; c++) {
		index2 = c * 2;
		index3 = c * 3;
		index4 = c * 4;
		
		switch(orient){
			case 1:
				texcoord_array[index2]   = 1.0-f2glf(uvl_list[c].v);
				texcoord_array[index2+1] = f2glf(uvl_list[c].u);
				break;
			case 2:
				texcoord_array[index2]   = 1.0-f2glf(uvl_list[c].u);
				texcoord_array[index2+1] = 1.0-f2glf(uvl_list[c].v);
				break;
			case 3:
				texcoord_array[index2]   = f2glf(uvl_list[c].v);
				texcoord_array[index2+1] = 1.0-f2glf(uvl_list[c].u);
				break;
			default:
				texcoord_array[index2]   = f2glf(uvl_list[c].u);
				texcoord_array[index2+1] = f2glf(uvl_list[c].v);
				break;
		}
		
		// TODO: missing Gr_color mult.?
		color_array[index4]      = bm->bm_flags & BM_FLAG_NO_LIGHTING ? 1.0 : f2glf(uvl_list[c].l);
		color_array[index4+1]    = color_array[c*4];
		color_array[index4+2]    = color_array[c*4];
		color_array[index4+3]    = 1.0;
		
		vertex_array[index3]     = f2glf(pointlist[c]->p3_vec.x);
		vertex_array[index3+1]   = f2glf(pointlist[c]->p3_vec.y);
		vertex_array[index3+2]   = -f2glf(pointlist[c]->p3_vec.z);
	}
	
	glVertexPointer(3, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array);  
	glDrawArrays(GL_TRIANGLE_FAN, 0, nv);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	return 0;
}

/*
 * 2d Sprites (Fireaballs, powerups, explosions). NOT hostages
 */
bool g3_draw_bitmap(vms_vector *pos,fix width,fix height,grs_bitmap *bm, object *obj)
{
	vms_vector pv,v1;
	int i;
	float color_a;
	GLfloat vertex_array[12], color_array[16], texcoord_array[8];

	r_bitmapc++;
	v1.z=0;
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_CLAMP_TO_EDGE);

	// Define alpha by looking for object TYPE or ID. We do this here so we have it seperated from the rest of the code.
	if (PlayerCfg.OglAlphaEffects && // if -gl_transparency draw following bitmaps
		(obj->type==OBJ_FIREBALL || // all types of explosions and energy-effects
		(obj->type==OBJ_WEAPON && (obj->id != PROXIMITY_ID && obj->id != SUPERPROX_ID && obj->id != ROBOT_SUPERPROX_ID && obj->id != PMINE_ID)) || // weapon fire except bombs
		obj->id==POW_EXTRA_LIFE || // extra life
		obj->id==POW_ENERGY || // energy powerup
		obj->id==POW_SHIELD_BOOST || // shield boost
		obj->id==POW_CLOAK || // cloak
		obj->id==POW_INVULNERABILITY || // invulnerability
		obj->id==POW_HOARD_ORB)) // Hoard Orb
		color_a = 0.6; // ... with 0.6 alpha
	else
		color_a = 1.0;

	width = fixmul(width,Matrix_scale.x);
	height = fixmul(height,Matrix_scale.y);
	for (i=0;i<4;i++){
		vm_vec_sub(&v1,pos,&View_position);
		vm_vec_rotate(&pv,&v1,&View_matrix);
		switch (i){
			case 0:
				texcoord_array[i*2] = 0.0;
				texcoord_array[i*2+1] = 0.0;
				pv.x+=-width;
				pv.y+=height;
				break;
			case 1:
				texcoord_array[i*2] = bm->gltexture->u;
				texcoord_array[i*2+1] = 0.0;
				pv.x+=width;
				pv.y+=height;
				break;
			case 2:
				texcoord_array[i*2] = bm->gltexture->u;
				texcoord_array[i*2+1] = bm->gltexture->v;
				pv.x+=width;
				pv.y+=-height;
				break;
			case 3:
				texcoord_array[i*2] = 0.0;
				texcoord_array[i*2+1] = bm->gltexture->v;
				pv.x+=-width;
				pv.y+=-height;
				break;
		}

		if (obj->id == 5 && obj->type == 1) // create small z-Offset for missile exploding effect - prevents ugly wall-clipping
			pv.z -= F1_0;

		color_array[i*4]    = 1.0;
		color_array[i*4+1]  = 1.0;
		color_array[i*4+2]  = 1.0;
		color_array[i*4+3]  = color_a;
		
		vertex_array[i*3]   = f2glf(pv.x);
		vertex_array[i*3+1] = f2glf(pv.y);
		vertex_array[i*3+2] = -f2glf(pv.z);
	}
	glVertexPointer(3, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array);  
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // Replaced GL_QUADS
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	return 0;
}

bool ogl_ubitmapm_c(int x, int y,grs_bitmap *bm,int c)
{
	GLfloat xo,yo,xf,yf,u1,u2,v1,v2,color_r,color_g,color_b;
	GLfloat color_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	GLfloat vertex_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	GLfloat texcoord_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	x+=grd_curcanv->cv_bitmap.bm_x;
	y+=grd_curcanv->cv_bitmap.bm_y;
	xo=x/(float)last_width;
	xf=(bm->bm_w+x)/(float)last_width;
	yo=1.0-y/(float)last_height;
	yf=1.0-(bm->bm_h+y)/(float)last_height;

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_CLAMP_TO_EDGE);
	
	if (bm->bm_x==0){
		u1=0;
		if (bm->bm_w==bm->gltexture->w)
			u2=bm->gltexture->u;
		else
			u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}else {
		u1=bm->bm_x/(float)bm->gltexture->tw;
		u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}
	if (bm->bm_y==0){
		v1=0;
		if (bm->bm_h==bm->gltexture->h)
			v2=bm->gltexture->v;
		else
			v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}else{
		v1=bm->bm_y/(float)bm->gltexture->th;
		v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}

	if (c<0) {
		color_r = 1.0;
		color_g = 1.0;
		color_b = 1.0;
	} else {
		color_r = CPAL2Tr(c);
		color_g = CPAL2Tg(c);
		color_b = CPAL2Tb(c);
	}

	color_array[0] = color_array[4] = color_array[8] = color_array[12] = color_r;
	color_array[1] = color_array[5] = color_array[9] = color_array[13] = color_g;
	color_array[2] = color_array[6] = color_array[10] = color_array[14] = color_b;
	color_array[3] = color_array[7] = color_array[11] = color_array[15] = 1.0;

	vertex_array[0] = xo;
	vertex_array[1] = yo;
	vertex_array[2] = xf;
	vertex_array[3] = yo;
	vertex_array[4] = xf;
	vertex_array[5] = yf;
	vertex_array[6] = xo;
	vertex_array[7] = yf;

	texcoord_array[0] = u1;
	texcoord_array[1] = v1;
	texcoord_array[2] = u2;
	texcoord_array[3] = v1;
	texcoord_array[4] = u2;
	texcoord_array[5] = v2;
	texcoord_array[6] = u1;
	texcoord_array[7] = v2;
	
	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array);  
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // replaced GL_QUADS
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	return 0;
}

bool ogl_ubitmapm(int x, int y,grs_bitmap *bm){
	return ogl_ubitmapm_c(x,y,bm,-1);
}

/*
 * Movies
 */
bool ogl_ubitblt_i(int dw,int dh,int dx,int dy, int sw, int sh, int sx, int sy, grs_bitmap * src, grs_bitmap * dest, int mipmap)
{
	GLfloat xo,yo,xs,ys,u1,v1;
	GLfloat color_array[] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
	GLfloat texcoord_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	GLfloat vertex_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	ogl_texture tex;
	r_ubitbltc++;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	ogl_init_texture(&tex, sw, sh, (mipmap?OGL_FLAG_MIPMAP:OGL_FLAG_ALPHA));
	tex.prio = 0.0;
	tex.lw=src->bm_rowsize;

	u1=v1=0;
	
	dx+=dest->bm_x;
	dy+=dest->bm_y;
	xo=dx/(float)last_width;
	xs=dw/(float)last_width;
	yo=1.0-dy/(float)last_height;
	ys=dh/(float)last_height;
	
	OGL_ENABLE(TEXTURE_2D);
	
	ogl_pal=gr_current_pal;
	ogl_loadtexture(src->bm_data, sx, sy, &tex, src->bm_flags, 0, NULL);
	ogl_pal=gr_palette;
	OGL_BINDTEXTURE(tex.handle);
	
	ogl_texwrap(&tex,GL_CLAMP_TO_EDGE);

	vertex_array[0] = xo;
	vertex_array[1] = yo;
	vertex_array[2] = xo+xs;
	vertex_array[3] = yo;
	vertex_array[4] = xo+xs;
	vertex_array[5] = ys-ys;
	vertex_array[6] = xo;
	vertex_array[7] = yo-ys;

	texcoord_array[0] = u1;
	texcoord_array[1] = v1;
	texcoord_array[2] = tex.u;
	texcoord_array[3] = v1;
	texcoord_array[4] = tex.u;
	texcoord_array[5] = tex.v;
	texcoord_array[6] = u1;
	texcoord_array[7] = tex.v;

	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array);  
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);//replaced GL_QUADS

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	ogl_freetexture(&tex);
	return 0;
}

bool ogl_ubitblt(int w,int h,int dx,int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest){
	return ogl_ubitblt_i(w,h,dx,dy,w,h,sx,sy,src,dest,0);
}

GLubyte *pixels = NULL;

void ogl_start_frame(void){
	r_polyc=0;r_tpolyc=0;r_bitmapc=0;r_ubitbltc=0;r_upixelc=0;

	OGL_VIEWPORT(grd_curcanv->cv_bitmap.bm_x,grd_curcanv->cv_bitmap.bm_y,Canvas_width,Canvas_height);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	glLineWidth(linedotscale);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL,0.02);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	glShadeModel(GL_SMOOTH);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();//clear matrix
	perspective(90.0,1.0,1.0,1000.0);   
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();//clear matrix
}

void ogl_end_frame(void){
	OGL_VIEWPORT(0,0,grd_curscreen->sc_w,grd_curscreen->sc_h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();//clear matrix
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();//clear matrix
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

void gr_flip(void)
{
	ogl_do_palfx();
	ogl_swap_buffers_internal();
	glClear(GL_COLOR_BUFFER_BIT);
}

//little hack to find the nearest bigger power of 2 for a given number
int pow2ize(int x){
	int i;
	for (i=2; i<x; i*=2) {}; //corrected by MD2211: was previously limited to 4096
	return i;
}

GLubyte *texbuf = NULL;

// Allocate the pixel buffers 'pixels' and 'texbuf' based on current screen resolution
void ogl_init_pixel_buffers(int w, int h)
{
	w = pow2ize(w);	// convert to OpenGL texture size
	h = pow2ize(h);

	if (pixels)
		d_free(pixels);
	pixels = d_malloc(w*h*4);

	if (texbuf)
		d_free(texbuf);
	texbuf = d_malloc(max(w, 1024)*max(h, 256)*4);	// must also fit big font texture

	if ((pixels == NULL) || (texbuf == NULL))
		Error("Not enough memory for current resolution");
}

void ogl_close_pixel_buffers(void)
{
	d_free(pixels);
	d_free(texbuf);
}

void ogl_filltexbuf(unsigned char *data, GLubyte *texp, int truewidth, int width, int height, int dxo, int dyo, int twidth, int theight, int type, int bm_flags, int data_format)
{
	int x,y,c,i;

	if ((width > max(grd_curscreen->sc_w, 1024)) || (height > max(grd_curscreen->sc_h, 256)))
		Error("Texture is too big: %ix%i", width, height);

	i=0;
	for (y=0;y<theight;y++)
	{
		i=dxo+truewidth*(y+dyo);
		for (x=0;x<twidth;x++)
		{
			if (x<width && y<height)
			{
				if (data_format)
				{
					int j;

					for (j = 0; j < data_format; ++j)
						(*(texp++)) = data[i * data_format + j];
					i++;
					continue;
				}
				else
				{
					c = data[i++];
				}
			}
			else
			{
				c = 256; // fill the pad space with transparency (or blackness)
			}

			if (c == 254 && (bm_flags & BM_FLAG_SUPER_TRANSPARENT))
			{
				(*(texp++)) = 255;
				(*(texp++)) = 255;
				(*(texp++)) = 255;
				(*(texp++)) = 0; // transparent pixel
			}
			else if ((c == 255 && (bm_flags & BM_FLAG_TRANSPARENT)) || c == 256)
			{
				(*(texp++))=0;
				(*(texp++))=0;
				(*(texp++))=0;
				(*(texp++))=0;//transparent pixel
			}
			else
			{
				(*(texp++))=ogl_pal[c*3]*4;
				(*(texp++))=ogl_pal[c*3+1]*4;
				(*(texp++))=ogl_pal[c*3+2]*4;
				(*(texp++))=255;//not transparent
			}
		}
	}
}

//loads a palettized bitmap into a ogl RGBA texture.
//Sizes and pads dimensions to multiples of 2 if necessary.
//In theory this could be a problem for repeating textures, but all real
//textures (not sprites, etc) in descent are 64x64, so we are ok.
//stores OpenGL textured id in *texid and u/v values required to get only the real data in *u/*v
static int ogl_loadtexture(unsigned char *data, int dxo, int dyo, ogl_texture *tex, int bm_flags, int data_format, grs_bitmap *bm)
{
	uint8_t *tmp = NULL;
	GLubyte	*bufP = texbuf;
	tex->tw = pow2ize (tex->w);
	tex->th = pow2ize (tex->h);//calculate smallest texture size that can accomodate us (must be multiples of 2)

	//calculate u/v values that would make the resulting texture correctly sized
	tex->u = (float) ((double) tex->w / (double) tex->tw);
	tex->v = (float) ((double) tex->h / (double) tex->th);

	if (data) {
		// buttfuck hack against all the buttfuck hacks
		if (bm && bm->bm_depth == 4) {
			tex->format = GL_RGBA;
			tex->internalformat = GL_RGBA8;
			tmp = calloc(1, tex->tw * tex->th * 4);
			if (!tmp) {
				printf("Error: texture OOM\n");
				abort();
			}
			for (int y = 0; y < tex->h; y++)
				memcpy(tmp + y * tex->w * 4, data + y * tex->w * 4, 4 * tex->w);
			bufP = tmp;
		} else if (bm_flags >= 0) {
			if (bm)
				Assert(bm->bm_depth == 0 || bm->bm_depth == 1);
			ogl_filltexbuf (data, texbuf, tex->lw, tex->w, tex->h, dxo, dyo, tex->tw, tex->th, 
								 tex->format, bm_flags, data_format);
		} else {
			if (!dxo && !dyo && (tex->w == tex->tw) && (tex->h == tex->th))
				bufP = data;
			else {
				int h, w, tw;
				
				h = tex->lw / tex->w;
				w = (tex->w - dxo) * h;
				data += tex->lw * dyo + h * dxo;
				bufP = texbuf;
				tw = tex->tw * h;
				h = tw - w;
				for (; dyo < tex->h; dyo++, data += tex->lw) {
					memcpy (bufP, data, w);
					bufP += w;
					memset (bufP, 0, h);
					bufP += h;
				}
				memset (bufP, 0, tex->th * tw - (bufP - texbuf));
				bufP = texbuf;
			}
		}
	}
	// Generate OpenGL texture IDs.
	glGenTextures (1, &tex->handle);
	// Give our data to OpenGL.
	OGL_BINDTEXTURE(tex->handle);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (tex->wantmip)
	{

		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GameCfg.TexFilt==2?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR_MIPMAP_NEAREST));
	}
	else
	{
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	glTexImage2D (
		GL_TEXTURE_2D, 0, tex->internalformat,
		tex->tw, tex->th, 0, tex->format, // RGBA textures.
		GL_UNSIGNED_BYTE, // imageData is a GLubyte pointer.
		bufP);
	if (tex->wantmip)
		glGenerateMipmap(GL_TEXTURE_2D);
	r_texcount++; 
	free(tmp);
	return 0;
}

unsigned char decodebuf[1024*1024];

void ogl_loadbmtexture_f(grs_bitmap *bm, int flags)
{
	unsigned char *buf;
#ifdef HAVE_LIBPNG
	char *bitmapname;
#endif

	while (bm->bm_parent)
		bm=bm->bm_parent;
	if (bm->gltexture && bm->gltexture->handle > 0)
		return;
	buf=bm->bm_data;
#ifdef HAVE_LIBPNG
	if ((bitmapname = piggy_game_bitmap_name(bm)))
	{
		char filename[64];
		png_data pdata;

		sprintf(filename, "textures/%s.png", bitmapname);
		if (read_png(filename, &pdata))
		{
			con_printf(CON_DEBUG,"%s: %ux%ux%i p=%i(%i) c=%i a=%i chans=%i\n", filename, pdata.width, pdata.height, pdata.depth, pdata.paletted, pdata.num_palette, pdata.color, pdata.alpha, pdata.channels);
			if (pdata.depth == 8 && pdata.color)
			{
				if (bm->gltexture == NULL)
					ogl_init_texture(bm->gltexture = ogl_get_free_texture(), pdata.width, pdata.height, flags | ((pdata.alpha || bm->bm_flags & BM_FLAG_TRANSPARENT) ? OGL_FLAG_ALPHA : 0));
				ogl_loadtexture(pdata.data, 0, 0, bm->gltexture, bm->bm_flags, pdata.paletted ? 0 : pdata.channels, bm);
				free(pdata.data);
				if (pdata.palette)
					free(pdata.palette);
				return;
			}
			else
			{
				con_printf(CON_DEBUG,"%s: unsupported texture format: must be rgb, rgba, or paletted, and depth 8\n", filename);
				free(pdata.data);
				if (pdata.palette)
					free(pdata.palette);
			}
		}
	}
#endif
	if (bm->gltexture == NULL){
 		ogl_init_texture(bm->gltexture = ogl_get_free_texture(), bm->bm_w, bm->bm_h, flags | ((bm->bm_flags & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT))? OGL_FLAG_ALPHA : 0));
	}
	else {
		if (bm->gltexture->handle>0)
			return;
		if (bm->gltexture->w==0){
			bm->gltexture->lw=bm->bm_w;
			bm->gltexture->w=bm->bm_w;
			bm->gltexture->h=bm->bm_h;
		}
	}

	if (bm->bm_flags & BM_FLAG_RLE){
		unsigned char * dbits;
		unsigned char * sbits;
		int i, data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		sbits = &bm->bm_data[4 + (bm->bm_h * data_offset)];
		dbits = decodebuf;
		Assert(bm->bm_h * bm->bm_w <= sizeof(decodebuf));

		for (i=0; i < bm->bm_h; i++ )    {
			gr_rle_decode(sbits,dbits);
			if ( bm->bm_flags & BM_FLAG_RLE_BIG )
				sbits += (int)INTEL_SHORT(*((short *)&(bm->bm_data[4+(i*data_offset)])));
			else
				sbits += (int)bm->bm_data[4+i];
			dbits += bm->bm_w;
		}
		buf=decodebuf;
	}
	ogl_loadtexture(buf, 0, 0, bm->gltexture, bm->bm_flags, 0, bm);
}

void ogl_loadbmtexture(grs_bitmap *bm)
{
	ogl_loadbmtexture_f(bm, (GameCfg.TexFilt?OGL_FLAG_MIPMAP:0));
}

void ogl_freetexture(ogl_texture *gltexture)
{
	if (gltexture->handle>0) {
		r_texcount--;
		glDeleteTextures( 1, &gltexture->handle );
		ogl_reset_texture(gltexture);
	}
}
void ogl_freebmtexture(grs_bitmap *bm){
	if (bm->gltexture){
		ogl_freetexture(bm->gltexture);
		bm->gltexture=NULL;
	}
}

/*
 * Menu / gauges 
 */
bool ogl_ubitmapm_cs(int x, int y,int dw, int dh, grs_bitmap *bm,int c, int scale) // to scale bitmaps
{
	GLfloat xo,yo,xf,yf,u1,u2,v1,v2,color_r,color_g,color_b,h,a;
	GLfloat color_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	GLfloat texcoord_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	GLfloat vertex_array[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	
	x+=grd_curcanv->cv_bitmap.bm_x;
	y+=grd_curcanv->cv_bitmap.bm_y;
	xo=x/(float)last_width;
	xf=(bm->bm_w+x)/(float)last_width;
	yo=1.0-y/(float)last_height;
	yf=1.0-(bm->bm_h+y)/(float)last_height;

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if (dw < 0)
		dw = grd_curcanv->cv_bitmap.bm_w;
	else if (dw == 0)
		dw = bm->bm_w;
	if (dh < 0)
		dh = grd_curcanv->cv_bitmap.bm_h;
	else if (dh == 0)
		dh = bm->bm_h;

	a = (double) grd_curscreen->sc_w / (double) grd_curscreen->sc_h;
	h = (double) scale / (double) F1_0;

	xo = x / ((double) last_width * h);
	xf = (dw + x) / ((double) last_width * h);
	yo = 1.0 - y / ((double) last_height * h);
	yf = 1.0 - (dh + y) / ((double) last_height * h);

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_CLAMP_TO_EDGE);
	
	if (bm->bm_x==0){
		u1=0;
		if (bm->bm_w==bm->gltexture->w)
			u2=bm->gltexture->u;
		else
			u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}else {
		u1=bm->bm_x/(float)bm->gltexture->tw;
		u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}
	if (bm->bm_y==0){
		v1=0;
		if (bm->bm_h==bm->gltexture->h)
			v2=bm->gltexture->v;
		else
			v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}else{
		v1=bm->bm_y/(float)bm->gltexture->th;
		v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}

	if (c < 0) {
		color_r = 1.0;
		color_g = 1.0;
		color_b = 1.0;
	} else {
		color_r = CPAL2Tr(c);
		color_g = CPAL2Tg(c);
		color_b = CPAL2Tb(c);
	}  

	color_array[0] = color_array[4] = color_array[8] = color_array[12] = color_r;
	color_array[1] = color_array[5] = color_array[9] = color_array[13] = color_g;
	color_array[2] = color_array[6] = color_array[10] = color_array[14] = color_b;
	color_array[3] = color_array[7] = color_array[11] = color_array[15] = 1.0;

	vertex_array[0] = xo;
	vertex_array[1] = yo;
	vertex_array[2] = xf;
	vertex_array[3] = yo;
	vertex_array[4] = xf;
	vertex_array[5] = yf;
	vertex_array[6] = xo;
	vertex_array[7] = yf;

	texcoord_array[0] = u1;
	texcoord_array[1] = v1;
	texcoord_array[2] = u2;
	texcoord_array[3] = v1;
	texcoord_array[4] = u2;
	texcoord_array[5] = v2;
	texcoord_array[6] = u1;
	texcoord_array[7] = v2;

	glVertexPointer(2, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array);  
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);//replaced GL_QUADS
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
	return 0;
}

bool ogl_ubitmapm_3d(vms_vector *p, vms_vector *dx, vms_vector *dy, grs_bitmap *bm,int c)
{
	GLfloat u1,u2,v1,v2;

	static const int rc_x[4] = {0, 1, 1, 0};
	static const int rc_y[4] = {0, 0, 1, 1};

	vms_vector pt[4];
	for (int n = 0; n < 4; n++) {
		vms_vector c = *p;
		vm_vec_scale_add2(&c, dx, fl2f(rc_x[n]));
		vm_vec_scale_add2(&c, dy, fl2f(rc_y[n]));
		g3s_point cpp;
		g3_rotate_point(&cpp, &c);
		g3_project_point(&cpp);
		pt[n] = cpp.p3_vec;
	}

	OGL_ENABLE(TEXTURE_2D);
	ogl_bindbmtex(bm);
	ogl_texwrap(bm->gltexture,GL_CLAMP_TO_EDGE);

	if (bm->bm_x==0){
		u1=0;
		if (bm->bm_w==bm->gltexture->w)
			u2=bm->gltexture->u;
		else
			u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}else {
		u1=bm->bm_x/(float)bm->gltexture->tw;
		u2=(bm->bm_w+bm->bm_x)/(float)bm->gltexture->tw;
	}
	if (bm->bm_y==0){
		v1=0;
		if (bm->bm_h==bm->gltexture->h)
			v2=bm->gltexture->v;
		else
			v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}else{
		v1=bm->bm_y/(float)bm->gltexture->th;
		v2=(bm->bm_h+bm->bm_y)/(float)bm->gltexture->th;
	}

	GLfloat color_array[4 * 4];
	GLfloat texcoord_array[4 * 2];
	GLfloat vertex_array[4 * 3];

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	for (int n = 0; n < 4; n++) {
		if (c<0) {
			color_array[n * 4 + 0] = 1.0;
			color_array[n * 4 + 1] = 1.0;
			color_array[n * 4 + 2] = 1.0;
		} else {
			color_array[n * 4 + 0] = CPAL2Tr(c);
			color_array[n * 4 + 1] = CPAL2Tg(c);
			color_array[n * 4 + 2] = CPAL2Tb(c);
		}
		color_array[n * 4 + 3] = 1.0;
		vertex_array[n * 3 + 0] = pt[n].x;
		vertex_array[n * 3 + 1] = pt[n].y;
		vertex_array[n * 3 + 2] = -pt[n].z;
	}

	texcoord_array[0 * 2 + 0] = u1;
	texcoord_array[0 * 2 + 1] = v1;
	texcoord_array[1 * 2 + 0] = u2;
	texcoord_array[1 * 2 + 1] = v1;
	texcoord_array[2 * 2 + 0] = u2;
	texcoord_array[2 * 2 + 1] = v2;
	texcoord_array[3 * 2 + 0] = u1;
	texcoord_array[3 * 2 + 1] = v2;

	glVertexPointer(3, GL_FLOAT, 0, vertex_array);
	glColorPointer(4, GL_FLOAT, 0, color_array);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array);
	glDrawArrays(GL_QUADS, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	return 0;
}
