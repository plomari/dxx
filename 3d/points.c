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
 * Routines for point definition, rotation, etc.
 * 
 */


#include "3d.h"
#include "globvars.h"


//code a point.  fills in the p3_codes field of the point, and returns the codes
ubyte g3_code_point(g3s_point *p)
{
	ubyte cc=0;

	if (p->p3_x > p->p3_z)
		cc |= CC_OFF_RIGHT;

	if (p->p3_y > p->p3_z)
		cc |= CC_OFF_TOP;

	if (p->p3_x < -p->p3_z)
		cc |= CC_OFF_LEFT;

	if (p->p3_y < -p->p3_z)
		cc |= CC_OFF_BOT;

	if (p->p3_z < 0)
		cc |= CC_BEHIND;

	return p->p3_codes = cc;

}

//rotates a point. returns codes.  does not check if already rotated
ubyte g3_rotate_point(g3s_point *dest,const vms_vector *src)
{
	vms_vector tempv;

	vm_vec_sub(&tempv,src,&View_position);

	vm_vec_rotate(&dest->p3_vec,&tempv,&View_matrix);

	dest->p3_flags = 0;	//no projected

	return g3_code_point(dest);

}

//checks for overflow & divides if ok, filling in r
//returns true if div is ok, else false
int checkmuldiv(fix *r,fix a,fix b,fix c)
{
	int64_t q = a * (int64_t)b;

	if (!c)
		return 0;

	q = q / c;

	if (q < FIX_MIN || q > FIX_MAX)
		return 0;

	*r = q;
	return 1;
}

static int checkadd(fix *r, fix a, fix b)
{
	int64_t res = (int64_t)a + (int64_t)b;

	if (res < FIX_MIN || res > FIX_MAX)
		return 0;

	*r = res;
	return 1;
}

static int checksub(fix *r, fix a, fix b)
{
	int64_t res = (int64_t)a - (int64_t)b;

	if (res < FIX_MIN || res > FIX_MAX)
		return 0;

	*r = res;
	return 1;
}

//projects a point
void g3_project_point(g3s_point *p)
{
	fix tx,ty;

	if (p->p3_flags & PF_PROJECTED || p->p3_codes & CC_BEHIND)
		return;

	if (checkmuldiv(&tx,p->p3_x,Canv_w2,p->p3_z) && checkmuldiv(&ty,p->p3_y,Canv_h2,p->p3_z) &&
		checkadd(&p->p3_sx, Canv_w2, tx) && checksub(&p->p3_sy, Canv_h2, ty))
	{
		p->p3_flags |= PF_PROJECTED;
	}
	else
		p->p3_flags |= PF_OVERFLOW;
}

vms_vector *g3_rotate_delta_vec(vms_vector *dest,const vms_vector *src)
{
	return vm_vec_rotate(dest,src,&View_matrix);
}

ubyte g3_add_delta_vec(g3s_point *dest,const g3s_point *src,const vms_vector *deltav)
{
	vm_vec_add(&dest->p3_vec,&src->p3_vec,deltav);

	dest->p3_flags = 0;		//not projected

	return g3_code_point(dest);
}

//calculate the depth of a point - returns the z coord of the rotated point
fix g3_calc_point_depth(const vms_vector *pnt)
{
	return ((pnt->x - View_position.x) * (int64_t)View_matrix.fvec.x +
		    (pnt->y - View_position.y) * (int64_t)View_matrix.fvec.y +
		    (pnt->z - View_position.z) * (int64_t)View_matrix.fvec.z) / 65536;
}
