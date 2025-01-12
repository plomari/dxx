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
 * object rendering
 *
 */

#include <string.h>	// for memset
#include <stdio.h>

#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"
#include "textures.h"
#include "byteswap.h"
#include "object.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "dxxerror.h"
#include "ai.h"
#include "hostage.h"
#include "morph.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "fuelcen.h"
#include "endlevel.h"
#include "sounds.h"
#include "collide.h"
#include "lighting.h"
#include "newdemo.h"
#include "player.h"
#include "weapon.h"
#include "newmenu.h"
#include "gauges.h"
#include "multi.h"
#include "menu.h"
#include "args.h"
#include "text.h"
#include "piggy.h"
#include "switch.h"
#include "gameseq.h"
#include "playsave.h"
#ifdef OGL
#include "ogl.h"
#endif
#include "../3d/globvars.h"
#include "gamefont.h"
#include "timer.h"

void obj_detach_all(object *parent);
void obj_detach_one(object *sub);

/*
 *  Global variables
 */

ubyte CollisionResult[MAX_OBJECT_TYPES][MAX_OBJECT_TYPES];
ubyte hitobj_list[MAX_OBJECTS][MAX_OBJECTS]; // last_hitobj of laser_inof struct can only keep track of one object. Still a persistent laser can hit several objects at the same time. This list keeps track of those while last_hitobj will only represent the most recent (the last) hitobj.

object *ConsoleObject;					//the object that is the player

static short free_obj_list[MAX_OBJECTS];

//Data for objects

// -- Object stuff

object Objects[MAX_OBJECTS];
int num_objects=0;
int Highest_object_index=0;
int Highest_ever_object_index=0;

// grs_bitmap *robot_bms[MAX_ROBOT_BITMAPS];	//all bitmaps for all robots

// int robot_bm_nums[MAX_ROBOT_TYPES];		//starting bitmap num for each robot
// int robot_n_bitmaps[MAX_ROBOT_TYPES];		//how many bitmaps for each robot

// char *robot_names[MAX_ROBOT_TYPES];		//name of each robot

//--unused-- int Num_robot_types=0;

int print_object_info = 0;
//@@int Object_viewer = 0;

//object * Slew_object = NULL;	// Object containing slew object info.

//--unused-- int Player_controller_type = 0;

window_rendered_data Window_rendered_data[MAX_RENDERED_WINDOWS];

//set viewer object to next object in array
void object_goto_next_viewer()
{
	int i, start_obj = 0;

	start_obj = Viewer - Objects;		//get viewer object number
	
	for (i=0;i<=Highest_object_index;i++) {

		start_obj++;
		if (start_obj > Highest_object_index ) start_obj = 0;

		if (Objects[start_obj].type != OBJ_NONE )	{
			Viewer = &Objects[start_obj];
			return;
		}
	}

	Error( "Couldn't find a viewer object!" );

}

object *obj_find_first_of_type (int type)
{
	int i;

	for (i=0;i<=Highest_object_index;i++)
		if (Objects[i].type==type)
			return (&Objects[i]);
	return ((object *)NULL);
}

//draw an object that has one bitmap & doesn't rotate
void draw_object_blob(object *obj,bitmap_index bmi)
{
	grs_bitmap * bm = &GameBitmaps[bmi.index];
	vms_vector pos = obj->pos;

	PIGGY_PAGE_IN( bmi );

	// draw these with slight offset to viewer preventing too much ugly clipping
	if ( obj->type == OBJ_FIREBALL && obj->id == VCLIP_VOLATILE_WALL_HIT )
	{
		vms_vector offs_vec;
		vm_vec_normalized_dir_quick(&offs_vec,&Viewer->pos,&obj->pos);
		vm_vec_scale_add2(&pos,&offs_vec,F1_0);
	}

	if (bm->bm_w > bm->bm_h)
		g3_draw_bitmap(&pos,obj->size,fixmuldiv(obj->size,bm->bm_h,bm->bm_w),bm);
	else
		g3_draw_bitmap(&pos,fixmuldiv(obj->size,bm->bm_w,bm->bm_h),obj->size,bm);
}

//draw an object that is a texture-mapped rod
void draw_object_tmap_rod(object *obj,bitmap_index bitmapi,int lighted)
{
	grs_bitmap * bitmap = &GameBitmaps[bitmapi.index];
	g3s_lrgb light;

	vms_vector delta,top_v,bot_v;
	g3s_point top_p,bot_p;

	PIGGY_PAGE_IN(bitmapi);

	bitmap->bm_handle = bitmapi.index;

	vm_vec_copy_scale(&delta,&obj->orient.uvec,obj->size);

	vm_vec_add(&top_v,&obj->pos,&delta);
	vm_vec_sub(&bot_v,&obj->pos,&delta);

	g3_rotate_point(&top_p,&top_v);
	g3_rotate_point(&bot_p,&bot_v);

	if (lighted)
	{
		light = compute_object_light(obj,&top_p.p3_vec);
	}
	else
	{
		light.r = light.g = light.b = f1_0;
	}

	g3_draw_rod_tmap(bitmap,&bot_p,obj->size,&top_p,obj->size,light);
}

int	Linear_tmap_polygon_objects = 1;

extern fix Max_thrust;

//used for robot engine glow
#define MAX_VELOCITY i2f(50)

//what darkening level to use when cloaked
#define CLOAKED_FADE_LEVEL		28

#define	CLOAK_FADEIN_DURATION_PLAYER	F2_0
#define	CLOAK_FADEOUT_DURATION_PLAYER	F2_0

#define	CLOAK_FADEIN_DURATION_ROBOT	F1_0
#define	CLOAK_FADEOUT_DURATION_ROBOT	F1_0

//do special cloaked render
void draw_cloaked_object(object *obj,g3s_lrgb light,fix *glow,size_t num_glow, fix cloak_start_time,fix cloak_end_time)
{
	fix cloak_delta_time,total_cloaked_time;
	fix light_scale=F1_0;
	int cloak_value=0;
	int fading=0;		//if true, fading, else cloaking
	fix	Cloak_fadein_duration=F1_0;
	fix	Cloak_fadeout_duration=F1_0;


	total_cloaked_time = cloak_end_time-cloak_start_time;

	switch (obj->type) {
		case OBJ_PLAYER:
			Cloak_fadein_duration = CLOAK_FADEIN_DURATION_PLAYER;
			Cloak_fadeout_duration = CLOAK_FADEOUT_DURATION_PLAYER;
			break;
		case OBJ_ROBOT:
			Cloak_fadein_duration = CLOAK_FADEIN_DURATION_ROBOT;
			Cloak_fadeout_duration = CLOAK_FADEOUT_DURATION_ROBOT;
			break;
		default:
			Int3();		//	Contact Mike: Unexpected object type in draw_cloaked_object.
	}

	cloak_delta_time = GameTime - cloak_start_time;

	if (cloak_delta_time < Cloak_fadein_duration/2) {

		light_scale = fixdiv(Cloak_fadein_duration/2 - cloak_delta_time,Cloak_fadein_duration/2);
		fading = 1;

	}
	else if (cloak_delta_time < Cloak_fadein_duration) {

		cloak_value = f2i(fixdiv(cloak_delta_time - Cloak_fadein_duration/2,Cloak_fadein_duration/2) * CLOAKED_FADE_LEVEL);

	} else if (GameTime < cloak_end_time-Cloak_fadeout_duration) {
		static int cloak_delta=0,cloak_dir=1;
		static fix cloak_timer=0;

		//note, if more than one cloaked object is visible at once, the
		//pulse rate will change!

		cloak_timer -= FrameTime;
		while (cloak_timer < 0) {

			cloak_timer += Cloak_fadeout_duration/12;

			cloak_delta += cloak_dir;

			if (cloak_delta==0 || cloak_delta==4)
				cloak_dir = -cloak_dir;
		}

		cloak_value = CLOAKED_FADE_LEVEL - cloak_delta;
	
	} else if (GameTime < cloak_end_time-Cloak_fadeout_duration/2) {

		cloak_value = f2i(fixdiv(total_cloaked_time - Cloak_fadeout_duration/2 - cloak_delta_time,Cloak_fadeout_duration/2) * CLOAKED_FADE_LEVEL);

	} else {

		light_scale = fixdiv(Cloak_fadeout_duration/2 - (total_cloaked_time - cloak_delta_time),Cloak_fadeout_duration/2);
		fading = 1;
	}


	if (fading) {
		fix save_glow;
		g3s_lrgb new_light;
		bitmap_index * alt_textures = NULL;

#ifdef NETWORK
		if ( obj->rtype.pobj_info.alt_textures > 0 )
			alt_textures = multi_player_textures[obj->rtype.pobj_info.alt_textures-1];
#endif

		new_light.r = fixmul(light.r,light_scale);
		new_light.g = fixmul(light.g,light_scale);
		new_light.b = fixmul(light.b,light_scale);
		save_glow = glow[0];
		glow[0] = fixmul(glow[0],light_scale);
		draw_polygon_model(&obj->pos,
				   &obj->orient,
				   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,
				   obj->rtype.pobj_info.model_num,obj->rtype.pobj_info.subobj_flags,
				   new_light,
				   glow, num_glow,
				   alt_textures );
		glow[0] = save_glow;
	}
	else {
		gr_settransblend(cloak_value, GR_BLEND_NORMAL);
		gr_setcolor(BM_XRGB(0,0,0));	//set to black (matters for s3)
		g3_set_flat_shading(true);
		draw_polygon_model(&obj->pos,
				   &obj->orient,
				   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,
				   obj->rtype.pobj_info.model_num,obj->rtype.pobj_info.subobj_flags,
				   light,
				   glow, num_glow,
				   NULL );
		g3_set_flat_shading(false);
		gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
	}

}

//draw an object which renders as a polygon model
void draw_polygon_object(object *obj)
{
	g3s_lrgb light;
	fix engine_glow_value[2] = { 0, -1 };		//element 0 is for engine glow, 1 for headlight

	light = compute_object_light(obj,NULL);

	//	If option set for bright players in netgame, brighten them!
#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && (obj->type == OBJ_PLAYER))
		if (Netgame.BrightPlayers)
			light.r = light.g = light.b = F1_0*2;
#endif

	//make robots brighter according to robot glow field
	if (obj->type == OBJ_ROBOT)
	{
		light.r += (Robot_info[obj->id].glow<<12); //convert 4:4 to 16:16
		light.g += (Robot_info[obj->id].glow<<12); //convert 4:4 to 16:16
		light.b += (Robot_info[obj->id].glow<<12); //convert 4:4 to 16:16
	}

	if (obj->type == OBJ_WEAPON)
	{
		if (obj->id == FLARE_ID)
		{
			light.r += F1_0*2;
			light.g += F1_0*2;
			light.b += F1_0*2;
		}
	}

	if (obj->type == OBJ_MARKER)
	{
 		light.r += F1_0*2;
		light.g += F1_0*2;
		light.b += F1_0*2;
	}


	//set engine glow value
	engine_glow_value[0] = f1_0/5;
	if (obj->movement_type == MT_PHYSICS) {

		if (obj->mtype.phys_info.flags & PF_USES_THRUST && obj->type==OBJ_PLAYER && obj->id==Player_num) {
			fix thrust_mag = vm_vec_mag_quick(&obj->mtype.phys_info.thrust);
			engine_glow_value[0] += (fixdiv(thrust_mag,Player_ship->max_thrust)*4)/5;
		}
		else {
			fix speed = vm_vec_mag_quick(&obj->mtype.phys_info.velocity);
			engine_glow_value[0] += (fixdiv(speed,MAX_VELOCITY)*3)/5;
		}
	}

	//set value for player headlight
	if (obj->type == OBJ_PLAYER) {
		if (Players[obj->id].flags & PLAYER_FLAGS_HEADLIGHT && !Endlevel_sequence)
			if (Players[obj->id].flags & PLAYER_FLAGS_HEADLIGHT_ON)
				engine_glow_value[1] = -2;		//draw white!
			else
				engine_glow_value[1] = -1;		//draw normal color (grey)
		else
			engine_glow_value[1] = -3;			//don't draw
	}

	if (obj->rtype.pobj_info.tmap_override != -1) {
		polymodel *pm = &Polygon_models[obj->rtype.pobj_info.model_num];
		bitmap_index bm_ptrs[12];

		int i;

		Assert(pm->n_textures<=12);

		for (i=0;i<12;i++)		//fill whole array, in case simple model needs more
			bm_ptrs[i] = Textures[obj->rtype.pobj_info.tmap_override];

		draw_polygon_model(&obj->pos,
				   &obj->orient,
				   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,
				   obj->rtype.pobj_info.model_num,
				   obj->rtype.pobj_info.subobj_flags,
				   light,
				   engine_glow_value, ARRAY_ELEMS(engine_glow_value),
				   bm_ptrs);
	}
	else {

		if (obj->type==OBJ_PLAYER && (Players[obj->id].flags&PLAYER_FLAGS_CLOAKED))
			draw_cloaked_object(obj,light,engine_glow_value,ARRAY_ELEMS(engine_glow_value),Players[obj->id].cloak_time,Players[obj->id].cloak_time+CLOAK_TIME_MAX);
		else if ((obj->type == OBJ_ROBOT) && (obj->ctype.ai_info.CLOAKED)) {
			ai_boss_info *info = ai_get_boss_info(obj - Objects);
			if (info)
				draw_cloaked_object(obj,light,engine_glow_value, ARRAY_ELEMS(engine_glow_value), info->cloak_start_time, info->cloak_end_time);
			else
				draw_cloaked_object(obj,light,engine_glow_value, ARRAY_ELEMS(engine_glow_value), GameTime-F1_0*10, GameTime+F1_0*10);
		} else {
			bitmap_index * alt_textures = NULL;
	
#ifdef NETWORK
			if ( obj->rtype.pobj_info.alt_textures > 0 )
				alt_textures = multi_player_textures[obj->rtype.pobj_info.alt_textures-1];
#endif

			if (obj->type == OBJ_ROBOT)
			{
			//	Snipers get bright when they fire.
			if (Ai_local_info[obj-Objects].next_fire < F1_0/8) {
				if (obj->ctype.ai_info.behavior == AIB_SNIPE)
				{
					light.r = 2*light.r + F1_0;
					light.g = 2*light.g + F1_0;
					light.b = 2*light.b + F1_0;
				}
			}
			}

			if (obj->type == OBJ_WEAPON && (Weapon_info[obj->id].model_num_inner > -1 )) {
				fix dist_to_eye = vm_vec_dist_quick(&Viewer->pos, &obj->pos);
				gr_settransblend(GR_FADE_OFF, GR_BLEND_ADDITIVE_A);
				if (dist_to_eye < Simple_model_threshhold_scale * F1_0*2)
					draw_polygon_model(&obj->pos,
							   &obj->orient,
							   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,
							   Weapon_info[obj->id].model_num_inner,
							   obj->rtype.pobj_info.subobj_flags,
							   light,
							   engine_glow_value, ARRAY_ELEMS(engine_glow_value),
							   alt_textures);
			}
			
			draw_polygon_model(&obj->pos,
					   &obj->orient,
					   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,obj->rtype.pobj_info.model_num,
					   obj->rtype.pobj_info.subobj_flags,
					   light,
					   engine_glow_value, ARRAY_ELEMS(engine_glow_value),
					   alt_textures);

#ifndef OGL // in software rendering must draw inner model last
			if (obj->type == OBJ_WEAPON && (Weapon_info[obj->id].model_num_inner > -1 )) {
				fix dist_to_eye = vm_vec_dist_quick(&Viewer->pos, &obj->pos);
				gr_settransblend(GR_FADE_OFF, GR_BLEND_ADDITIVE_A);
				if (dist_to_eye < Simple_model_threshhold_scale * F1_0*2)
					draw_polygon_model(&obj->pos,
							   &obj->orient,
							   (vms_angvec *)&obj->rtype.pobj_info.anim_angles,
							   Weapon_info[obj->id].model_num_inner,
							   obj->rtype.pobj_info.subobj_flags,
							   light,
							   engine_glow_value, ARRAY_ELEMS(engine_glow_value),
							   alt_textures);
			}
#endif

			if (obj->type == OBJ_WEAPON && (Weapon_info[obj->id].model_num_inner > -1 ))
				gr_settransblend(GR_FADE_OFF, GR_BLEND_NORMAL);
		}
	}

}

//------------------------------------------------------------------------------
// These variables are used to keep a list of the 3 closest robots to the viewer.
// The code works like this: Every time render object is called with a polygon model,
// it finds the distance of that robot to the viewer.  If this distance if within 10
// segments of the viewer, it does the following: If there aren't already 3 robots in
// the closet-robots list, it just sticks that object into the list along with its distance.
// If the list already contains 3 robots, then it finds the robot in that list that is
// farthest from the viewer. If that object is farther than the object currently being
// rendered, then the new object takes over that far object's slot.  *Then* after all
// objects are rendered, object_render_targets is called an it draws a target on top
// of all the objects.

//091494: #define MAX_CLOSE_ROBOTS 3
//--unused-- static int Object_draw_lock_boxes = 0;
//091494: static int Object_num_close = 0;
//091494: static object * Object_close_ones[MAX_CLOSE_ROBOTS];
//091494: static fix Object_close_distance[MAX_CLOSE_ROBOTS];

//091494: set_close_objects(object *obj)
//091494: {
//091494: 	fix dist;
//091494:
//091494: 	if ( (obj->type != OBJ_ROBOT) || (Object_draw_lock_boxes==0) )	
//091494: 		return;
//091494:
//091494: 	// The following code keeps a list of the 10 closest robots to the
//091494: 	// viewer.  See comments in front of this function for how this works.
//091494: 	dist = vm_vec_dist( &obj->pos, &Viewer->pos );
//091494: 	if ( dist < i2f(20*10) )	{				
//091494: 		if ( Object_num_close < MAX_CLOSE_ROBOTS )	{
//091494: 			Object_close_ones[Object_num_close] = obj;
//091494: 			Object_close_distance[Object_num_close] = dist;
//091494: 			Object_num_close++;
//091494: 		} else {
//091494: 			int i, farthest_robot;
//091494: 			fix farthest_distance;
//091494: 			// Find the farthest robot in the list
//091494: 			farthest_robot = 0;
//091494: 			farthest_distance = Object_close_distance[0];
//091494: 			for (i=1; i<Object_num_close; i++ )	{
//091494: 				if ( Object_close_distance[i] > farthest_distance )	{
//091494: 					farthest_distance = Object_close_distance[i];
//091494: 					farthest_robot = i;
//091494: 				}
//091494: 			}
//091494: 			// If this object is closer to the viewer than
//091494: 			// the farthest in the list, replace the farthest with this object.
//091494: 			if ( farthest_distance > dist )	{
//091494: 				Object_close_ones[farthest_robot] = obj;
//091494: 				Object_close_distance[farthest_robot] = dist;
//091494: 			}
//091494: 		}
//091494: 	}
//091494: }

int	Player_fired_laser_this_frame=-1;



// -----------------------------------------------------------------------------
//this routine checks to see if an robot rendered near the middle of
//the screen, and if so and the player had fired, "warns" the robot
void set_robot_location_info(object *objp)
{
	if (Player_fired_laser_this_frame != -1) {
		g3s_point temp;

		g3_rotate_point(&temp,&objp->pos);

		if (temp.p3_codes & CC_BEHIND)		//robot behind the screen
			return;

		//the code below to check for object near the center of the screen
		//completely ignores z, which may not be good

		if ((abs(temp.p3_x) < F1_0*4) && (abs(temp.p3_y) < F1_0*4)) {
			objp->ctype.ai_info.danger_laser_num = Player_fired_laser_this_frame;
			objp->ctype.ai_info.danger_laser_signature = Objects[Player_fired_laser_this_frame].signature;
		}
	}


}

//	------------------------------------------------------------------------------------------------------------------
void create_small_fireball_on_object(object *objp, fix size_scale, int sound_flag)
{
	fix			size;
	vms_vector	pos, rand_vec;
	int			segnum;

	pos = objp->pos;
	make_random_vector(&rand_vec);

	vm_vec_scale(&rand_vec, objp->size/2);

	vm_vec_add2(&pos, &rand_vec);

	size = fixmul(size_scale, F1_0/2 + d_rand()*4/2);

	segnum = find_point_seg(&pos, objp->segnum);
	if (segnum != -1) {
		object *expl_obj;
		expl_obj = object_create_explosion(segnum, &pos, size, VCLIP_SMALL_EXPLOSION);
		if (!expl_obj)
			return;
		obj_attach(objp,expl_obj);
		if (d_rand() < 8192) {
			fix	vol = F1_0/2;
			if (objp->type == OBJ_ROBOT)
				vol *= 2;
			if (sound_flag)
				digi_link_sound_to_object(SOUND_EXPLODING_WALL, objp-Objects, 0, vol);
		}
	}
}

static void draw_object_debug(object *obj)
{
	int objnum = obj - Objects;

	glDisable(GL_DEPTH_TEST);

	g3s_point g1;
	g3_rotate_point(&g1, &obj->pos);
	g3_project_point(&g1);

	gr_setcolor(gr_find_closest_color(0,0,63));
	g3_draw_sphere(&g1, fl2f(0.5));

	vms_vector v[] = {obj->orient.rvec, obj->orient.uvec, obj->orient.fvec};
	for (int n = 0; n < 3; n++) {
		vms_vector x, d;
		d = v[n];
		vm_vec_scale(&d, fl2f(5));
		vm_vec_add(&x, &obj->pos, &d);
		g3s_point gx;
		g3_rotate_point(&gx, &x);
		g3_project_point(&gx);

		int c[3] = {0};
		c[n] = 63;

		gr_setcolor(gr_find_closest_color(c[0],c[1],c[2]));
		g3_draw_line(&g1, &gx);
	}

	char t[512];
	t[0] = '\0';

	APPENDF(t,
		"type: %d\n"
		"flags: 0x%x\n"
		"id: %d\n"
		"index: %d\n"
		"ctype: %d\n"
		"shield: %f\n",
		 obj->type,
		 (unsigned int)obj->flags,
		 obj->id,
		 objnum,
		 obj->control_type,
		 f2fl(obj->shields));

	if (obj->lifeleft != IMMORTAL_TIME)
		APPENDF(t, "lifeleft: %f\n", f2fl(obj->lifeleft));

	if (obj->flags & OF_HAS_TRIGGERS) {
		APPENDF(t, "triggers:");
		for (int n = 0; n < Num_object_triggers; n++) {
			trigger *trig = &ObjectTriggers[n];
			if (trig->object_id == objnum)
				APPENDF(t, " %d(%d)", n, trig->type);
		}
		APPENDF(t, "\n");
	}

	grd_curcanv->cv_font = GAME_FONT;
	gr_set_fontcolor( BM_XRGB(28,28,28), -1 );

	vms_vector right = View_matrix.rvec;
	vms_vector up = View_matrix.uvec;

	vm_vec_scale(&right, fl2f(0.01));
	vm_vec_scale(&up, fl2f(-0.01));

	gr_3d_string(&obj->pos, &right, &up, t);

	glEnable(GL_DEPTH_TEST);
}

// -----------------------------------------------------------------------------
//	Render an object.  Calls one of several routines based on type
void render_object(object *obj)
{
	if ( obj == Viewer )
		return;

	if ( obj->type==OBJ_NONE )
	{
		Int3();
		return;
	}

	switch (obj->render_type)
	{
		case RT_NONE:
			break; //doesn't render, like the player

		case RT_POLYOBJ:
			if ( PlayerCfg.AlphaEffects && obj->type == OBJ_MARKER ) // set nice transparency/blending for certrain objects
				gr_settransblend( 10, GR_BLEND_ADDITIVE_A );

			draw_polygon_object(obj);

			if (obj->type == OBJ_ROBOT) //"warn" robot if being shot at
				set_robot_location_info(obj);
			break;

		case RT_MORPH:
			draw_morph_object(obj);
			break;

		case RT_FIREBALL:
			if ( PlayerCfg.AlphaEffects ) // set nice transparency/blending for certrain objects
				gr_settransblend( GR_FADE_OFF, GR_BLEND_ADDITIVE_C );

			draw_fireball(obj);
			break;

		case RT_WEAPON_VCLIP:
			if ( PlayerCfg.AlphaEffects && obj->id != PROXIMITY_ID && obj->id != SUPERPROX_ID ) // set nice transparency/blending for certrain objects
				gr_settransblend( 7, GR_BLEND_ADDITIVE_A );

			draw_weapon_vclip(obj);
			break;

		case RT_HOSTAGE:
			draw_hostage(obj);
			break;

		case RT_POWERUP:
			if ( PlayerCfg.AlphaEffects ) // set nice transparency/blending for certrain objects
				switch ( obj->id )
				{
					case POW_EXTRA_LIFE:
					case POW_ENERGY:
					case POW_SHIELD_BOOST:
					case POW_CLOAK:
					case POW_INVULNERABILITY:
					case POW_HOARD_ORB:
						gr_settransblend( 7, GR_BLEND_ADDITIVE_A );
						break;
				}

			draw_powerup(obj);
			break;

		case RT_LASER:
			if ( PlayerCfg.AlphaEffects ) // set nice transparency/blending for certrain objects
				gr_settransblend( 7, GR_BLEND_ADDITIVE_A );

			Laser_render(obj);
			break;

		// D2X-XL things: ignore
		case RT_THRUSTER:
		case RT_EXPLBLAST:
		case RT_SHRAPNELS:
		case RT_PARTICLES:
		case RT_LIGHTNING:
		case RT_SOUND:
		case RT_SHOCKWAVE:
			break;

		default: Error("Unknown render_type <%d>",obj->render_type);
 	}

 	gr_settransblend( GR_FADE_OFF, GR_BLEND_NORMAL ); // revert any transparency/blending setting back to normal

	if ( obj->render_type != RT_NONE && Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_render_object(obj);

	// Abuse this as indication whether to show debug stuff
	extern int highlight_seg;
	if (highlight_seg >= 0)
		draw_object_debug(obj);

}

#define vm_angvec_zero(v) (v)->p=(v)->b=(v)->h=0

void reset_player_object()
{
	int i;

	//Init physics

	vm_vec_zero(&ConsoleObject->mtype.phys_info.velocity);
	vm_vec_zero(&ConsoleObject->mtype.phys_info.thrust);
	vm_vec_zero(&ConsoleObject->mtype.phys_info.rotvel);
	vm_vec_zero(&ConsoleObject->mtype.phys_info.rotthrust);
	ConsoleObject->mtype.phys_info.brakes = ConsoleObject->mtype.phys_info.turnroll = 0;
	ConsoleObject->mtype.phys_info.mass = Player_ship->mass;
	ConsoleObject->mtype.phys_info.drag = Player_ship->drag;
	ConsoleObject->mtype.phys_info.flags |= PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST;

	//Init render info

	ConsoleObject->render_type = RT_POLYOBJ;
	ConsoleObject->rtype.pobj_info.model_num = Player_ship->model_num;		//what model is this?
	ConsoleObject->rtype.pobj_info.subobj_flags = 0;		//zero the flags
	ConsoleObject->rtype.pobj_info.tmap_override = -1;		//no tmap override!

	for (i=0;i<MAX_SUBMODELS;i++)
		vm_angvec_zero(&ConsoleObject->rtype.pobj_info.anim_angles[i]);

	// Clear misc

	ConsoleObject->flags = 0;

}


//make object0 the player, setting all relevant fields
void init_player_object()
{
	ConsoleObject->type = OBJ_PLAYER;
	ConsoleObject->id = 0;					//no sub-types for player

	ConsoleObject->signature = 0;			//player has zero, others start at 1

	ConsoleObject->size = Polygon_models[Player_ship->model_num].rad;

	ConsoleObject->control_type = CT_SLEW;			//default is player slewing
	ConsoleObject->movement_type = MT_PHYSICS;		//change this sometime

	ConsoleObject->lifeleft = IMMORTAL_TIME;

	ConsoleObject->attached_obj = -1;

	reset_player_object();

}

//sets up the free list & init player & whatever else
void init_objects()
{
	int i;

	collide_init();

	for (i=0;i<MAX_OBJECTS;i++) {
		free_obj_list[i] = i;
		Objects[i].type = OBJ_NONE;
		Objects[i].segnum = -1;
	}

	for (i=0;i<MAX_SEGMENTS;i++)
		Segments[i].objects = -1;

	ConsoleObject = Viewer = &Objects[0];

	init_player_object();
	obj_link(ConsoleObject-Objects,0);	//put in the world in segment 0

	num_objects = 1;						//just the player
	Highest_object_index = 0;

	
}

//after calling init_object(), the network code has grabbed specific
//object slots without allocating them.  Go though the objects & build
//the free list, then set the apporpriate globals
void special_reset_objects(void)
{
	int i;

	num_objects=MAX_OBJECTS;

	Highest_object_index = 0;
	Assert(Objects[0].type != OBJ_NONE);		//0 should be used

	for (i=MAX_OBJECTS;i--;)
		if (Objects[i].type == OBJ_NONE)
			free_obj_list[--num_objects] = i;
		else
			if (i > Highest_object_index)
				Highest_object_index = i;
}

//link the object into the list for its segment
void obj_link(int objnum,int segnum)
{
	object *obj = &Objects[objnum];

	Assert(objnum != -1);

	Assert(obj->segnum == -1);

	Assert(segnum>=0 && segnum<=Highest_segment_index);

	obj->segnum = segnum;
	
	obj->next = Segments[segnum].objects;
	obj->prev = -1;

	Segments[segnum].objects = objnum;

	if (obj->next != -1) Objects[obj->next].prev = objnum;
	
	//list_seg_objects( segnum );
	//check_duplicate_objects();

	Assert(Objects[0].next != 0);
	if (Objects[0].next == 0)
		Objects[0].next = -1;

	Assert(Objects[0].prev != 0);
	if (Objects[0].prev == 0)
		Objects[0].prev = -1;
}

void obj_unlink(int objnum)
{
	object  *obj = &Objects[objnum];
	segment *seg = &Segments[obj->segnum];

	Assert(objnum != -1);

	if (obj->prev == -1)
		seg->objects = obj->next;
	else
		Objects[obj->prev].next = obj->next;

	if (obj->next != -1) Objects[obj->next].prev = obj->prev;

	obj->segnum = -1;

	Assert(Objects[0].next != 0);
	Assert(Objects[0].prev != 0);
}

// Returns a new, unique signature for a new object
int obj_get_signature()
{
	static short sig = 0; // Yes! Short! a) We do not need higher values b) the demo system only stores shorts
	int free = 0, i = 0;

	while (!free)
	{
		free = 1;
		sig++;
		if (sig < 0)
			sig = 0;
		for (i = 0; i < MAX_OBJECTS; i++)
		{
			if ((sig == Objects[i].signature) && (Objects[i].type != OBJ_NONE))
			{
				free = 0;
			}
		}
	}

	return sig;
}

int Debris_object_count=0;

int	Unused_object_slots;

//returns the number of a free object, updating Highest_object_index.
//Generally, obj_create() should be called to get an object, since it
//fills in important fields and does the linking.
//returns -1 if no free objects
int obj_allocate(void)
{
	int objnum;

	if ( num_objects >= MAX_OBJECTS ) {
		return -1;
	}

	objnum = free_obj_list[num_objects++];

	if (objnum > Highest_object_index) {
		Highest_object_index = objnum;
		if (Highest_object_index > Highest_ever_object_index)
			Highest_ever_object_index = Highest_object_index;
	}

{
int	i;
Unused_object_slots=0;
for (i=0; i<=Highest_object_index; i++)
	if (Objects[i].type == OBJ_NONE)
		Unused_object_slots++;
}
	return objnum;
}

//frees up an object.  Generally, obj_delete() should be called to get
//rid of an object.  This function deallocates the object entry after
//the object has been unlinked
void obj_free(int objnum)
{
	free_obj_list[--num_objects] = objnum;
	Assert(num_objects >= 0);

	if (objnum == Highest_object_index)
	{
		int o = Highest_object_index;
		for (;;)
		{
			--o;
			if (Objects[o].type != OBJ_NONE)
				break;
			if (o == 0)
				break;
		}
		Highest_object_index = o;
	}
}

//-----------------------------------------------------------------------------
//	If there are more than MAX_USED_OBJECTS objects in use, try to free
//  disposable ones.
static void free_object_slots(void)
{
	int	num_to_free = num_objects - MAX_USED_OBJECTS;

	if (num_to_free <= 0)
		return;

	int candidates[MAX_OBJECTS - MAX_USED_OBJECTS][4];
	int num_candidates[4];

	for (int i = 0; i <= Highest_object_index; i++) {
		if (Objects[i].flags & OF_SHOULD_BE_DEAD) {
			num_to_free--;
			if (num_to_free <= 0)
				return;
		} else {
			int cand_num = -1;
			switch (Objects[i].type) {
			case OBJ_DEBRIS:
				cand_num = 0;
				break;
			case OBJ_FIREBALL:
				if (Objects[i].ctype.expl_info.delete_objnum == -1)
					cand_num = 1;
				break;
			case OBJ_WEAPON:
				cand_num = Objects[i].id == FLARE_ID ? 2 : 3;
				break;
			}
			if (cand_num >= 0 && num_candidates[cand_num] < ARRAY_ELEMS(candidates[0])) {
				candidates[cand_num][num_candidates[cand_num]] = i;
				num_candidates[cand_num]++;
			}
		}
	}

	for (int cand = 0; cand < 4; cand++) {
		for (int n = 0; n < num_candidates[cand]; n++) {
			if (!num_to_free)
				return;
			Objects[candidates[cand][n]].flags |= OF_SHOULD_BE_DEAD;
			num_to_free--;
		}
	}
}

//-----------------------------------------------------------------------------
//initialize a new object.  adds to the list for the given segment
//note that segnum is really just a suggestion, since this routine actually
//searches for the correct segment
//returns the object number
int obj_create(enum object_type_t type,ubyte id,int segnum,const vms_vector *pos,
				const vms_matrix *orient,fix size,ubyte ctype,ubyte mtype,ubyte rtype)
{
	int objnum;
	object *obj;

	// Some consistency checking. FIXME: Add more debug output here to probably trace all possible occurances back.
	if (segnum < 0 || segnum > Highest_segment_index) {
		Int3();
		return -1;
	}

	Assert(ctype <= CT_CNTRLCEN);

	if (type==OBJ_DEBRIS && Debris_object_count>=Max_debris_objects && !PERSISTENT_DEBRIS)
		return -1;

	if (get_seg_masks(pos, segnum, 0, __FILE__, __LINE__).centermask != 0)
		if ((segnum=find_point_seg(pos,segnum))==-1) {
			return -1;		//don't create this object
		}

	// Find next free object
	objnum = obj_allocate();

	if (objnum == -1) {		//no free objects
                Int3(); // so we notice
		return -1;
        }

	Assert(Objects[objnum].type == OBJ_NONE);		//make sure unused

	obj = &Objects[objnum];

	Assert(obj->segnum == -1);

	int signature = obj_get_signature();

	// Zero out object structure to keep weird bugs from happening
	// in uninitialized fields.
	memset( obj, 0, sizeof(object) );

	obj->signature				= signature;
	obj->type 					= type;
	obj->id 						= id;
	obj->last_pos				= *pos;
	obj->pos 					= *pos;
	obj->size 					= size;
	obj->flags 					= 0;
	//@@if (orient != NULL)
	//@@	obj->orient 			= *orient;

	obj->orient 				= orient?*orient:vmd_identity_matrix;

	obj->control_type 		= ctype;
	obj->movement_type 		= mtype;
	obj->render_type 			= rtype;
	obj->contains_type		= -1;

	obj->lifeleft 				= IMMORTAL_TIME;		//assume immortal
	obj->attached_obj			= -1;

	if (obj->control_type == CT_POWERUP)
		obj->ctype.powerup_info.count = 1;

	// Init physics info for this object
	if (obj->movement_type == MT_PHYSICS) {

		vm_vec_zero(&obj->mtype.phys_info.velocity);
		vm_vec_zero(&obj->mtype.phys_info.thrust);
		vm_vec_zero(&obj->mtype.phys_info.rotvel);
		vm_vec_zero(&obj->mtype.phys_info.rotthrust);

		obj->mtype.phys_info.mass		= 0;
		obj->mtype.phys_info.drag 		= 0;
		obj->mtype.phys_info.brakes	= 0;
		obj->mtype.phys_info.turnroll	= 0;
		obj->mtype.phys_info.flags		= 0;
	}

	if (obj->render_type == RT_POLYOBJ)
		obj->rtype.pobj_info.tmap_override = -1;

	obj->shields 				= 20*F1_0;

	segnum = find_point_seg(pos,segnum);		//find correct segment

	Assert(segnum!=-1);

	obj->segnum = -1;					//set to zero by memset, above
	obj_link(objnum,segnum);

	//	Set (or not) persistent bit in phys_info.
	if (obj->type == OBJ_WEAPON) {
		Assert(obj->control_type == CT_WEAPON);
		obj->mtype.phys_info.flags |= (Weapon_info[obj->id].persistent*PF_PERSISTENT);
		obj->ctype.laser_info.creation_time = GameTime;
		obj->ctype.laser_info.last_hitobj = -1;
		memset(&hitobj_list[objnum], 0, sizeof(ubyte)*MAX_OBJECTS);
		obj->ctype.laser_info.multiplier = F1_0;
	}

	if (obj->control_type == CT_POWERUP)
		obj->ctype.powerup_info.creation_time = GameTime;

	if (obj->control_type == CT_EXPLOSION)
		obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = -1;

	if (obj->type == OBJ_DEBRIS)
		Debris_object_count++;

	return objnum;
}

extern void newdemo_record_guided_end();

//remove object from the world
void obj_delete(int objnum)
{
	int pnum;
	object *obj = &Objects[objnum];

	ai_delete_object(objnum);
	trigger_delete_object(objnum);

	Assert(objnum != -1);
	Assert(objnum != 0 );
	Assert(obj->type != OBJ_NONE);
	Assert(obj != ConsoleObject);

	if (obj->type==OBJ_WEAPON && obj->id==GUIDEDMISS_ID && obj->ctype.laser_info.parent_type==OBJ_PLAYER)
	{
		pnum=Objects[obj->ctype.laser_info.parent_num].id;

		if (pnum!=Player_num) {
			Guided_missile[pnum]=NULL;
		}
		else if (Newdemo_state==ND_STATE_RECORDING)
			newdemo_record_guided_end();
		
	}

	if (obj == Viewer)		//deleting the viewer?
		Viewer = ConsoleObject;						//..make the player the viewer

	if (obj->flags & OF_ATTACHED)		//detach this from object
		obj_detach_one(obj);

	if (obj->attached_obj != -1)		//detach all objects from this
		obj_detach_all(obj);

	if (obj->type == OBJ_DEBRIS)
		Debris_object_count--;

	obj_unlink(objnum);

	Assert(Objects[0].next != 0);

	obj->type = OBJ_NONE;		//unused!
	obj->signature = -1;
	obj->segnum=-1;				// zero it!

	obj_free(objnum);
}

#define	DEATH_SEQUENCE_LENGTH			(F1_0*5)
#define	DEATH_SEQUENCE_EXPLODE_TIME	(F1_0*2)

int		Player_is_dead = 0;			//	If !0, then player is dead, but game continues so he can watch.
object	*Dead_player_camera = NULL;	//	Object index of object watching deader.
object	*Viewer_save;
int		Player_flags_save;
int		Player_exploded = 0;
int		Death_sequence_aborted=0;
int		Player_eggs_dropped=0;
fix		Camera_to_player_dist_goal=F1_0*4;
ubyte		Control_type_save, Render_type_save;

//	------------------------------------------------------------------------------------------------------------------
void dead_player_end(void)
{
	if (!Player_is_dead)
		return;

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_restore_cockpit();

	Player_is_dead = 0;
	Player_exploded = 0;
	obj_delete(Dead_player_camera-Objects);
	Dead_player_camera = NULL;
	select_cockpit(PlayerCfg.CockpitMode[0]);
	Viewer = Viewer_save;
	ConsoleObject->type = OBJ_PLAYER;
	ConsoleObject->flags = Player_flags_save;

	Assert((Control_type_save == CT_FLYING) || (Control_type_save == CT_SLEW));

	ConsoleObject->control_type = Control_type_save;
	ConsoleObject->render_type = Render_type_save;
	Players[Player_num].flags &= ~PLAYER_FLAGS_INVULNERABLE;
	Player_eggs_dropped = 0;

}

//	------------------------------------------------------------------------------------------------------------------
//	Camera is less than size of player away from
void set_camera_pos(vms_vector *camera_pos, object *objp)
{
	int	count = 0;
	fix	camera_player_dist;
	fix	far_scale;

	camera_player_dist = vm_vec_dist_quick(camera_pos, &objp->pos);

	if (camera_player_dist < Camera_to_player_dist_goal) { //2*objp->size) {
		//	Camera is too close to player object, so move it away.
		vms_vector	player_camera_vec;
		fvi_query	fq;
		fvi_info		hit_data;
		vms_vector	local_p1;

		vm_vec_sub(&player_camera_vec, camera_pos, &objp->pos);
		if ((player_camera_vec.x == 0) && (player_camera_vec.y == 0) && (player_camera_vec.z == 0))
			player_camera_vec.x += F1_0/16;

		hit_data.hit_type = HIT_WALL;
		far_scale = F1_0;

		while ((hit_data.hit_type != HIT_NONE) && (count++ < 6)) {
			vms_vector	closer_p1;
			vm_vec_normalize_quick(&player_camera_vec);
			vm_vec_scale(&player_camera_vec, Camera_to_player_dist_goal);

			fq.p0 = &objp->pos;
			vm_vec_add(&closer_p1, &objp->pos, &player_camera_vec);		//	This is the actual point we want to put the camera at.
			vm_vec_scale(&player_camera_vec, far_scale);						//	...but find a point 50% further away...
			vm_vec_add(&local_p1, &objp->pos, &player_camera_vec);		//	...so we won't have to do as many cuts.

			fq.p1 = &local_p1;
			fq.startseg = objp->segnum;
			fq.rad = 0;
			fq.thisobjnum = objp-Objects;
			fq.ignore_obj_list = NULL;
			fq.flags = 0;
			find_vector_intersection( &fq, &hit_data);

			if (hit_data.hit_type == HIT_NONE) {
				*camera_pos = closer_p1;
			} else {
				make_random_vector(&player_camera_vec);
				far_scale = 3*F1_0/2;
			}
		}
	}
}

extern void drop_player_eggs(object *objp);
extern int get_explosion_vclip(object *obj,int stage);
extern void multi_cap_objects();

//	------------------------------------------------------------------------------------------------------------------
void dead_player_frame(void)
{
	static fix	time_dead = 0;
	vms_vector	fvec;

	if (Player_is_dead)
	{
		time_dead += FrameTime;

		//	If unable to create camera at time of death, create now.
		if (Dead_player_camera == Viewer_save) {
			int		objnum;
			object	*player = &Objects[Players[Player_num].objnum];

			objnum = obj_create(OBJ_CAMERA, 0, player->segnum, &player->pos, &player->orient, 0, CT_NONE, MT_NONE, RT_NONE);

			if (objnum != -1)
				Viewer = Dead_player_camera = &Objects[objnum];
			else {
				Int3();
			}
		}		

		ConsoleObject->mtype.phys_info.rotvel.x = max(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead)/4;
		ConsoleObject->mtype.phys_info.rotvel.y = max(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead)/2;
		ConsoleObject->mtype.phys_info.rotvel.z = max(0, DEATH_SEQUENCE_EXPLODE_TIME - time_dead)/3;

		Camera_to_player_dist_goal = min(time_dead*8, F1_0*20) + ConsoleObject->size;

		set_camera_pos(&Dead_player_camera->pos, ConsoleObject);

		if (time_dead < DEATH_SEQUENCE_EXPLODE_TIME + F1_0 * 2)
		{
			vm_vec_sub(&fvec, &ConsoleObject->pos, &Dead_player_camera->pos);
			vm_vector_2_matrix(&Dead_player_camera->orient, &fvec, NULL, NULL);
			Dead_player_camera->mtype.phys_info = ConsoleObject->mtype.phys_info;

			if (Dead_player_camera->mtype.phys_info.flags & PF_WIGGLE)
			{
				Dead_player_camera->mtype.phys_info.flags = (Dead_player_camera->mtype.phys_info.flags & ~PF_WIGGLE);
			}

		}
		else
		{
			Dead_player_camera->movement_type = MT_PHYSICS;
		}

		if (time_dead > DEATH_SEQUENCE_EXPLODE_TIME) {
			if (!Player_exploded) {

				if (Players[Player_num].hostages_on_board > 1)
					HUD_init_message(HM_DEFAULT, TXT_SHIP_DESTROYED_2, Players[Player_num].hostages_on_board);
				else if (Players[Player_num].hostages_on_board == 1)
					HUD_init_message(HM_DEFAULT, TXT_SHIP_DESTROYED_1);
				else
					HUD_init_message(HM_DEFAULT, TXT_SHIP_DESTROYED_0);
				Players[Player_num].hostages_on_board = 0;

				Player_exploded = 1;
#ifdef NETWORK
				if (Game_mode & GM_NETWORK)
					multi_powcap_cap_objects();
#endif
				
				drop_player_eggs(ConsoleObject);
				Player_eggs_dropped = 1;
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
				{
					multi_send_player_explode(MULTI_PLAYER_EXPLODE);
				}
#endif

				explode_badass_player(ConsoleObject);

				//is this next line needed, given the badass call above?
				explode_object(ConsoleObject,0);
				ConsoleObject->flags &= ~OF_SHOULD_BE_DEAD;		//don't really kill player
				ConsoleObject->render_type = RT_NONE;				//..just make him disappear
				ConsoleObject->type = OBJ_GHOST;						//..and kill intersections
				Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
			}
		} else {
			if (d_rand() < FrameTime*4) {
				#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_create_explosion(Player_num);
				#endif
				create_small_fireball_on_object(ConsoleObject, F1_0, 1);
			}
		}


		if (Death_sequence_aborted) { //time_dead > DEATH_SEQUENCE_LENGTH) {
			if (!Player_eggs_dropped) {
			
#ifdef NETWORK
				if (Game_mode & GM_NETWORK)
					multi_powcap_cap_objects();
#endif
				
				drop_player_eggs(ConsoleObject);
				Player_eggs_dropped = 1;
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
				{
					multi_send_player_explode(MULTI_PLAYER_EXPLODE);
				}
#endif
			}

			DoPlayerDead();		//kill_player();
		}
	}
	else
		time_dead = 0;
}

//	------------------------------------------------------------------------------------------------------------------
void start_player_death_sequence(object *player)
{
	int	objnum;

	Assert(player == ConsoleObject);
	if ((Player_is_dead != 0) || (Dead_player_camera != NULL) || ((Game_mode & GM_MULTI) && (Players[Player_num].connected != CONNECT_PLAYING)))
		return;

	//Assert(Player_is_dead == 0);
	//Assert(Dead_player_camera == NULL);

	reset_rear_view();

	if (!(Game_mode & GM_MULTI))
		HUD_clear_messages();

	Death_sequence_aborted = 0;

	#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		int killer_pnum = Player_num;
		if (Players[Player_num].killer_objnum > 0 && Players[Player_num].killer_objnum < Highest_object_index)
			killer_pnum = Objects[Players[Player_num].killer_objnum].id;
		
		// If Hoard, increase number of orbs by 1. Only if you haven't killed yourself. This prevents cheating
		if (Game_mode & GM_HOARD)
			if (Players[Player_num].secondary_ammo[PROXIMITY_INDEX]<12)
				if (!(Players[Player_num].killer_objnum == Players[Player_num].objnum || ((Game_mode & GM_TEAM) && get_team(Player_num) == get_team(killer_pnum))))
					Players[Player_num].secondary_ammo[PROXIMITY_INDEX]++;
		multi_send_kill(Players[Player_num].objnum);
	}
	#endif
	
	PaletteRedAdd = 40;
	Player_is_dead = 1;

	//Players[Player_num].flags &= ~(PLAYER_FLAGS_AFTERBURNER);

	vm_vec_zero(&player->mtype.phys_info.rotthrust);
	vm_vec_zero(&player->mtype.phys_info.thrust);

	objnum = obj_create(OBJ_CAMERA, 0, player->segnum, &player->pos, &player->orient, 0, CT_NONE, MT_NONE, RT_NONE);
	Viewer_save = Viewer;
	if (objnum != -1)
		Viewer = Dead_player_camera = &Objects[objnum];
	else {
		Int3();
		Dead_player_camera = Viewer;
	}

	select_cockpit(CM_LETTERBOX);
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_letterbox();

	Player_flags_save = player->flags;
	Control_type_save = player->control_type;
	Render_type_save = player->render_type;

	player->flags &= ~OF_SHOULD_BE_DEAD;
//	Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
	player->control_type = CT_NONE;
	player->shields = F1_0*1000;

	PALETTE_FLASH_SET(0,0,0);
}

//	------------------------------------------------------------------------------------------------------------------
void obj_delete_all_that_should_be_dead()
{
	int i;
	object *objp;
	int		local_dead_player_object=-1;

	// Move all objects
	objp = Objects;

	for (i=0;i<=Highest_object_index;i++) {
		if ((objp->type!=OBJ_NONE) && (objp->flags&OF_SHOULD_BE_DEAD) )	{
			Assert(!(objp->type==OBJ_FIREBALL && objp->ctype.expl_info.delete_time!=-1));
			if (objp->type==OBJ_PLAYER) {
				if ( objp->id == Player_num ) {
					if (local_dead_player_object == -1) {
						start_player_death_sequence(objp);
						local_dead_player_object = objp-Objects;
					} else
						Int3();	//	Contact Mike: Illegal, killed player twice in this frame!
									// Ok to continue, won't start death sequence again!
					// kill_player();
				}
			} else {					
				obj_delete(i);
			}
		}
		objp++;
	}
}

//when an object has moved into a new segment, this function unlinks it
//from its old segment, and links it into the new segment
void obj_relink(int objnum,int newsegnum)
{

	Assert((objnum >= 0) && (objnum <= Highest_object_index));
	Assert((newsegnum <= Highest_segment_index) && (newsegnum >= 0));

	obj_unlink(objnum);

	obj_link(objnum,newsegnum);

}

// for getting out of messed up linking situations (i.e. caused by demo playback)
void obj_relink_all(void)
{
	int segnum;
	int objnum;
	object *obj;
	
	for (segnum=0; segnum <= Highest_segment_index; segnum++)
		Segments[segnum].objects = -1;
	
	for (objnum=0,obj=&Objects[0];objnum<=Highest_object_index;objnum++,obj++)
		if (obj->type != OBJ_NONE)
		{
			segnum = obj->segnum;
			obj->next = obj->prev = obj->segnum = -1;
			
			if (segnum > Highest_segment_index)
				segnum = 0;
			obj_link(objnum, segnum);
		}
}

//process a continuously-spinning object
void
spin_object(object *obj)
{
	vms_angvec rotangs;
	vms_matrix rotmat, new_pm;

	Assert(obj->movement_type == MT_SPINNING);

	rotangs.p = fixmul(obj->mtype.spin_rate.x,FrameTime);
	rotangs.h = fixmul(obj->mtype.spin_rate.y,FrameTime);
	rotangs.b = fixmul(obj->mtype.spin_rate.z,FrameTime);

	vm_angles_2_matrix(&rotmat,&rotangs);

	vm_matrix_x_matrix(&new_pm,&obj->orient,&rotmat);
	obj->orient = new_pm;

	check_and_fix_matrix(&obj->orient);
}

int Drop_afterburner_blob_flag;		//ugly hack
extern void multi_send_drop_blobs(char);
extern void fuelcen_check_for_goal (segment *);

//see if wall is volatile, and if so, cause damage to player
//returns true if player is in lava
int check_volatile_wall(object *obj,int segnum,int sidenum,vms_vector *hitpt);

//	Time at which this object last created afterburner blobs.
fix	Last_afterburner_time[MAX_OBJECTS];

//--------------------------------------------------------------------
//move an object for the current frame
void object_move_one( object * obj )
{
	int	previous_segment = obj->segnum;

	obj->last_pos = obj->pos;			// Save the current position

	if ((obj->type==OBJ_PLAYER) && (Player_num==obj->id))	{
#ifdef NETWORK
      if (Game_mode & GM_CAPTURE)
			 fuelcen_check_for_goal (&Segments[obj->segnum]);
      if (Game_mode & GM_HOARD)
			 fuelcen_check_for_hoard_goal (&Segments[obj->segnum]);
#endif

		fix fuel=fuelcen_give_fuel( &Segments[obj->segnum], INITIAL_ENERGY-Players[Player_num].energy );
		if (fuel > 0 )	{
			Players[Player_num].energy += fuel;
		}

		fix shields = repaircen_give_shields( &Segments[obj->segnum], INITIAL_SHIELDS-Players[Player_num].shields );
		if (shields > 0) {
			Players[Player_num].shields += shields;
		}
	}

	if (obj->lifeleft != IMMORTAL_TIME) //if not immortal...
		obj->lifeleft -= FrameTime; //...inevitable countdown towards death

	Drop_afterburner_blob_flag = 0;

	switch (obj->control_type) {

		case CT_NONE: break;

		case CT_FLYING:

			read_flying_controls( obj );

			break;

		case CT_REPAIRCEN: Int3();	// -- hey! these are no longer supported!! -- do_repair_sequence(obj); break;

		case CT_POWERUP: do_powerup_frame(obj); break;
	
		case CT_MORPH:			//morph implies AI
			do_morph_frame(obj);
			//NOTE: FALLS INTO AI HERE!!!!

		case CT_AI:
			//NOTE LINK TO CT_MORPH ABOVE!!!
			if (Game_suspended & SUSP_ROBOTS) return;
			do_ai_frame(obj);
			break;

		case CT_WEAPON:		Laser_do_weapon_sequence(obj); break;
		case CT_EXPLOSION:	do_explosion_sequence(obj); break;

		case CT_SLEW:
			if ( keyd_pressed[KEY_PAD5] ) slew_stop();
			if ( keyd_pressed[KEY_NUMLOCK] ) 		{
				slew_reset_orient( obj );
			}
			slew_frame(0 );		// Does velocity addition for us.
			break;


//		case CT_FLYTHROUGH:
//			do_flythrough(obj,0);			// HACK:do_flythrough should operate on an object!!!!
//			//check_object_seg(obj);
//			return;	// DON'T DO THE REST OF OBJECT STUFF SINCE THIS IS A SPECIAL CASE!!!
//			break;

		case CT_DEBRIS: do_debris_frame(obj); break;

		case CT_LIGHT: break;		//doesn't do anything

		case CT_REMOTE: break;		//doesn't do anything

		case CT_CNTRLCEN: do_controlcen_frame(obj); break;

		// D2X-XL
		case CT_WAYPOINT: break;

		default:

			Error("Unknown control type %d in object %i, sig/type/id = %i/%i/%i",obj->control_type, (int)(obj-Objects), obj->signature, obj->type, obj->id);

			break;

	}

	if (obj->lifeleft < 0 ) {		// We died of old age
		obj->flags |= OF_SHOULD_BE_DEAD;
		if ( obj->type==OBJ_WEAPON && Weapon_info[obj->id].damage_radius )
			explode_badass_weapon(obj,&obj->pos);
		else if ( obj->type==OBJ_ROBOT)	//make robots explode
			explode_object(obj,0);
	}

	if (obj->type == OBJ_NONE || obj->flags&OF_SHOULD_BE_DEAD)
		return;         // object has been deleted

	switch (obj->movement_type) {

		case MT_NONE:			break;				//this doesn't move

		case MT_PHYSICS:		do_physics_sim(obj);	break;	//move by physics

		case MT_SPINNING:		spin_object(obj); break;

	}

	//	If player and moved to another segment, see if hit any triggers.
	// also check in player under a lavafall
	if (obj->type == OBJ_PLAYER && obj->movement_type==MT_PHYSICS)	{

		if (previous_segment != obj->segnum) {
			int	connect_side,i;
#ifdef NETWORK
			int	old_level = Current_level_num;
#endif
			for (i=0;i<n_phys_segs-1;i++) {
				connect_side = find_connect_side(&Segments[phys_seglist[i+1]], &Segments[phys_seglist[i]]);
				if (connect_side != -1)
					check_trigger(&Segments[phys_seglist[i]], connect_side, obj-Objects,0);

				//maybe we've gone on to the next level.  if so, bail!
#ifdef NETWORK
				if (Current_level_num != old_level)
					return;
#endif
			}
		}

		{
			int sidemask,under_lavafall=0;
			static int lavafall_hiss_playing[MAX_PLAYERS]={0};

			sidemask = get_seg_masks(&obj->pos, obj->segnum, obj->size, __FILE__, __LINE__).sidemask;
			if (sidemask) {
				int sidenum,bit,wall_num;
	
				for (sidenum=0,bit=1;sidenum<6;bit<<=1,sidenum++)
					if ((sidemask & bit) && ((wall_num=Segments[obj->segnum].sides[sidenum].wall_num)!=-1) && Walls[wall_num].type==WALL_ILLUSION) {
						int type;
						if ((type=check_volatile_wall(obj,obj->segnum,sidenum,&obj->pos))!=0) {
							int sound = (type==1)?SOUND_LAVAFALL_HISS:SOUND_SHIP_IN_WATERFALL;
							under_lavafall = 1;
							if (!lavafall_hiss_playing[obj->id]) {
								digi_link_sound_to_object3( sound, obj-Objects, 1, F1_0, i2f(256), -1, -1);
								lavafall_hiss_playing[obj->id] = 1;
							}
						}
					}
			}
	
			if (!under_lavafall && lavafall_hiss_playing[obj->id]) {
				digi_kill_sound_linked_to_object( obj-Objects);
				lavafall_hiss_playing[obj->id] = 0;
			}
		}
	}

	//see if guided missile has flown through exit trigger
	if (obj==Guided_missile[Player_num] && obj->signature==Guided_missile_sig[Player_num]) {
		if (previous_segment != obj->segnum) {
			int	connect_side;
			connect_side = find_connect_side(&Segments[obj->segnum], &Segments[previous_segment]);
			if (connect_side != -1) {
				int wall_num,trigger_num;
				wall_num = Segments[previous_segment].sides[connect_side].wall_num;
				if ( wall_num != -1 ) {
					trigger_num = Walls[wall_num].trigger;
					if (trigger_num != -1)
						if (Triggers[trigger_num].type == TT_EXIT)
							Guided_missile[Player_num]->lifeleft = 0;
				}
			}
		}
	}

	if (Drop_afterburner_blob_flag) {
		Assert(obj==ConsoleObject);
		drop_afterburner_blobs(obj, 2, i2f(5)/2, -1);	//	-1 means use default lifetime
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_drop_blobs(Player_num);
#endif
		Drop_afterburner_blob_flag = 0;
	}

	if ((obj->type == OBJ_WEAPON) && (Weapon_info[obj->id].afterburner_size)) {
		int	objnum = obj-Objects;
		fix	vel = vm_vec_mag_quick(&obj->mtype.phys_info.velocity);
		fix	delay, lifetime;

		if (vel > F1_0*200)
			delay = F1_0/16;
		else if (vel > F1_0*40)
			delay = fixdiv(F1_0*13,vel);
		else
			delay = F1_0/4;

		lifetime = (delay * 3)/2;
		if (!(Game_mode & GM_MULTI)) {
			delay /= 2;
			lifetime *= 2;
		}

		if ((Last_afterburner_time[objnum] + delay < GameTime) || (Last_afterburner_time[objnum] > GameTime)) {
			drop_afterburner_blobs(obj, 1, i2f(Weapon_info[obj->id].afterburner_size)/16, lifetime);
			Last_afterburner_time[objnum] = GameTime;
		}
	}
}

//--------------------------------------------------------------------
//move all objects for the current frame
void object_move_all()
{
	int i;
	object *objp;

	free_object_slots();

	obj_delete_all_that_should_be_dead();

	if (PlayerCfg.AutoLeveling)
		ConsoleObject->mtype.phys_info.flags |= PF_LEVELLING;
	else
		ConsoleObject->mtype.phys_info.flags &= ~PF_LEVELLING;

	// Move all objects
	objp = Objects;

	for (i=0;i<=Highest_object_index;i++) {
		if ( (objp->type != OBJ_NONE) && (!(objp->flags&OF_SHOULD_BE_DEAD)) )	{
			object_move_one( objp );
		}
		objp++;
	}
}

//called after load.  Takes number of objects,  and objects should be
//compressed.  resets free list, marks unused objects as unused
void reset_objects(int n_objs)
{
	int i;

	num_objects = n_objs;

	Assert(num_objects>0);

	for (i=num_objects;i<MAX_OBJECTS;i++) {
		free_obj_list[i] = i;
		memset( &Objects[i], 0, sizeof(object) );
		Objects[i].type = OBJ_NONE;
		Objects[i].segnum = -1;
	}

	Highest_object_index = num_objects-1;

	Debris_object_count = 0;
}

//Tries to find a segment for an object, using find_point_seg()
int find_object_seg(object * obj )
{
	return find_point_seg(&obj->pos,obj->segnum);
}


//If an object is in a segment, set its segnum field and make sure it's
//properly linked.  If not in any segment, returns 0, else 1.
//callers should generally use find_vector_intersection()
int update_object_seg(object * obj )
{
	int newseg;

	newseg = find_object_seg(obj);

	if (newseg == -1)
		return 0;

	if ( newseg != obj->segnum )
		obj_relink(obj-Objects, newseg );

	return 1;
}


//go through all objects and make sure they have the correct segment numbers
void
fix_object_segs()
{
	int i;

	for (i=0;i<=Highest_object_index;i++) {
		if (Objects[i].type != OBJ_NONE &&
			Objects[i].type != OBJ_CAMBOT &&
			Objects[i].type != OBJ_EFFECT)
		{
			if (update_object_seg(&Objects[i]) == 0) {
				Int3();
				compute_segment_center(&Objects[i].pos,&Segments[Objects[i].segnum]);
			}
		}
	}
}


//--unused-- void object_use_new_object_list( object * new_list )
//--unused-- {
//--unused-- 	int i, segnum;
//--unused-- 	object *obj;
//--unused--
//--unused-- 	// First, unlink all the old objects for the segments array
//--unused-- 	for (segnum=0; segnum <= Highest_segment_index; segnum++) {
//--unused-- 		Segments[segnum].objects = -1;
//--unused-- 	}
//--unused-- 	// Then, erase all the objects
//--unused-- 	reset_objects(1);
//--unused--
//--unused-- 	// Fill in the object array
//--unused-- 	memcpy( Objects, new_list, sizeof(object)*MAX_OBJECTS );
//--unused--
//--unused-- 	Highest_object_index=-1;
//--unused--
//--unused-- 	// Relink 'em
//--unused-- 	for (i=0; i<MAX_OBJECTS; i++ )	{
//--unused-- 		obj = &Objects[i];
//--unused-- 		if ( obj->type != OBJ_NONE )	{
//--unused-- 			num_objects++;
//--unused-- 			Highest_object_index = i;
//--unused-- 			segnum = obj->segnum;
//--unused-- 			obj->next = obj->prev = obj->segnum = -1;
//--unused-- 			obj_link(i,segnum);
//--unused-- 		} else {
//--unused-- 			obj->next = obj->prev = obj->segnum = -1;
//--unused-- 		}
//--unused-- 	}
//--unused-- 	
//--unused-- }

//delete objects, such as weapons & explosions, that shouldn't stay between levels
//if clear_all is set, clear even proximity bombs
void clear_transient_objects(int clear_all)
{
	int objnum;
	object *obj;

	for (objnum=0,obj=&Objects[0];objnum<=Highest_object_index;objnum++,obj++)
		if (((obj->type == OBJ_WEAPON) && !(Weapon_info[obj->id].flags&WIF_PLACABLE) && (clear_all || ((obj->id != PROXIMITY_ID) && (obj->id != SUPERPROX_ID)))) ||
			 obj->type == OBJ_FIREBALL ||
			 obj->type == OBJ_DEBRIS ||
			 obj->type == OBJ_DEBRIS ||
			 (obj->type!=OBJ_NONE && obj->flags & OF_EXPLODING)) {
			obj_delete(objnum);
		}
}

//attaches an object, such as a fireball, to another object, such as a robot
void obj_attach(object *parent,object *sub)
{
	Assert(sub->type == OBJ_FIREBALL);
	Assert(sub->control_type == CT_EXPLOSION);

	Assert(sub->ctype.expl_info.next_attach==-1);
	Assert(sub->ctype.expl_info.prev_attach==-1);

	Assert(parent->attached_obj==-1 || Objects[parent->attached_obj].ctype.expl_info.prev_attach==-1);

	sub->ctype.expl_info.next_attach = parent->attached_obj;

	if (sub->ctype.expl_info.next_attach != -1)
		Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach = sub-Objects;

	parent->attached_obj = sub-Objects;

	sub->ctype.expl_info.attach_parent = parent-Objects;
	sub->flags |= OF_ATTACHED;

	Assert(sub->ctype.expl_info.next_attach != sub-Objects);
	Assert(sub->ctype.expl_info.prev_attach != sub-Objects);
}

//dettaches one object
void obj_detach_one(object *sub)
{
	Assert(sub->flags & OF_ATTACHED);
	Assert(sub->ctype.expl_info.attach_parent != -1);

	if ((Objects[sub->ctype.expl_info.attach_parent].type == OBJ_NONE) || (Objects[sub->ctype.expl_info.attach_parent].attached_obj == -1))
	{
		sub->flags &= ~OF_ATTACHED;
		return;
	}

	if (sub->ctype.expl_info.next_attach != -1) {
		Assert(Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach==sub-Objects);
		Objects[sub->ctype.expl_info.next_attach].ctype.expl_info.prev_attach = sub->ctype.expl_info.prev_attach;
	}

	if (sub->ctype.expl_info.prev_attach != -1) {
		Assert(Objects[sub->ctype.expl_info.prev_attach].ctype.expl_info.next_attach==sub-Objects);
		Objects[sub->ctype.expl_info.prev_attach].ctype.expl_info.next_attach = sub->ctype.expl_info.next_attach;
	}
	else {
		Assert(Objects[sub->ctype.expl_info.attach_parent].attached_obj==sub-Objects);
		Objects[sub->ctype.expl_info.attach_parent].attached_obj = sub->ctype.expl_info.next_attach;
	}

	sub->ctype.expl_info.next_attach = sub->ctype.expl_info.prev_attach = -1;
	sub->flags &= ~OF_ATTACHED;

}

//dettaches all objects from this object
void obj_detach_all(object *parent)
{
	while (parent->attached_obj != -1)
		obj_detach_one(&Objects[parent->attached_obj]);
}

//creates a marker object in the world.  returns the object number
int drop_marker_object(vms_vector *pos,int segnum,vms_matrix *orient, int marker_num)
{
	int objnum;

	Assert(Marker_model_num != -1);

	objnum = obj_create(OBJ_MARKER, marker_num, segnum, pos, orient, Polygon_models[Marker_model_num].rad, CT_NONE, MT_SPINNING, RT_POLYOBJ);

	if (objnum >= 0) {
		object *obj = &Objects[objnum];

		obj->rtype.pobj_info.model_num = Marker_model_num;

		fix scale = F1_0 / 2;
		vms_vector spin_vec = {0};
		if (objnum & 1)
			vm_vec_scale_add2(&spin_vec, &obj->orient.fvec, (objnum & 8)  ? scale : -scale);
		if (objnum & 2)
			vm_vec_scale_add2(&spin_vec, &obj->orient.uvec, (objnum & 16) ? scale : -scale);
		if (objnum & 4)
			vm_vec_scale_add2(&spin_vec, &obj->orient.rvec, (objnum & 32) ? scale : -scale);
		obj->mtype.spin_rate = spin_vec;

		//	MK, 10/16/95: Using lifeleft to make it flash, thus able to trim lightlevel from all objects.
		obj->lifeleft = IMMORTAL_TIME - 1;
	}

	return objnum;	
}

extern int Ai_last_missile_camera;

//	*viewer is a viewer, probably a missile.
//	wake up all robots that were rendered last frame subject to some constraints.
void wake_up_rendered_objects(object *viewer, int window_num)
{
	int	i;

	//	Make sure that we are processing current data.
	if (timer_query() != Window_rendered_data[window_num].time) {
		return;
	}

	Ai_last_missile_camera = viewer-Objects;

	for (i=0; i<Window_rendered_data[window_num].num_objects; i++) {
		int	objnum;
		object *objp;
		int	fcval = d_tick_count & 3;

		objnum = Window_rendered_data[window_num].rendered_objects[i];
		if ((objnum & 3) == fcval) {
			objp = &Objects[objnum];
	
			if (objp->type == OBJ_ROBOT) {
				if (vm_vec_dist_quick(&viewer->pos, &objp->pos) < F1_0*100) {
					ai_local		*ailp = &Ai_local_info[objnum];
					if (ailp->player_awareness_type == 0) {
						objp->ctype.ai_info.SUB_FLAGS |= SUB_FLAGS_CAMERA_AWAKE;
						ailp->player_awareness_type = PA_WEAPON_ROBOT_COLLISION;
						ailp->player_awareness_time = F1_0*3;
						ailp->previous_visibility = 2;
					}
				}
			}
		}
	}
}






