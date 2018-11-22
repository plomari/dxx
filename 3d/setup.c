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
 * Setup for 3d library
 * 
 */


#include <stdlib.h>

#include "dxxerror.h"

#include "3d.h"
#include "globvars.h"

#include "ogl.h"

//start the frame
void g3_start_frame(void)
{
	fix s;

	//set int w,h & fixed-point w,h/2
	Canv_w2 = grd_curcanv->cv_w << 15;
	Canv_h2 = grd_curcanv->cv_h << 15;

	//compute aspect ratio for this canvas

	s = fixmuldiv(grd_curscreen->sc_aspect, grd_curcanv->cv_h, grd_curcanv->cv_w);

	if (s <= f1_0) {	   //scale x
		Window_scale.x = s;
		Window_scale.y = f1_0;
	}
	else {
		Window_scale.y = fixdiv(f1_0,s);
		Window_scale.x = f1_0;
	}
	
	Window_scale.z = f1_0;		//always 1

	ogl_prepare_state_3d();
}

void g3_end_frame(void)
{
	ogl_prepare_state_2d();
}
