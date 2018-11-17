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
 * Game loop for Inferno
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>
#include <setjmp.h>

#include "ogl_init.h"

#include "pstypes.h"
#include "console.h"
#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "dxxerror.h"
#include "joy.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "fuelcen.h"
#include "digi.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "newmenu.h"
#include "gamefont.h"
#include "endlevel.h"
#include "kconfig.h"
#include "mouse.h"
#include "switch.h"
#include "controls.h"
#include "songs.h"
#include "gamepal.h"

#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "fvi.h"
#include "ai.h"
#include "robot.h"
#include "playsave.h"
#include "fix.h"
#include "hudmsg.h"
#include "movie.h"
#include "event.h"
#include "window.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#ifdef NETWORK
extern char IWasKicked;
#endif

#ifndef NDEBUG
int	Mark_count = 0;                 // number of debugging marks set
#endif

static fix64 last_timer_value=0;
fix ThisLevelTime=0;

grs_canvas	Screen_3d_window;							// The rectangle for rendering the mine to

int	force_cockpit_redraw=0;
int	PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

//	Toggle_var points at a variable which gets !ed on del-T press.
int	Dummy_var;
int	*Toggle_var = &Dummy_var;

#ifdef EDITOR
//flag for whether initial fade-in has been done
char	faded_in;
#endif

int	Game_suspended=0; //if non-zero, nothing moves but player
fix	Auto_fire_fusion_cannon_time = 0;
fix	Fusion_charge = 0;
int	Game_turbo_mode = 0;
int	Game_mode = GM_GAME_OVER;
int	Global_laser_firing_count = 0;
int	Global_missile_firing_count = 0;
fix	Next_flare_fire_time = 0;
#define	FLARE_BIG_DELAY	(F1_0*2)

//	Function prototypes for GAME.C exclusively.

void GameProcessFrame(void);
void FireLaser(void);
void slide_textures(void);
void powerup_grab_cheat_all(void);
void game_init_render_sub_buffers(int x, int y, int w, int h);

//	Other functions
extern void multi_check_for_killgoal_winner();

extern int ReadControls(d_event *event);		// located in gamecntl.c
extern void do_final_boss_frame(void);


// text functions

extern ubyte DefiningMarkerMessage;
extern char Marker_input[];

//	==============================================================================================

//this is called once per game
void init_game()
{
	init_objects();

	init_special_effects();

	init_exploding_walls();
}


void reset_palette_add()
{
	PaletteRedAdd 		= 0;
	PaletteGreenAdd	= 0;
	PaletteBlueAdd		= 0;
}


u_int32_t Game_screen_mode = SM(640,480);

//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void init_cockpit()
{
	//Initialize the on-screen canvases

	if (!Game_wind)
		return;

#ifndef OGL
	if ( Game_screen_mode != (GameArg.GfxHiresGFXAvailable? SM(640,480) : SM(320,200)) && PlayerCfg.CockpitMode[1] != CM_LETTERBOX) {
		PlayerCfg.CockpitMode[1] = CM_FULL_SCREEN;
	}
#endif

	gr_set_current_canvas(NULL);

	switch( PlayerCfg.CockpitMode[1] ) {
		case CM_FULL_COCKPIT:
			game_init_render_sub_buffers(0, 0, SWIDTH, (SHEIGHT*2)/3);
			break;

		case CM_REAR_VIEW:
		{	int x1 = 0, y1 = 0, x2 = SWIDTH, y2 = (SHEIGHT*2)/3;
			grs_bitmap *bm;

			PIGGY_PAGE_IN(cockpit_bitmap[PlayerCfg.CockpitMode[1]+(HIRESMODE?(Num_cockpits/2):0)]);
			bm=&GameBitmaps[cockpit_bitmap[PlayerCfg.CockpitMode[1]+(HIRESMODE?(Num_cockpits/2):0)].index];
			gr_bitblt_find_transparent_area(bm, &x1, &y1, &x2, &y2);
			game_init_render_sub_buffers(x1*((float)SWIDTH/bm->bm_w), y1*((float)SHEIGHT/bm->bm_h), (x2-x1+1)*((float)SWIDTH/bm->bm_w), (y2-y1+2)*((float)SHEIGHT/bm->bm_h));
			break;
		}

		case CM_FULL_SCREEN:
			game_init_render_sub_buffers(0, 0, SWIDTH, SHEIGHT);
			break;

		case CM_STATUS_BAR:
			game_init_render_sub_buffers( 0, 0, SWIDTH, (HIRESMODE?(SHEIGHT*2)/2.6:(SHEIGHT*2)/2.72) );
			break;

		case CM_LETTERBOX:	{
			int x,y,w,h;

			x = 0; w = SM_W(Game_screen_mode);
			h = (SM_H(Game_screen_mode) * 3) / 4; // true letterbox size (16:9)
			y = (SM_H(Game_screen_mode)-h)/2;

			gr_rect(x,0,w,SM_H(Game_screen_mode)-h);
			gr_rect(x,SM_H(Game_screen_mode)-h,w,SM_H(Game_screen_mode));

			game_init_render_sub_buffers( x, y, w, h );
			break;
		}
	}

	gr_set_current_canvas(NULL);
}

//selects a given cockpit (or lack of one).  See types in game.h
void select_cockpit(int mode)
{
	if (mode != PlayerCfg.CockpitMode[1]) {		//new mode
		PlayerCfg.CockpitMode[1]=mode;
		init_cockpit();
	}
}

//force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit()
{
	force_cockpit_redraw=1;
	last_drawn_cockpit = -1;
}

void game_init_render_sub_buffers( int x, int y, int w, int h )
{
	gr_clear_canvas(0);
	gr_init_sub_canvas( &Screen_3d_window, &grd_curscreen->sc_canvas, x, y, w, h );
}


// Sets up the canvases we will be rendering to
void game_init_render_buffers(int render_w, int render_h)
{
	game_init_render_sub_buffers( 0, 0, render_w, render_h );
}

static int time_paused=0;

void stop_time()
{
	if (time_paused==0) {
		fix64 time;
		timer_update();
		time = timer_query();
		last_timer_value = time - last_timer_value;
		if (last_timer_value < 0) {
			last_timer_value = 0;
		}
	}
	time_paused++;
}

void start_time()
{
	time_paused--;
	Assert(time_paused >= 0);
	if (time_paused==0) {
		fix64 time;
		timer_update();
		time = timer_query();
		last_timer_value = time - last_timer_value;
	}
}

void game_flush_inputs()
{
	int dx,dy,dz;
	joy_flush();
	mouse_flush();
	mouse_get_delta( &dx, &dy, &dz );	// Read mouse
	memset(&Controls,0,sizeof(control_info));
}

/*
 * timer that every 50ms sets d_tick_step true and increments d_tick_count
 */
void calc_d_tick()
{
	static fix timer = 0;

	d_tick_step = 0;

	timer += FrameTime;
	if (timer >= F1_0/20)
	{
		d_tick_step = 1;
		d_tick_count++;
		if (d_tick_count > 1000000)
			d_tick_count = 0;
		timer = (timer-(F1_0/20));
	}
}

void reset_time()
{
	timer_update();
	last_timer_value = timer_query();
}

void calc_frame_time()
{
	fix64 timer_value;
	fix last_frametime = FrameTime;

	timer_update();
	timer_value = timer_query();
	FrameTime = timer_value - last_timer_value;

	while (FrameTime < f1_0 / (GameCfg.VSync?MAXIMUM_FPS:GameArg.SysMaxFPS))
	{
		if (GameArg.SysUseNiceFPS && !GameCfg.VSync)
			timer_delay(f1_0 / GameArg.SysMaxFPS - FrameTime);
		timer_update();
		timer_value = timer_query();
		FrameTime = timer_value - last_timer_value;
	}

	if ( Game_turbo_mode )
		FrameTime *= 2;

	last_timer_value = timer_value;

	if (FrameTime < 0)				//if bogus frametime...
		FrameTime = (last_frametime==0?1:last_frametime);		//...then use time from last frame

	GameTime += FrameTime;

	if (GameTime < 0 || GameTime > i2f(0x7fff - 600))
		GameTime = FrameTime;	//wrap when goes negative, or ~9hrs

	calc_d_tick();
}

void move_player_2_segment(segment *seg,int side)
{
	vms_vector vp;

	compute_segment_center(&ConsoleObject->pos,seg);
	compute_center_point_on_side(&vp,seg,side);
	vm_vec_sub2(&vp,&ConsoleObject->pos);
	vm_vector_2_matrix(&ConsoleObject->orient,&vp,NULL,NULL);

	obj_relink( ConsoleObject-Objects, SEG_PTR_2_NUM(seg) );

}

void do_photos();
void level_with_floor();

//initialize flying
void fly_init(object *obj)
{
	obj->control_type = CT_FLYING;
	obj->movement_type = MT_PHYSICS;

	vm_vec_zero(&obj->mtype.phys_info.velocity);
	vm_vec_zero(&obj->mtype.phys_info.thrust);
	vm_vec_zero(&obj->mtype.phys_info.rotvel);
	vm_vec_zero(&obj->mtype.phys_info.rotthrust);
}

void test_anim_states();

extern int been_in_editor;

//	------------------------------------------------------------------------------------
void do_cloak_stuff(void)
{
	int i;

	for (i = 0; i < N_players; i++)
		if (Players[i].flags & PLAYER_FLAGS_CLOAKED) {
			if (Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime < 0 &&
				Players[Player_num].cloak_time+CLOAK_TIME_MAX-GameTime > -F1_0*2)
			{
				Players[i].flags &= ~PLAYER_FLAGS_CLOAKED;
				if (i == Player_num) {
					digi_play_sample( SOUND_CLOAK_OFF, F1_0);
#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_CLOAK_OFF, F1_0);
					maybe_drop_net_powerup(POW_CLOAK);
					if ( Newdemo_state != ND_STATE_PLAYBACK )
						multi_send_decloak(); // For demo recording
#endif
				}
			}
		}
}

int FakingInvul=0;

//	------------------------------------------------------------------------------------
void do_invulnerable_stuff(void)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE) {
		if (Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime < 0 &&
			Players[Player_num].invulnerable_time+INVULNERABLE_TIME_MAX-GameTime > -F1_0*2)
		{
			Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
			if (FakingInvul==0)
			{
				digi_play_sample( SOUND_INVULNERABILITY_OFF, F1_0);
				#ifdef NETWORK
				if (Game_mode & GM_MULTI)
				{
					multi_send_play_sound(SOUND_INVULNERABILITY_OFF, F1_0);
					maybe_drop_net_powerup(POW_INVULNERABILITY);
				}
				#endif
			}
			FakingInvul=0;
		}
	}
}

ubyte	Last_afterburner_state = 0;
fix Last_afterburner_charge = 0;

#define AFTERBURNER_LOOP_START	((GameArg.SndDigiSampleRate==SAMPLE_RATE_22K)?32027:(32027/2))		//20098
#define AFTERBURNER_LOOP_END		((GameArg.SndDigiSampleRate==SAMPLE_RATE_22K)?48452:(48452/2))		//25776

int	Ab_scale = 4;

//	------------------------------------------------------------------------------------
#ifdef NETWORK
extern void multi_send_sound_function (char,char);
#endif

void do_afterburner_stuff(void)
{
	static sbyte func_play = 0;

	if (!(Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER))
		Afterburner_charge = 0;

	if (Endlevel_sequence || Player_is_dead)
	{
		digi_kill_sound_linked_to_object( Players[Player_num].objnum);
#ifdef NETWORK
		if (Game_mode & GM_MULTI && func_play)
		{
			multi_send_sound_function (0,0);
			func_play = 0;
		}
#endif
	}

	if ((Controls.afterburner_state != Last_afterburner_state && Last_afterburner_charge) || (Last_afterburner_state && Last_afterburner_charge && !Afterburner_charge)) {

		if (Afterburner_charge && Controls.afterburner_state && (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)) {
			digi_link_sound_to_object3( SOUND_AFTERBURNER_IGNITE, Players[Player_num].objnum, 1, F1_0, i2f(256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END );
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
			{
				multi_send_sound_function (3,SOUND_AFTERBURNER_IGNITE);
				func_play = 1;
			}
#endif
		} else {
			digi_kill_sound_linked_to_object( Players[Player_num].objnum);
			digi_link_sound_to_object2( SOUND_AFTERBURNER_PLAY, Players[Player_num].objnum, 0, F1_0, i2f(256));
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
			{
			 	multi_send_sound_function (0,0);
				func_play = 0;
			}
#endif
		}
	}

	//@@if (Controls.afterburner_state && Afterburner_charge)
	//@@	afterburner_shake();

	Last_afterburner_state = Controls.afterburner_state;
	Last_afterburner_charge = Afterburner_charge;
}

// -- //	------------------------------------------------------------------------------------
// -- //	if energy < F1_0/2, recharge up to F1_0/2
// -- void recharge_energy_frame(void)
// -- {
// -- 	if (Players[Player_num].energy < Weapon_info[0].energy_usage) {
// -- 		Players[Player_num].energy += FrameTime/4;
// --
// -- 		if (Players[Player_num].energy > Weapon_info[0].energy_usage)
// -- 			Players[Player_num].energy = Weapon_info[0].energy_usage;
// -- 	}
// -- }

//	Amount to diminish guns towards normal, per second.
#define	DIMINISH_RATE 16 // gots to be a power of 2, else change the code in diminish_palette_towards_normal

extern fix Flash_effect;

 //adds to rgb values for palette flash
void PALETTE_FLASH_ADD(int _dr, int _dg, int _db)
{
	int	maxval;

	PaletteRedAdd += _dr;
	PaletteGreenAdd += _dg;
	PaletteBlueAdd += _db;

	if (Flash_effect)
		maxval = 60;
	else
		maxval = MAX_PALETTE_ADD;

	if (PaletteRedAdd > maxval)
		PaletteRedAdd = maxval;

	if (PaletteGreenAdd > maxval)
		PaletteGreenAdd = maxval;

	if (PaletteBlueAdd > maxval)
		PaletteBlueAdd = maxval;

	if (PaletteRedAdd < -maxval)
		PaletteRedAdd = -maxval;

	if (PaletteGreenAdd < -maxval)
		PaletteGreenAdd = -maxval;

	if (PaletteBlueAdd < -maxval)
		PaletteBlueAdd = -maxval;
}

fix	Time_flash_last_played;

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards normal.
void diminish_palette_towards_normal(void)
{
	int	dec_amount = 0;
	float brightness_correction = 1-((float)gr_palette_get_gamma()/64); // to compensate for brightness setting of the game

	// Diminish at DIMINISH_RATE units/second.
	if (FrameTime < (F1_0/DIMINISH_RATE))
	{
		static fix diminish_timer = 0;
		diminish_timer += FrameTime;
		if (diminish_timer >= (F1_0/DIMINISH_RATE))
		{
			diminish_timer -= (F1_0/DIMINISH_RATE);
			dec_amount = 1;
		}
	}
	else
	{
		dec_amount = f2i(FrameTime*DIMINISH_RATE); // one second = DIMINISH_RATE counts
		if (dec_amount == 0)
			dec_amount++; // make sure we decrement by something
	}

	if (Flash_effect) {
		int	force_do = 0;
		static fix Flash_step_up_timer = 0;

		// Part of hack system to force update of palette after exiting a menu.
		if (Time_flash_last_played) {
			force_do = 1;
			PaletteRedAdd ^= 1; // Very Tricky! In gr_palette_step_up, if all stepups same as last time, won't do anything!
		}

		if ((Time_flash_last_played + F1_0/8 < GameTime) || (Time_flash_last_played > GameTime)) {
			digi_play_sample( SOUND_CLOAK_OFF, Flash_effect/4);
			Time_flash_last_played = GameTime;
		}

		Flash_effect -= FrameTime;
		Flash_step_up_timer += FrameTime;
		if (Flash_effect < 0)
			Flash_effect = 0;

		if (force_do || (Flash_step_up_timer >= F1_0/26)) // originally time interval based on (d_rand() > 4096)
		{
			Flash_step_up_timer -= (F1_0/26);
			if ( (Newdemo_state==ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd) )
				newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

			gr_palette_step_up( PaletteRedAdd*brightness_correction, PaletteGreenAdd*brightness_correction, PaletteBlueAdd*brightness_correction );

			return;
		}

	}

	if (PaletteRedAdd > 0 ) { PaletteRedAdd -= dec_amount; if (PaletteRedAdd < 0 ) PaletteRedAdd = 0; }
	if (PaletteRedAdd < 0 ) { PaletteRedAdd += dec_amount; if (PaletteRedAdd > 0 ) PaletteRedAdd = 0; }

	if (PaletteGreenAdd > 0 ) { PaletteGreenAdd -= dec_amount; if (PaletteGreenAdd < 0 ) PaletteGreenAdd = 0; }
	if (PaletteGreenAdd < 0 ) { PaletteGreenAdd += dec_amount; if (PaletteGreenAdd > 0 ) PaletteGreenAdd = 0; }

	if (PaletteBlueAdd > 0 ) { PaletteBlueAdd -= dec_amount; if (PaletteBlueAdd < 0 ) PaletteBlueAdd = 0; }
	if (PaletteBlueAdd < 0 ) { PaletteBlueAdd += dec_amount; if (PaletteBlueAdd > 0 ) PaletteBlueAdd = 0; }

	if ( (Newdemo_state==ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd) )
		newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

	gr_palette_step_up( PaletteRedAdd*brightness_correction, PaletteGreenAdd*brightness_correction, PaletteBlueAdd*brightness_correction );
}

int	Redsave, Bluesave, Greensave;

void palette_save(void)
{
	Redsave = PaletteRedAdd; Bluesave = PaletteBlueAdd; Greensave = PaletteGreenAdd;
}

void palette_restore(void)
{
	float brightness_correction = 1-((float)gr_palette_get_gamma()/64);

	PaletteRedAdd = Redsave; PaletteBlueAdd = Bluesave; PaletteGreenAdd = Greensave;
	gr_palette_step_up( PaletteRedAdd*brightness_correction, PaletteGreenAdd*brightness_correction, PaletteBlueAdd*brightness_correction );

	//	Forces flash effect to fixup palette next frame.
	Time_flash_last_played = 0;
}

extern void dead_player_frame(void);

//	--------------------------------------------------------------------------------------------------
int allowed_to_fire_laser(void)
{
	if (Player_is_dead) {
		Global_missile_firing_count = 0;
		return 0;
	}

	//	Make sure enough time has elapsed to fire laser, but if it looks like it will
	//	be a long while before laser can be fired, then there must be some mistake!
	if (Next_laser_fire_time > GameTime)
		if (Next_laser_fire_time < GameTime + 2*F1_0)
			return 0;

	return 1;
}

int allowed_to_fire_flare(void)
{
	if (Next_flare_fire_time > GameTime)
		if (Next_flare_fire_time < GameTime + FLARE_BIG_DELAY)	//	In case time is bogus, never wait > 1 second.
			return 0;

	if (Players[Player_num].energy >= Weapon_info[FLARE_ID].energy_usage)
		Next_flare_fire_time = GameTime + F1_0/4;
	else
		Next_flare_fire_time = GameTime + FLARE_BIG_DELAY;

	return 1;
}

int allowed_to_fire_missile(void)
{
	//	Make sure enough time has elapsed to fire missile, but if it looks like it will
	//	be a long while before missile can be fired, then there must be some mistake!
	if (Next_missile_fire_time > GameTime)
		if (Next_missile_fire_time < GameTime + 5*F1_0)
			return 0;

	return 1;
}

void full_palette_save(void)
{
	palette_save();
	apply_modified_palette();
	reset_palette_add();
	gr_palette_load( gr_palette );
}

extern int Death_sequence_aborted;

#ifdef USE_SDLMIXER
#define EXT_MUSIC_TEXT "Jukebox/Audio CD"
#else
#define EXT_MUSIC_TEXT "Audio CD"
#endif

static int free_help(newmenu *menu, d_event *event, void *userdata)
{
	userdata = userdata;

	if (event->type == EVENT_WINDOW_CLOSE)
	{
		newmenu_item *items = newmenu_get_items(menu);
		d_free(items);
	}

	return 0;
}

void show_help()
{
	int nitems = 0;
	newmenu_item *m;

	MALLOC(m, newmenu_item, 26);
	if (!m)
		return;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_ESC;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "SHIFT-ESC\t  SHOW GAME LOG";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F1\t  THIS SCREEN";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_F2;
#if !(defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F2/F3\t  SAVE/LOAD GAME";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F1\t  Fast Save";
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F2/F3 (\x85-SHIFT-s/\x85-o)\t  SAVE/LOAD GAME";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F1 (\x85-s)\t  Fast Save";
#endif
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F3\t  SWITCH COCKPIT MODES";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_F4;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_F5;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "ALT-F7\t  SWITCH HUD MODES";
#if !(defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_PAUSE;
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Pause (\x85-P)\t  Pause";
#endif
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_PRTSCN;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_1TO5;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_6TO10;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Shift-F1/F2\t  Cycle left/right window";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Shift-F4\t  GuideBot menu";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-Shift-F4\t  Rename GuideBot";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Shift-F5/F6\t  Drop primary/secondary";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Shift-number\t  GuideBot commands";
#if !(defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-Shift-F9\t  Eject Audio CD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-Shift-F10\t  Play/Pause " EXT_MUSIC_TEXT;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-Shift-F11/F12\t  Previous/Next Song";
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "\x85-E\t  Eject Audio CD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "\x85-Up/Down\t  Play/Pause " EXT_MUSIC_TEXT;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "\x85-Left/Right\t  Previous/Next Song";
#endif
#if (defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(Use \x85-# for F#. e.g. \x85-1 for F1)";
#endif

	newmenu_dotiny( NULL, TXT_KEYS, nitems, m, 0, free_help, NULL );
}

void show_netgame_help()
{
	int nitems = 0;
	newmenu_item *m;

	MALLOC(m, newmenu_item, 17);
	if (!m)
		return;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F1\t  THIS SCREEN";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "ALT-0\t  DROP FLAG";
#if !(defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F2/F3\t  SAVE/LOAD COOP GAME";
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Alt-F2/F3 (\x85-SHIFT-s/\x85-o)\t  SAVE/LOAD COOP GAME";
#endif
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "ALT-F4\t  SHOW PLAYER NAMES ON HUD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F7\t  TOGGLE KILL LIST";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F8\t  SEND MESSAGE";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(SHIFT-)F9 to F12\t  (DEFINE)SEND MACRO";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "PAUSE\t  SHOW NETGAME INFORMATION";

#if (defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(Use \x85-# for F#. e.g. \x85-1 for F1)";
#endif

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "MULTIPLAYER MESSAGE COMMANDS:";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "Handicap: (*)\t  SET YOUR STARTING SHIELDS TO (*) [10-100]";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(*): TEXT\t  SEND TEXT TO PLAYER (*)";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "move: (*)\t  MOVE PLAYER (*) TO OTHER TEAM (Host-only)";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "kick: (*)\t  KICK PLAYER (*) FROM GAME (Host-only)";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "KillReactor\t  BLOW UP THE MINE (Host-only)";

	newmenu_dotiny( NULL, TXT_KEYS, nitems, m, 0, free_help, NULL );
}

void show_newdemo_help()
{
	newmenu_item *m;
	int nitems = 0;

	MALLOC(m, newmenu_item, 15);
	if (!m)
		return;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "ESC\t  QUIT DEMO PLAYBACK";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F1\t  THIS SCREEN";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = TXT_HELP_F2;
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F3\t  SWITCH COCKPIT MODES";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "F4\t  TOGGLE PERCENTAGE DISPLAY";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "UP\t  PLAY";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "DOWN\t  PAUSE";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "RIGHT\t  ONE FRAME FORWARD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "LEFT\t  ONE FRAME BACKWARD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "SHIFT-RIGHT\t  FAST FORWARD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "SHIFT-LEFT\t  FAST BACKWARD";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "CTRL-RIGHT\t  JUMP TO END";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "CTRL-LEFT\t  JUMP TO START";
#if (defined(__APPLE__) || defined(macintosh))
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "(Use \x85-# for F#. e.g. \x85-1 for F1)";
#endif

	newmenu_dotiny( NULL, "DEMO PLAYBACK CONTROLS", nitems, m, 0, free_help, NULL );
}

//temp function until Matt cleans up game sequencing
extern void temp_reset_stuff_on_level();

#define LEAVE_TIME 0x4000		//how long until we decide key is down	(Used to be 0x4000)

//deal with rear view - switch it on, or off, or whatever
void check_rear_view()
{
	static int leave_mode;
	static fix64 entry_time;

	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;

	if ( Controls.rear_view_state) {	//key/button has gone down
		Controls.rear_view_state = 0;

		if (Rear_view) {
			Rear_view = 0;
			if (PlayerCfg.CockpitMode[1]==CM_REAR_VIEW) {
				select_cockpit(PlayerCfg.CockpitMode[0]);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_restore_rearview();
		}
		else {
			Rear_view = 1;
			leave_mode = 0;		//means wait for another key
			entry_time = timer_query();
			if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT) {
				select_cockpit(CM_REAR_VIEW);
			}
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_rearview();
		}
	}
	else
		if (Controls.rear_view_state) {

			if (leave_mode == 0 && (timer_query() - entry_time) > LEAVE_TIME)
				leave_mode = 1;
		}
		else
		{
			if (leave_mode==1 && Rear_view) {
				Rear_view = 0;
				if (PlayerCfg.CockpitMode[1]==CM_REAR_VIEW) {
					select_cockpit(PlayerCfg.CockpitMode[0]);
				}
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_restore_rearview();
			}
		}
}

void reset_rear_view(void)
{
	if (Rear_view) {
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_restore_rearview();
	}

	Rear_view = 0;
	select_cockpit(PlayerCfg.CockpitMode[0]);
}

int Config_menu_flag;

int Cheats_enabled=0;

extern int Laser_rapid_fire;
extern void do_lunacy_on(), do_lunacy_off();

extern int Physics_cheat_flag,Robots_kill_robots_cheat;
extern char BounceCheat,HomingCheat,OldHomingState[20];
extern char AcidCheatOn, Monster_mode;
extern int Buddy_dude_cheat;

//turns off active cheats
void turn_cheats_off()
{
	int i;

	if (HomingCheat)
		for (i=0;i<20;i++)
			Weapon_info[i].homing_flag=OldHomingState[i];

	if (AcidCheatOn)
	{
		AcidCheatOn=0;
	}

	Buddy_dude_cheat = 0;
	BounceCheat=0;
   HomingCheat=0;
	do_lunacy_off();
	Laser_rapid_fire = 0;
	Physics_cheat_flag = 0;
	Monster_mode = 0;
	Robots_kill_robots_cheat=0;
	Robot_firing_enabled = 1;
}

//turns off all cheats & resets cheater flag
void game_disable_cheats()
{
	turn_cheats_off();
	Cheats_enabled=0;
}

extern int netplayerinfo_on;

//	game_setup()
// ----------------------------------------------------------------------------

int game_handler(window *wind, d_event *event, void *data);

window *game_setup(void)
{
	window *game_wind;

	PlayerCfg.CockpitMode[1] = PlayerCfg.CockpitMode[0];
	last_drawn_cockpit = -1;	// Force cockpit to redraw next time a frame renders.
	Endlevel_sequence = 0;

	game_wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, game_handler, NULL);
	if (!game_wind)
		return NULL;

	window_set_grab_input(game_wind, true);
	window_set_opaque(game_wind, true);

	reset_palette_add();
	init_cockpit();
	init_gauges();
	netplayerinfo_on = 0;

#ifdef EDITOR
	if (!Cursegp)
	{
		Cursegp = &Segments[0];
		Curside = 0;
	}
	
	if (Segments[ConsoleObject->segnum].segnum == -1)      //segment no longer exists
		obj_relink( ConsoleObject-Objects, SEG_PTR_2_NUM(Cursegp) );

	if (!check_obj_seg(ConsoleObject))
		move_player_2_segment(Cursegp,Curside);
#endif

	Viewer = ConsoleObject;
	fly_init(ConsoleObject);
	Game_suspended = 0;
	reset_time();
	FrameTime = 0;			//make first frame zero

#ifdef EDITOR
	if (Current_level_num == 0) {	//not a real level
		init_player_stats_game();
		init_ai_objects();
	}
#endif

	fix_object_segs();

	return game_wind;
}

void game_render_frame();

window *Game_wind = NULL;

// Event handler for the game
int game_handler(window *wind, d_event *event, void *data)
{
	data = data;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			gr_set_current_canvas(NULL);

			game_flush_inputs();

			if (time_paused)
				start_time();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				digi_resume_digi_sounds();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				palette_restore();
			
			reset_cockpit();
			break;

		case EVENT_WINDOW_DEACTIVATED:
			if (!(((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)) && (!Endlevel_sequence)) )
				stop_time();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				digi_pause_digi_sounds();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				full_palette_save();

			break;

		case EVENT_JOYSTICK_BUTTON_UP:
		case EVENT_JOYSTICK_BUTTON_DOWN:
		case EVENT_JOYSTICK_MOVED:
		case EVENT_MOUSE_BUTTON_UP:
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_MOVED:
		case EVENT_KEY_COMMAND:
		case EVENT_KEY_RELEASE:
		case EVENT_IDLE:
			return ReadControls(event);

		case EVENT_WINDOW_DRAW:
			ProcessControls();

			if (!time_paused)
			{
				calc_frame_time();
				GameProcessFrame();

				if (gr_focus_lost)
					do_game_pause();
			}

			if (force_cockpit_redraw) {			//screen need redrawing?
				init_cockpit();
				force_cockpit_redraw=0;
			}
			game_render_frame();

			break;

		case EVENT_WINDOW_CLOSE:
			digi_stop_digi_sounds();

			if ( (Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED) )
				newdemo_stop_recording();

#ifdef NETWORK
			multi_leave_game();
#endif

			if ( Newdemo_state == ND_STATE_PLAYBACK )
				newdemo_stop_playback();

			songs_play_song( SONG_TITLE, 1 );

			game_disable_cheats();
			Game_mode = GM_GAME_OVER;
			Game_wind = NULL;
			break;

		default:
			break;
	}

	return 0;
}

// Initialise game, actually runs in main event loop
void game()
{
	Game_wind = game_setup();
}

//called at the end of the program
void close_game()
{
	close_gauges();
	restore_effect_bitmap_icons();
}


#ifdef EDITOR
extern void player_follow_path(object *objp);
extern void check_create_player_path(void);
#endif

extern	int Do_appearance_effect;

object *Missile_viewer=NULL;
int Missile_viewer_sig=-1;

int Marker_viewer_num[2]={-1,-1};
int Coop_view_player[2]={-1,-1};

//returns ptr to escort robot, or NULL
object *find_escort()
{
	int i;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			if (Robot_info[Objects[i].id].companion)
				return &Objects[i];

	return NULL;
}

extern void process_super_mines_frame(void);
extern void do_seismic_stuff(void);
extern int Level_shake_duration;

//if water or fire level, make occasional sound
void do_ambient_sounds()
{
	int has_water,has_lava;
	int sound;

	has_lava = (Segment2s[ConsoleObject->segnum].s2_flags & S2F_AMBIENT_LAVA);
	has_water = (Segment2s[ConsoleObject->segnum].s2_flags & S2F_AMBIENT_WATER);

	if (has_lava) {							//has lava
		sound = SOUND_AMBIENT_LAVA;
		if (has_water && (d_rand() & 1))	//both, pick one
			sound = SOUND_AMBIENT_WATER;
	}
	else if (has_water)						//just water
		sound = SOUND_AMBIENT_WATER;
	else
		return;

	if (((d_rand() << 3) < FrameTime)) {						//play the sound
		fix volume = d_rand() + f1_0/2;
		digi_play_sample(sound,volume);
	}
}

// -- extern void lightning_frame(void);

extern void omega_charge_frame(void);

void flicker_lights();

void game_leave_menus(void)
{
	if (!Game_wind)
		return;
	for (;;) // go through all windows and actually close them if they want to
	{
		window *wind = window_get_front();
		if (!wind)
			break;
		if (wind == Game_wind)
			break;
		if (!window_close(wind))
			break;
	}
}

void GameProcessFrame(void)
{
	fix player_shields = Players[Player_num].shields;
	int player_was_dead = Player_is_dead;

	update_player_stats();
	diminish_palette_towards_normal();		//	Should leave palette effect up for as long as possible by putting right before render.
	do_afterburner_stuff();
	do_cloak_stuff();
	do_invulnerable_stuff();
	remove_obsolete_stuck_objects();
	init_ai_frame();
	do_final_boss_frame();

	if ((Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT) && (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT_ON)) {
		static int turned_off=0;
		Players[Player_num].energy -= (FrameTime*3/8);
		if (Players[Player_num].energy < i2f(10)) {
			if (!turned_off) {
				Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
				turned_off = 1;
#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_flags(Player_num);
#endif
			}
		}
		else
			turned_off = 0;

		if (Players[Player_num].energy <= 0) {
			Players[Player_num].energy = 0;
			Players[Player_num].flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_flags(Player_num);
#endif
		}
	}


#ifdef EDITOR
	check_create_player_path();
	player_follow_path(ConsoleObject);
#endif

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		multi_do_frame();
		if (Netgame.PlayTimeAllowed && ThisLevelTime>=i2f((Netgame.PlayTimeAllowed*5*60)))
			multi_check_for_killgoal_winner();
	}
#endif

	dead_player_frame();
	if (Newdemo_state != ND_STATE_PLAYBACK)
		do_controlcen_dead_frame();

	process_super_mines_frame();
	do_seismic_stuff();
	do_ambient_sounds();

#ifdef NETWORK
	if ((Game_mode & GM_MULTI) && Netgame.PlayTimeAllowed)
		ThisLevelTime +=FrameTime;
#endif

	digi_sync_sounds();

	if (Endlevel_sequence) {
		do_endlevel_frame();
		powerup_grab_cheat_all();
		do_special_effects();
		return;					//skip everything else
	}

	if (Newdemo_state != ND_STATE_PLAYBACK)
		do_exploding_wall_frame();
	if ((Newdemo_state != ND_STATE_PLAYBACK) || (Newdemo_vcr_state != ND_STATE_PAUSED)) {
		do_special_effects();
		wall_frame_process();
		triggers_frame_process();
	}

	if (Control_center_destroyed) {
		if (Newdemo_state==ND_STATE_RECORDING )
			newdemo_record_control_center_destroyed();
	}

	flash_frame();

	if ( Newdemo_state == ND_STATE_PLAYBACK ) {
		newdemo_playback_one_frame();
		if ( Newdemo_state != ND_STATE_PLAYBACK )		{
			if (Game_wind)
				window_close(Game_wind);		// Go back to menu
			return;
		}
	}
	else
	{ // Note the link to above!

		Players[Player_num].homing_object_dist = -1;		//	Assume not being tracked.  Laser_do_weapon_sequence modifies this.

		object_move_all();
		powerup_grab_cheat_all();

		if (Endlevel_sequence)	//might have been started during move
			return;

		fuelcen_update_all();

		do_ai_frame_all();

		if (allowed_to_fire_laser())
			FireLaser();				// Fire Laser!

		if (Auto_fire_fusion_cannon_time) {
			if (Primary_weapon != FUSION_INDEX)
				Auto_fire_fusion_cannon_time = 0;
			else if (GameTime + FrameTime/2 >= Auto_fire_fusion_cannon_time) {
				Auto_fire_fusion_cannon_time = 0;
				Global_laser_firing_count = 1;
			} else if (d_tick_step) {
				vms_vector	rand_vec;
				fix			bump_amount;

				Global_laser_firing_count = 0;

				ConsoleObject->mtype.phys_info.rotvel.x += (d_rand() - 16384)/8;
				ConsoleObject->mtype.phys_info.rotvel.z += (d_rand() - 16384)/8;

				make_random_vector(&rand_vec);

				bump_amount = F1_0*4;

				if (Fusion_charge > F1_0*2)
					bump_amount = Fusion_charge*4;

				bump_one_object(ConsoleObject, &rand_vec, bump_amount);
			}
			else
			{
				Global_laser_firing_count = 0;
			}
		}

		if (Global_laser_firing_count)
			Global_laser_firing_count -= do_laser_firing_player();

		if (Global_laser_firing_count < 0)
			Global_laser_firing_count = 0;
	}

	if (Do_appearance_effect) {
		create_player_appearance_effect(ConsoleObject);
		Do_appearance_effect = 0;
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && Netgame.InvulAppear)
		{
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			Players[Player_num].invulnerable_time = GameTime-i2f(27);
			FakingInvul=1;
		}
#endif
	}

	omega_charge_frame();
	slide_textures();
	flicker_lights();

	//if the player is taking damage, give up guided missile control
	if (Players[Player_num].shields != player_shields)
		release_guided_missile(Player_num);

	// Check if we have to close in-game menus for multiplayer
	if ((Game_mode & GM_MULTI) && (Players[Player_num].connected == CONNECT_PLAYING))
	{
		if ( Endlevel_sequence || ((Control_center_destroyed) && (Countdown_seconds_left <= 1)) || // close menus when end of level...
			(Automap_active && ((Player_is_dead != player_was_dead) || (Players[Player_num].shields<=0 && player_shields>0))) ) // close autmap when dying ...
			game_leave_menus();
	}
}

//!!extern int Goal_blue_segnum,Goal_red_segnum;
//!!extern int Hoard_goal_eclip;
//!!
//!!//do cool pulsing lights in hoard goals
//!!hoard_light_pulse()
//!!{
//!!	if (Game_mode & GM_HOARD) {
//!!		fix light;
//!!		int frame;
//!!
//!!		frame = Effects[Hoard_goal_eclip].frame_count;
//!!
//!!		frame++;
//!!
//!!		if (frame >= Effects[Hoard_goal_eclip].vc.num_frames)
//!!			frame = 0;
//!!
//!!		light = abs(frame - 5) * f1_0 / 5;
//!!
//!!		Segment2s[Goal_red_segnum].static_light = Segment2s[Goal_blue_segnum].static_light = light;
//!!	}
//!!}


ubyte	Slide_segs[MAX_SEGMENTS];
int	Slide_segs_computed;

void compute_slide_segs(void)
{
	int	segnum, sidenum;

	for (segnum=0;segnum<=Highest_segment_index;segnum++) {
		Slide_segs[segnum] = 0;
		for (sidenum=0;sidenum<6;sidenum++) {
			int tmn = Segments[segnum].sides[sidenum].tmap_num;
			if (TmapInfo[tmn].slide_u != 0 || TmapInfo[tmn].slide_v != 0)
				Slide_segs[segnum] |= 1 << sidenum;
		}
	}

	Slide_segs_computed = 1;
}

//	-----------------------------------------------------------------------------
void slide_textures(void)
{
	int segnum,sidenum,i;

	if (!Slide_segs_computed)
		compute_slide_segs();

	for (segnum=0;segnum<=Highest_segment_index;segnum++) {
		if (Slide_segs[segnum]) {
			for (sidenum=0;sidenum<6;sidenum++) {
				if (Slide_segs[segnum] & (1 << sidenum)) {
					int tmn = Segments[segnum].sides[sidenum].tmap_num;
					if (TmapInfo[tmn].slide_u != 0 || TmapInfo[tmn].slide_v != 0) {
						for (i=0;i<4;i++) {
							Segments[segnum].sides[sidenum].uvls[i].u += fixmul(FrameTime,TmapInfo[tmn].slide_u*(1<<8));
							Segments[segnum].sides[sidenum].uvls[i].v += fixmul(FrameTime,TmapInfo[tmn].slide_v*(1<<8));
							if (Segments[segnum].sides[sidenum].uvls[i].u > f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].u -= f1_0;
							}
							if (Segments[segnum].sides[sidenum].uvls[i].v > f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].v -= f1_0;
							}
							if (Segments[segnum].sides[sidenum].uvls[i].u < -f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].u += f1_0;
							}
							if (Segments[segnum].sides[sidenum].uvls[i].v < -f2_0) {
								int j;
								for (j=0;j<4;j++)
									Segments[segnum].sides[sidenum].uvls[j].v += f1_0;
							}
						}
					}
				}
			}
		}
	}
}

flickering_light Flickering_lights[MAX_FLICKERING_LIGHTS];

int Num_flickering_lights=0;

void flicker_lights()
{
	int l;
	flickering_light *f;

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++) {
		segment *segp = &Segments[f->segnum];

		//make sure this is actually a light
		if (! (WALL_IS_DOORWAY(segp, f->sidenum) & WID_RENDER_FLAG))
			continue;
		if (! (TmapInfo[segp->sides[f->sidenum].tmap_num].lighting | TmapInfo[segp->sides[f->sidenum].tmap_num2 & 0x3fff].lighting))
			continue;

		if (f->timer == 0x80000000)		//disabled
			continue;

		if ((f->timer -= FrameTime) < 0) {

			while (f->timer < 0)
				f->timer += f->delay;

			f->mask = ((f->mask&0x80000000)?1:0) + (f->mask<<1);

			if (f->mask & 1)
				add_light(f->segnum,f->sidenum);
			else
				subtract_light(f->segnum,f->sidenum);
		}
	}
}

//returns ptr to flickering light structure, or NULL if can't find
flickering_light *find_flicker(int segnum,int sidenum)
{
	int l;
	flickering_light *f;

	//see if there's already an entry for this seg/side

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++)
		if (f->segnum == segnum && f->sidenum == sidenum)	//found it!
			return f;

	return NULL;
}

//turn flickering off (because light has been turned off)
void disable_flicker(int segnum,int sidenum)
{
	flickering_light *f;

	if ((f=find_flicker(segnum,sidenum)) != NULL)
		f->timer = 0x80000000;
}

//turn flickering off (because light has been turned on)
void enable_flicker(int segnum,int sidenum)
{
	flickering_light *f;

	if ((f=find_flicker(segnum,sidenum)) != NULL)
		f->timer = 0;
}


#ifdef EDITOR

//returns 1 if ok, 0 if error
int add_flicker(int segnum, int sidenum, fix delay, unsigned long mask)
{
	int l;
	flickering_light *f;

	//see if there's already an entry for this seg/side

	f = Flickering_lights;

	for (l=0;l<Num_flickering_lights;l++,f++)
		if (f->segnum == segnum && f->sidenum == sidenum)	//found it!
			break;

	if (mask==0) {		//clearing entry
		if (l == Num_flickering_lights)
			return 0;
		else {
			int i;
			for (i=l;i<Num_flickering_lights-1;i++)
				Flickering_lights[i] = Flickering_lights[i+1];
			Num_flickering_lights--;
			return 1;
		}
	}

	if (l == Num_flickering_lights) {
		if (Num_flickering_lights == MAX_FLICKERING_LIGHTS)
			return 0;
		else
			Num_flickering_lights++;
	}

	f->segnum = segnum;
	f->sidenum = sidenum;
	f->delay = f->timer = delay;
	f->mask = mask;

	return 1;
}

#endif

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
void FireLaser()
{

	Global_laser_firing_count = Controls.fire_primary_state?Weapon_info[Primary_weapon_to_weapon_info[Primary_weapon]].fire_count:0;

	if ((Primary_weapon == FUSION_INDEX) && (Global_laser_firing_count)) {
		if ((Players[Player_num].energy < F1_0*2) && (Auto_fire_fusion_cannon_time == 0)) {
			Global_laser_firing_count = 0;
		} else {
			static int Fusion_next_sound_time = 0;

			if (Fusion_charge == 0)
				Players[Player_num].energy -= F1_0*2;

			Fusion_charge += FrameTime;
			Players[Player_num].energy -= FrameTime;

			if (Players[Player_num].energy <= 0) {
				Players[Player_num].energy = 0;
				Auto_fire_fusion_cannon_time = GameTime -1;	//	Fire now!
			} else
				Auto_fire_fusion_cannon_time = GameTime + FrameTime/2 + 1;		//	Fire the fusion cannon at this time in the future.

			if (Fusion_charge < F1_0*2)
				PALETTE_FLASH_ADD(Fusion_charge >> 11, 0, Fusion_charge >> 11);
			else
				PALETTE_FLASH_ADD(Fusion_charge >> 11, Fusion_charge >> 11, 0);

			if (Fusion_next_sound_time > GameTime + F1_0/8 + D_RAND_MAX/4) //gametime has wrapped or something is screwed
				Fusion_next_sound_time = GameTime - 1;

			if (Fusion_next_sound_time < GameTime) {
				if (Fusion_charge > F1_0*2) {
					digi_play_sample( 11, F1_0 );
					apply_damage_to_player(ConsoleObject, ConsoleObject, d_rand() * 4, 0);
				} else {
					create_awareness_event(ConsoleObject, PA_WEAPON_ROBOT_COLLISION);
					digi_play_sample( SOUND_FUSION_WARMUP, F1_0 );
					#ifdef NETWORK
					if (Game_mode & GM_MULTI)
						multi_send_play_sound(SOUND_FUSION_WARMUP, F1_0);
					#endif
				}
				Fusion_next_sound_time = GameTime + F1_0/8 + d_rand()/4;
			}
		}
	}
}


//	-------------------------------------------------------------------------------------------------------
//	If player is close enough to objnum, which ought to be a powerup, pick it up!
//	This could easily be made difficulty level dependent.
void powerup_grab_cheat(object *player, int objnum)
{
	fix	powerup_size;
	fix	player_size;
	fix	dist;

	Assert(Objects[objnum].type == OBJ_POWERUP);

	powerup_size = Objects[objnum].size;
	player_size = player->size;

	dist = vm_vec_dist_quick(&Objects[objnum].pos, &player->pos);

	if ((dist < 2*(powerup_size + player_size)) && !(Objects[objnum].flags & OF_SHOULD_BE_DEAD)) {
		vms_vector	collision_point;

		vm_vec_avg(&collision_point, &Objects[objnum].pos, &player->pos);
		collide_player_and_powerup(player, &Objects[objnum], &collision_point);
	}
}

//	-------------------------------------------------------------------------------------------------------
//	Make it easier to pick up powerups.
//	For all powerups in this segment, pick them up at up to twice pickuppable distance based on dot product
//	from player to powerup and player's forward vector.
//	This has the effect of picking them up more easily left/right and up/down, but not making them disappear
//	way before the player gets there.
void powerup_grab_cheat_all(void)
{
	segment	*segp;
	int		objnum;

	segp = &Segments[ConsoleObject->segnum];
	objnum = segp->objects;

	while (objnum != -1) {
		if (Objects[objnum].type == OBJ_POWERUP)
			powerup_grab_cheat(ConsoleObject, objnum);
		objnum = Objects[objnum].next;
	}

}

int	Last_level_path_created = -1;

#ifdef SHOW_EXIT_PATH

//	------------------------------------------------------------------------------------------------------------------
//	Create path for player from current segment to goal segment.
//	Return true if path created, else return false.
int mark_player_path_to_segment(int segnum)
{
	int		i;
	object	*objp = ConsoleObject;
	short		player_path_length=0;
	int		player_hide_index=-1;

	if (Last_level_path_created == Current_level_num) {
		return 0;
	}

	Last_level_path_created = Current_level_num;

	if (create_path_points(objp, objp->segnum, segnum, Point_segs_free_ptr, &player_path_length, 100, 0, 0, -1) == -1) {
		return 0;
	}

	player_hide_index = Point_segs_free_ptr - Point_segs;
	Point_segs_free_ptr += player_path_length;

	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		ai_reset_all_paths();
		return 0;
	}

	for (i=1; i<player_path_length; i++) {
		int			segnum, objnum;
		vms_vector	seg_center;
		object		*obj;

		segnum = Point_segs[player_hide_index+i].segnum;
		seg_center = Point_segs[player_hide_index+i].point;

		objnum = obj_create( OBJ_POWERUP, POW_ENERGY, segnum, &seg_center, &vmd_identity_matrix, Powerup_info[POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP);
		if (objnum == -1) {
			Int3();		//	Unable to drop energy powerup for path
			return 1;
		}

		obj = &Objects[objnum];
		obj->rtype.vclip_info.vclip_num = Powerup_info[obj->id].vclip_num;
		obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
		obj->rtype.vclip_info.framenum = 0;
		obj->lifeleft = F1_0*100 + d_rand() * 4;
	}

	return 1;
}

//	Return true if it happened, else return false.
int create_special_path(void)
{
	int	i,j;

	//	---------- Find exit doors ----------
	for (i=0; i<=Highest_segment_index; i++)
		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
                    int wall_num = Segments[i].sides[j].wall_num;
                    if (wall_num == -1)
                        continue;
                    int trigger_num = Walls[wall_num].trigger;
                    if (trigger_num == -1)
                        continue;
                    trigger *trig = &Triggers[trigger_num];
                    if (trig->type == TT_EXIT)
                        return mark_player_path_to_segment(i);
                }

	return 0;
}

#endif


#ifndef RELEASE
int	Max_obj_count_mike = 0;

//	Shows current number of used objects.
void show_free_objects(void)
{
	if (!(d_tick_count & 8)) {
		int	i;
		int	count=0;

		for (i=0; i<=Highest_object_index; i++)
			if (Objects[i].type != OBJ_NONE)
				count++;

		if (count > Max_obj_count_mike) {
			Max_obj_count_mike = count;
		}
	}
}

#endif

/*
 * reads a flickering_light structure from a CFILE
 */
void flickering_light_read(flickering_light *fl, CFILE *fp)
{
	fl->segnum = cfile_read_short(fp);
	fl->sidenum = cfile_read_short(fp);
	fl->mask = cfile_read_int(fp);
	fl->timer = cfile_read_fix(fp);
	fl->delay = cfile_read_fix(fp);
}

void flickering_light_write(flickering_light *fl, PHYSFS_file *fp)
{
	PHYSFS_writeSLE16(fp, fl->segnum);
	PHYSFS_writeSLE16(fp, fl->sidenum);
	PHYSFS_writeULE32(fp, fl->mask);
	PHYSFSX_writeFix(fp, fl->timer);
	PHYSFSX_writeFix(fp, fl->delay);
}
