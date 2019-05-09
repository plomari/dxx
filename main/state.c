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
 * Functions to save/restore game state.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "ogl.h"

#include "pstypes.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "dxxerror.h"
#include "gamefont.h"
#include "gameseg.h"
#include "switch.h"
#include "game.h"
#include "newmenu.h"
#include "cfile.h"
#include "fuelcen.h"
#include "hash.h"
#include "key.h"
#include "piggy.h"
#include "player.h"
#include "cntrlcen.h"
#include "morph.h"
#include "weapon.h"
#include "render.h"
#include "gameseq.h"
#include "gauges.h"
#include "newdemo.h"
#include "automap.h"
#include "piggy.h"
#include "paging.h"
#include "titles.h"
#include "text.h"
#include "mission.h"
#include "pcx.h"
#include "u_mem.h"
#include "args.h"
#include "ai.h"
#include "fireball.h"
#include "controls.h"
#include "laser.h"
#include "state.h"
#include "multi.h"
#include "gr.h"
#include "physfsx.h"
#include "playsave.h"

extern void game_disable_cheats();

#define STATE_VERSION 28
#define STATE_COMPATIBLE_VERSION 20
// 0 - Put DGSS (Descent Game State Save) id at tof.
// 1 - Added Difficulty level save
// 2 - Added Cheats_enabled flag
// 3 - Added between levels save.
// 4 - Added mission support
// 5 - Mike changed ai and object structure.
// 6 - Added buggin' cheat save
// 7 - Added other cheat saves and game_id.
// 8 - Added AI stuff for escort and thief.
// 9 - Save palette with screen shot
// 12- Saved last_was_super array
// 13- Saved palette flash stuff
// 14- Save cloaking wall stuff
// 15- Save additional ai info
// 16- Save Light_subtracted
// 17- New marker save
// 18- Took out saving of old cheat status
// 19- Saved cheats_enabled flag
// 20- First_secret_visit
// 22- Omega_charge
// 23- extend object count and make it dynamic (for segments too)
// 24- save Missile_gun, and D2X-XL spawn position
// 25- extend struct wall
// 26- correctly save D2X-XL multi-boss info (old D2 levels use new format too)
//	   save D2X-XL trigger info
// 27- Save D2X-XL equipment centers
//	   add TLV extension records
// 28- Save player.saved_* fields

// Things to change on next incompatible savegame change:
// - add a way to save/restore objects in a backward/forward compatible way

#define NUM_SAVES 255
#define THUMBNAIL_W 100
#define THUMBNAIL_H 50
#define DESC_LENGTH 20

#define END_MAGIC 0xD00FB0A2
// Hack to make old versions read newer savegames.
#define END_MAGIC23 0xD00FB0A1

#define TLV_MAGIC 0xF00DBEEE

typedef struct st24_wall {
	int     segnum,sidenum;
	fix     hps;
	int     linked_wall;
	ubyte   type;
	ubyte   flags;
	ubyte   state;
	sbyte   trigger;
	sbyte   clip_num;
	ubyte   keys;
	sbyte   controlling_trigger;
	sbyte   cloak_value;
} __pack__ st24_wall;

typedef struct tlv_rec {
	uint32_t type;				// one of TLV_TYPE_*
	uint32_t compat_version;	// min required STATE_VERSION of reader
	uint32_t flags;				// TLV_FLAG_* bitfields
	uint32_t size;				// size of the payload after this struct
} __pack__ tlv_rec;

enum {
	TLV_TYPE_SPEEDBOOST 	= 1,

	TLV_FLAG_OPT			= (1 << 0),  // if not understood, skip and ignore
	TLV_FLAG_XL_ONLY		= (1 << 1),  // needed by d2x-xl levels only
									     // (these often require extensions)
};

extern void apply_all_changed_light(void);

extern int Do_appearance_effect;

extern int Laser_rapid_fire;
extern int Physics_cheat_flag;
extern int Lunacy;
extern void do_lunacy_on(void);
extern void do_lunacy_off(void);

extern void init_player_stats_new_ship(void);

void ShowLevelIntro(int level_num);

extern void do_cloak_invul_secret_stuff(fix old_gametime);
extern void copy_defaults_to_robot(object *objp);


extern int First_secret_visit;
extern	fix	Flash_effect, Time_flash_last_played;

char dgss_id[4] = "DGSS";

uint state_game_id;

static void serdes_player(struct serdes *sd, player *pl)
{
	if (sd->loading)
		*pl = (player){0};

	sd_char_arr(sd, pl->callsign);
	sd_ubyte_arr(sd, pl->net_address);
	sd_sbyte(sd, &pl->connected);
	sd_int(sd, &pl->objnum);

	if (sd->file_version < 28) {
		// used to be n_packets_got/n_packets_sent
		sd_pad(sd, 4);
		sd_pad(sd, 4);
	} else {
		sd_fix(sd, &pl->saved_cloak);
		sd_fix(sd, &pl->saved_invulnerable);
	}

	sd_uint(sd, &pl->flags);
	sd_fix(sd, &pl->energy);
	sd_fix(sd, &pl->shields);
	sd_ubyte(sd, &pl->lives);
	sd_sbyte(sd, &pl->level);
	sd_ubyte(sd, &pl->laser_level);
	sd_sbyte(sd, &pl->starting_level);
	sd_short(sd, &pl->killer_objnum);
	sd_ushort(sd, &pl->primary_weapon_flags);
	sd_ushort(sd, &pl->secondary_weapon_flags);
	sd_ushort_arr(sd, pl->primary_ammo);
	sd_ushort_arr(sd, pl->secondary_ammo);

	sd_pad(sd, 2);

	sd_int(sd, &pl->last_score);
	sd_int(sd, &pl->score);
	sd_fix(sd, &pl->time_level);
	sd_fix(sd, &pl->time_total);

	sd_fix(sd, &pl->cloak_time);
	sd_fix(sd, &pl->invulnerable_time);

	sd_short(sd, &pl->KillGoalCount);
	sd_short(sd, &pl->net_killed_total);
	sd_short(sd, &pl->net_kills_total);
	sd_short(sd, &pl->num_kills_level);
	sd_short(sd, &pl->num_kills_total);
	sd_short(sd, &pl->num_robots_level);
	sd_short(sd, &pl->num_robots_total);
	sd_ushort(sd, &pl->hostages_rescued_total);
	sd_ushort(sd, &pl->hostages_total);
	sd_ubyte(sd, &pl->hostages_on_board);
	sd_ubyte(sd, &pl->hostages_level);
	sd_fix(sd, &pl->homing_object_dist);
	sd_sbyte(sd, &pl->hours_level);
	sd_sbyte(sd, &pl->hours_total);
}

//-------------------------------------------------------------------
int state_callback(newmenu *menu, d_event *event, grs_bitmap *sc_bmp[])
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	
	if ( (citem >= 0) && (event->type == EVENT_NEWMENU_DRAW) )
	{
		if ( sc_bmp[citem] )	{
			ogl_ubitmapm_cs((grd_curcanv->cv_w/2)-FSPACX(THUMBNAIL_W/2),items[0].y-FSPACY(10)-THUMBNAIL_H*FSPACY(1),THUMBNAIL_W*FSPACX(1),THUMBNAIL_H*FSPACY(1),sc_bmp[citem],-1,F1_0);
		}
		
		return 1;
	}
	
	return 0;
}

#define SAVEGAME_FNAME_LEN (CALLSIGN_LEN + 20)

static void gen_savegame_fname(char *dst, size_t dst_size, int num, bool secret)
{
	if (secret) {
		snprintf(dst, dst_size, "%dsecret.sgc", num + 1);
	} else {
		snprintf(dst, dst_size, "%s.sg%d", Players[Player_num].callsign, num);
	}
}

//Since GameCfg.LastSavegameSlot should ALWAYS point to a valid savegame slot, we use this to check if we once already actually SAVED a game. If yes, state_quick_item will be equal GameCfg.LastSavegameSlot, otherwise it should be -1 on every new mission and tell us we need to select a slot for quicksave.
int state_quick_item = -1;

/* Present a menu for selection of a savegame filename.
 * For saving, dsc should be a pre-allocated buffer into which the new
 * savegame description will be stored.
 * For restoring, dsc should be NULL, in which case empty slots will not be
 * selectable and savagames descriptions will not be editable.
 */
int state_get_savegame_filename(char * fname, char * dsc, char * caption, int blind_save)
{
	PHYSFS_file * fp;
	int i, choice, version, nsaves;
	newmenu_item m[NUM_SAVES];
	char filename[NUM_SAVES][SAVEGAME_FNAME_LEN];
	char desc[NUM_SAVES][DESC_LENGTH + 16];
	grs_bitmap *sc_bmp[NUM_SAVES];
	char id[5];
	int valid;

	// Padding for the thumbnail
	caption = tprintf(50, "%s\n\n\n\n\n", caption);

	nsaves=0;
	for (i=0;i<NUM_SAVES; i++ )	{
		sc_bmp[i] = NULL;
		gen_savegame_fname(filename[i], sizeof(filename[i]), i, false);
		valid = 0;
		fp = PHYSFSX_openReadBuffered(filename[i]);
		if ( fp ) {
			//Read id
			PHYSFS_read(fp, id, 4, 1);
			if ( !memcmp( id, dgss_id, 4 )) {
				//Read version
				PHYSFS_read(fp, &version, sizeof(int), 1);
				if (version >= STATE_COMPATIBLE_VERSION) {
					// Read description
					cfile_read_fixed_str(fp, DESC_LENGTH, desc[i]);
					// Read thumbnail
					sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W,THUMBNAIL_H );
					PHYSFS_read(fp, sc_bmp[i]->bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);
					if (version >= 9) {
						ubyte pal[256*3];
						PHYSFS_read(fp, pal, 3, 256);
						gr_remap_bitmap_good( sc_bmp[i], pal, -1, -1 );
					}
					nsaves++;
					valid = 1;
				}
			}
			PHYSFS_close(fp);
		} 
		m[i].type = dsc ? NM_TYPE_INPUT_MENU : NM_TYPE_MENU;
		if (!valid) {
			strcpy( desc[i], TXT_EMPTY );
			if (dsc == NULL)
				m[i].type = NM_TYPE_TEXT;
		}
		m[i].text_len = DESC_LENGTH-1;
		m[i].text = desc[i];
	}

	if ( dsc == NULL && nsaves < 1 )	{
		nm_messagebox( NULL, "No saved games were found!", "Ok" );
		return 0;
	}

	if (blind_save && state_quick_item < 0)
		blind_save = 0;		// haven't picked a slot yet

	if (blind_save)
		choice = GameCfg.LastSavegameSlot;
	else
		choice = newmenu_do2( NULL, caption, NUM_SAVES, m, (int (*)(newmenu *, d_event *, void *))state_callback, sc_bmp, GameCfg.LastSavegameSlot, NULL );

	for (i=0; i<NUM_SAVES; i++ )	{
		if ( sc_bmp[i] )
			gr_free_bitmap( sc_bmp[i] );
	}

	if (choice >= 0) {
		strcpy( fname, filename[choice] );
		if ( dsc != NULL ) strcpy( dsc, desc[choice] );
		state_quick_item = choice;
		if (GameCfg.LastSavegameSlot != choice) {
			GameCfg.LastSavegameSlot = choice;
			WriteConfigFile();
		}
		return choice;
	}
	return -1;
}

static int state_get_save_file(char * fname, char * dsc, int blind_save)
{
	return state_get_savegame_filename(fname, dsc, "Save Game", blind_save);
}

static int state_get_restore_file(char * fname )
{
	return state_get_savegame_filename(fname, NULL, "Select Game to Restore", 0);
}

#define	DESC_OFFSET	8

//	-----------------------------------------------------------------------------------
//	Imagine if C had a function to copy a file...
int copy_file(char *old_file, char *new_file)
{
	sbyte	*buf;
	int		buf_size;
	PHYSFS_file *in_file, *out_file;

	out_file = PHYSFS_openWrite(new_file);

	if (out_file == NULL)
		return -1;

	in_file = PHYSFS_openRead(old_file);

	if (in_file == NULL)
		return -2;

	buf_size = PHYSFS_fileLength(in_file);
	while (buf_size && !(buf = d_malloc(buf_size)))
		buf_size /= 2;
	if (buf_size == 0)
		return -5;	// likely to be an empty file

	while (!PHYSFS_eof(in_file))
	{
		int bytes_read;

		bytes_read = PHYSFS_read(in_file, buf, 1, buf_size);
		if (bytes_read < 0)
			Error("Cannot read from file <%s>", old_file);

		Assert(bytes_read == buf_size || PHYSFS_eof(in_file));

		if (PHYSFS_write(out_file, buf, 1, bytes_read) < bytes_read)
			Error("Cannot write to file <%s>", new_file);
	}

	d_free(buf);

	if (!PHYSFS_close(in_file))
	{
		PHYSFS_close(out_file);
		return -3;
	}

	if (!PHYSFS_close(out_file))
		return -4;

	return 0;
}

extern int Final_boss_is_dead;

//	-----------------------------------------------------------------------------------
int state_save_all(int between_levels, int secret_save, char *filename_override, int blind_save)
{
	int	filenum = -1;
	char	filename[128], desc[DESC_LENGTH+1];
	int i,j;
	PHYSFS_file *fp;
	ubyte *pal;
	GLint gl_draw_buffer;

	Assert(between_levels == 0);	//between levels save ripped out

#ifdef NETWORK
	if ( Game_mode & GM_MULTI )	{
		return 0;
	}
#endif

	if (Final_boss_is_dead)		//don't allow save while final boss is dying
		return 0;

	//	If this is a secret save and the control center has been destroyed, don't allow
	//	return to the base level.
	if (secret_save && (Control_center_destroyed)) {
		PHYSFS_delete(SECRETB_FILENAME);
		return 0;
	}

	stop_time();

	if (secret_save == 1) {
		filename_override = filename;
		sprintf(filename_override, SECRETB_FILENAME);
	} else if (secret_save == 2) {
		filename_override = filename;
		sprintf(filename_override, SECRETC_FILENAME);
	} else {
		filenum = state_get_save_file(filename, desc, blind_save);
		if (filenum < 0) {
			start_time();
			return 0;
		}
	}
		
	//	Do special secret level stuff.
	//	If secret.sgc exists, then copy it to the savegame slot file.
	//	If it doesn't exist, then delete the savegame slot file.
	//	Same for secret.sgb if saving from a secret level.
	if (!secret_save) {
		int	rval;

		if (filenum >= 0) {
			char temp_fname[SAVEGAME_FNAME_LEN];

			gen_savegame_fname(temp_fname, sizeof(temp_fname), filenum, true);

			if (PHYSFS_exists(temp_fname))
			{
				if (!PHYSFS_delete(temp_fname) && cfexist(temp_fname))
					Error("Cannot delete file <%s>", temp_fname);
			}

			char *target = Current_level_num < 0
				? SECRETB_FILENAME
				: SECRETC_FILENAME;

			if (PHYSFS_exists(target))
			{
				rval = copy_file(target, temp_fname);
				Assert(rval == 0);	//	Oops, error copying secret.sgc to temp_fname!
			}
		}
	}

	fp = PHYSFSX_openWriteBuffered(filename);
	if ( !fp ) {
		nm_messagebox(NULL, "Error writing savegame.\nPossibly out of disk\nspace.", TXT_OK);
		start_time();
		return 0;
	}

	struct serdes *sd = &(struct serdes){
		.fp = fp,
		.loading = false,
		.file_version = STATE_VERSION,
	};

//Save id
	PHYSFS_write(fp, dgss_id, 4, 1);

//Save version
	i = STATE_VERSION;
	PHYSFS_write(fp, &i, sizeof(int), 1);

//Save description
	cfile_write_fixed_str(fp, DESC_LENGTH, desc);
	
// Save the current screen shot...

	if (1) {
		grs_canvas cnv;
		gr_init_sub_canvas(&cnv, &grd_curscreen->sc_canvas, 0, 0, THUMBNAIL_W, THUMBNAIL_H);

		grs_canvas * cnv_save;
		cnv_save = grd_curcanv;

		gr_set_current_canvas(&cnv);

		render_frame(0, 0);

		gr_set_current_canvas(cnv_save);

		ubyte *buf = d_malloc(THUMBNAIL_W * THUMBNAIL_H * 4);
		ubyte *thm = d_malloc(THUMBNAIL_W * THUMBNAIL_H);
		glGetIntegerv(GL_DRAW_BUFFER, &gl_draw_buffer);
		glReadBuffer(gl_draw_buffer);
		glReadPixels(0, SHEIGHT - THUMBNAIL_H, THUMBNAIL_W, THUMBNAIL_H, GL_RGBA, GL_UNSIGNED_BYTE, buf);
		for (int y = 0; y < THUMBNAIL_H; y++) {
			ubyte *dst = &thm[(THUMBNAIL_H - 1 - y) * THUMBNAIL_W];
			ubyte *src = &buf[y * THUMBNAIL_W * 4];
			for (int x = 0; x < THUMBNAIL_W; x++) {
				*dst++ = gr_find_closest_color(src[0] / 4, src[1] / 4, src[2] / 4);
				src += 4;
			}
		}
		d_free(buf);

		pal = gr_palette;

		PHYSFS_write(fp, thm, THUMBNAIL_W * THUMBNAIL_H, 1);
		d_free(thm);

		PHYSFS_write(fp, pal, 3, 256);
	}
	else
	{
	 	ubyte color = 0;
	 	for ( i=0; i<THUMBNAIL_W*THUMBNAIL_H; i++ )
			PHYSFS_write(fp, &color, sizeof(ubyte), 1);		
	} 

// Save the Between levels flag... now reused for secret level saving.
	int write_entered_from_level = 0;
	if (Current_level_num < 0)
		write_entered_from_level = Entered_from_level;
	PHYSFS_write(fp, &write_entered_from_level, sizeof(int), 1);

// Save the mission info...
	cfile_write_fixed_str(fp, 9, Current_mission_filename);

//Save level info
	PHYSFS_write(fp, &Current_level_num, sizeof(int), 1);
	PHYSFS_write(fp, &Next_level_num, sizeof(int), 1);

//Save GameTime
	PHYSFS_write(fp, &GameTime, sizeof(fix), 1);

//Save player info
	serdes_player(sd, &Players[Player_num]);

// Save the current weapon info
	PHYSFS_write(fp, &Primary_weapon, sizeof(sbyte), 1);
	PHYSFS_write(fp, &Secondary_weapon, sizeof(sbyte), 1);

// Save the difficulty level
	PHYSFS_write(fp, &Difficulty_level, sizeof(int), 1);

// Save cheats enabled
	PHYSFS_write(fp, &Cheats_enabled, sizeof(int), 1);

	if ( !between_levels )	{

	//Finish all morph objects
		for (i=0; i<=Highest_object_index; i++ )	{
			if ( (Objects[i].type != OBJ_NONE) && (Objects[i].render_type==RT_MORPH))	{
				morph_data *md;
				md = find_morph_data(&Objects[i]);
				if (md) {					
					md->obj->control_type = md->morph_save_control_type;
					md->obj->movement_type = md->morph_save_movement_type;
					md->obj->render_type = RT_POLYOBJ;
					md->obj->mtype.phys_info = md->morph_save_phys_info;
					md->obj = NULL;
				} else {						//maybe loaded half-morphed from disk
					Objects[i].flags |= OF_SHOULD_BE_DEAD;
					Objects[i].render_type = RT_POLYOBJ;
					Objects[i].control_type = CT_NONE;
					Objects[i].movement_type = MT_NONE;
				}
			}
		}
	
	//Save object info
		i = Highest_object_index+1;
		PHYSFS_write(fp, &i, sizeof(int), 1);
		PHYSFS_write(fp, Objects, sizeof(object), i);
		
	//Save wall info
		i = Num_walls;
		PHYSFS_write(fp, &i, sizeof(int), 1);
		PHYSFS_write(fp, Walls, sizeof(wall), i);

	//Save exploding wall info
		i = MAX_EXPLODING_WALLS;
		PHYSFS_write(fp, &i, sizeof(int), 1);
		PHYSFS_write(fp, expl_wall_list, sizeof(*expl_wall_list), i);
	
	//Save door info
		i = Num_open_doors;
		PHYSFS_write(fp, &i, sizeof(int), 1);
		PHYSFS_write(fp, ActiveDoors, sizeof(active_door), i);
	
	//Save cloaking wall info
		i = Num_cloaking_walls;
		PHYSFS_write(fp, &i, sizeof(int), 1);
		PHYSFS_write(fp, CloakingWalls, sizeof(cloaking_wall), i);
	
	//Save trigger info
		PHYSFS_write(fp, &Num_triggers, sizeof(int), 1);
		PHYSFS_write(fp, &Num_object_triggers, sizeof(int), 1);
		for (i = 0; i < Num_triggers + Num_object_triggers; i++) {
			trigger *trig = &Triggers[i];
			CFILE_var_w(fp, &trig->type);
			CFILE_var_w(fp, &trig->num_links);
			CFILE_var_w(fp, &trig->value);
			CFILE_var_w(fp, &trig->time);
			CFILE_var_w(fp, &trig->flags);
			CFILE_var_w(fp, &trig->time_b);
			CFILE_var_w(fp, &trig->last_operated);
			CFILE_var_w(fp, &trig->last_player);
			CFILE_var_w(fp, &trig->object_id);
			PHYSFS_write(fp, trig->seg, sizeof(trig->seg[0]), trig->num_links);
			PHYSFS_write(fp, trig->side, sizeof(trig->side[0]), trig->num_links);
		}
	
	//Save tmap info
		for (i = 0; i <= Highest_segment_index; i++)
		{
			for (j = 0; j < 6; j++)
			{
				PHYSFS_write(fp, &Segments[i].sides[j].wall_num, sizeof(short), 1);
				PHYSFS_write(fp, &Segments[i].sides[j].tmap_num, sizeof(short), 1);
				PHYSFS_write(fp, &Segments[i].sides[j].tmap_num2, sizeof(short), 1);
			}
		}
	
	// Save the fuelcen info
		PHYSFS_write(fp, &Control_center_destroyed, sizeof(int), 1);
		PHYSFS_write(fp, &Countdown_timer, sizeof(int), 1);
		PHYSFS_write(fp, &Num_robot_centers, sizeof(int), 1);
		PHYSFS_write(fp, RobotCenters, sizeof(matcen_info), Num_robot_centers);
		PHYSFS_write(fp, &ControlCenterTriggers, sizeof(control_center_triggers), 1);
		PHYSFS_write(fp, &Num_fuelcenters, sizeof(int), 1);
		PHYSFS_write(fp, Station, sizeof(FuelCenter), Num_fuelcenters);
	
	// Save the control cen info
		PHYSFS_write(fp, &Control_center_been_hit, sizeof(int), 1);
		PHYSFS_write(fp, &Control_center_player_been_seen, sizeof(int), 1);
		PHYSFS_write(fp, &Control_center_next_fire_time, sizeof(int), 1);
		PHYSFS_write(fp, &Control_center_present, sizeof(int), 1);
		PHYSFS_write(fp, &(int){0}, sizeof(int), 1); // Dead_controlcen_object_num
	
	// Save the AI state
		ai_save_state( fp );
	
	// Save the automap visited info
		PHYSFS_write(fp, Automap_visited, sizeof(ubyte), Highest_segment_index + 1);

	}
	PHYSFS_write(fp, &state_game_id, sizeof(uint), 1);
	PHYSFS_write(fp, &Laser_rapid_fire, sizeof(int), 1);
	PHYSFS_write(fp, &Lunacy, sizeof(int), 1);  //  Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
	PHYSFS_write(fp, &Lunacy, sizeof(int), 1);

	// Save automap marker info

	PHYSFS_write(fp, MarkerObject, sizeof(MarkerObject) ,1);
	for (int n = 0; n < 144; n++)
		PHYSFS_write(fp, &(char){0}, 1, 1); // skip obsolete MarkerOwner
	PHYSFS_write(fp, MarkerMessage, sizeof(MarkerMessage), 1);

	PHYSFS_write(fp, &Afterburner_charge, sizeof(fix), 1);

	//save last was super information
	PHYSFS_write(fp, &Primary_last_was_super, sizeof(Primary_last_was_super), 1);
	PHYSFS_write(fp, &Secondary_last_was_super, sizeof(Secondary_last_was_super), 1);

	//	Save flash effect stuff
	PHYSFS_write(fp, &Flash_effect, sizeof(int), 1);
	PHYSFS_write(fp, &Time_flash_last_played, sizeof(int), 1);
	PHYSFS_write(fp, &PaletteRedAdd, sizeof(int), 1);
	PHYSFS_write(fp, &PaletteGreenAdd, sizeof(int), 1);
	PHYSFS_write(fp, &PaletteBlueAdd, sizeof(int), 1);

        i = Highest_segment_index + 1;
	PHYSFS_write(fp, Light_subtracted, sizeof(Light_subtracted[0]), i);

	PHYSFS_write(fp, &First_secret_visit, sizeof(First_secret_visit), 1);

        PHYSFS_write(fp, &Omega_charge, sizeof(Omega_charge), 1);

	// Beyond this point we might write optional data or D2X-XL only data.
	uint32_t magic = END_MAGIC23;
	PHYSFS_write(fp, &magic, sizeof(magic), 1);

	// Save missile alternating gun point (left/right).
	PHYSFS_write(fp, &Missile_gun, sizeof(Missile_gun), 1);
	// Save spawn point (can change at runtime with D2X-XL levels)
	PHYSFS_write(fp, &Player_init[Player_num], sizeof(Player_init[Player_num]), 1);

	PHYSFS_write(fp, &Num_equip_centers, sizeof(int), 1);
	PHYSFS_write(fp, EquipCenters, sizeof(matcen_info), Num_equip_centers);

	tlv_rec tlv = {
		.type = TLV_TYPE_SPEEDBOOST,
		.flags = TLV_FLAG_OPT | TLV_FLAG_XL_ONLY,
		.compat_version = 27,
		.size = sizeof(Player_Speedboost),
	};
	magic = TLV_MAGIC;
	PHYSFS_write(fp, &magic, sizeof(magic), 1);
	PHYSFS_write(fp, &tlv, sizeof(tlv), 1);
	PHYSFS_write(fp, &Player_Speedboost, sizeof(Player_Speedboost), 1);

	magic = END_MAGIC;
	if (PHYSFS_write(fp, &magic, sizeof(magic), 1) < 1)
	{
		nm_messagebox(NULL, "Error writing savegame.\nPossibly out of disk\nspace.", TXT_OK);
		PHYSFS_close(fp);
		PHYSFS_delete(filename);
	} else  {
		PHYSFS_close(fp);
	}
	
	start_time();

	if (!secret_save)
		HUD_init_message(HM_DEFAULT, "Game saved");

	return 1;
}

//	-----------------------------------------------------------------------------------
//	Set the player's position from the globals Secret_return_segment and Secret_return_orient.
void set_pos_from_return_segment(void)
{
	int	plobjnum = Players[Player_num].objnum;

	compute_segment_center(&Objects[plobjnum].pos, &Segments[Secret_return_segment]);
	obj_relink(plobjnum, Secret_return_segment);
	reset_player_object();
	Objects[plobjnum].orient = Secret_return_orient;
}

//	-----------------------------------------------------------------------------------
int state_restore_all(int in_game, int secret_restore, char *filename_override)
{
	char filename[128];
	int	filenum = -1;
	int ObjectStartLocation;
	int version,i, j, segnum;
	object * obj;
	PHYSFS_file *fp;
	int current_level, next_level;
	int between_levels = 0;
	char desc[DESC_LENGTH+1];
	char id[5];
	char org_callsign[CALLSIGN_LEN+16];
	fix	old_gametime = GameTime;
	short TempTmapNum[MAX_SEGMENTS][MAX_SIDES_PER_SEGMENT];
	short TempTmapNum2[MAX_SEGMENTS][MAX_SIDES_PER_SEGMENT];

#ifdef NETWORK
	if ( Game_mode & GM_MULTI )	{
		return 0;
	}
#endif

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_stop_recording();

	if ( Newdemo_state != ND_STATE_NORMAL )
		return 0;

	stop_time();

	if (filename_override) {
		strcpy(filename, filename_override);
	} else {
		filenum = state_get_restore_file(filename);
		if (filenum < 0) {
			start_time();
			return 0;
		}
	}

	if ( !secret_restore && in_game ) {
		int choice;
		choice =  nm_messagebox( NULL, "Restore Game?", "Yes", "No" );
		if ( choice != 0 )	{
			start_time();
			return 0;
		}
	}

	start_time();

	fp = PHYSFSX_openReadBuffered(filename);
	if ( !fp ) return 0;

	struct serdes *sd = &(struct serdes){
		.fp = fp,
		.loading = true,
	};

//Read id
	//FIXME: check for swapped file, react accordingly...
	PHYSFS_read(fp, id, 4, 1);
	if ( memcmp( id, dgss_id, 4 )) {
		PHYSFS_close(fp);
		return 0;
	}

//Read version
	PHYSFS_read(fp, &version, sizeof(int), 1);
	if (version < STATE_COMPATIBLE_VERSION)	{
		PHYSFS_close(fp);
		return 0;
	}

	sd->file_version = version;

// Read description
	cfile_read_fixed_str(fp, DESC_LENGTH, desc);

// Skip the current screen shot...
	PHYSFS_seek(fp, PHYSFS_tell(fp) + THUMBNAIL_W * THUMBNAIL_H);

// And now...skip the goddamn palette stuff that somebody forgot to add
	PHYSFS_seek(fp, PHYSFS_tell(fp) + 768);

// Read the Between levels flag... reused for secret level stuff
	int read_entered_from_level = 0;
	PHYSFS_read(fp, &read_entered_from_level, sizeof(int), 1);

// Read the mission info...
	char mission[10];
	cfile_read_fixed_str(fp, 9, mission);

	if (!load_mission_by_name( mission ))	{
		nm_messagebox( NULL, tprintf(80, "Error!\nUnable to load mission\n'%s'\n", mission), "Ok");
		PHYSFS_close(fp);
		return 0;
	}

//Read level info
	PHYSFS_read(fp, &current_level, sizeof(int), 1);
	PHYSFS_read(fp, &next_level, sizeof(int), 1);

	if (current_level < 0) {
		// Reuse this (otherwise unused) field for secret level savegames.
		Entered_from_level = read_entered_from_level;
	} else {
		Assert(read_entered_from_level == 0);	//between levels save ripped out
	}

	//	MK, 1/1/96
	//	Do special secret level stuff.
	//	If savegame slot exists, then copy it to secret.sgc.
	//	If it doesn't exist, then delete secret.sgc
	//	If we're loading a game saved from the secret level (as opposed to
	//	running this function when changing between secret and normal levels),
	//	then do the same with secret.sgb.
	if (!secret_restore) {
		int	rval;

		if (filenum >= 0) {
			char temp_fname[SAVEGAME_FNAME_LEN];

			gen_savegame_fname(temp_fname, sizeof(temp_fname), filenum, true);

			PHYSFS_delete(SECRETB_FILENAME);
			PHYSFS_delete(SECRETC_FILENAME);

			char *target = current_level < 0
				? SECRETB_FILENAME
				: SECRETC_FILENAME;

			if (PHYSFS_exists(temp_fname))
			{
				rval = copy_file(temp_fname, target);
				Assert(rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
			}

		}
	}

//Restore GameTime
	PHYSFS_read(fp, &GameTime, sizeof(fix), 1);

// Start new game....
	Game_mode = GM_NORMAL;
#ifdef NETWORK
	change_playernum_to(0);
#endif
	strcpy( org_callsign, Players[0].callsign );
	N_players = 1;
	if (!secret_restore) {
		InitPlayerObject();				//make sure player's object set up
		init_player_stats_game();		//clear all stats
	}

//Read player info

	{
		StartNewLevelSub(current_level, 1, secret_restore);

		player saved_player;
		serdes_player(sd, &saved_player);

		if (secret_restore) {

			if (secret_restore == 1) {		//	This means he didn't die, so he keeps what he got in the secret level.
				Players[Player_num].level = saved_player.level;
				Players[Player_num].last_score = saved_player.last_score;
				Players[Player_num].time_level = saved_player.time_level;

				Players[Player_num].num_robots_level = saved_player.num_robots_level;
				Players[Player_num].num_robots_total = saved_player.num_robots_total;
				Players[Player_num].hostages_rescued_total = saved_player.hostages_rescued_total;
				Players[Player_num].hostages_total = saved_player.hostages_total;
				Players[Player_num].hostages_on_board = saved_player.hostages_on_board;
				Players[Player_num].hostages_level = saved_player.hostages_level;
				Players[Player_num].homing_object_dist = saved_player.homing_object_dist;
				Players[Player_num].hours_level = saved_player.hours_level;
				Players[Player_num].hours_total = saved_player.hours_total;
				do_cloak_invul_secret_stuff(old_gametime);
				saved_player = Players[Player_num];
			} else {
				// Keep keys even if they died on secret level (otherwise game becomes impossible)
				// Example: Cameron 'Stryker' Fultz's Area 51
				int preserve_flags = PLAYER_FLAGS_BLUE_KEY |
									 PLAYER_FLAGS_RED_KEY |
									 PLAYER_FLAGS_GOLD_KEY;
				if (PlayerCfg.ExtendedAmmoRack)
					preserve_flags |= PLAYER_FLAGS_AMMO_RACK;
				preserve_flags |= PLAYER_FLAGS_SETTINGS;
				saved_player.flags |= Players[Player_num].flags & preserve_flags;
			}
		}

		Players[Player_num] = saved_player;
	}
	strcpy( Players[Player_num].callsign, org_callsign );

// Set the right level
	if ( between_levels )
		Players[Player_num].level = next_level;

// Restore the weapon states
	PHYSFS_read(fp, &Primary_weapon, sizeof(sbyte), 1);
	PHYSFS_read(fp, &Secondary_weapon, sizeof(sbyte), 1);

	select_weapon(Primary_weapon, 0, 0, 0);
	select_weapon(Secondary_weapon, 1, 0, 0);

// Restore the difficulty level
	PHYSFS_read(fp, &Difficulty_level, sizeof(int), 1);

// Restore the cheats enabled flag

	game_disable_cheats(); // disable cheats first
	PHYSFS_read(fp, &Cheats_enabled, sizeof(int), 1);

	if ( !between_levels )	{
		Do_appearance_effect = 0;			// Don't do this for middle o' game stuff.

		ObjectStartLocation = PHYSFS_tell(fp);
		//Clear out all the objects from the lvl file
		for (segnum=0; segnum <= Highest_segment_index; segnum++)
			Segments[segnum].objects = -1;
		reset_objects(1);
	
		//Read objects, and pop 'em into their respective segments.
		PHYSFS_read(fp, &i, sizeof(int), 1);
		Highest_object_index = i-1;
		PHYSFS_read(fp, Objects, sizeof(object) * i, 1);
	
		for (i=0; i<=Highest_object_index; i++ )	{
			obj = &Objects[i];
			obj->rtype.pobj_info.alt_textures = -1;
			segnum = obj->segnum;
			obj->next = obj->prev = obj->segnum = -1;
			if ( obj->type != OBJ_NONE )	{
				obj_link(i,segnum);
			}

			//look for, and fix, boss with bogus shields
			if (obj->type == OBJ_ROBOT && Robot_info[obj->id].boss_flag) {
				fix save_shields = obj->shields;

				copy_defaults_to_robot(obj);		//calculate starting shields

				//if in valid range, use loaded shield value
				if (save_shields > 0 && save_shields <= obj->shields)
					obj->shields = save_shields;
				else
					obj->shields /= 2;  //give player a break
			}

			// If weapon, restore the most recent hitobj to the list
			if (obj->type == OBJ_WEAPON)
			{
				Assert(obj->control_type == CT_WEAPON);

				if (obj->ctype.laser_info.last_hitobj > 0 && obj->ctype.laser_info.last_hitobj < MAX_OBJECTS)
				{
					memset(&hitobj_list[i], 0, sizeof(ubyte)*MAX_OBJECTS);
					hitobj_list[i][obj->ctype.laser_info.last_hitobj] = 1;
				}
			}
		}	
		special_reset_objects();

		//	1 = Didn't die on secret level.
		//	2 = Died on secret level.
		if (secret_restore && (Current_level_num >= 0)) {
			set_pos_from_return_segment();
			if (secret_restore == 2)
				init_player_stats_new_ship();
		}

		//Restore wall info
		PHYSFS_read(fp, &i, sizeof(int), 1);
		Num_walls = i;
		if (version <= 24) {
			for (int n = 0; n < Num_walls; n++) {
				st24_wall old;
				PHYSFS_read(fp, &old, sizeof(old), 1);
				Walls[n].segnum = old.segnum;
				Walls[n].sidenum = old.sidenum;
				Walls[n].hps = old.hps;
				Walls[n].linked_wall = old.linked_wall;
				Walls[n].type = old.type;
				Walls[n].flags = old.flags;
				Walls[n].state = old.state;
				Walls[n].trigger = old.trigger;
				Walls[n].clip_num = old.clip_num;
				Walls[n].keys = old.keys;
				Walls[n].cloak_value = old.cloak_value;
				if (old.controlling_trigger >= 0)
					Walls[n].flags |= WALL_HAS_TRIGGERS;
			}
		} else {
			PHYSFS_read(fp, Walls, sizeof(wall), Num_walls);
		}

		//now that we have the walls, check if any sounds are linked to
		//walls that are now open
		for (i=0;i<Num_walls;i++) {
			if (Walls[i].type == WALL_OPEN)
				digi_kill_sound_linked_to_segment(Walls[i].segnum,Walls[i].sidenum,-1);	//-1 means kill any sound
		}

		//Restore exploding wall info
		if (version >= 10) {
			PHYSFS_read(fp, &i, sizeof(int), 1);
			PHYSFS_read(fp, expl_wall_list, sizeof(*expl_wall_list), i);
		}

		//Restore door info
		PHYSFS_read(fp, &i, sizeof(int), 1);
		Num_open_doors = i;
		PHYSFS_read(fp, ActiveDoors, sizeof(active_door), Num_open_doors);
	
		if (version >= 14) {		//Restore cloaking wall info
			PHYSFS_read(fp, &i, sizeof(int), 1);
			Num_cloaking_walls = i;
			PHYSFS_read(fp, CloakingWalls, sizeof(cloaking_wall), Num_cloaking_walls);
		}
	
		//Restore trigger info
		PHYSFS_read(fp, &Num_triggers, sizeof(int), 1);
		Num_object_triggers = 0;
		if (version >= 26)
			PHYSFS_read(fp, &Num_object_triggers, sizeof(int), 1);
		for (i = 0; i < Num_triggers + Num_object_triggers; i++) {
			trigger *trig = &Triggers[i];
			*trig = TRIGGER_DEFAULTS;
			CFILE_var_r(fp, &trig->type);
			if (version < 26) {
				ubyte flags;
				CFILE_var_r(fp, &flags);
				trig->flags = flags;
			}
			CFILE_var_r(fp, &trig->num_links);
			if (version < 26)
				CFILE_var_r(fp, &(sbyte){0}); // pad
			CFILE_var_r(fp, &trig->value);
			CFILE_var_r(fp, &trig->time);
			if (version >= 26) {
				CFILE_var_r(fp, &trig->flags);
				CFILE_var_r(fp, &trig->time_b);
				CFILE_var_r(fp, &trig->last_operated);
				CFILE_var_r(fp, &trig->last_player);
				CFILE_var_r(fp, &trig->object_id);
			}
			int read_links = version < 26 ? 10 : trig->num_links;
			PHYSFS_read(fp, trig->seg, sizeof(trig->seg[0]), read_links);
			PHYSFS_read(fp, trig->side, sizeof(trig->side[0]), read_links);
		}

		//Restore tmap info (to temp values so we can use compiled-in tmap info to compute static_light
		for (i=0; i<=Highest_segment_index; i++ )	{
			for (j=0; j<6; j++ )	{
				PHYSFS_read(fp, &Segments[i].sides[j].wall_num, sizeof(short), 1);
				PHYSFS_read(fp, &TempTmapNum[i][j], sizeof(short), 1);
				PHYSFS_read(fp, &TempTmapNum2[i][j], sizeof(short), 1);
			}
		}
	
		//Restore the fuelcen info
		PHYSFS_read(fp, &Control_center_destroyed, sizeof(int), 1);
		PHYSFS_read(fp, &Countdown_timer, sizeof(int), 1);
		PHYSFS_read(fp, &Num_robot_centers, sizeof(int), 1);
		PHYSFS_read(fp, RobotCenters, sizeof(matcen_info), Num_robot_centers);
		PHYSFS_read(fp, &ControlCenterTriggers, sizeof(control_center_triggers), 1);
		PHYSFS_read(fp, &Num_fuelcenters, sizeof(int), 1);
		PHYSFS_read(fp, Station, sizeof(FuelCenter), Num_fuelcenters);
	
		// Restore the control cen info
		PHYSFS_read(fp, &Control_center_been_hit, sizeof(int), 1);
		PHYSFS_read(fp, &Control_center_player_been_seen, sizeof(int), 1);
		PHYSFS_read(fp, &Control_center_next_fire_time, sizeof(int), 1);
		PHYSFS_read(fp, &Control_center_present, sizeof(int), 1);
		PHYSFS_read(fp, &(int){0}, sizeof(int), 1); // Dead_controlcen_object_num
		if (Control_center_destroyed)
			Total_countdown_time = Countdown_timer/F0_5; // we do not need to know this, but it should not be 0 either...
	
		// Restore the AI state
		ai_restore_state( fp, version );
	
		// Restore the automap visited info
                i = MAX_SEGMENTS;
                if (version >= 23)
                    i = Highest_segment_index + 1;
		PHYSFS_read(fp, Automap_visited, sizeof(ubyte), i);

		//	Restore hacked up weapon system stuff.
		Auto_fire_fusion_cannon_time = 0;
		Next_laser_fire_time = GameTime;
		Next_missile_fire_time = GameTime;
		Last_laser_fired_time = GameTime;

	}
	state_game_id = 0;

	if ( version >= 7 )	{
		PHYSFS_read(fp, &state_game_id, sizeof(uint), 1);
		PHYSFS_read(fp, &Laser_rapid_fire, sizeof(int), 1);
		PHYSFS_read(fp, &Lunacy, sizeof(int), 1);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
		PHYSFS_read(fp, &Lunacy, sizeof(int), 1);
		if ( Lunacy )
			do_lunacy_on();
	}

	if (version >= 17) {
		PHYSFS_read(fp, MarkerObject, sizeof(MarkerObject), 1);
		for (int n = 0; n < 144; n++)
			PHYSFS_read(fp, &(char){0}, 1, 1); // skip obsolete MarkerOwner
		PHYSFS_read(fp, MarkerMessage, sizeof(MarkerMessage), 1);
	}
	else {
		int num,dummy;

		// skip dummy info

		PHYSFS_read(fp, &num,sizeof(int), 1);           // was NumOfMarkers
		PHYSFS_read(fp, &dummy,sizeof(int), 1);         // was CurMarker

		PHYSFS_seek(fp, PHYSFS_tell(fp) + num * (sizeof(vms_vector) + 40));

		for (num=0;num<NUM_MARKERS;num++)
			MarkerObject[num] = -1;
	}

	if (version>=11) {
		if (secret_restore != 1)
			PHYSFS_read(fp, &Afterburner_charge, sizeof(fix), 1);
		else {
			fix	dummy_fix;
			PHYSFS_read(fp, &dummy_fix, sizeof(fix), 1);
		}
	}
	if (version>=12) {
		//read last was super information
		if (secret_restore != 1) {
			PHYSFS_read(fp, &Primary_last_was_super, sizeof(Primary_last_was_super), 1);
			PHYSFS_read(fp, &Secondary_last_was_super, sizeof(Secondary_last_was_super), 1);
		} else {
			cfskip(fp, sizeof(Primary_last_was_super));
			cfskip(fp, sizeof(Secondary_last_was_super));
		}
	}

	if (version >= 12) {
		PHYSFS_read(fp, &Flash_effect, sizeof(int), 1);
		PHYSFS_read(fp, &Time_flash_last_played, sizeof(int), 1);
		PHYSFS_read(fp, &PaletteRedAdd, sizeof(int), 1);
		PHYSFS_read(fp, &PaletteGreenAdd, sizeof(int), 1);
		PHYSFS_read(fp, &PaletteBlueAdd, sizeof(int), 1);
	} else {
		Flash_effect = 0;
		Time_flash_last_played = 0;
		PaletteRedAdd = 0;
		PaletteGreenAdd = 0;
		PaletteBlueAdd = 0;
	}

	//	Load Light_subtracted
	if (version >= 16) {
                i = MAX_SEGMENTS;
                if (version >= 23)
                    i = Highest_segment_index + 1;
		PHYSFS_read(fp, Light_subtracted, sizeof(Light_subtracted[0]), i);
		apply_all_changed_light();
	} else {
		int	i;
		for (i=0; i<=Highest_segment_index; i++)
			Light_subtracted[i] = 0;
	}

	// static_light should now be computed - now actually set tmap info
	for (i=0; i<=Highest_segment_index; i++ )	{
		for (j=0; j<6; j++ )	{
			Segments[i].sides[j].tmap_num=TempTmapNum[i][j];
			Segments[i].sides[j].tmap_num2=TempTmapNum2[i][j];
		}
	}

	First_secret_visit = 1;
        if (version >= 20)
            PHYSFS_read(fp, &First_secret_visit, sizeof(First_secret_visit), 1);

	if (secret_restore)
		First_secret_visit = 0;

	if (version >= 22)
            PHYSFS_read(fp, &Omega_charge, sizeof(fix), 1);

	if (version >= 23) {
		uint32_t oldmagic = 0;
		PHYSFS_read(fp, &oldmagic, sizeof(oldmagic), 1);
		if (END_MAGIC23 != oldmagic) {
			fprintf(stderr, "wrong old save game magic\n");
			Int3();
		}
	}

	if (version >= 24) {
		PHYSFS_read(fp, &Missile_gun, sizeof(Missile_gun), 1);
		PHYSFS_read(fp, &Player_init[Player_num], sizeof(Player_init[Player_num]), 1);

		if (version >= 27) {
			PHYSFS_read(fp, &Num_equip_centers, sizeof(int), 1);
			PHYSFS_read(fp, EquipCenters, sizeof(matcen_info), Num_equip_centers);
		}

		while (1) {
			uint32_t magic = 0;
			PHYSFS_read(fp, &magic, sizeof(magic), 1);
			if (magic == TLV_MAGIC) {
				tlv_rec tlv = {0};
				PHYSFS_read(fp, &tlv, sizeof(tlv), 1);
				int64_t end_pos = cftell(fp) + tlv.size;
				if (tlv.compat_version > STATE_VERSION)
					goto tlv_problem;

				switch (tlv.type) {
				case TLV_TYPE_SPEEDBOOST:
					PHYSFS_read(fp, &Player_Speedboost, sizeof(Player_Speedboost), 1);
					break;
				default:
					goto tlv_problem;
				}

				if (cftell(fp) != end_pos) {
					fprintf(stderr, "incorrect amount of data read\n");
					cfseek(fp, end_pos, SEEK_SET);
				}

				continue;
			tlv_problem:

				fprintf(stderr, "incompatible savegame data\n");
				if ((tlv.flags & TLV_FLAG_OPT) ||
					(is_d2x_xl_level() && (tlv.flags & TLV_FLAG_XL_ONLY)))
				{
					fprintf(stderr, "it's optional -> skipping\n");
					cfseek(fp, end_pos, SEEK_SET);
					continue;
				} else {
					Int3();
					break;
				}
			} else if (END_MAGIC != magic) {
				fprintf(stderr, "wrong save game magic\n");
				Int3();
			}
			break;
		}
	}

	PHYSFS_close(fp);

	reset_time();

	return 1;
}
