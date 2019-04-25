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
 * Game Controls Stuff
 *
 */

//#define DOOR_DEBUGGING

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "pstypes.h"
#include "window.h"
#include "console.h"
#include "inferno.h"
#include "game.h"
#include "player.h"
#include "key.h"
#include "object.h"
#include "menu.h"
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
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "digi.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "songs.h"
#include "newmenu.h"
#include "gamefont.h"
#include "endlevel.h"
#include "config.h"
#include "kconfig.h"
#include "mouse.h"
#include "titles.h"
#include "gr.h"
#include "playsave.h"
#include "movie.h"
#include "scores.h"
#include "collide.h"
#include "controls.h"
#include "multi.h"
#include "desc_id.h"
#include "cntrlcen.h"
#include "fuelcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "ai.h"
#include "switch.h"
#include "escort.h"
#include "window.h"

#include <SDL.h>

bool Rear_view;
static fix64 Rear_down_time;

static fix64 Converter_down_time;

//	External Variables ---------------------------------------------------------

extern char WaitForRefuseAnswer,RefuseThisPlayer,RefuseTeam;

extern int	Global_missile_firing_count;

extern int	Physics_cheat_flag;

extern fix	Show_view_text_timer;

extern ubyte DefiningMarkerMessage;

//	Function prototypes --------------------------------------------------------


extern void CyclePrimary();
extern void CycleSecondary();
extern void InitMarkerInput();
extern int  MarkerInputMessage(int key);
extern int	allowed_to_fire_missile(void);
extern int	allowed_to_fire_flare(void);
extern int	create_special_path(void);
extern void	kconfig_center_headset(void);
extern void newdemo_strip_frames(char *, int);
extern void toggle_cockpit(void);
extern void dump_used_textures_all();
extern void DropMarker();
extern void DropSecondaryWeapon();
extern void DropCurrentWeapon();

void do_cheat_menu(void);

int HandleGameKey(int key);
int HandleSystemKey(int key);
int HandleTestKey(int key);

#define key_isfunc(k) (((k&0xff)>=KEY_F1 && (k&0xff)<=KEY_F10) || (k&0xff)==KEY_F11 || (k&0xff)==KEY_F12)

// Functions ------------------------------------------------------------------

#define LEAVE_TIME 0x4000		//how long until we decide key is down	(Used to be 0x4000)

// Handles a key whose down state is represented by key_state, and whose
// logical state is stored in *state. The logic is that if the key is hit for
// short time, the state will "stick" as being asserted. Hitting the key again
// will release it. Keeping the key down for a longer time will de-assert the
// key state as the key is released.
// Initialize *down_time with 0 and keep it as additional state.
// Returns whether *state was changed.
static bool handle_sticky_key(int key_state, bool *state, fix64 *down_time,
							  enum key_stick_type config)
{
	fix64 now = timer_query();
	bool old_state = *state;

	if (key_state) {
		if (*down_time <= 0) {
			// Key went down.
			*state = config == KEY_STICK_NORMAL ? true : !*state;
			*down_time = now;
		}
	} else {
		if (*down_time > 0) {
			// Key went up.
			if ((config == KEY_STICK_STICKY && now - *down_time >= LEAVE_TIME) ||
				config == KEY_STICK_NORMAL)
				*state = false; // non-sticky
			*down_time = 0;
		}
	}

	return *state != old_state;
}

//deal with rear view - switch it on, or off, or whatever
static void process_rear_view_key(void)
{
	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;

	if (handle_sticky_key(Controls.rear_view_state, &Rear_view, &Rear_down_time,
						  PlayerCfg.KeyStickRearView)) {
		if (Rear_view) {
			if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
				select_cockpit(CM_REAR_VIEW);
		} else {
			if (PlayerCfg.CockpitMode[1]==CM_REAR_VIEW)
				select_cockpit(PlayerCfg.CockpitMode[0]);
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_rearview();
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

#define CONVERTER_RATE  20		//10 units per second xfer rate
#define CONVERTER_SCALE  2		//2 units energy -> 1 unit shields

#define CONVERTER_SOUND_DELAY (f1_0/2)		//play every half second

static void transfer_energy_to_shield(bool sticky_conversion)
{
	fix e;		//how much energy gets transfered
	static fix last_play_time=0;

	e = min(min(FrameTime*CONVERTER_RATE,Players[Player_num].energy - INITIAL_ENERGY),(MAX_SHIELDS-Players[Player_num].shields)*CONVERTER_SCALE);

	if (e <= 0) {
		// If the conversion is in the "background", don't be noisy.
		if (sticky_conversion)
			return;

		if (Players[Player_num].energy <= INITIAL_ENERGY) {
			HUD_init_message(HM_DEFAULT, "Need more than %i energy to enable transfer", f2i(INITIAL_ENERGY));
		}
		else if (Players[Player_num].shields >= MAX_SHIELDS) {
			HUD_init_message(HM_DEFAULT, "No transfer: Shields already at max");
		}
		return;
	}

	Players[Player_num].energy  -= e;
	Players[Player_num].shields += e/CONVERTER_SCALE;

	if (last_play_time > GameTime)
		last_play_time = 0;

	if (GameTime > last_play_time+CONVERTER_SOUND_DELAY) {
		digi_play_sample_once(SOUND_CONVERT_ENERGY, F1_0);
		last_play_time = GameTime;
	}

}

void update_vcr_state();
void do_weapon_n_item_stuff(void);


// Control Functions

fix newdemo_single_frame_time;

void update_vcr_state(void)
{
	if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_RIGHT] && d_tick_step)
		Newdemo_vcr_state = ND_STATE_FASTFORWARD;
	else if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_LEFT] && d_tick_step)
		Newdemo_vcr_state = ND_STATE_REWINDING;
	else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_RIGHT] && ((GameTime - newdemo_single_frame_time) >= F1_0) && d_tick_step)
		Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
	else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_LEFT] && ((GameTime - newdemo_single_frame_time) >= F1_0) && d_tick_step)
		Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
	else if ((Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_REWINDING))
		Newdemo_vcr_state = ND_STATE_PLAYBACK;
}

//returns which bomb will be dropped next time the bomb key is pressed
int which_bomb()
{
	int bomb;

	//use the last one selected, unless there aren't any, in which case use
	//the other if there are any


   // If hoard game, only let the player drop smart mines
   if (Game_mode & GM_HOARD)
		return SMART_MINE_INDEX;

	bomb = Secondary_last_was_super[PROXIMITY_INDEX]?SMART_MINE_INDEX:PROXIMITY_INDEX;

	if (Players[Player_num].secondary_ammo[bomb] == 0 &&
			Players[Player_num].secondary_ammo[SMART_MINE_INDEX+PROXIMITY_INDEX-bomb] != 0) {
		bomb = SMART_MINE_INDEX+PROXIMITY_INDEX-bomb;
		Secondary_last_was_super[bomb%SUPER_WEAPON] = (bomb == SMART_MINE_INDEX);
	}



	return bomb;
}


void do_weapon_n_item_stuff()
{
	int i;

	if (Controls.fire_flare_count > 0)
	{
		Controls.fire_flare_count = 0;
		if (allowed_to_fire_flare())
			Flare_create(ConsoleObject);
	}

	if (allowed_to_fire_missile() && Controls.fire_secondary_state)
		Global_missile_firing_count += Weapon_info[Secondary_weapon_to_weapon_info[Secondary_weapon]].fire_count;

	if (Global_missile_firing_count) {
		do_missile_firing(0);
		Global_missile_firing_count--;
	}

	if (Controls.cycle_primary_count > 0)
	{
		for (i=0;i<Controls.cycle_primary_count;i++)
			CyclePrimary ();
		Controls.cycle_primary_count = 0;
	}
	if (Controls.cycle_secondary_count > 0)
	{
		for (i=0;i<Controls.cycle_secondary_count;i++)
			CycleSecondary ();
		Controls.cycle_secondary_count = 0;
	}
	if (Controls.select_weapon_count > 0)
	{
		Controls.select_weapon_count--;
		do_weapon_select(Controls.select_weapon_count>4?Controls.select_weapon_count-5:Controls.select_weapon_count,Controls.select_weapon_count>4?1:0);
		Controls.select_weapon_count = 0;
	}
	if (Controls.headlight_count > 0)
	{
		for (i=0;i<Controls.headlight_count;i++)
			toggle_headlight_active ();
		Controls.headlight_count = 0;
	}

	if (Global_missile_firing_count < 0)
		Global_missile_firing_count = 0;

	//	Drop proximity bombs.
	while (Controls.drop_bomb_count > 0)
	{
		do_missile_firing(1);
		Controls.drop_bomb_count--;
	}

	if (Controls.toggle_bomb_count > 0)
	{
		int bomb = Secondary_last_was_super[PROXIMITY_INDEX]?PROXIMITY_INDEX:SMART_MINE_INDEX;
	
		if (!Players[Player_num].secondary_ammo[PROXIMITY_INDEX] && !Players[Player_num].secondary_ammo[SMART_MINE_INDEX])
		{
			digi_play_sample_once( SOUND_BAD_SELECTION, F1_0 );
			HUD_init_message(HM_DEFAULT, "No bombs available!");
		}
		else
		{	
			if (Players[Player_num].secondary_ammo[bomb]==0)
			{
				digi_play_sample_once( SOUND_BAD_SELECTION, F1_0 );
				HUD_init_message(HM_DEFAULT, "No %s available!",(bomb==SMART_MINE_INDEX)?"Smart mines":"Proximity bombs");
			}
			else
			{
				Secondary_last_was_super[PROXIMITY_INDEX]=!Secondary_last_was_super[PROXIMITY_INDEX];
				digi_play_sample_once( SOUND_GOOD_SELECTION_SECONDARY, F1_0 );
			}
		}
		Controls.toggle_bomb_count = 0;
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_CONVERTER) {
		bool converter_active =
			Players[Player_num].flags & PLAYER_FLAGS_CONVERTER_ON;

		handle_sticky_key(Controls.energy_to_shield_state,
						  &converter_active,
						  &Converter_down_time,
						  PlayerCfg.KeyStickEnergyConvert);

		Players[Player_num].flags =
			(Players[Player_num].flags & ~(unsigned)PLAYER_FLAGS_CONVERTER_ON) |
			(converter_active ? PLAYER_FLAGS_CONVERTER_ON : 0);

		if (converter_active)
			transfer_energy_to_shield(!Controls.energy_to_shield_state);
	}
}


extern void game_render_frame();
extern void show_extra_views();
extern fix Flash_effect;

void apply_modified_palette(void)
{
//@@    int				k,x,y;
//@@    grs_bitmap	*sbp;
//@@    grs_canvas	*save_canv;
//@@    int				color_xlate[256];
//@@
//@@
//@@    if (!Flash_effect && ((PaletteRedAdd < 10) || (PaletteRedAdd < (PaletteGreenAdd + PaletteBlueAdd))))
//@@		return;
//@@
//@@    reset_cockpit();
//@@
//@@    save_canv = grd_curcanv;
//@@    gr_set_current_canvas(&grd_curscreen->sc_canvas);
//@@
//@@    sbp = &grd_curscreen->sc_canvas.cv_bitmap;
//@@
//@@    for (x=0; x<256; x++)
//@@		color_xlate[x] = -1;
//@@
//@@    for (k=0; k<4; k++) {
//@@		for (y=0; y<grd_curscreen->sc_h; y+= 4) {
//@@			  for (x=0; x<grd_curscreen->sc_w; x++) {
//@@					int	color, new_color;
//@@					int	r, g, b;
//@@					int	xcrd, ycrd;
//@@
//@@					ycrd = y+k;
//@@					xcrd = x;
//@@
//@@					color = gr_ugpixel(sbp, xcrd, ycrd);
//@@
//@@					if (color_xlate[color] == -1) {
//@@						r = gr_palette[color*3+0];
//@@						g = gr_palette[color*3+1];
//@@						b = gr_palette[color*3+2];
//@@
//@@						r += PaletteRedAdd;		 if (r > 63) r = 63;
//@@						g += PaletteGreenAdd;   if (g > 63) g = 63;
//@@						b += PaletteBlueAdd;		if (b > 63) b = 63;
//@@
//@@						color_xlate[color] = gr_find_closest_color_current(r, g, b);
//@@
//@@					}
//@@
//@@					new_color = color_xlate[color];
//@@
//@@					gr_setcolor(new_color);
//@@					gr_upixel(xcrd, ycrd);
//@@			  }
//@@		}
//@@    }
}

void format_time(char *str, int secs_int)
{
	int h, m, s;

	h = secs_int/3600;
	s = secs_int%3600;
	m = s / 60;
	s = s % 60;
	sprintf(str, "%1d:%02d:%02d", h, m, s );
}

extern int netplayerinfo_on;

static void menu_save(void)
{
	if (!Player_is_dead && !((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)))
		state_save_all(0, 0, NULL, 0); // 0 means not between levels.
}

static void menu_load(void)
{
	if (!((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)))
		state_restore_all(1, 0, NULL);
}

enum game_menu {
	GAME_MENU_ABORT,
	GAME_MENU_CONT,
	GAME_MENU_CONFIG,
	GAME_MENU_SAVE,
	GAME_MENU_LOAD,
	GAME_MENU_COUNT
};

static int game_menu_handler(newmenu *menu, d_event *event, void *userdata)
{
	switch (event->type) {
	case EVENT_NEWMENU_SELECTED:
		newmenu_set_visible(menu, false);

		switch(newmenu_get_citem(menu)) {
		case GAME_MENU_ABORT:
			window_close(Game_wind);
			break;
		case GAME_MENU_CONT:
			break;
		case GAME_MENU_CONFIG:
			do_options_menu();
			break;
		case GAME_MENU_SAVE:
			menu_save();
			break;
		case GAME_MENU_LOAD:
			menu_load();
			break;
		default:
			Int3();
		}
		return 0;
	default:;
	}

	return 0;
}

static void run_quit_menu(void)
{
	newmenu_item m[GAME_MENU_COUNT] = {0};

	nm_set_item_menu(&m[GAME_MENU_ABORT], "Abort game");
	nm_set_item_menu(&m[GAME_MENU_CONT], "Continue game");
	nm_set_item_menu(&m[GAME_MENU_CONFIG], "Options...");
	nm_set_item_menu(&m[GAME_MENU_SAVE], "Save game...");
	nm_set_item_menu(&m[GAME_MENU_LOAD], "Load game...");

	newmenu_do2(NULL, "Game Menu", GAME_MENU_COUNT, m, game_menu_handler, NULL, 0, NULL );
}

//Process selected keys until game unpaused
int pause_handler(window *wind, d_event *event, char *msg)
{
	int key;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			break;

		case EVENT_KEY_COMMAND:
			key = event_key_get(event);

			switch (key)
			{
				case 0:
					break;
				case KEY_ESC:
					window_close(wind);
					run_quit_menu();
					return 1;
				case KEY_F1:
					show_help();
					return 1;
				case KEY_ENTER:
				case KEY_PADENTER:
				case KEY_SPACEBAR:
				case KEY_PAUSE:
					window_close(wind);
					return 1;
				default:
					break;
			}
			break;

		case EVENT_IDLE:
			timer_delay2(50);
			break;

		case EVENT_WINDOW_DRAW:
			show_boxed_message(msg, 1);
			break;

		case EVENT_WINDOW_CLOSE:
			songs_resume();
			d_free(msg);
			break;

		default:
			break;
	}

	return 0;
}

int do_game_pause(void)
{
	char *msg;
	char total_time[9],level_time[9];

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		netplayerinfo_on= !netplayerinfo_on;
		return(KEY_PAUSE);
	}
#endif

	MALLOC(msg, char, 1024);
	if (!msg)
		return 0;

	songs_pause();

	format_time(total_time, f2i(Players[Player_num].time_total) + Players[Player_num].hours_total*3600);
	format_time(level_time, f2i(Players[Player_num].time_level) + Players[Player_num].hours_level*3600);
	if (Newdemo_state!=ND_STATE_PLAYBACK)
		snprintf(msg,1024,"PAUSE\n\nSkill level:  %s\nHostages on board:  %d\nTime on level: %s\nTotal time in game: %s",MENU_DIFFICULTY_TEXT(Difficulty_level),Players[Player_num].hostages_on_board,level_time,total_time);
	else
	  	snprintf(msg,1024,"PAUSE\n\nSkill level:  %s\nHostages on board:  %d\n",MENU_DIFFICULTY_TEXT(Difficulty_level),Players[Player_num].hostages_on_board);
	gr_set_current_canvas(NULL);

	if (!window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))pause_handler, msg))
		d_free(msg);

	return 0 /*key*/;	// Keycode returning ripped out (kreatordxx)
}

int HandleEndlevelKey(int key)
{
	switch (key)
	{
		case KEY_COMMAND+KEY_P:
		case KEY_PAUSE:
			do_game_pause();
			return 1;

		case KEY_ESC:
			stop_endlevel_sequence();
			last_drawn_cockpit=-1;
			return 1;
	}

	return 0;
}

int HandleDeathInput(d_event *event)
{
	if (event->type == EVENT_KEY_COMMAND)
	{
		int key = event_key_get(event);

		if (Player_exploded && !key_isfunc(key) && key != KEY_PAUSE && key)
			Death_sequence_aborted  = 1;		//Any key but func or modifier aborts
		if (key == KEY_ESC)
			Death_sequence_aborted = 1;
	}

	if (Player_exploded && (event->type == EVENT_JOYSTICK_BUTTON_UP || event->type == EVENT_MOUSE_BUTTON_UP))
		Death_sequence_aborted = 1;

	if (Death_sequence_aborted)
	{
		game_flush_inputs();
		return 1;
	}

	return 0;
}

int HandleDemoKey(int key)
{
	switch (key) {
		case KEY_F1:	show_newdemo_help();	break;
		case KEY_F2:	do_options_menu();	break;
		case KEY_F3:
			 if (Viewer->type == OBJ_PLAYER)
				toggle_cockpit();
			 break;
		case KEY_F4:	Newdemo_show_percentage = !Newdemo_show_percentage; break;
		case KEY_F7:
#ifdef NETWORK
			Show_kill_list = (Show_kill_list+1) % ((Newdemo_game_mode & GM_TEAM) ? 4 : 3);
#endif
			break;
		case KEY_ESC:
			if (GameArg.SysAutoDemo)
			{
				int choice;
				choice = nm_messagebox(NULL, TXT_ABORT_AUTODEMO, TXT_YES, TXT_NO);
				if (choice == 0)
					GameArg.SysAutoDemo = 0;
				else
					break;
			}
			newdemo_stop_playback();
			break;
		case KEY_UP:
			Newdemo_vcr_state = ND_STATE_PLAYBACK;
			break;
		case KEY_DOWN:
			Newdemo_vcr_state = ND_STATE_PAUSED;
			break;
		case KEY_LEFT:
			newdemo_single_frame_time = GameTime;
			Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
			break;
		case KEY_RIGHT:
			newdemo_single_frame_time = GameTime;
			Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
			break;
		case KEY_CTRLED + KEY_RIGHT:
			newdemo_goto_end(0);
			break;
		case KEY_CTRLED + KEY_LEFT:
			newdemo_goto_beginning();
			break;

		case KEY_PAUSE:
			do_game_pause();
			break;

		case KEY_PRINT_SCREEN:
		{
			if (PlayerCfg.PRShot)
			{
				gr_set_current_canvas(NULL);
				render_frame(0, 0);
				gr_set_curfont(MEDIUM2_FONT);
				gr_printf(SWIDTH-FSPACX(92),SHEIGHT-LINE_SPACING,"DXX-Rebirth\n");
				gr_flip();
				save_screen_shot(0);
			}
			else
			{
				int old_state;
				old_state = Newdemo_show_percentage;
				Newdemo_show_percentage = 0;
				game_render_frame_mono(0);
				if (GameArg.DbgUseDoubleBuffer)
					gr_flip();
				save_screen_shot(0);
				Newdemo_show_percentage = old_state;
			}
			break;
		}

		default:
			return 0;
	}

	return 1;
}

//switch a cockpit window to the next function
int select_next_window_function(int w)
{
	Assert(w==0 || w==1);

	switch (PlayerCfg.Cockpit3DView[w]) {
		case CV_NONE:
			PlayerCfg.Cockpit3DView[w] = CV_REAR;
			break;
		case CV_REAR:
			if (find_escort()) {
				PlayerCfg.Cockpit3DView[w] = CV_ESCORT;
				break;
			}
			//if no ecort, fall through
		case CV_ESCORT:
			Coop_view_player[w] = -1;		//force first player
#ifdef NETWORK
			//fall through
		case CV_COOP:
			Marker_viewer_num[w] = -1;
			if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_TEAM)) {
				PlayerCfg.Cockpit3DView[w] = CV_COOP;
				while (1) {
					Coop_view_player[w]++;
					if (Coop_view_player[w] == (MAX_PLAYERS-1)) {
						PlayerCfg.Cockpit3DView[w] = CV_MARKER;
						goto case_marker;
					}
					if (Players[Coop_view_player[w]].connected != CONNECT_PLAYING)
						continue;
					if (Coop_view_player[w]==Player_num)
						continue;

					if (Game_mode & GM_MULTI_COOP)
						break;
					else if (get_team(Coop_view_player[w]) == get_team(Player_num))
						break;
				}
				break;
			}
			//if not multi, fall through
		case CV_MARKER:
		case_marker:;
			if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) && Netgame.Allow_marker_view) {	//anarchy only
				PlayerCfg.Cockpit3DView[w] = CV_MARKER;
				if (Marker_viewer_num[w] == -1)
					Marker_viewer_num[w] = Player_num * 2;
				else if (Marker_viewer_num[w] == Player_num * 2)
					Marker_viewer_num[w]++;
				else
					PlayerCfg.Cockpit3DView[w] = CV_NONE;
			}
			else
#endif
				PlayerCfg.Cockpit3DView[w] = CV_NONE;
			break;
	}
	write_player_file();

	return 1;	 //screen_changed
}

#ifdef DOOR_DEBUGGING
dump_door_debugging_info()
{
	object *obj;
	vms_vector new_pos;
	fvi_query fq;
	fvi_info hit_info;
	int fate;
	PHYSFS_file *dfile;
	int wall_num;

	obj = &Objects[Players[Player_num].objnum];
	vm_vec_scale_add(&new_pos,&obj->pos,&obj->orient.fvec,i2f(100));

	fq.p0						= &obj->pos;
	fq.startseg				= obj->segnum;
	fq.p1						= &new_pos;
	fq.rad					= 0;
	fq.thisobjnum			= Players[Player_num].objnum;
	fq.ignore_obj_list	= NULL;
	fq.flags					= 0;

	fate = find_vector_intersection(&fq,&hit_info);

	dfile = PHYSFSX_openWriteBuffered("door.out");

	PHYSFSX_printf(dfile,"FVI hit_type = %d\n",fate);
	PHYSFSX_printf(dfile,"    hit_seg = %d\n",hit_info.hit_seg);
	PHYSFSX_printf(dfile,"    hit_side = %d\n",hit_info.hit_side);
	PHYSFSX_printf(dfile,"    hit_side_seg = %d\n",hit_info.hit_side_seg);
	PHYSFSX_printf(dfile,"\n");

	if (fate == HIT_WALL) {

		wall_num = Segments[hit_info.hit_seg].sides[hit_info.hit_side].wall_num;
		PHYSFSX_printf(dfile,"wall_num = %d\n",wall_num);

		if (wall_num != -1) {
			wall *wall = &Walls[wall_num];
			active_door *d;
			int i;

			PHYSFSX_printf(dfile,"    segnum = %d\n",wall->segnum);
			PHYSFSX_printf(dfile,"    sidenum = %d\n",wall->sidenum);
			PHYSFSX_printf(dfile,"    hps = %x\n",wall->hps);
			PHYSFSX_printf(dfile,"    linked_wall = %d\n",wall->linked_wall);
			PHYSFSX_printf(dfile,"    type = %d\n",wall->type);
			PHYSFSX_printf(dfile,"    flags = %x\n",wall->flags);
			PHYSFSX_printf(dfile,"    state = %d\n",wall->state);
			PHYSFSX_printf(dfile,"    trigger = %d\n",wall->trigger);
			PHYSFSX_printf(dfile,"    clip_num = %d\n",wall->clip_num);
			PHYSFSX_printf(dfile,"    keys = %x\n",wall->keys);
			PHYSFSX_printf(dfile,"    controlling_trigger = %d\n",wall->controlling_trigger);
			PHYSFSX_printf(dfile,"    cloak_value = %d\n",wall->cloak_value);
			PHYSFSX_printf(dfile,"\n");


			for (i=0;i<Num_open_doors;i++) {		//find door
				d = &ActiveDoors[i];
				if (d->front_wallnum[0]==wall-Walls || d->back_wallnum[0]==wall-Walls || (d->n_parts==2 && (d->front_wallnum[1]==wall-Walls || d->back_wallnum[1]==wall-Walls)))
					break;
			}

			if (i>=Num_open_doors)
				PHYSFSX_printf(dfile,"No active door.\n");
			else {
				PHYSFSX_printf(dfile,"Active door %d:\n",i);
				PHYSFSX_printf(dfile,"    n_parts = %d\n",d->n_parts);
				PHYSFSX_printf(dfile,"    front_wallnum = %d,%d\n",d->front_wallnum[0],d->front_wallnum[1]);
				PHYSFSX_printf(dfile,"    back_wallnum = %d,%d\n",d->back_wallnum[0],d->back_wallnum[1]);
				PHYSFSX_printf(dfile,"    time = %x\n",d->time);
			}

		}
	}

	PHYSFSX_printf(dfile,"\n");
	PHYSFSX_printf(dfile,"\n");

	PHYSFS_close(dfile);

}
#endif

//this is for system-level keys, such as help, etc.
//returns 1 if screen changed
int HandleSystemKey(int key)
{
	if (!Player_is_dead)
		switch (key)
		{

			#ifdef DOOR_DEBUGGING
			case KEY_LAPOSTRO+KEY_SHIFTED:
				dump_door_debugging_info();
				return 1;
			#endif

			case KEY_ESC:
			{
				run_quit_menu();
				return 1;
			}

			case KEY_SHIFTED+KEY_F1:
				select_next_window_function(0);
				return 1;
			case KEY_SHIFTED+KEY_F2:
				select_next_window_function(1);
				return 1;
		}

	switch (key)
	{
		case KEY_PAUSE:
			do_game_pause();	break;

		case KEY_PRINT_SCREEN:
		{
			if (PlayerCfg.PRShot)
			{
				gr_set_current_canvas(NULL);
				render_frame(0, 0);
				gr_set_curfont(MEDIUM2_FONT);
				gr_printf(SWIDTH-FSPACX(92),SHEIGHT-LINE_SPACING,"DXX-Rebirth\n");
				gr_flip();
				save_screen_shot(0);
			}
			else
			{
				game_render_frame_mono(0);
				if(GameArg.DbgUseDoubleBuffer)
					gr_flip();
				save_screen_shot(0);
			}
			break;
		}

		case KEY_F1:				if (Game_mode & GM_MULTI) show_netgame_help(); else show_help();	break;

		case KEY_F2:
			{
				do_options_menu();
				break;
			}

		case KEY_F3:
			if (!Player_is_dead && Viewer->type==OBJ_PLAYER) //if (!(Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID && Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num] && PlayerCfg.GuidedInBigWindow))
			{
				toggle_cockpit();
			}
			break;

		case KEY_F5:
			if ( Newdemo_state == ND_STATE_RECORDING )
				newdemo_stop_recording();
			else if ( Newdemo_state == ND_STATE_NORMAL )
				newdemo_start_recording();
			break;
#ifdef NETWORK
		case KEY_ALTED + KEY_F4:
			Show_reticle_name = (Show_reticle_name+1)%2;
			break;

		case KEY_F7:
			Show_kill_list = (Show_kill_list+1) % ((Game_mode & GM_TEAM) ? 4 : 3);
			if (Game_mode & GM_MULTI)
				multi_sort_kill_list();
			break;

		case KEY_F8:
			multi_send_message_start();
			break;

		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
			multi_send_macro(key);
			break;		// send taunt macros

		case KEY_SHIFTED + KEY_F9:
		case KEY_SHIFTED + KEY_F10:
		case KEY_SHIFTED + KEY_F11:
		case KEY_SHIFTED + KEY_F12:
			multi_define_macro(key);
			break;		// redefine taunt macros

#endif

		case KEY_ALTED+KEY_F2:
			menu_save();
			break;

		case KEY_ALTED+KEY_F1:	if (!Player_is_dead) state_save_all(0, 0, NULL, 1);	break;
		case KEY_ALTED+KEY_F3:
			menu_load();
			break;

		case KEY_F4 + KEY_SHIFTED:
			do_escort_menu();
			break;

		case KEY_F4 + KEY_SHIFTED + KEY_ALTED:
			change_guidebot_name();
			break;

		case KEY_ALTED + KEY_SHIFTED + KEY_F10:
			songs_pause_resume();
			break;

		case KEY_MINUS + KEY_ALTED:
		case KEY_ALTED + KEY_SHIFTED + KEY_F11:
			songs_play_level_song( Current_level_num, -1 );
			break;
		case KEY_EQUAL + KEY_ALTED:
		case KEY_ALTED + KEY_SHIFTED + KEY_F12:
			songs_play_level_song( Current_level_num, 1 );
			break;

		default:
			return 0;
			break;
	}

	return 1;
}

extern void DropFlag();

int HandleGameKey(int key)
{
	switch (key) {

		case KEY_1 + KEY_SHIFTED:
		case KEY_2 + KEY_SHIFTED:
		case KEY_3 + KEY_SHIFTED:
		case KEY_4 + KEY_SHIFTED:
		case KEY_5 + KEY_SHIFTED:
		case KEY_6 + KEY_SHIFTED:
		case KEY_7 + KEY_SHIFTED:
		case KEY_8 + KEY_SHIFTED:
		case KEY_9 + KEY_SHIFTED:
		case KEY_0 + KEY_SHIFTED:
			if (PlayerCfg.EscortHotKeys)
			{
				if (!(Game_mode & GM_MULTI))
					set_escort_special_goal(key);
				else
					HUD_init_message(HM_DEFAULT, "No Guide-Bot in Multiplayer!");
				game_flush_inputs();
				return 1;
			}

		case KEY_ALTED+KEY_F7:
			PlayerCfg.HudMode=(PlayerCfg.HudMode+1)%GAUGE_HUD_NUMMODES;
			write_player_file();
			return 1;

#ifdef NETWORK
		case KEY_F6:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && !(Game_mode & GM_TEAM))
			{
				RefuseThisPlayer=1;
				HUD_init_message(HM_MULTI, "Player accepted!");
			}
			return 1;
		case KEY_ALTED + KEY_1:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUD_init_message(HM_MULTI, "Player accepted!");
					RefuseTeam=1;
					game_flush_inputs();
				}
			return 1;
		case KEY_ALTED + KEY_2:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUD_init_message(HM_MULTI, "Player accepted!");
					RefuseTeam=2;
					game_flush_inputs();
				}
			return 1;
#endif

		default:
			break;

	}	 //switch (key)

	if (!Player_is_dead)
		switch (key)
		{
			case KEY_F5 + KEY_SHIFTED:
				DropCurrentWeapon();
				break;

			case KEY_F6 + KEY_SHIFTED:
				DropSecondaryWeapon();
				break;

#ifdef NETWORK
			case KEY_0 + KEY_ALTED:
				DropFlag ();
				game_flush_inputs();
				break;
#endif

			case KEY_F4:
				if (!DefiningMarkerMessage)
					InitMarkerInput();
				break;

			default:
				return 0;
		}
	else
		return 0;

	return 1;
}

void kill_all_robots(void)
{
	int	i, dead_count=0;
	//int	boss_index = -1;

	// Kill all bots except for Buddy bot and boss.  However, if only boss and buddy left, kill boss.
	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT) {
			if (!Robot_info[Objects[i].id].companion && !Robot_info[Objects[i].id].boss_flag) {
				dead_count++;
				Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
			}
		}

// --		// Now, if more than boss and buddy left, un-kill boss.
// --		if ((dead_count > 2) && (boss_index != -1)) {
// --			Objects[boss_index].flags &= ~(OF_EXPLODING|OF_SHOULD_BE_DEAD);
// --			dead_count--;
// --		} else if (boss_index != -1)
// --			HUD_init_message(HM_DEFAULT, "Toasted the BOSS!");

	// Toast the buddy if nothing else toasted!
	if (dead_count == 0)
		for (i=0; i<=Highest_object_index; i++)
			if (Objects[i].type == OBJ_ROBOT)
				if (Robot_info[Objects[i].id].companion) {
					Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
					HUD_init_message(HM_DEFAULT, "Toasted the Buddy! *sniff*");
					dead_count++;
				}

	HUD_init_message(HM_DEFAULT, "%i robots toasted!", dead_count);
}

//	--------------------------------------------------------------------------
//	Detonate reactor.
//	Award player all powerups in mine.
//	Place player just outside exit.
//	Kill all bots in mine.
//	Yippee!!
void kill_and_so_forth(void)
{
	int     i, j;

	HUD_init_message(HM_DEFAULT, "Killing, awarding, etc.!");

	for (i=0; i<=Highest_object_index; i++) {
		switch (Objects[i].type) {
			case OBJ_ROBOT:
				Objects[i].flags |= OF_EXPLODING|OF_SHOULD_BE_DEAD;
				break;
			case OBJ_POWERUP:
				do_powerup(&Objects[i]);
				break;
		}
	}

	do_controlcen_destroyed_stuff(NULL);

	for (i=0; i<Num_triggers; i++) {
		if (Triggers[i].type == TT_EXIT) {
			for (j=0; j<Num_walls; j++) {
				if (Walls[j].trigger == i) {
					compute_segment_center(&ConsoleObject->pos, &Segments[Walls[j].segnum]);
					obj_relink(ConsoleObject-Objects,Walls[j].segnum);
					goto kasf_done;
				}
			}
		}
	}

kasf_done: ;

}

int HandleTestKey(int key)
{
	switch (key)
	{

		case KEY_DEBUGGED+KEY_1:	create_special_path();	break;

		case KEY_DEBUGGED+KEY_Y:
			do_controlcen_destroyed_stuff(NULL);
			break;

		case KEY_DEBUGGED+KEY_S:				digi_reset(); break;

		case KEY_DEBUGGED+KEY_P:
			if (Game_suspended & SUSP_ROBOTS)
				Game_suspended &= ~SUSP_ROBOTS;		//robots move
			else
				Game_suspended |= SUSP_ROBOTS;		//robots don't move
			break;

		case KEY_DEBUGGED+KEY_R:
			Robot_firing_enabled = !Robot_firing_enabled;
			break;

		case KEY_DEBUGGED+KEY_R+KEY_SHIFTED:
			kill_all_robots();
			break;

		case KEY_DEBUGGED+KEY_N: Show_view_text_timer = 0x30000; object_goto_next_viewer(); break;
		case KEY_DEBUGGED+KEY_SHIFTED+KEY_N: Viewer=ConsoleObject; break;
		case KEY_DEBUGGED+KEY_O: toggle_outline_mode(); break;
		case KEY_DEBUGGED+KEY_T:
			if (GameArg.SysMaxFPS == 30)
				GameArg.SysMaxFPS = 300;
			else
				GameArg.SysMaxFPS = 30;
			break;

		case KEY_PAD5: slew_stop(); break;

		case KEY_DEBUGGED + KEY_C:
			do_cheat_menu();
			break;
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_A:
			do_megawow_powerup(10);
			break;
		case KEY_DEBUGGED + KEY_A:	{
			do_megawow_powerup(200);
				break;
		}
		case KEY_DEBUGGED+KEY_F:
			GameArg.SysFPSIndicator = !GameArg.SysFPSIndicator;
			break;

		case KEY_DEBUGGED+KEY_SPACEBAR:		//KEY_F7:				// Toggle physics flying
			slew_stop();
			game_flush_inputs();
			if ( ConsoleObject->control_type != CT_FLYING ) {
				fly_init(ConsoleObject);
				Game_suspended &= ~SUSP_ROBOTS;	//robots move
			} else {
				slew_init(ConsoleObject);			//start player slewing
				Game_suspended |= SUSP_ROBOTS;	//robots don't move
			}
			break;

		case KEY_DEBUGGED+KEY_COMMA: Render_zoom = fixmul(Render_zoom,62259); break;
		case KEY_DEBUGGED+KEY_PERIOD: Render_zoom = fixmul(Render_zoom,68985); break;

		case KEY_DEBUGGED+KEY_G:
			GameTime = i2f(0x7fff - 600) - (F1_0*10);
			HUD_init_message(HM_DEFAULT, "GameTime %i - Reset in 10 seconds!", GameTime);
			break;
		default:
			return 0;
			break;
	}

	return 1;
}

//	Cheat functions ------------------------------------------------------------
extern char *jcrypt (char *);

char *LamerCheats[]={   "!UyN#E$I",	// gabba-gabbahey
								"ei5cQ-ZQ", // mo-therlode
								"q^EpZxs8", // c-urrygoat
								"mxk(DyyP", // zi-ngermans
								"cBo#@y@P", // ea-tangelos
								"CLygLBGQ", // e-ricaanne
								"xAnHQxZX", // jos-huaakira
								"cKc[KUWo", // wh-ammazoom
							};

#define N_LAMER_CHEATS (sizeof(LamerCheats) / sizeof(*LamerCheats))

char *WowieCheat        ="F_JMO3CV";    //only Matt knows / h-onestbob
char *AllKeysCheat      ="%v%MrgbU";    //only Matt knows / or-algroove
char *InvulCheat        ="Wv_\\JJ\\Z";  //only Matt knows / almighty
char *HomingCheatString ="t\\LIhSB[";   //only Matt knows / l-pnlizard
char *BouncyCheat       ="bGbiChQJ";    //only Matt knows / duddaboo
char *FullMapCheat      ="PI<XQHRI";    //only Matt knows / rockrgrl
char *LevelWarpCheat    ="ZQHtqbb\"";   //only Matt knows / f-reespace
char *MonsterCheat      ="nfpEfRQp";    //only Matt knows / godzilla
char *BuddyLifeCheat    ="%A-BECuY";    //only Matt knows / he-lpvishnu
char *BuddyDudeCheat    ="u#uzIr%e";    //only Matt knows / g-owingnut
char *KillRobotsCheat   ="&wxbs:5O";    //only Matt knows / spaniard
char *FinishLevelCheat  ="%bG_bZ<D";    //only Matt knows / d-elshiftb
char *RapidFireCheat    ="*jLgHi'J";    //only Matt knows / wildfire

char *RobotsKillRobotsCheat ="rT6xD__S"; // New for 1.1 / silkwing
char *AhimsaCheat       ="!Uscq_yc";    // New for 1.1 / im-agespace

char *AccessoryCheat    ="dWdz[kCK";    // al-ifalafel
char *AcidCheat         ="qPmwxz\"S";   // bit-tersweet
char *FramerateCheat    ="rQ60#ZBN";    // f-rametime

char CheatBuffer[]="AAAAAAAAAAAAAAA";

#define CHEATSPOT 14
#define CHEATEND 15

void do_cheat_penalty ()
 {
  digi_play_sample( SOUND_CHEATER, F1_0);
  Cheats_enabled=1;
  Players[Player_num].score=0;
 }


//	Main Cheat function

char BounceCheat=0;
char HomingCheat=0;
char AcidCheatOn=0;
char OldHomingState[20];
extern char Monster_mode;

extern int Robots_kill_robots_cheat;

static bool FinalCheats(int key)
{
  int i;
  char *cryptstring;

   key=key_ascii();

	if (key == 255)
		return false;

  for (i=0;i<15;i++)
   CheatBuffer[i]=CheatBuffer[i+1];

  CheatBuffer[CHEATSPOT]=key;

  cryptstring=jcrypt(&CheatBuffer[7]);

	for (i=0;i<N_LAMER_CHEATS;i++)
	  if (!(strcmp (cryptstring,LamerCheats[i])))
			{
				 do_cheat_penalty();
				 Players[Player_num].shields=i2f(1);
				 Players[Player_num].energy=i2f(1);
#ifdef NETWORK
		  if (Game_mode & GM_MULTI)
			{
			 Network_message_reciever = 100;		// Send to everyone...
				sprintf( Network_message, "%s is crippled...get him!",Players[Player_num].callsign);
			}
#endif
			HUD_init_message(HM_DEFAULT, "Take that...cheater!");
			return true;
		}

  if (!(strcmp (cryptstring,AcidCheat)))
		{
				if (AcidCheatOn)
				{
				 AcidCheatOn=0;
				 HUD_init_message(HM_DEFAULT, "Coming down...");
				}
				else
				{
				 AcidCheatOn=1;
				 HUD_init_message (HM_DEFAULT, "Going up!");
				}

				return true;

		}

  if (!(strcmp (cryptstring,FramerateCheat)))
		{
			GameArg.SysFPSIndicator = !GameArg.SysFPSIndicator;
			return true;
		}

    if (!strcmp(&CheatBuffer[7], "killboss"))
    {
		bool any = false;
        for (i=0; i<=Highest_object_index; i++) {
            if (Objects[i].type == OBJ_ROBOT && Robot_info[Objects[i].id].boss_flag) {
                Objects[i].shields=i2f(1);
				any = true;
            }
        }
        if (any)
			HUD_init_message(HM_DEFAULT, "Fuck boss fights indeed.");
        return true;
    }

  if (Game_mode & GM_MULTI)
   return false;

  if (!(strcmp (&CheatBuffer[8],"blueorb")))
   {
	 	if (Players[Player_num].shields < MAX_SHIELDS) {
	 		fix boost = 3*F1_0 + 3*F1_0*(NDL - Difficulty_level);
	 		if (Difficulty_level == 0)
	 			boost += boost/2;
	 		Players[Player_num].shields += boost;
	 		if (Players[Player_num].shields > MAX_SHIELDS)
	 			Players[Player_num].shields = MAX_SHIELDS;
	 		powerup_basic(0, 0, 15, SHIELD_SCORE, "%s %s %d",TXT_SHIELD,TXT_BOOSTED_TO,f2ir(Players[Player_num].shields));
			do_cheat_penalty();
	 	} else
			HUD_init_message(HM_DEFAULT|HM_REDUNDANT, TXT_MAXED_OUT,TXT_SHIELD);
		return true;
   }

  if (!(strcmp(cryptstring,BuddyLifeCheat)))
   {
	 do_cheat_penalty();
	 HUD_init_message(HM_DEFAULT, "What's this? Another buddy bot!");
	 create_buddy_bot();
	 return true;
   }


  if (!(strcmp(cryptstring,BuddyDudeCheat)))
   {
	 do_cheat_penalty();
	 Buddy_dude_cheat = !Buddy_dude_cheat;
	 if (Buddy_dude_cheat) {
		HUD_init_message(HM_DEFAULT, "%s gets angry!",PlayerCfg.GuidebotName);
		strcpy(PlayerCfg.GuidebotName,"Wingnut");
	 }
	 else {
		strcpy(PlayerCfg.GuidebotName,PlayerCfg.GuidebotNameReal);
		HUD_init_message(HM_DEFAULT, "%s calms down",PlayerCfg.GuidebotName);
	 }
	 return true;
  }


  if (!(strcmp(cryptstring,MonsterCheat)))
   {
    Monster_mode=1-Monster_mode;
	 do_cheat_penalty();
	 HUD_init_message(HM_DEFAULT, Monster_mode?"Oh no, there goes Tokyo!":"What have you done, I'm shrinking!!");
	 return true;
   }


  if (!(strcmp (cryptstring,BouncyCheat)))
	{
		do_cheat_penalty();
		HUD_init_message(HM_DEFAULT, "Bouncing weapons!");
		BounceCheat=1;
		return true;
	}

	if (!(strcmp(cryptstring,LevelWarpCheat)))
	 {
		newmenu_item m;
		char text[10]="";
		int new_level_num;
		int item;
		nm_set_item_input(&m, 10, text);
		item = newmenu_do( NULL, TXT_WARP_TO_LEVEL, 1, &m, NULL, NULL );
		if (item != -1) {
			new_level_num = atoi(m.text);
			if ((new_level_num >= 1 && new_level_num <= Last_level) ||
				(new_level_num <= -1 && new_level_num >= Last_secret_level))
			{
				StartNewLevel(new_level_num, 0);
				do_cheat_penalty();
				return true;
			}
		}
		return true;
	 }

  if (!(strcmp (cryptstring,WowieCheat)))
	{

				HUD_init_message(HM_DEFAULT, TXT_WOWIE_ZOWIE);
		do_cheat_penalty();

			if (Piggy_hamfile_version < 3) // SHAREWARE
			{
				Players[Player_num].primary_weapon_flags = ~((1<<PHOENIX_INDEX) | (1<<OMEGA_INDEX) | (1<<FUSION_INDEX) | HAS_FLAG(SUPER_LASER_INDEX));
				Players[Player_num].secondary_weapon_flags = ~((1<<SMISSILE4_INDEX) | (1<<MEGA_INDEX) | (1<<SMISSILE5_INDEX));
			}
			else
			{
				Players[Player_num].primary_weapon_flags = 0xffff ^ HAS_FLAG(SUPER_LASER_INDEX);		//no super laser
				Players[Player_num].secondary_weapon_flags = 0xffff;
			}

			Players[Player_num].primary_ammo[VULCAN_INDEX] =
				max(Players[Player_num].primary_ammo[VULCAN_INDEX], VULCAN_AMMO_MAX);

				for (i=0; i<MAX_SECONDARY_WEAPONS; i++) {
					Players[Player_num].secondary_ammo[i] =
						max(Players[Player_num].secondary_ammo[i],
							Secondary_ammo_max[i]);
				}

				if (Piggy_hamfile_version < 3) // SHAREWARE
				{
					Players[Player_num].secondary_ammo[SMISSILE4_INDEX] = 0;
					Players[Player_num].secondary_ammo[SMISSILE5_INDEX] = 0;
					Players[Player_num].secondary_ammo[MEGA_INDEX] = 0;
				}

				if (Game_mode & GM_HOARD)
					Players[Player_num].secondary_ammo[PROXIMITY_INDEX] = 12;

				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_laser_level(Players[Player_num].laser_level, MAX_LASER_LEVEL);

				Players[Player_num].energy = MAX_ENERGY;
				Players[Player_num].laser_level = MAX_SUPER_LASER_LEVEL;
				Players[Player_num].flags |= PLAYER_FLAGS_QUAD_LASERS;
				update_laser_weapon_info();
				return true;
	}


  if (!(strcmp (cryptstring,AllKeysCheat)))
	{
		do_cheat_penalty();
				HUD_init_message(HM_DEFAULT, TXT_ALL_KEYS);
				Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
		return true;
	}

    if (!strcmp(&CheatBuffer[strlen(CheatBuffer) - strlen("unlock")], "unlock"))
    {
        do_cheat_penalty();
        HUD_init_message(HM_DEFAULT, "Unlock doors!");

        for (int n = 0; n < Num_walls; n++)
            Walls[n].flags &= ~(unsigned)WALL_DOOR_LOCKED;

		return true;
    }

    if (!strcmp(&CheatBuffer[strlen(CheatBuffer) - strlen("freewalk")], "freewalk"))
    {
        do_cheat_penalty();
        HUD_init_message(HM_DEFAULT, "Who needs walls?");

        for (int n = 0; n < Num_walls; n++)
            Walls[n].type = WALL_ILLUSION;
        for (int n = 0; n < Num_triggers; n++) {
            if (Triggers[n].type == TT_EXIT)
                Triggers[n].type = NUM_TRIGGER_TYPES;
        }

        return true;
    }

    if (!strcmp(&CheatBuffer[strlen(CheatBuffer) - strlen("wireframe")], "wireframe"))
		toggle_outline_mode();

    if (!strcmp(&CheatBuffer[strlen(CheatBuffer) - strlen("notriggers")], "notriggers"))
    {
        do_cheat_penalty();
        HUD_init_message(HM_DEFAULT, "Disabling fly triggers.");

        for (int n = 0; n < Num_walls; n++)
            Walls[n].trigger = -1;

		return true;
    }

    if (!strcmp(&CheatBuffer[strlen(CheatBuffer) - strlen("fuckrobots")], "fuckrobots"))
	{
		do_cheat_penalty();
		int cnt = 0;

		for (int n = 0; n <= Highest_object_index; n++) {
			if (Objects[n].type == OBJ_ROBOT && !Robot_info[Objects[n].id].boss_flag) {
				apply_damage_to_robot(&Objects[n], Objects[n].shields+1, ConsoleObject-Objects);
				cnt++;
			}
		}

		HUD_init_message(HM_DEFAULT, "fucked %d assholes", cnt);

		return true;
	}

	if (!strcmp(&CheatBuffer[strlen(CheatBuffer) - strlen("cloak")], "cloak"))
	{
		do_cheat_penalty();
		Players[Player_num].flags ^= PLAYER_FLAGS_CLOAKED;
		HUD_init_message(HM_DEFAULT, "%s %s!", "Cloak", (Players[Player_num].flags&PLAYER_FLAGS_CLOAKED)?TXT_ON:TXT_OFF);
		Players[Player_num].cloak_time = GameTime+i2f(1000);
		return true;
	}

	if (!strcmp(&CheatBuffer[strlen(CheatBuffer) - strlen("darkness")], "darkness")) {
		do_cheat_penalty();
		HUD_init_message(HM_DEFAULT, "The light has died.");
		No_flash_effects = true;
	}

	if (!strcmp(&CheatBuffer[strlen(CheatBuffer) - strlen("jumpseg")], "jumpseg")) {
		char num_text[10] = "4809";
		newmenu_item m[1];
		nm_set_item_input(&m[0], sizeof(num_text), num_text);

		newmenu_do(NULL, "Jump to segment", 1, m, NULL, NULL);

		int seg = atoi(num_text);

		if (seg < 0 || seg >= Num_segments) {
			HUD_init_message(HM_DEFAULT, "Invalid segment number.");
		} else {
			vms_vector pos;
			compute_segment_center(&pos, &Segments[seg]);

			ConsoleObject->pos = pos;
			obj_relink(ConsoleObject-Objects, seg);

			do_cheat_penalty();
		}
		return true;
	}

	if (!strcmp(&CheatBuffer[strlen(CheatBuffer) - strlen("showtrigger")], "showtrigger"))
	{
		extern int highlight_seg;
		extern int highlight_side;

		vms_vector p1;
		vm_vec_add(&p1, &Viewer->pos, &Viewer->orient.fvec);

		fvi_query q = {
			.p0 = &Viewer->pos,
			.p1 = &p1,
			.startseg = Viewer->segnum,
			.rad = 0,
			.thisobjnum = Viewer - Objects,
			.flags = FQ_INFINITE|FQ_WALL_SIDES,
		};

		fvi_info hit;
		if (find_vector_intersection(&q, &hit) == HIT_WALL) {
			highlight_seg = hit.hit_seg;
			highlight_side = hit.hit_side;
		}

		return true;
	}

	if (!strcmp(&CheatBuffer[strlen(CheatBuffer) - strlen("debugmode")], "debugmode"))
	{
		Debug_mode = !Debug_mode;
		HUD_init_message(HM_DEFAULT, "Debug mode: %s", Debug_mode ? "on" : "off");
		if (Debug_mode)
			HUD_init_message(HM_DEFAULT, "Please use responsibly.");
		return true;
	}


  if (!(strcmp (cryptstring,InvulCheat)))
		{
		do_cheat_penalty();
				Players[Player_num].flags ^= PLAYER_FLAGS_INVULNERABLE;
				HUD_init_message(HM_DEFAULT, "%s %s!", TXT_INVULNERABILITY, (Players[Player_num].flags&PLAYER_FLAGS_INVULNERABLE)?TXT_ON:TXT_OFF);
				Players[Player_num].invulnerable_time = GameTime+i2f(1000);
				return true;
		}
  if (!(strcmp (cryptstring,AccessoryCheat)))
		{
				do_cheat_penalty();
				Players[Player_num].flags |=PLAYER_FLAGS_HEADLIGHT;
				Players[Player_num].flags |=PLAYER_FLAGS_AFTERBURNER;
				Players[Player_num].flags |=PLAYER_FLAGS_AMMO_RACK;
				Players[Player_num].flags |=PLAYER_FLAGS_CONVERTER;

				Afterburner_charge = f1_0;

				HUD_init_message(HM_DEFAULT, "Accessories!!");
				return true;
		}
  if (!(strcmp (cryptstring,FullMapCheat)))
		{
				do_cheat_penalty();
				Players[Player_num].flags |=PLAYER_FLAGS_MAP_ALL;

				HUD_init_message(HM_DEFAULT, "Full Map!!");
				return true;
		}


  if (!(strcmp (cryptstring,HomingCheatString)))
		{
			if (!HomingCheat) {
				do_cheat_penalty();
				HomingCheat=1;
				for (i=0;i<20;i++)
				 {
				  OldHomingState[i]=Weapon_info[i].homing_flag;
				  Weapon_info[i].homing_flag=1;
				 }
				HUD_init_message(HM_DEFAULT, "Homing weapons!");
				return true;
			}
		}

  if (!(strcmp (cryptstring,KillRobotsCheat)))
		{
				do_cheat_penalty();
				kill_all_robots();
				return true;
		}

  if (!(strcmp (cryptstring,FinishLevelCheat)))
		{
				do_cheat_penalty();
				kill_and_so_forth();
				return true;
		}

	if (!(strcmp (cryptstring,RobotsKillRobotsCheat))) {
		Robots_kill_robots_cheat = !Robots_kill_robots_cheat;
		if (Robots_kill_robots_cheat) {
			HUD_init_message(HM_DEFAULT, "Rabid robots!");
			do_cheat_penalty();
		}
		else
			HUD_init_message(HM_DEFAULT, "Kill the player!");
		return true;
	}

	if (!(strcmp (cryptstring,AhimsaCheat))) {
		Robot_firing_enabled = !Robot_firing_enabled;
		if (!Robot_firing_enabled) {
			HUD_init_message(HM_DEFAULT, "%s", "Robot firing OFF!");
			do_cheat_penalty();
		}
		else
			HUD_init_message(HM_DEFAULT, "%s", "Robot firing ON!");
		return true;
	}

	if (!(strcmp (cryptstring,RapidFireCheat))) {
		if (Laser_rapid_fire) {
			Laser_rapid_fire = 0;
			HUD_init_message(HM_DEFAULT, "%s", "Rapid fire OFF!");
		}
		else {
			Laser_rapid_fire = 0xbada55;
			do_cheat_penalty();
			HUD_init_message(HM_DEFAULT, "%s", "Rapid fire ON!");
		}
		return true;
	}

	return false;

}


// Internal Cheat Menu
void do_cheat_menu()
{
	int mmn;
	newmenu_item mm[16];
	char score_text[21];

	sprintf( score_text, "%d", Players[Player_num].score );

	nm_set_item_checkbox(&mm[0],TXT_INVULNERABILITY,Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE);
	nm_set_item_checkbox(&mm[1],TXT_CLOAKED,Players[Player_num].flags & PLAYER_FLAGS_CLOAKED);
	nm_set_item_checkbox(&mm[2],"All keys",0);
	nm_set_item_number(&mm[3], "% Energy", f2i(Players[Player_num].energy), 0, 200);
	nm_set_item_number(&mm[4], "% Shields", f2i(Players[Player_num].shields), 0, 200);
	nm_set_item_text(& mm[5], "Score:");
	nm_set_item_input(&mm[6], 10, score_text);
	nm_set_item_number(&mm[7], "Laser Level", Players[Player_num].laser_level+1, 0, MAX_SUPER_LASER_LEVEL+1);
	nm_set_item_number(&mm[8], "Missiles", Players[Player_num].secondary_ammo[CONCUSSION_INDEX], 0, 200);

	mmn = newmenu_do("Wimp Menu",NULL,9, mm, NULL, NULL );

	if (mmn > -1 )  {
		if ( mm[0].value )  {
			Players[Player_num].flags |= PLAYER_FLAGS_INVULNERABLE;
			Players[Player_num].invulnerable_time = GameTime+i2f(1000);
		} else
			Players[Player_num].flags &= ~PLAYER_FLAGS_INVULNERABLE;
		if ( mm[1].value )
		{
			Players[Player_num].flags |= PLAYER_FLAGS_CLOAKED;
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_cloak();
			#endif
			ai_do_cloak_stuff();
			Players[Player_num].cloak_time = (GameTime+CLOAK_TIME_MAX>i2f(0x7fff-600)?GameTime-i2f(0x7fff-600):GameTime);
		}
		else
			Players[Player_num].flags &= ~PLAYER_FLAGS_CLOAKED;

		if (mm[2].value) Players[Player_num].flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
		Players[Player_num].energy=i2f(mm[3].value);
		Players[Player_num].shields=i2f(mm[4].value);
		Players[Player_num].score = atoi(mm[6].text);
		//if (mm[7].value) Players[Player_num].laser_level=0;
		//if (mm[8].value) Players[Player_num].laser_level=1;
		//if (mm[9].value) Players[Player_num].laser_level=2;
		//if (mm[10].value) Players[Player_num].laser_level=3;
		Players[Player_num].laser_level = mm[7].value-1;
		Players[Player_num].secondary_ammo[CONCUSSION_INDEX] = mm[8].value;
		init_gauges();
	}
}

int ReadControls(d_event *event)
{
	int key;
	static ubyte exploding_flag=0;

	Player_fired_laser_this_frame=-1;

	if (Player_exploded) {
		if (exploding_flag==0)  {
			exploding_flag = 1;			// When player starts exploding, clear all input devices...
			game_flush_inputs();
		}
	} else {
		exploding_flag=0;
	}
	if (Player_is_dead )
		if (HandleDeathInput(event))
			return 1;

	if (Newdemo_state == ND_STATE_PLAYBACK)
		update_vcr_state();

	if (event->type == EVENT_KEY_COMMAND)
	{
		key = event_key_get(event);

		if (DefiningMarkerMessage)
		{
			return MarkerInputMessage(key);
		}

#ifdef NETWORK
		if ( (Game_mode & GM_MULTI) && (multi_sending_message || multi_defining_message) )
		{
			return multi_message_input_sub(key);
		}
#endif

		if (Endlevel_sequence)
		{
			if (HandleEndlevelKey(key))
				return 1;
		}
		else if (Newdemo_state == ND_STATE_PLAYBACK )
		{
			if (HandleDemoKey(key))
				return 1;
		}
		else
		{
			if (FinalCheats(key))
				return 1;

			if (HandleSystemKey(key)) return 1;
			if (HandleGameKey(key)) return 1;
		}

		if (Debug_mode && HandleTestKey(key))
			return 1;

		if (call_default_handler(event))
			return 1;
	}

	if (!Endlevel_sequence && !Player_is_dead && (Newdemo_state != ND_STATE_PLAYBACK)) {
		kconfig_read_controls(event, 0);
	}

	return 0;
}

void ProcessControls(void)
{
	kconfig_process_controls_frame();

	if (!Endlevel_sequence && !Player_is_dead && (Newdemo_state != ND_STATE_PLAYBACK)) {
		process_rear_view_key();

		// If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
		if ( Controls.automap_state )
		{
			Controls.automap_state = 0;
			if (Player_is_dead || (!((Game_mode & GM_MULTI) && Control_center_destroyed && (Countdown_seconds_left < 10))))
			{
				do_automap(0);
				return;
			}
		}
		if (Player_is_dead)
			return;
		do_weapon_n_item_stuff();
	}
}
