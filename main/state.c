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
#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#include <errno.h>
# ifdef _MSC_VER
#  include <windows.h>
# endif
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "ogl_init.h"

#include "pstypes.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "error.h"
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

#define STATE_VERSION 25
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

// Things to change on next incompatible savegame change:
// - add a way to save/restore objects in a backward/forward compatible way
// - merge Segment2s back into Segment
// - extend D2X-XL fields (especially Segment func/props ones)

#define NUM_SAVES 10
#define THUMBNAIL_W 100
#define THUMBNAIL_H 50
#define DESC_LENGTH 20

#define END_MAGIC 0xD00FB0A2
// Hack to make old versions read newer savegames.
#define END_MAGIC23 0xD00FB0A1

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

extern void apply_all_changed_light(void);

extern int Do_appearance_effect;
extern fix Fusion_next_sound_time;

extern int Laser_rapid_fire;
extern int Physics_cheat_flag;
extern int Lunacy;
extern void do_lunacy_on(void);
extern void do_lunacy_off(void);

int state_save_all_sub(char *filename, char *desc, int between_levels);
int state_restore_all_sub(char *filename, int secret_restore);

extern int First_secret_visit;

int sc_last_item= 0;

char dgss_id[4] = "DGSS";

uint state_game_id;

//-------------------------------------------------------------------
int state_callback(newmenu *menu, d_event *event, grs_bitmap *sc_bmp[])
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	
	if ( (citem > 0) && (event->type == EVENT_NEWMENU_DRAW) )
	{
		if ( sc_bmp[citem-1] )	{
			grs_canvas *save_canv = grd_curcanv;
			grs_canvas *temp_canv = gr_create_canvas(THUMBNAIL_W*2,(THUMBNAIL_H*24/10));
			grs_point vertbuf[3] = {{0,0}, {0,0}, {i2f(THUMBNAIL_W*2),i2f(THUMBNAIL_H*24/10)} };
			gr_set_current_canvas(temp_canv);
			scale_bitmap(sc_bmp[citem-1], vertbuf, 0 );
			gr_set_current_canvas( save_canv );
			ogl_ubitmapm_cs((grd_curcanv->cv_bitmap.bm_w/2)-FSPACX(THUMBNAIL_W/2),items[0].y-FSPACY(3),THUMBNAIL_W*FSPACX(1),THUMBNAIL_H*FSPACY(1),&temp_canv->cv_bitmap,255,F1_0);
			gr_free_canvas(temp_canv);
		}
		
		return 1;
	}
	
	return 0;
}

#if 0
void rpad_string( char * string, int max_chars )
{
	int i, end_found;

	end_found = 0;
	for( i=0; i<max_chars; i++ )	{
		if ( *string == 0 )
			end_found = 1;
		if ( end_found )
			*string = ' ';
		string++;
	}
	*string = 0;		// NULL terminate
}
#endif

int state_default_item = 0;

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
	newmenu_item m[NUM_SAVES+1];
	char filename[NUM_SAVES][FILENAME_LEN + 9];
	char desc[NUM_SAVES][DESC_LENGTH + 16];
	grs_bitmap *sc_bmp[NUM_SAVES];
	char id[5];
	int valid;

	nsaves=0;
	m[0].type = NM_TYPE_TEXT; m[0].text = "\n\n\n\n";
	for (i=0;i<NUM_SAVES; i++ )	{
		sc_bmp[i] = NULL;
		sprintf( filename[i], GameArg.SysUsePlayersDir? "Players/%s.sg%x" : "%s.sg%x", Players[Player_num].callsign, i );
		valid = 0;
		fp = PHYSFSX_openReadBuffered(filename[i]);
		if ( fp ) {
			//Read id
			PHYSFS_read(fp, id, sizeof(char) * 4, 1);
			if ( !memcmp( id, dgss_id, 4 )) {
				//Read version
				PHYSFS_read(fp, &version, sizeof(int), 1);
				if (version >= STATE_COMPATIBLE_VERSION) {
					// Read description
					cfile_read_fixed_str(fp, DESC_LENGTH, desc[i]);
					//rpad_string( desc[i], DESC_LENGTH-1 );
					if (dsc == NULL) m[i+1].type = NM_TYPE_MENU;
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
		if (!valid) {
			strcpy( desc[i], TXT_EMPTY );
			//rpad_string( desc[i], DESC_LENGTH-1 );
			if (dsc == NULL) m[i+1].type = NM_TYPE_TEXT;
		}
		if (dsc != NULL) {
			m[i+1].type = NM_TYPE_INPUT_MENU;
		}
		m[i+1].text_len = DESC_LENGTH-1;
		m[i+1].text = desc[i];
	}

	if ( dsc == NULL && nsaves < 1 )	{
		nm_messagebox( NULL, 1, "Ok", "No saved games were found!" );
		return 0;
	}

	sc_last_item = -1;

	if (blind_save && state_default_item < 0)
	{
		blind_save = 0;		// haven't picked a slot yet
		state_default_item = 0;
	}

	if (blind_save)
		choice = state_default_item + 1;
	else
		choice = newmenu_do2( NULL, caption, NUM_SAVES+1, m, (int (*)(newmenu *, d_event *, void *))state_callback, sc_bmp, state_default_item + 1, NULL );

	for (i=0; i<NUM_SAVES; i++ )	{
		if ( sc_bmp[i] )
			gr_free_bitmap( sc_bmp[i] );
	}

	if (choice > 0) {
		strcpy( fname, filename[choice-1] );
		if ( dsc != NULL ) strcpy( dsc, desc[choice-1] );
		state_default_item = choice - 1;
		return choice;
	}
	return 0;
}

int state_get_save_file(char * fname, char * dsc, int blind_save)
{
	return state_get_savegame_filename(fname, dsc, "Save Game", blind_save);
}

int state_get_restore_file(char * fname )
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
	int	rval, filenum = -1;
	char	filename[128], desc[DESC_LENGTH+1];

	Assert(between_levels == 0);	//between levels save ripped out

#ifdef NETWORK
	if ( Game_mode & GM_MULTI )	{
		return 0;
	}
#endif

	if ((Current_level_num < 0) && (secret_save == 0)) {
		HUD_init_message( "Can't save in secret level!" );
		return 0;
	}

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
		if (!(filenum = state_get_save_file(filename, desc, blind_save)))
		{
			start_time();
			return 0;
		}
	}
		
	//	MK, 1/1/96
	//	Do special secret level stuff.
	//	If secret.sgc exists, then copy it to Nsecret.sgc (where N = filenum).
	//	If it doesn't exist, then delete Nsecret.sgc
	if (!secret_save) {
		int	rval;
		char	temp_fname[32], fc;

		if (filenum != -1) {

			if (filenum >= 10)
				fc = (filenum-10) + 'a';
			else
				fc = '0' + filenum;

			sprintf(temp_fname, GameArg.SysUsePlayersDir? "Players/%csecret.sgc" : "%csecret.sgc", fc);

			if (PHYSFS_exists(temp_fname))
			{
				if (!PHYSFS_delete(temp_fname) && cfexist(temp_fname))
					Error("Cannot delete file <%s>", temp_fname);
			}

			if (PHYSFS_exists(SECRETC_FILENAME))
			{
				rval = copy_file(SECRETC_FILENAME, temp_fname);
				Assert(rval == 0);	//	Oops, error copying secret.sgc to temp_fname!
			}
		}
	}

	rval = state_save_all_sub(filename, desc, between_levels);

	if (rval && !secret_save)
		HUD_init_message("Game saved");

	return rval;
}

extern	fix	Flash_effect, Time_flash_last_played;


int state_save_all_sub(char *filename, char *desc, int between_levels)
{
	int i,j;
	PHYSFS_file *fp;
	grs_canvas * cnv;
	ubyte *pal;
	GLint gl_draw_buffer;

	Assert(between_levels == 0);	//between levels save ripped out

	#ifndef NDEBUG
	if (GameArg.SysUsePlayersDir && strncmp(filename, "Players/", 8))
		Int3();
	#endif

	fp = PHYSFSX_openWriteBuffered(filename);
	if ( !fp ) {
		nm_messagebox(NULL, 1, TXT_OK, "Error writing savegame.\nPossibly out of disk\nspace.");
		start_time();
		return 0;
	}

//Save id
	PHYSFS_write(fp, dgss_id, sizeof(char) * 4, 1);

//Save version
	i = STATE_VERSION;
	PHYSFS_write(fp, &i, sizeof(int), 1);

//Save description
	cfile_write_fixed_str(fp, DESC_LENGTH, desc);
	
// Save the current screen shot...

	cnv = gr_create_canvas( THUMBNAIL_W, THUMBNAIL_H );
	if ( cnv )
	{
		ubyte *buf;
		int k;
		grs_canvas * cnv_save;
		cnv_save = grd_curcanv;

		gr_set_current_canvas( cnv );

		render_frame(0, 0);

		buf = d_malloc(THUMBNAIL_W * THUMBNAIL_H * 3);
		glGetIntegerv(GL_DRAW_BUFFER, &gl_draw_buffer);
		glReadBuffer(gl_draw_buffer);
		glReadPixels(0, SHEIGHT - THUMBNAIL_H, THUMBNAIL_W, THUMBNAIL_H, GL_RGB, GL_UNSIGNED_BYTE, buf);
		k = THUMBNAIL_H;
		for (i = 0; i < THUMBNAIL_W * THUMBNAIL_H; i++) {
			if (!(j = i % THUMBNAIL_W))
				k--;
			cnv->cv_bitmap.bm_data[THUMBNAIL_W * k + j] =
				gr_find_closest_color(buf[3*i]/4, buf[3*i+1]/4, buf[3*i+2]/4);
		}
		d_free(buf);

		pal = gr_palette;

		PHYSFS_write(fp, cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);

		gr_set_current_canvas(cnv_save);
		gr_free_canvas( cnv );
		PHYSFS_write(fp, pal, 3, 256);
	}
	else
	{
	 	ubyte color = 0;
	 	for ( i=0; i<THUMBNAIL_W*THUMBNAIL_H; i++ )
			PHYSFS_write(fp, &color, sizeof(ubyte), 1);		
	} 

// Save the Between levels flag...
	PHYSFS_write(fp, &between_levels, sizeof(int), 1);

// Save the mission info...
	cfile_write_fixed_str(fp, 9, Current_mission_filename);

//Save level info
	PHYSFS_write(fp, &Current_level_num, sizeof(int), 1);
	PHYSFS_write(fp, &Next_level_num, sizeof(int), 1);

//Save GameTime
	PHYSFS_write(fp, &GameTime, sizeof(fix), 1);

//Save player info
	PHYSFS_write(fp, &Players[Player_num], sizeof(player), 1);

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
		PHYSFS_write(fp, Triggers, sizeof(trigger), Num_triggers);
	
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
		PHYSFS_write(fp, &Dead_controlcen_object_num, sizeof(int), 1);
	
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
	PHYSFS_write(fp, MarkerOwner, sizeof(MarkerOwner), 1);
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

	magic = END_MAGIC;
	if (PHYSFS_write(fp, &magic, sizeof(magic), 1) < 1)
	{
		nm_messagebox(NULL, 1, TXT_OK, "Error writing savegame.\nPossibly out of disk\nspace.");
		PHYSFS_close(fp);
		PHYSFS_delete(filename);
	} else  {
		PHYSFS_close(fp);

		#ifdef MACINTOSH		// set the type and creator of the saved game file
		{
			FInfo finfo;
			OSErr err;
			Str255 pfilename;
	
			strcpy(pfilename, filename);
			c2pstr(pfilename);
			err = HGetFInfo(0, 0, pfilename, &finfo);
			finfo.fdType = 'SVGM';
			finfo.fdCreator = 'DCT2';
			err = HSetFInfo(0, 0, pfilename, &finfo);
		}
		#endif
	}
	
	start_time();

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

#ifdef NETWORK
	if ( Game_mode & GM_MULTI )	{
		return 0;
	}
#endif

	if (in_game && (Current_level_num < 0) && (secret_restore == 0)) {
		HUD_init_message( "Can't restore in secret level!" );
		return 0;
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_stop_recording();

	if ( Newdemo_state != ND_STATE_NORMAL )
		return 0;

	stop_time();

	if (filename_override) {
		strcpy(filename, filename_override);
		filenum = NUM_SAVES+1; // place outside of save slots
	} else if (!(filenum = state_get_restore_file(filename)))	{
		start_time();
		return 0;
	}
	
	//	MK, 1/1/96
	//	Do special secret level stuff.
	//	If Nsecret.sgc (where N = filenum) exists, then copy it to secret.sgc.
	//	If it doesn't exist, then delete secret.sgc
	if (!secret_restore) {
		int	rval;
		char	temp_fname[32], fc;

		if (filenum != -1) {
			if (filenum >= 10)
				fc = (filenum-10) + 'a';
			else
				fc = '0' + filenum;
			
			sprintf(temp_fname, GameArg.SysUsePlayersDir? "Players/%csecret.sgc" : "%csecret.sgc", fc);

			if (PHYSFS_exists(temp_fname))
			{
				rval = copy_file(temp_fname, SECRETC_FILENAME);
				Assert(rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
			} else
				PHYSFS_delete(SECRETC_FILENAME);
		}
	}

	if ( !secret_restore && in_game ) {
		int choice;
		choice =  nm_messagebox( NULL, 2, "Yes", "No", "Restore Game?" );
		if ( choice != 0 )	{
			start_time();
			return 0;
		}
	}

	start_time();

	return state_restore_all_sub(filename, secret_restore);
}

extern void init_player_stats_new_ship(void);

void ShowLevelIntro(int level_num);

extern void do_cloak_invul_secret_stuff(fix old_gametime);
extern void copy_defaults_to_robot(object *objp);

int state_restore_all_sub(char *filename, int secret_restore)
{
	int ObjectStartLocation;
	int version,i, j, segnum;
	object * obj;
	PHYSFS_file *fp;
	int current_level, next_level;
	int between_levels;
	char desc[DESC_LENGTH+1];
	char id[5];
	char org_callsign[CALLSIGN_LEN+16];
	fix	old_gametime = GameTime;
	short TempTmapNum[MAX_SEGMENTS][MAX_SIDES_PER_SEGMENT];
	short TempTmapNum2[MAX_SEGMENTS][MAX_SIDES_PER_SEGMENT];

	#ifndef NDEBUG
	if (GameArg.SysUsePlayersDir && strncmp(filename, "Players/", 8))
		Int3();
	#endif

	fp = PHYSFSX_openReadBuffered(filename);
	if ( !fp ) return 0;

//Read id
	//FIXME: check for swapped file, react accordingly...
	PHYSFS_read(fp, id, sizeof(char) * 4, 1);
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

// Read description
	cfile_read_fixed_str(fp, DESC_LENGTH, desc);

// Skip the current screen shot...
	PHYSFS_seek(fp, PHYSFS_tell(fp) + THUMBNAIL_W * THUMBNAIL_H);

// And now...skip the goddamn palette stuff that somebody forgot to add
	PHYSFS_seek(fp, PHYSFS_tell(fp) + 768);

// Read the Between levels flag...
	PHYSFS_read(fp, &between_levels, sizeof(int), 1);

	Assert(between_levels == 0);	//between levels save ripped out

// Read the mission info...
	char mission[10];
	cfile_read_fixed_str(fp, 9, mission);

	if (!load_mission_by_name( mission ))	{
		nm_messagebox( NULL, 1, "Ok", "Error!\nUnable to load mission\n'%s'\n", mission );
		PHYSFS_close(fp);
		return 0;
	}

//Read level info
	PHYSFS_read(fp, &current_level, sizeof(int), 1);
	PHYSFS_read(fp, &next_level, sizeof(int), 1);

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

		if (secret_restore) {
			player	dummy_player;

			PHYSFS_read(fp, &dummy_player, sizeof(player), 1);
			if (secret_restore == 1) {		//	This means he didn't die, so he keeps what he got in the secret level.
				Players[Player_num].level = dummy_player.level;
				Players[Player_num].last_score = dummy_player.last_score;
				Players[Player_num].time_level = dummy_player.time_level;

				Players[Player_num].num_robots_level = dummy_player.num_robots_level;
				Players[Player_num].num_robots_total = dummy_player.num_robots_total;
				Players[Player_num].hostages_rescued_total = dummy_player.hostages_rescued_total;
				Players[Player_num].hostages_total = dummy_player.hostages_total;
				Players[Player_num].hostages_on_board = dummy_player.hostages_on_board;
				Players[Player_num].hostages_level = dummy_player.hostages_level;
				Players[Player_num].homing_object_dist = dummy_player.homing_object_dist;
				Players[Player_num].hours_level = dummy_player.hours_level;
				Players[Player_num].hours_total = dummy_player.hours_total;
				do_cloak_invul_secret_stuff(old_gametime);
			} else {
				Players[Player_num] = dummy_player;
			}
		} else {
			PHYSFS_read(fp, &Players[Player_num], sizeof(player), 1);
		}
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
		if (version <= 24) {
			PHYSFS_read(fp, &i, sizeof(int), 1);
			Num_walls = i;
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
		PHYSFS_read(fp, Triggers, sizeof(trigger), Num_triggers);
	
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
		PHYSFS_read(fp, &Dead_controlcen_object_num, sizeof(int), 1);
	
		// Restore the AI state
		ai_restore_state( fp, version );
	
		// Restore the automap visited info
                i = MAX_SEGMENTS;
                if (version >= 23)
                    i = Highest_segment_index + 1;
		PHYSFS_read(fp, Automap_visited, sizeof(ubyte), i);

		//	Restore hacked up weapon system stuff.
		Fusion_next_sound_time = GameTime;
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
		PHYSFS_read(fp, MarkerOwner, sizeof(MarkerOwner), 1);
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
		PHYSFS_read(fp, &Primary_last_was_super, sizeof(Primary_last_was_super), 1);
		PHYSFS_read(fp, &Secondary_last_was_super, sizeof(Secondary_last_was_super), 1);
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

		uint32_t magic = 0;
		PHYSFS_read(fp, &magic, sizeof(magic), 1);
		if (END_MAGIC != magic) {
			fprintf(stderr, "wrong save game magic\n");
			Int3();
		}
	}

	PHYSFS_close(fp);

// Load in bitmaps, etc..
//!!	piggy_load_level_data();	//already done by StartNewLevelSub()

	return 1;
}
