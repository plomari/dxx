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
 * Header for rendering-based functions
 *
 */


#ifndef _RENDER_H
#define _RENDER_H

#include "3d.h"

#include "object.h"

#define MAX_RENDER_SEGS     1500

extern int Render_depth; //how many segments deep to render
extern int Simple_model_threshhold_scale; // switch to simpler model when the object has depth greater than this value times its radius.
extern int Max_debris_objects; // How many debris objects to create

void render_frame(fix eye_offset, int window_num);  //draws the world into the current canvas

// cycle the flashing light for when mine destroyed
void flash_frame();

int find_seg_side_face(short x,short y,int *seg,int *side,int *face,int *poly);

// these functions change different rendering parameters
// all return the new value of the parameter

// how may levels deep to render
int inc_render_depth(void);
int dec_render_depth(void);
int reset_render_depth(void);

// how many levels deep to render in perspective
int inc_perspective_depth(void);
int dec_perspective_depth(void);
int reset_perspective_depth(void);

// misc toggles
int toggle_outline_mode(void);
int toggle_show_only_curside(void);

// When any render function needs to know what's looking at it, it
// should access Render_viewer_object members.
extern fix Render_zoom;     // the player's zoom factor

extern int N_render_segs;
extern short Render_list[MAX_RENDER_SEGS];

//
// Routines for conditionally rotating & projecting points
//

// This must be called at the start of the frame if rotate_list() will be used
void render_start_frame(void);

// Given a list of point numbers, rotate any that haven't been rotated
// this frame
g3s_codes rotate_list(int nv, int *pointnumlist);

// Given a list of point numbers, project any that haven't been projected
void project_list(int nv, int *pointnumlist);

extern void render_mine(int start_seg_num, fix eye_offset, int window_num);

extern void update_rendered_data(int window_num, object *viewer, int rear_view_flag);

#endif /* _RENDER_H */
