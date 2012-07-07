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
 * Drawing routines
 * 
 */


#include "dxxerror.h"

#include "3d.h"
#include "globvars.h"
#include "texmap.h"

//returns true if a plane is facing the viewer. takes the unrotated surface 
//normal of the plane, and a point on it.  The normal need not be normalized
bool g3_check_normal_facing(vms_vector *v,vms_vector *norm)
{
	vms_vector tempv;

	vm_vec_sub(&tempv,&View_position,v);

	return (vm_vec_dot(&tempv,norm) > 0);
}

bool do_facing_check(vms_vector *norm,g3s_point **vertlist,vms_vector *p)
{
	if (norm) {		//have normal

		Assert(norm->x || norm->y || norm->z);

		return g3_check_normal_facing(p,norm);
	}
	else {	//normal not specified, so must compute

		vms_vector tempv;

		//get three points (rotated) and compute normal

		vm_vec_perp(&tempv,&vertlist[0]->p3_vec,&vertlist[1]->p3_vec,&vertlist[2]->p3_vec);

		return (vm_vec_dot(&tempv,&vertlist[1]->p3_vec) < 0);
	}
}

//like g3_draw_poly(), but checks to see if facing.  If surface normal is
//NULL, this routine must compute it, which will be slow.  It is better to 
//pre-compute the normal, and pass it to this function.  When the normal
//is passed, this function works like g3_check_normal_facing() plus
//g3_draw_poly().
//returns -1 if not facing, 1 if off screen, 0 if drew
bool g3_check_and_draw_poly(int nv,g3s_point **pointlist,vms_vector *norm,vms_vector *pnt)
{
	if (do_facing_check(norm,pointlist,pnt))
		return g3_draw_poly(nv,pointlist);
	else
		return 255;
}

bool g3_check_and_draw_tmap(int nv,g3s_point **pointlist,g3s_uvl *uvl_list,g3s_lrgb *light_rgb,grs_bitmap *bm,vms_vector *norm,vms_vector *pnt)
{
	if (do_facing_check(norm,pointlist,pnt))
		return g3_draw_tmap(nv,pointlist,uvl_list,light_rgb,bm);
	else
		return 255;
}
