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
 * Info on canvases, screens, etc.
 *
 */


#ifndef _SCREEN_H
#define _SCREEN_H

#include "gr.h"

// What graphics modes the game & editor open

//from editor.c
extern grs_canvas *Canv_editor;		// the full on-scrren editor canvas
extern grs_canvas *Canv_editor_game;	// the game window on the editor screen

//About the screen

extern grs_canvas		Screen_3d_window;		// The rectangle for rendering the mine to

void game_init_render_buffers(int render_max_w, int render_max_h);
void set_display_mode(int mode);
extern int Default_display_mode;

#endif /* _SCREENS_H */
