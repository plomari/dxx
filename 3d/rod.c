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
 * Rod routines
 * 
 */


#include "3d.h"
#include "globvars.h"
#include "fix.h"

grs_point blob_vertices[4];
g3s_point rod_points[4];
static const g3s_point *rod_point_list[] = {&rod_points[0],&rod_points[1],&rod_points[2],&rod_points[3]};

g3s_uvl uvl_list[4] = { { 0x0200,0x0200,0 },
			{ 0xfe00,0x0200,0 },
			{ 0xfe00,0xfe00,0 },
			{ 0x0200,0xfe00,0 } };
g3s_lrgb lrgb_list[4] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };

//compute the corners of a rod.  fills in vertbuf.
int calc_rod_corners(g3s_point *bot_point,fix bot_width,g3s_point *top_point,fix top_width)
{
	vms_vector delta_vec,top,tempv,rod_norm;
	ubyte codes_and;
	int i;

	//compute vector from one point to other, do cross product with vector
	//from eye to get perpendiclar

	vm_vec_sub(&delta_vec,&bot_point->p3_vec,&top_point->p3_vec);

	//unscale for aspect

	delta_vec.x = fixdiv(delta_vec.x,Matrix_scale.x);
	delta_vec.y = fixdiv(delta_vec.y,Matrix_scale.y);

	//calc perp vector

	//do lots of normalizing to prevent overflowing.  When this code works,
	//it should be optimized

	vm_vec_normalize(&delta_vec);

	vm_vec_copy_normalize(&top,&top_point->p3_vec);

	vm_vec_cross(&rod_norm,&delta_vec,&top);

	vm_vec_normalize(&rod_norm);

	//scale for aspect

	rod_norm.x = fixmul(rod_norm.x,Matrix_scale.x);
	rod_norm.y = fixmul(rod_norm.y,Matrix_scale.y);

	//now we have the usable edge.  generate four points

	//top points

	vm_vec_copy_scale(&tempv,&rod_norm,top_width);
	tempv.z = 0;

	vm_vec_add(&rod_points[0].p3_vec,&top_point->p3_vec,&tempv);
	vm_vec_sub(&rod_points[1].p3_vec,&top_point->p3_vec,&tempv);

	vm_vec_copy_scale(&tempv,&rod_norm,bot_width);
	tempv.z = 0;

	vm_vec_sub(&rod_points[2].p3_vec,&bot_point->p3_vec,&tempv);
	vm_vec_add(&rod_points[3].p3_vec,&bot_point->p3_vec,&tempv);


	//now code the four points

	for (i=0,codes_and=0xff;i<4;i++)
		codes_and &= g3_code_point(&rod_points[i]);

	if (codes_and)
		return 1;		//1 means off screen

	//clear flags for new points (not projected)

	for (i=0;i<4;i++)
		rod_points[i].p3_flags = 0;

	return 0;
}

//draw a bitmap object that is always facing you
//returns 1 if off screen, 0 if drew
bool g3_draw_rod_tmap(grs_bitmap *bitmap,g3s_point *bot_point,fix bot_width,g3s_point *top_point,fix top_width,g3s_lrgb light)
{
	if (calc_rod_corners(bot_point,bot_width,top_point,top_width))
		return 0;

	uvl_list[0].l = uvl_list[1].l = uvl_list[2].l = uvl_list[3].l = (light.r+light.g+light.b)/3;
	lrgb_list[0].r = lrgb_list[1].r = lrgb_list[2].r = lrgb_list[3].r = light.r;
	lrgb_list[0].g = lrgb_list[1].g = lrgb_list[2].g = lrgb_list[3].g = light.g;
	lrgb_list[0].b = lrgb_list[1].b = lrgb_list[2].b = lrgb_list[3].b = light.b;

	return g3_draw_tmap(4,rod_point_list,uvl_list,lrgb_list,bitmap);
}
