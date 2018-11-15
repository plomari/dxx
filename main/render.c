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
 * Rendering Stuff
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "segment.h"
#include "dxxerror.h"
#include "bm.h"
#include "texmap.h"
#include "render.h"
#include "game.h"
#include "object.h"
#include "laser.h"
#include "textures.h"
#include "screens.h"
#include "segpoint.h"
#include "wall.h"
#include "texmerge.h"
#include "physics.h"
#include "3d.h"
#include "gameseg.h"
#include "vclip.h"
#include "lighting.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "automap.h"
#include "endlevel.h"
#include "key.h"
#include "newmenu.h"
#include "u_mem.h"
#include "piggy.h"
#include "gamefont.h"
#include "switch.h"
#include "gauges.h"
#include "internal.h"
#include "gamemine.h"
#include "timer.h"
#include "effects.h"
#include "playsave.h"
#include "ogl_init.h"

#define INITIAL_LOCAL_LIGHT (F1_0/4)    // local light value in segment of occurence (of light emission)

// (former) "detail level" values
int Render_depth = MAX_RENDER_SEGS; //how many segments deep to render
int Simple_model_threshhold_scale = 50; // switch to simpler model when the object has depth greater than this value times its radius.
int Max_debris_objects = 15; // How many debris objects to create

int framecount=-1;
short Rotated_last[MAX_VERTICES];

// When any render function needs to know what's looking at it, it should 
// access Viewer members.
object * Viewer = NULL;

vms_vector Viewer_eye;  //valid during render

int	N_render_segs;

fix Render_zoom = 0x9000;					//the player's zoom factor

int highlight_seg = -1;
int highlight_side = -1;

int Outline_mode=0;

static void find_connect_side2(int segnum, int sidenum,
							   int *conn_seg, int *conn_side)
{
	*conn_seg = Segments[segnum].children[sidenum];
	*conn_side = -1;
	if (*conn_seg >= 0) {
		// (wasn't there a helper somewhere?)
		for (int n = 0; n < MAX_SIDES_PER_SEGMENT; n++) {
			if (Segments[*conn_seg].children[n] == segnum) {
				*conn_side = n;
				break;
			}
		}
		Assert(*conn_side >= 0);
	}
}

int toggle_outline_mode(void)
{
	return Outline_mode = !Outline_mode;
}

void draw_outline(int nverts,const g3s_point **pointlist)
{
	int i;

	gr_setcolor(BM_XRGB(63,63,63));

	for (i=0;i<nverts-1;i++)
		g3_draw_line(pointlist[i],pointlist[i+1]);

	g3_draw_line(pointlist[i],pointlist[0]);

}

extern fix Seismic_tremor_magnitude;

fix flash_scale;

#define FLASH_CYCLE_RATE f1_0

//cycle the flashing light for when mine destroyed
void flash_frame()
{
	static fixang flash_ang=0;

	if (!Control_center_destroyed && !Seismic_tremor_magnitude)
		return;

	if (Endlevel_sequence)
		return;

	if (PaletteBlueAdd > 10 )		//whiting out
		return;

	if (Seismic_tremor_magnitude) {
		fix	added_flash;

		added_flash = abs(Seismic_tremor_magnitude);
		if (added_flash < F1_0)
			added_flash *= 16;

		flash_ang += fixmul(FLASH_CYCLE_RATE, fixmul(FrameTime, added_flash+F1_0));
		fix_fastsincos(flash_ang,&flash_scale,NULL);
		flash_scale = (flash_scale + F1_0*3)/4;	//	gets in range 0.5 to 1.0
	} else {
		flash_ang += fixmul(FLASH_CYCLE_RATE,FrameTime);
		fix_fastsincos(flash_ang,&flash_scale,NULL);
		flash_scale = (flash_scale + f1_0)/2;
		if (Difficulty_level == 0)
			flash_scale = (flash_scale+F1_0*3)/4;
	}


}

// ----------------------------------------------------------------------------
//	Render a face.
//	It would be nice to not have to pass in segnum and sidenum, but
//	they are used for our hideously hacked in headlight system.
//	vp is a pointer to vertex ids.
//	tmap1, tmap2 are texture map ids.  tmap2 is the pasty one.
void render_face(int segnum, int sidenum, int nv, int *vp, int tmap1, int tmap2, uvl *uvlp, int wid_flags)
{
	// -- Using new headlight system...fix			face_light;
	grs_bitmap  *bm;
	grs_bitmap  *bm2 = NULL;

	fix			reflect;
	g3s_uvl			uvl_copy[8];
	g3s_lrgb		dyn_light[8];
	int			i;
	const g3s_point		*pointlist[8];

	Assert(nv <= 8);

	for (i=0; i<nv; i++) {
		uvl_copy[i].u = uvlp[i].u;
		uvl_copy[i].v = uvlp[i].v;
		dyn_light[i].r = dyn_light[i].g = dyn_light[i].b = uvl_copy[i].l = uvlp[i].l;
		pointlist[i] = &Segment_points[vp[i]];
	}

	int wall_num = Segments[segnum].sides[sidenum].wall_num;

	//handle cloaked walls
	if (wid_flags & WID_CLOAKED_FLAG) {
		Assert(wall_num != -1);
		gr_settransblend(Walls[wall_num].cloak_value, GR_BLEND_NORMAL);
		gr_setcolor(BM_XRGB(0, 0, 0));  // set to black (matters for s3)

		g3_draw_poly(nv, pointlist);    // draw as flat poly

		gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);

		return;
	}

	// -- Using new headlight system...face_light = -vm_vec_dot(&Viewer->orient.fvec,norm);

	if (tmap1 >= NumTextures) {
		Int3();
		Segments[segnum].sides[sidenum].tmap_num = 0;
	}

	if (GameArg.DbgAltTexMerge){
		PIGGY_PAGE_IN(Textures[tmap1]);
		bm = &GameBitmaps[Textures[tmap1].index];
		if (tmap2){
			PIGGY_PAGE_IN(Textures[tmap2&0x3FFF]);
			bm2 = &GameBitmaps[Textures[tmap2&0x3FFF].index];
		}
		if (bm2 && (bm2->bm_flags&BM_FLAG_SUPER_TRANSPARENT)){
			bm = texmerge_get_cached_bitmap( tmap1, tmap2 );
			bm2 = NULL;
		}
	} else {
		// New code for overlapping textures...
		if (tmap2 != 0) {
			bm = texmerge_get_cached_bitmap( tmap1, tmap2 );
		} else {
			bm = &GameBitmaps[Textures[tmap1].index];
			PIGGY_PAGE_IN(Textures[tmap1]);
		}
	}

	Assert( !(bm->bm_flags & BM_FLAG_PAGED_OUT) );

	//set light values for each vertex & build pointlist
	for (int i = 0; i < nv; i++)
	{
		//the uvl struct has static light already in it

		//scale static light for destruction effect
		if (Control_center_destroyed || Seismic_tremor_magnitude)	//make lights flash
			uvl_copy[i].l = fixmul(flash_scale,uvl_copy[i].l);
		//add in dynamic light (from explosions, etc.)
		uvl_copy[i].l += (Dynamic_light[vp[i]].r+Dynamic_light[vp[i]].g+Dynamic_light[vp[i]].b)/3;
		//saturate at max value
		if (uvl_copy[i].l > MAX_LIGHT)
			uvl_copy[i].l = MAX_LIGHT;

		// And now the same for the ACTUAL (rgb) light we want to use

		//scale static light for destruction effect
		if (Seismic_tremor_magnitude)	//make lights flash
			dyn_light[i].r = dyn_light[i].g = dyn_light[i].b = fixmul(flash_scale,uvl_copy[i].l);
		else if (Control_center_destroyed)	//make lights flash
		{
			if (PlayerCfg.DynLightColor) // let the mine glow red a little
			{
				dyn_light[i].r = fixmul(flash_scale>=f0_5*1.5?flash_scale:f0_5*1.5,uvl_copy[i].l);
				dyn_light[i].g = dyn_light[i].b = fixmul(flash_scale,uvl_copy[i].l);
			}
			else
				dyn_light[i].r = dyn_light[i].g = dyn_light[i].b = fixmul(flash_scale,uvl_copy[i].l);
		}

		// add light color
		dyn_light[i].r += Dynamic_light[vp[i]].r;
		dyn_light[i].g += Dynamic_light[vp[i]].g;
		dyn_light[i].b += Dynamic_light[vp[i]].b;
		// saturate at max value
		if (dyn_light[i].r > MAX_LIGHT)
			dyn_light[i].r = MAX_LIGHT;
		if (dyn_light[i].g > MAX_LIGHT)
			dyn_light[i].g = MAX_LIGHT;
		if (dyn_light[i].b > MAX_LIGHT)
			dyn_light[i].b = MAX_LIGHT;
		if (PlayerCfg.AlphaEffects) // due to additive blending, transparent sprites will become invivible in font of white surfaces (lamps). Fix that with a little desaturation
		{
			dyn_light[i].r *= .93;
			dyn_light[i].g *= .93;
			dyn_light[i].b *= .93;
		}
	}

	if ( PlayerCfg.AlphaEffects && ( TmapInfo[tmap1].eclip_num == ECLIP_NUM_FUELCEN || TmapInfo[tmap1].eclip_num == ECLIP_NUM_FORCE_FIELD ) ) // set nice transparency/blending for some special effects (if we do more, we should maybe use switch here)
		gr_settransblend(GR_FADE_OFF, GR_BLEND_ADDITIVE_C);

	if (bm2) {
		g3_draw_tmap_2(nv,pointlist,uvl_copy,dyn_light,bm,bm2,((tmap2&0xC000)>>14) & 3);
	} else {
		g3_draw_tmap(nv,pointlist,uvl_copy,dyn_light,bm);
	}

	gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL); // revert any transparency/blending setting back to normal

	if (Outline_mode)
		draw_outline(nv, pointlist);
}

void highlight_seg_side(segment *segp, int sidenum)
{
    for (int vert = 0; vert < 4; vert++)
        g3_draw_line(&Segment_points[segp->verts[Side_to_verts[sidenum][vert]]],&Segment_points[segp->verts[Side_to_verts[sidenum][(vert+1)%4]]]);
    for (int vert = 0; vert < 2; vert++)
        g3_draw_line(&Segment_points[segp->verts[Side_to_verts[sidenum][vert]]],&Segment_points[segp->verts[Side_to_verts[sidenum][(vert+2)%4]]]);
}

static void draw_links(segment *segp, int sidenum)
{
	side		*sidep = &segp->sides[sidenum];

    vms_vector cp;
    compute_center_point_on_side(&cp, segp, sidenum);
    g3s_point cpp;
    g3_rotate_point(&cpp, &cp);
    g3_project_point(&cpp);

    if (sidep->wall_num == -1)
        return;

    wall *wallp = &Walls[sidep->wall_num];
    if (wallp->trigger == -1)
        return;

    gr_setcolor(BM_XRGB(0,63,0));

	trigger *trigp = &Triggers[wallp->trigger];

    gr_setcolor (BM_XRGB(255,0,0));
    g3_draw_sphere(&cpp, 1 << 16);

    for (int i=0; i < trigp->num_links; i++) {
        segment *tseg = &Segments[trigp->seg[i]];
        int tside = trigp->side[i];

        vms_vector tcp;
        compute_center_point_on_side(&tcp, tseg, tside);

        g3s_point tcpp;
        g3_rotate_point(&tcpp, &tcp);
        g3_project_point(&tcpp);

		gr_setcolor(gr_find_closest_color(0,0,63));
		g3_draw_sphere(&tcpp, 1 << 16);

		gr_setcolor(gr_find_closest_color(0,63,0));
        g3_draw_line(&cpp, &tcpp);
    }
}

void highlight_trigger(segment *segp, int sidenum)
{
	int segnum = segp - Segments;

    draw_links(segp, sidenum);

	int conn_seg, conn_side;
	find_connect_side2(segnum, sidenum, &conn_seg, &conn_side);

	bool any_triggers = false, otherside_triggers = false;
	for (int n = 0; n < Num_triggers; n++) {
		trigger *tr = &Triggers[n];

		for (int i = 0; i < tr->num_links; i++) {
			if ((tr->seg[i] == segnum && tr->side[i] == sidenum) ||
				(tr->seg[i] == conn_seg && tr->side[i] == conn_side))
			{
				// tr apparently affects this side...
				// find segments which use this trigger and draw it
				for (int x = 0; x <= Highest_segment_index; x++) {
					for (int s = 0; s < MAX_SIDES_PER_SEGMENT; s++) {
						int w = Segments[x].sides[s].wall_num;
						if (w >= 0 && Walls[w].trigger == n)
							draw_links(&Segments[x], s);
					}
				}
			}
		}
	}
}

void highlight_all_triggers(void)
{
	if (highlight_seg >= 0)
		highlight_trigger(&Segments[highlight_seg], highlight_side);
}

static void highlight_side_triggers(int segnum, int sidenum)
{
	segment *segp = &Segments[segnum];
	int		vertnum_list[4];

#define ARRAY_OR_DEF(idx, arr, def) \
	((idx) >= 0 && (idx) < ARRAY_ELEMS(arr) && (arr)[(idx)] ? (arr)[(idx)] : (def))
#define APPENDF(s, ...)  do { 								\
	int pos_ = strlen(s);									\
	snprintf((s) + pos_, sizeof(s) - pos_, __VA_ARGS__);	\
} while(0)

	char t[512];
	t[0] = '\0';

	bool is_trigger = false;
	bool is_target = false;

	int wall_num = Segments[segnum].sides[sidenum].wall_num;
	if (wall_num >= 0) {
		struct wall *wall = &Walls[wall_num];

		static const char *const Wall_flags[] = {
			"blasted",	// WALL_BLASTED            1
			"opened",	// WALL_DOOR_OPENED		   2
			"unused",	//                         4
			"locked",	// WALL_DOOR_LOCKED        8
			"aclose",	// WALL_DOOR_AUTO          16
			"il_off",	// WALL_ILLUSION_OFF       32
			"switch",	// WALL_WALL_SWITCH        64
			"bproof",	// WALL_BUDDY_PROOF        128
		};

		char flags_str[80];
		flags_str[0] = '\0';
		for (int n = 0; n < ARRAY_ELEMS(Wall_flags); n++) {
			if (wall->flags & (1 << n))
				APPENDF(flags_str, "%s%s", flags_str[0] ? "+" : "", Wall_flags[n]);
		}

		static const char *const Wall_states[] = {
			[WALL_DOOR_CLOSED] = "closed",
			[WALL_DOOR_OPENING] =  "opening",
			[WALL_DOOR_WAITING] = "waiting",
			[WALL_DOOR_CLOSING] = "closing",
			[WALL_DOOR_OPEN] = "open",
			[WALL_DOOR_CLOAKING] = "cloaking",
			[WALL_DOOR_DECLOAKING] = "decloaking",
		};

		static const char *const Keys[] = {
			[KEY_NONE]	= "-",
			[KEY_BLUE]  = "blue",
			[KEY_RED] 	= "red",
			[KEY_GOLD]  = "yellow",
		};

		APPENDF(t,
				 "type: %d (%s)\n"
				 "flags: 0x%x (%s)\n"
				 "state: %d (%s)\n"
				 "keys: %d (%s)\n\n",
				 wall->type,
				 ARRAY_OR_DEF(wall->type, Wall_names, "?"),
				 wall->flags,
				 flags_str,
				 wall->state,
				 ARRAY_OR_DEF(wall->state, Wall_states, "?"),
				 wall->keys,
			     ARRAY_OR_DEF(wall->keys, Keys, "?"));

		if (wall->trigger >= 0) {
			trigger *tr = &Triggers[wall->trigger];

			is_trigger = true;

			#define TF_NO_MESSAGE       1   // Don't show a message when triggered
#define TF_ONE_SHOT         2   // Only trigger once
#define TF_DISABLED         4   // Set after one-shot fires


			static const char *const Trigger_name[] = {
				[TT_OPEN_DOOR] = "open_door",
				[TT_CLOSE_DOOR] = "close_door",
				[TT_MATCEN] = "matcen",
				[TT_EXIT] = "exit",
				[TT_SECRET_EXIT] = "secret",
				[TT_ILLUSION_OFF] = "ill_off",
				[TT_ILLUSION_ON] = "ill_on",
				[TT_UNLOCK_DOOR] = "unlock_door",
				[TT_LOCK_DOOR] = "lock_door",
				[TT_OPEN_WALL] = "open_wall",
				[TT_CLOSE_WALL] = "close_wall",
				[TT_ILLUSORY_WALL] = "ill_wall",
				[TT_LIGHT_OFF] = "lights_off",
				[TT_LIGHT_ON] = "lights_on",
			};

			APPENDF(t,
				"trigger: %d\n"
				"type: %d (%s)\n"
				"flags: 0x%x (%s%s%s)\n"
				"value: %f\n"
				"time: %f\n"
				"num_links: %d\n\n",
				wall->trigger,
				tr->type,
			    ARRAY_OR_DEF(tr->type, Trigger_name, "?"),
			    tr->flags,
			    tr->flags & TF_NO_MESSAGE ? "nomsg " : "",
			    tr->flags & TF_ONE_SHOT ? "oneshot " : "",
			    tr->flags & TF_DISABLED ? "disabled " : "",
			    f2fl(tr->value),
			    f2fl(tr->time),
			    tr->num_links);
		}
	}

	int conn_seg, conn_side;
	find_connect_side2(segnum, sidenum, &conn_seg, &conn_side);

	// Weird but apparently works. Observed with WALL_OPEN.
	if (wall_num >= 0 && conn_side >= 0 &&
		Segments[conn_seg].sides[conn_side].wall_num < 0)
	{
		APPENDF(t, "no opposite wall!\n");
	}

	bool any_triggers = false, otherside_triggers = false;
	for (int n = 0; n < Num_triggers; n++) {
		trigger *tr = &Triggers[n];

		for (int i = 0; i < tr->num_links; i++) {
			if (tr->seg[i] == segnum && tr->side[i] == sidenum) {
				if (!any_triggers)
					APPENDF(t, "triggered by:");
				any_triggers = true;
				APPENDF(t, " %d[%d/%d]", n, i, tr->num_links);
			}
			if (tr->seg[i] == conn_seg && tr->side[i] == conn_side)
				otherside_triggers = true;
		}
	}

	// Now why the fuck did they not just use a normal Triggers[] entry?
	for (int n = 0; n < ControlCenterTriggers.num_links; n++) {
		if (ControlCenterTriggers.seg[n] == segnum &&
			ControlCenterTriggers.side[n] == sidenum)
		{
			any_triggers = true;
			APPENDF(t, "toggled by reactor destruction\n");
		}
	}

	if (any_triggers)
		APPENDF(t, "\n");
	if (otherside_triggers)
		APPENDF(t, "trigger target on other side!\n");
	is_target = any_triggers || otherside_triggers;
	if (is_target)
		APPENDF(t, "\n");

	bool highlight = segnum == highlight_seg && sidenum == highlight_side;

	if (t[0] || highlight) {
		get_side_verts(vertnum_list, segnum, sidenum);

		float r = 1, g = 1, b = 1;

		if (is_trigger)
			b = 0;
		if (is_target)
			r = 0;
		if (highlight)
			g = 0;

		gr_setcolor(gr_find_closest_color(63*r,63*g,63*b));

		highlight_seg_side(segp, sidenum);

		vms_vector cp;
		compute_center_point_on_side(&cp, segp, sidenum);

		grd_curcanv->cv_font = GAME_FONT;
		gr_set_fontcolor( BM_XRGB(28,28,28), -1 );

		glDisable(GL_DEPTH_TEST);

		vms_vector right;
		vm_vec_normalized_dir(&right, &Vertices[vertnum_list[0]], &Vertices[vertnum_list[3]]);

		vms_vector up;
		vm_vec_normalized_dir(&up, &Vertices[vertnum_list[2]], &Vertices[vertnum_list[3]]);

		vm_vec_scale(&right, fl2f(0.01));
		vm_vec_scale(&up, fl2f(0.01));

		gr_3d_string(&cp, &right, &up, t);

		glEnable(GL_DEPTH_TEST);
	}
}

// -----------------------------------------------------------------------------------
//	Render a side.
//	Check for normal facing.  If so, render faces on side dictated by sidep->type.
void render_side(segment *segp, int sidenum)
{
	int		vertnum_list[4];
	side		*sidep = &segp->sides[sidenum];
	uvl		temp_uvls[3];
	fix		min_dot, max_dot;
	vms_vector	normals[2];
	int		wid_flags;
	int segnum = segp-Segments;


	wid_flags = WALL_IS_DOORWAY(segp,sidenum);

	if (!(wid_flags & WID_RENDER_FLAG))
		goto end;

	get_side_verts(vertnum_list,segp-Segments,sidenum);

	normals[0] = segp->sides[sidenum].normals[0];
	normals[1] = segp->sides[sidenum].normals[1];

 	//	Regardless of whether this side is comprised of a single quad, or two triangles, we need to know one normal, so
 	//	deal with it, get the dot product.
	int which_vertnum =
		/* Silly, but consistent with how it was at release */
		(sidep->type == SIDE_IS_QUAD) ? 0 :
		(sidep->type == SIDE_IS_TRI_13) ? 1 : 0;
	vms_vector tvec;
	vm_vec_sub(&tvec, &Viewer_eye, &Vertices[vertnum_list[which_vertnum]]);
	vm_vec_normalize_quick(&tvec);
	fix v_dot_n0 = vm_vec_dot(&tvec, &normals[0]);

	if (sidep->type == SIDE_IS_QUAD) {
		if (v_dot_n0 >= 0) {
			render_face(segp-Segments, sidenum, 4, vertnum_list, sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
		}
	} else {

		//	Although this side has been triangulated, because it is not planar, see if it is acceptable
		//	to render it as a single quadrilateral.  This is a function of how far away the viewer is, how non-planar
		//	the face is, how normal to the surfaces the view is.
		//	Now, if both dot products are close to 1.0, then render two triangles as a single quad.
		fix v_dot_n1 = vm_vec_dot(&tvec, &normals[1]);

		if (v_dot_n0 < v_dot_n1) {
			min_dot = v_dot_n0;
			max_dot = v_dot_n1;
		} else {
			min_dot = v_dot_n1;
			max_dot = v_dot_n0;
		}

		if (sidep->type == SIDE_IS_TRI_02) {
			if (v_dot_n0 >= 0) {
				render_face(segp-Segments, sidenum, 3, vertnum_list, sidep->tmap_num, sidep->tmap_num2, sidep->uvls, wid_flags);
			}

			if (v_dot_n1 >= 0) {
				temp_uvls[0] = sidep->uvls[0];		temp_uvls[1] = sidep->uvls[2];		temp_uvls[2] = sidep->uvls[3];
				vertnum_list[1] = vertnum_list[2];	vertnum_list[2] = vertnum_list[3];	// want to render from vertices 0, 2, 3 on side
				render_face(segp-Segments, sidenum, 3, &vertnum_list[0], sidep->tmap_num, sidep->tmap_num2, temp_uvls, wid_flags);
			}
		} else if (sidep->type ==  SIDE_IS_TRI_13) {
			if (v_dot_n1 >= 0) {
				render_face(segp-Segments, sidenum, 3, &vertnum_list[1], sidep->tmap_num, sidep->tmap_num2, &sidep->uvls[1], wid_flags);	// rendering 1,2,3, so just skip 0
			}

			if (v_dot_n0 >= 0) {
				temp_uvls[0] = sidep->uvls[0];		temp_uvls[1] = sidep->uvls[1];		temp_uvls[2] = sidep->uvls[3];
				vertnum_list[2] = vertnum_list[3];		// want to render from vertices 0,1,3
				render_face(segp-Segments, sidenum, 3, vertnum_list, sidep->tmap_num, sidep->tmap_num2, temp_uvls, wid_flags);
			}

		} else
			Int3();
	}

    end:;

	if (highlight_seg >= 0)
		highlight_side_triggers(segnum, sidenum);
}

extern ubyte DemoDoingRight,DemoDoingLeft;

void do_render_object(int objnum, int window_num)
{
	object *obj = &Objects[objnum];
	int count = 0;
	int n;

	Assert(objnum < MAX_OBJECTS);

   if (Newdemo_state==ND_STATE_PLAYBACK)  
	 {
	  if ((DemoDoingLeft==6 || DemoDoingRight==6) && Objects[objnum].type==OBJ_PLAYER)
		{
			// A nice fat hack: keeps the player ship from showing up in the
			// small extra view when guiding a missile in the big window
			
  			return; 
		}
	 }

	//	Added by MK on 09/07/94 (at about 5:28 pm, CDT, on a beautiful, sunny late summer day!) so
	//	that the guided missile system will know what objects to look at.
	//	I didn't know we had guided missiles before the release of D1. --MK
	if ((Objects[objnum].type == OBJ_ROBOT) || (Objects[objnum].type == OBJ_PLAYER)) {
		//Assert(Window_rendered_data[window_num].rendered_objects < MAX_RENDERED_OBJECTS);
		//	This peculiar piece of code makes us keep track of the most recently rendered objects, which
		//	are probably the higher priority objects, without overflowing the buffer
		if (Window_rendered_data[window_num].num_objects >= MAX_RENDERED_OBJECTS) {
			Int3();
			Window_rendered_data[window_num].num_objects /= 2;
		}
		Window_rendered_data[window_num].rendered_objects[Window_rendered_data[window_num].num_objects++] = objnum;
	}

	if ((count++ > MAX_OBJECTS) || (obj->next == objnum)) {
		Int3();					// infinite loop detected
		obj->next = -1;		// won't this clean things up?
		return;					// get out of this infinite loop!
	}

	render_object(obj);

	for (n=obj->attached_obj;n!=-1;n=Objects[n].ctype.expl_info.next_attach) {
		Assert(Objects[n].type == OBJ_FIREBALL);
		Assert(Objects[n].control_type == CT_EXPLOSION);
		Assert(Objects[n].flags & OF_ATTACHED);

		render_object(&Objects[n]);
	}
}

//increment counter for checking if points rotated
//This must be called at the start of the frame if rotate_list() will be used
void render_start_frame()
{
	framecount++;

	if (framecount==0) {		//wrap!

		memset(Rotated_last,0,sizeof(Rotated_last));		//clear all to zero
		framecount=1;											//and set this frame to 1
	}
}

//Given a lit of point numbers, rotate any that haven't been rotated this frame
g3s_codes rotate_list(int nv,int *pointnumlist)
{
	int i,pnum;
	g3s_point *pnt;
	g3s_codes cc;

	cc.and = 0xff;  cc.or = 0;

	for (i=0;i<nv;i++) {

		pnum = pointnumlist[i];

		pnt = &Segment_points[pnum];

		if (Rotated_last[pnum] != framecount) {

			g3_rotate_point(pnt,&Vertices[pnum]);

			Rotated_last[pnum] = framecount;
		}

		cc.and &= pnt->p3_codes;
		cc.or  |= pnt->p3_codes;
	}

	return cc;

}

//Given a lit of point numbers, project any that haven't been projected
void project_list(int nv,int *pointnumlist)
{
	int i,pnum;

	for (i=0;i<nv;i++) {

		pnum = pointnumlist[i];

		if (!(Segment_points[pnum].p3_flags & PF_PROJECTED))

			g3_project_point(&Segment_points[pnum]);

	}
}

typedef struct rect {
	short left,top,right,bot;
} rect;

ubyte code_window_point(fix x,fix y,rect *w)
{
	ubyte code=0;

	if (x <= w->left)  code |= 1;
	if (x >= w->right) code |= 2;

	if (y <= w->top) code |= 4;
	if (y >= w->bot) code |= 8;

	return code;
}

unsigned char visited[MAX_SEGMENTS];
short Render_list[MAX_RENDER_SEGS];
ubyte processed[MAX_RENDER_SEGS];		//whether each entry has been processed
short render_pos[MAX_SEGMENTS];	//where in render_list does this segment appear?
rect render_windows[MAX_RENDER_SEGS];

// render_seg_to_render_obj_list[render_seg_id] = -1 or index into render_objs
static int render_seg_to_render_objs[MAX_RENDER_SEGS];
// each entry is starts a list
struct render_obj_item {
    int objnum; // index into Objects[]
    int next;   // next list item in render_objs
};

// there needs to be an entry for each time an object is in some segment
// an object can be in multiple segments, in the worst (unrealistic) case, all
// objects are large enough to be in all segments (MAX_SEGMENTS*MAX_OBJECTS)
// this attempts to provide some sort of reasonable limit
// (do not confuse with MAX_RENDERED_OBJECTS)
#define MAX_RENDER_OBJS (MAX_RENDER_SEGS+MAX_OBJECTS)
static struct render_obj_item render_objs[MAX_RENDER_OBJS];
static int render_objs_next;

//Given two sides of segment, tell the two verts which form the 
//edge between them
// Invariant:
//	Two_sides_to_edge[i][j][k] == Two_sides_to_edge[j][i][k]
short Two_sides_to_edge[6][6][2] = {
	{ {-1,-1}, {3,7}, {-1,-1}, {2,6}, {6,7}, {2,3} },
	{ {3,7}, {-1,-1}, {0,4}, {-1,-1}, {4,7}, {0,3} },
	{ {-1,-1}, {0,4}, {-1,-1}, {1,5}, {4,5}, {0,1} },
	{ {2,6}, {-1,-1}, {1,5}, {-1,-1}, {5,6}, {1,2} },
	{ {6,7}, {4,7}, {4,5}, {5,6}, {-1,-1}, {-1,-1} },
	{ {2,3}, {0,3}, {0,1}, {1,2}, {-1,-1}, {-1,-1} }
};

//given an edge specified by two verts, give the two sides on that edge
// Invariant:
//	Edge_to_sides[i][j][k] == Edge_to_sides[j][i][k]
int Edge_to_sides[8][8][2] = {
	{ {-1,-1}, {2,5}, {-1,-1}, {1,5}, {1,2}, {-1,-1}, {-1,-1}, {-1,-1} },
	{ {2,5}, {-1,-1}, {3,5}, {-1,-1}, {-1,-1}, {2,3}, {-1,-1}, {-1,-1} },
	{ {-1,-1}, {3,5}, {-1,-1}, {0,5}, {-1,-1}, {-1,-1}, {0,3}, {-1,-1} },
	{ {1,5}, {-1,-1}, {0,5}, {-1,-1}, {-1,-1}, {-1,-1}, {-1,-1}, {0,1} },
	{ {1,2}, {-1,-1}, {-1,-1}, {-1,-1}, {-1,-1}, {2,4}, {-1,-1}, {1,4} },
	{ {-1,-1}, {2,3}, {-1,-1}, {-1,-1}, {2,4}, {-1,-1}, {3,4}, {-1,-1} },
	{ {-1,-1}, {-1,-1}, {0,3}, {-1,-1}, {-1,-1}, {3,4}, {-1,-1}, {0,4} },
	{ {-1,-1}, {-1,-1}, {-1,-1}, {0,1}, {1,4}, {-1,-1}, {0,4}, {-1,-1} },
};

//given an edge, tell what side is on that edge
int find_seg_side(segment *seg,int *verts,int notside)
{
	int i;
	int vv0=-1,vv1=-1;
	int side0,side1;
	int *eptr;
	int	v0,v1;
	int	*vp;

	v0 = verts[0];
	v1 = verts[1];
	vp = seg->verts;

	for (i=0; i<8; i++) {
		int svv = *vp++;	// seg->verts[i];

		if (vv0==-1 && svv == v0) {
			vv0 = i;
			if (vv1 != -1)
				break;
		}

		if (vv1==-1 && svv == v1) {
			vv1 = i;
			if (vv0 != -1)
				break;
		}
	}

	if (vv0 == -1 || vv1 == -1)
		return -1;

	eptr = Edge_to_sides[vv0][vv1];

	side0 = eptr[0];
	side1 = eptr[1];

	Assert(side0!=-1 && side1!=-1);

	if (side0 != notside) {
		Assert(side1==notside);
		return side0;
	}
	else {
		Assert(side0==notside);
		return side1;
	}

}

//find the two segments that join a given seg though two sides, and
//the sides of those segments the abut. 
int find_joining_side_norms(vms_vector *norm0_0,vms_vector *norm0_1,vms_vector *norm1_0,vms_vector *norm1_1,vms_vector **pnt0,vms_vector **pnt1,segment *seg,int s0,int s1)
{
	segment *seg0,*seg1;
	int edge_verts[2];
	int notside0,notside1;
	int edgeside0,edgeside1;

	Assert(s0!=-1 && s1!=-1);

	seg0 = &Segments[seg->children[s0]];
	seg1 = &Segments[seg->children[s1]];

	edge_verts[0] = seg->verts[Two_sides_to_edge[s0][s1][0]];
	edge_verts[1] = seg->verts[Two_sides_to_edge[s0][s1][1]];

	Assert(edge_verts[0]!=-1 && edge_verts[1]!=-1);

	notside0 = find_connect_side(seg,seg0);
	Assert(notside0 != -1);
	notside1 = find_connect_side(seg,seg1);
	Assert(notside1 != -1);

	edgeside0 = find_seg_side(seg0,edge_verts,notside0);
	if (edgeside0 == -1) return 0;
	edgeside1 = find_seg_side(seg1,edge_verts,notside1);
	if (edgeside1 == -1) return 0;

	*norm0_0 = seg0->sides[edgeside0].normals[0];
	*norm0_1 = seg0->sides[edgeside0].normals[1];
	*norm1_0 = seg1->sides[edgeside1].normals[0];
	*norm1_1 = seg1->sides[edgeside1].normals[1];

	*pnt0 = &Vertices[seg0->verts[Side_to_verts[edgeside0][seg0->sides[edgeside0].type==3?1:0]]];
	*pnt1 = &Vertices[seg1->verts[Side_to_verts[edgeside1][seg1->sides[edgeside1].type==3?1:0]]];

	return 1;
}

//see if the order matters for these two children.
//returns 0 if order doesn't matter, 1 if c0 before c1, -1 if c1 before c0
int compare_children(segment *seg,short c0,short c1)
{
	vms_vector norm0_0,norm0_1,*pnt0,temp;
	vms_vector norm1_0,norm1_1,*pnt1;
	fix d0_0,d0_1,d1_0,d1_1,d0,d1;
	int t;

	if (Side_opposite[c0] == c1) return 0;

	Assert(c0!=-1 && c1!=-1);

	//find normals of adjoining sides

	t = find_joining_side_norms(&norm0_0,&norm0_1,&norm1_0,&norm1_1,&pnt0,&pnt1,seg,c0,c1);

	if (!t) // can happen - 4D rooms!
		return 0;

	vm_vec_sub(&temp,&Viewer_eye,pnt0);
	d0_0 = vm_vec_dot(&norm0_0,&temp);
	d0_1 = vm_vec_dot(&norm0_1,&temp);

	vm_vec_sub(&temp,&Viewer_eye,pnt1);
	d1_0 = vm_vec_dot(&norm1_0,&temp);
	d1_1 = vm_vec_dot(&norm1_1,&temp);

	d0 = (d0_0 < 0 || d0_1 < 0)?-1:1;
	d1 = (d1_0 < 0 || d1_1 < 0)?-1:1;

	if (d0 < 0 && d1 < 0)
		return 0;

	if (d0 < 0)
		return 1;
	else if (d1 < 0)
		return -1;
	else
		return 0;

}

int ssc_total=0,ssc_swaps=0;

//short the children of segment to render in the correct order
//returns non-zero if swaps were made
int sort_seg_children(segment *seg,int n_children,short *child_list)
{
	int i,j;
	int r;
	int made_swaps,count;

	if (n_children == 0)
		return 0;

	ssc_total++;

	//for each child,  compare with other children and see if order matters
	//if order matters, fix if wrong

	count = 0;

	do {
		made_swaps = 0;

		for (i=0;i<n_children-1;i++) {
			for (j=i+1;child_list[i]!=-1 && j<n_children;j++) {
				if (child_list[j]!=-1) {
					r = compare_children(seg,child_list[i],child_list[j]);

					if (r == 1) {
						int temp = child_list[i];
						child_list[i] = child_list[j];
						child_list[j] = temp;
						made_swaps=1;
					}
				}
			}
		}

	} while (made_swaps && ++count<n_children);

	if (count)
		ssc_swaps++;

	return count;
}

void add_obj_to_seglist(int objnum,int listnum)
{
    int index;

    if (render_objs_next >= MAX_RENDER_OBJS) {
        Int3();
        return;
    }

    index = render_objs_next++;
    render_objs[index].objnum = objnum;
    render_objs[index].next = render_seg_to_render_objs[listnum];

    render_seg_to_render_objs[listnum] = index;
}

#define SORT_LIST_SIZE MAX_RENDER_OBJS

typedef struct sort_item {
	int objnum;
	fix dist;
} sort_item;

sort_item sort_list[SORT_LIST_SIZE];
int n_sort_items;

//compare function for object sort. 
static int sort_func(const void *pa, const void *pb)
{
    const sort_item *a = pa, *b = pb;
	fix delta_dist;
	object *obj_a,*obj_b;

	delta_dist = a->dist - b->dist;

	obj_a = &Objects[a->objnum];
	obj_b = &Objects[b->objnum];

	if (abs(delta_dist) < (obj_a->size + obj_b->size)) {		//same position

		//these two objects are in the same position.  see if one is a fireball
		//or laser or something that should plot on top.  Don't do this for
		//the afterburner blobs, though.

		if (obj_a->type == OBJ_WEAPON || (obj_a->type == OBJ_FIREBALL && obj_a->id != VCLIP_AFTERBURNER_BLOB)) {
			if (!(obj_b->type == OBJ_WEAPON || obj_b->type == OBJ_FIREBALL))
				return -1;	//a is weapon, b is not, so say a is closer
		} else {
			if (obj_b->type == OBJ_WEAPON || (obj_b->type == OBJ_FIREBALL && obj_b->id != VCLIP_AFTERBURNER_BLOB))
				return 1;	//b is weapon, a is not, so say a is farther
		}

		//no special case, fall through to normal return
	}

	return delta_dist;	//return distance
}

void build_object_lists(int n_segs)
{
	int nn;

	for (nn=0;nn<MAX_RENDER_SEGS;nn++)
		render_seg_to_render_objs[nn] = -1;

	render_objs_next = 0;

	for (nn=0;nn<n_segs;nn++) {
		int segnum;

		segnum = Render_list[nn];

		if (segnum != -1) {
			int objnum;
			object *obj;

			for (objnum=Segments[segnum].objects;objnum!=-1;objnum = obj->next) {

				obj = &Objects[objnum];
				
				if (obj->type == OBJ_NONE)
					continue;

				Assert( obj->segnum == segnum );

				if (obj->flags & OF_ATTACHED)
					continue;		//ignore this object

				int new_segnum = segnum;
				int list_pos = nn;

				if (obj->type != OBJ_CNTRLCEN && !(obj->type==OBJ_ROBOT && obj->id==65)) {		//don't migrate controlcen
					segmasks m;
	
					m = get_seg_masks(&obj->pos, new_segnum, obj->size, __FILE__, __LINE__);
	
					if (m.sidemask) {
						int sn,sf;

						for (sn=0,sf=1;sn<6;sn++,sf<<=1) {
							if (m.sidemask & sf) {
								segment *seg = &Segments[new_segnum];
		
								if (WALL_IS_DOORWAY(seg,sn) & WID_FLY_FLAG) {		//can explosion migrate through
									int child = seg->children[sn];
									int checknp;

									for (checknp=list_pos;checknp--;) {
										if (Render_list[checknp] == child) {
											new_segnum = child;
											list_pos = checknp;
										}
									}
								}
							}
						}
					}
				}

				add_obj_to_seglist(objnum,list_pos);
			}
		}
	}

	//now that there's a list for each segment, sort the items in those lists
	for (nn=0;nn<n_segs;nn++) {
		int segnum;

		segnum = Render_list[nn];

		if (segnum != -1) {
			int lookn,i;

			//first count the number of objects & copy into sort list

			lookn = render_seg_to_render_objs[nn];
			n_sort_items = 0;

			// the code below copies it into the sort list unconditionally
			static_assert(SORT_LIST_SIZE >= MAX_RENDER_OBJS, "increase SORT_LIST_SIZE");

			while (lookn!=-1) {
				int t = render_objs[lookn].objnum;
				sort_list[n_sort_items].objnum = t;
				//NOTE: maybe use depth, not dist - quicker computation
				sort_list[n_sort_items].dist = vm_vec_dist(&Objects[t].pos,&Viewer_eye);
				n_sort_items++;
				lookn = render_objs[lookn].next;
			}

			qsort(sort_list,n_sort_items,sizeof(*sort_list), sort_func);

			//now copy back into list

			lookn = render_seg_to_render_objs[nn];
			i = n_sort_items - 1;
			while (lookn!=-1) {
				render_objs[lookn].objnum = sort_list[i].objnum;
				lookn = render_objs[lookn].next;
				i--;
			}
		}
	}
}

vms_angvec Player_head_angles;

int Rear_view=0;
extern ubyte RenderingType;

void start_lighting_frame(object *viewer);
#include "internal.h"

//renders onto current canvas
void render_frame(fix eye_offset, int window_num)
{
	int start_seg_num;

	if (Endlevel_sequence) {
		render_endlevel_frame(eye_offset);
		return;
	}

	if ( Newdemo_state == ND_STATE_RECORDING && eye_offset >= 0 )	{
		if (RenderingType==0)
			newdemo_record_start_frame(FrameTime );
		if (RenderingType!=255)
			newdemo_record_viewer_object(Viewer);
	}

	start_lighting_frame(Viewer);		//this is for ugly light-smoothing hack
  
	g3_start_frame();

	Viewer_eye = Viewer->pos;

	if (eye_offset)	{
		vm_vec_scale_add2(&Viewer_eye,&Viewer->orient.rvec,eye_offset);
	}

	start_seg_num = find_point_seg(&Viewer_eye,Viewer->segnum);

	if (start_seg_num==-1)
		start_seg_num = Viewer->segnum;

	if (Rear_view && (Viewer==ConsoleObject)) {
		vms_matrix headm,viewm;
		Player_head_angles.p = Player_head_angles.b = 0;
		Player_head_angles.h = 0x7fff;
		vm_angles_2_matrix(&headm,&Player_head_angles);
		vm_matrix_x_matrix(&viewm,&Viewer->orient,&headm);
		g3_set_view_matrix(&Viewer_eye,&viewm,Render_zoom);
	} else	{
		g3_set_view_matrix(&Viewer_eye,&Viewer->orient,Render_zoom);
	}

	if (window_num > 0)
		gr_clear_canvas(BM_XRGB(0, 0, 0));

	render_mine(start_seg_num, eye_offset, window_num);

	gr_setcolor(BM_XRGB(31,0,31));

	g3_end_frame();
}

void update_rendered_data(int window_num, object *viewer, int rear_view_flag)
{
	Assert(window_num < MAX_RENDERED_WINDOWS);
	Window_rendered_data[window_num].time = timer_query();
	Window_rendered_data[window_num].viewer = viewer;
	Window_rendered_data[window_num].rear_view = rear_view_flag;
}

//build a list of segments to be rendered
//fills in Render_list & N_render_segs
void build_segment_list(int start_seg_num, int window_num)
{
	int	lcnt,scnt,ecnt;
	int	l,c;

	int sky_seg = -1;

	memset(visited, 0, sizeof(visited[0])*(Highest_segment_index+1));
	memset(render_pos, -1, sizeof(render_pos[0])*(Highest_segment_index+1));
	memset(processed, 0, sizeof(processed));

	lcnt = scnt = 0;

	Render_list[lcnt] = start_seg_num; visited[start_seg_num]=1;
	lcnt++;
	ecnt = lcnt;
	render_pos[start_seg_num] = 0;

	render_windows[0].left=render_windows[0].top=0;
	render_windows[0].right=grd_curcanv->cv_bitmap.bm_w-1;
	render_windows[0].bot=grd_curcanv->cv_bitmap.bm_h-1;

	//breadth-first renderer
	for (l=0;l<Render_depth;l++) {

		//while (scnt < ecnt) {
		for (scnt=0;scnt < ecnt;scnt++) {
			int rotated,segnum;
			rect *check_w;
			short child_list[MAX_SIDES_PER_SEGMENT];		//list of ordered sides to process
			int n_children;										//how many sides in child_list
			segment *seg;

			if (processed[scnt])
				continue;

			processed[scnt]=1;

			segnum = Render_list[scnt];
			check_w = &render_windows[scnt];

			if (segnum == -1)
				continue;

			seg = &Segments[segnum];
			rotated=0;

			//look at all sides of this segment.
			//tricky code to look at sides in correct order follows

			for (c=n_children=0;c<MAX_SIDES_PER_SEGMENT;c++) {		//build list of sides
				int wid = WALL_IS_DOORWAY(seg, c);

				int ch = seg->children[c];

				if (wid & WID_RENDPAST_FLAG) {
					const sbyte *sv = Side_to_verts[c];
					ubyte codes_and=0xff;
					int i;

					rotate_list(8,seg->verts);
					rotated=1;

					for (i=0;i<4;i++)
						codes_and &= Segment_points[seg->verts[sv[i]]].p3_codes;

					if (codes_and & CC_BEHIND)
						continue;

					child_list[n_children++] = c;
				} else if (ch < 0 && wall_check_transparency(seg, c)) {
					sky_seg = Sky_box_segment;
				}
			}

			//now order the sides in some magical way
			sort_seg_children(seg,n_children,child_list);

			for (c=0;c<n_children;c++) {
				int siden = child_list[c];
				int ch = seg->children[siden];

				int i;
				ubyte codes_and_3d,codes_and_2d;
				short _x,_y,min_x=32767,max_x=-32767,min_y=32767,max_y=-32767;
				int no_proj_flag=0;	//a point wasn't projected

				if (rotated<2) {
					if (!rotated)
						rotate_list(8,seg->verts);
					project_list(8,seg->verts);
					rotated=2;
				}

				for (i=0,codes_and_3d=codes_and_2d=0xff;i<4;i++) {
					int p = seg->verts[Side_to_verts[siden][i]];
					g3s_point *pnt = &Segment_points[p];

					if (! (pnt->p3_flags&PF_PROJECTED)) {
						no_proj_flag=1;
						break;
					}

					_x = f2i(pnt->p3_sx);
					_y = f2i(pnt->p3_sy);

					codes_and_3d &= pnt->p3_codes;
					codes_and_2d &= code_window_point(_x,_y,check_w);

					if (_x < min_x) min_x = _x;
					if (_x > max_x) max_x = _x;

					if (_y < min_y) min_y = _y;
					if (_y > max_y) max_y = _y;

				}

				if (no_proj_flag || (!codes_and_3d && !codes_and_2d)) {	//maybe add this segment
					int rp = render_pos[ch];
					rect *new_w = &render_windows[lcnt];

					if (no_proj_flag) {
						*new_w = *check_w;
					} else {
						new_w->left  = max(check_w->left,min_x);
						new_w->right = min(check_w->right,max_x);
						new_w->top   = max(check_w->top,min_y);
						new_w->bot   = min(check_w->bot,max_y);
					}

					//see if this seg already visited, and if so, does current window
					//expand the old window?
					if (rp != -1) {
						if (new_w->left < render_windows[rp].left ||
									new_w->top < render_windows[rp].top ||
									new_w->right > render_windows[rp].right ||
									new_w->bot > render_windows[rp].bot)
						{

							new_w->left  = min(new_w->left,render_windows[rp].left);
							new_w->right = max(new_w->right,render_windows[rp].right);
							new_w->top   = min(new_w->top,render_windows[rp].top);
							new_w->bot   = max(new_w->bot,render_windows[rp].bot);

							Render_list[lcnt] = -1;
							render_windows[rp] = *new_w;		//get updated window
							processed[rp] = 0;		//force reprocess
						}
					} else {
						if (lcnt >= MAX_RENDER_SEGS)
							goto done_list;
						render_pos[ch] = lcnt;
						Render_list[lcnt] = ch;
						lcnt++;
						visited[ch] = 1;
					}
				}
			}
		}

		scnt = ecnt;
		ecnt = lcnt;
	}

done_list:

	if (sky_seg >= 0 && lcnt < MAX_RENDER_SEGS) {
		Render_list[lcnt] = sky_seg;
		lcnt++;
		visited[sky_seg] = 1;
	}

	N_render_segs = lcnt;

}

//renders onto current canvas
void render_mine(int start_seg_num,fix eye_offset, int window_num)
{
	int		nn;

	//	Initialize number of objects (actually, robots!) rendered this frame.
	Window_rendered_data[window_num].num_objects = 0;

	//set up for rendering

	render_start_frame();

	build_segment_list(start_seg_num, window_num);		//fills in Render_list & N_render_segs

	build_object_lists(N_render_segs);

	if (eye_offset<=0) // Do for left eye or zero.
		set_dynamic_light();

	// Two pass rendering. Since sprites and some level geometry can have transparency (blending), we need some fancy sorting.
	// GL_DEPTH_TEST helps to sort everything in view but we should make sure translucent sprites are rendered after geometry to prevent them to turn walls invisible (if rendered BEFORE geometry but still in FRONT of it).
	// If walls use blending, they should be rendered along with objects (in same pass) to prevent some ugly clipping.

    // First Pass: render opaque level geometry and level geometry with alpha pixels (high Alpha-Test func)
	for (nn=N_render_segs;nn--;)
	{
		int segnum = Render_list[nn];

		if (segnum!=-1 && visited[segnum]!=255)
		{

			// render segment
			segment		*seg = &Segments[segnum];
			g3s_codes 	cc;
			int			sn;

			Assert(segnum!=-1 && segnum<=Highest_segment_index);

			cc=rotate_list(8,seg->verts);

			if (! cc.and) {		//all off screen?

				for (sn=0; sn<MAX_SIDES_PER_SEGMENT; sn++) {
					if (WALL_IS_DOORWAY(seg,sn) == WID_TRANSPARENT_WALL || WALL_IS_DOORWAY(seg,sn) == WID_TRANSILLUSORY_WALL || WALL_IS_DOORWAY(seg,sn) & WID_CLOAKED_FLAG)
					{
						// Note: geometry with alpha blending should render in the second pass instead
						glAlphaFunc(GL_GEQUAL,1.0);
						render_side(seg, sn);
						glAlphaFunc(GL_GEQUAL,0.02);
					}
					else
						render_side(seg, sn);
				}
			}
		}
	}

	memset(visited, 0, sizeof(visited[0])*(Highest_segment_index+1));

    // Second pass: Render objects and level geometry with alpha pixels (normal Alpha-Test func) and eclips with blending
	for (nn=N_render_segs;nn--;)
	{
		int segnum = Render_list[nn];

		if (segnum!=-1 && visited[segnum]!=255)
		{
			segment		*seg = &Segments[segnum];
			g3s_codes 	cc;
			int			sn;

			Assert(segnum!=-1 && segnum<=Highest_segment_index);

			cc=rotate_list(8,seg->verts);

			if (! cc.and) {		//all off screen?

				if (Viewer->type!=OBJ_ROBOT)
				Automap_visited[segnum]=1;

				for (sn=0; sn<MAX_SIDES_PER_SEGMENT; sn++) {
					if (WALL_IS_DOORWAY(seg,sn) == WID_TRANSPARENT_WALL || WALL_IS_DOORWAY(seg,sn) == WID_TRANSILLUSORY_WALL || WALL_IS_DOORWAY(seg,sn) & WID_CLOAKED_FLAG)
					{
						glAlphaFunc(GL_LESS,1.0);
						glDepthMask(GL_FALSE);
						render_side(seg, sn);
						glDepthMask(GL_TRUE);
						glAlphaFunc(GL_GEQUAL,0.02);
					}
				}
			}

			visited[segnum]=255;

			int index = render_seg_to_render_objs[nn];

			if (index < 0)
				continue;

			// render objects
			for (;index >= 0;index = render_objs[index].next) {
				int ObjNumber = render_objs[index].objnum;

				do_render_object(ObjNumber, window_num);	// note link to above else
			}
		}
	}

    highlight_all_triggers();
}
