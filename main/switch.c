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
 * New Triggers and Switches.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "gauges.h"
#include "newmenu.h"
#include "game.h"
#include "switch.h"
#include "inferno.h"
#include "segment.h"
#include "error.h"
#include "gameseg.h"
#include "wall.h"
#include "texmap.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "player.h"
#include "endlevel.h"
#include "gameseq.h"
#include "multi.h"
#include "palette.h"
#include "robot.h"
#include "bm.h"
#include "kconfig.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

trigger Triggers[MAX_TRIGGERS];
int Num_triggers;

extern int Do_appearance_effect;

//link Links[MAX_WALL_LINKS];
//int Num_links;

#ifdef EDITOR
fix trigger_time_count=F1_0;

//-----------------------------------------------------------------
// Initializes all the switches.
void trigger_init()
{
	int i;

	Num_triggers = 0;

	for (i=0;i<MAX_TRIGGERS;i++)
		{
		Triggers[i].type = 0;
		Triggers[i].flags = 0;
		Triggers[i].num_links = 0;
		Triggers[i].value = 0;
		Triggers[i].time = -1;
		}
}
#endif

//-----------------------------------------------------------------
// Executes a link, attached to a trigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
void do_link(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			wall_toggle(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]);
  		}
  	}
}

//close a door
void do_close_door(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++)
			wall_close_door(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]);
  	}
}

//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been shot out).
int do_light_on(sbyte trigger_num)
{
	int i,ret=0;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			int segnum,sidenum;
			segnum = Triggers[trigger_num].seg[i];
			sidenum = Triggers[trigger_num].side[i];

			//check if tmap2 casts light before turning the light on.  This
			//is to keep us from turning on blown-out lights
			if (TmapInfo[Segments[segnum].sides[sidenum].tmap_num2 & 0x3fff].lighting) {
				ret |= add_light(segnum, sidenum); 		//any light sets flag
				enable_flicker(segnum, sidenum);
			}
		}
	}

	return ret;
}

//turns lighting off.  returns true if lights were actually turned off. (they
//would not be if they had previously been shot out).
int do_light_off(sbyte trigger_num)
{
	int i,ret=0;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			int segnum,sidenum;
			segnum = Triggers[trigger_num].seg[i];
			sidenum = Triggers[trigger_num].side[i];

			//check if tmap2 casts light before turning the light off.  This
			//is to keep us from turning off blown-out lights
			if (TmapInfo[Segments[segnum].sides[sidenum].tmap_num2 & 0x3fff].lighting) {
				ret |= subtract_light(segnum, sidenum); 	//any light sets flag
				disable_flicker(segnum, sidenum);
			}
  		}
  	}

	return ret;
}

// Unlocks all doors linked to the switch.
void do_unlock_doors(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			Walls[Segments[Triggers[trigger_num].seg[i]].sides[Triggers[trigger_num].side[i]].wall_num].flags &= ~WALL_DOOR_LOCKED;
			Walls[Segments[Triggers[trigger_num].seg[i]].sides[Triggers[trigger_num].side[i]].wall_num].keys = KEY_NONE;
  		}
  	}
}

// Locks all doors linked to the switch.
void do_lock_doors(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			Walls[Segments[Triggers[trigger_num].seg[i]].sides[Triggers[trigger_num].side[i]].wall_num].flags |= WALL_DOOR_LOCKED;
  		}
  	}
}

// Changes walls pointed to by a trigger. returns true if any walls changed
int do_change_walls(sbyte trigger_num)
{
	int i,ret=0;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			segment *segp,*csegp;
			short side,cside;
			int new_wall_type;

			segp = &Segments[Triggers[trigger_num].seg[i]];
			side = Triggers[trigger_num].side[i];

			if (segp->children[side] < 0)
			{
				csegp = NULL;
				cside = -1;
			}
			else
			{
				csegp = &Segments[segp->children[side]];
				cside = find_connect_side(segp, csegp);
				Assert(cside != -1);
			}

			//segp->sides[side].wall_num = -1;
			//csegp->sides[cside].wall_num = -1;

			switch (Triggers[trigger_num].type) {
				case TT_OPEN_WALL:		new_wall_type = WALL_OPEN; break;
				case TT_CLOSE_WALL:		new_wall_type = WALL_CLOSED; break;
				case TT_ILLUSORY_WALL:	new_wall_type = WALL_ILLUSION; break;
			        default:
					Assert(0); /* new_wall_type unset */
					return(0);
					break;
			}

			if (Walls[segp->sides[side].wall_num].type == new_wall_type &&
			    (cside < 0 || csegp->sides[cside].wall_num < 0 ||
			     Walls[csegp->sides[cside].wall_num].type == new_wall_type))
				continue;		//already in correct state, so skip

			ret = 1;

			switch (Triggers[trigger_num].type) {

				case TT_OPEN_WALL:
					if ((TmapInfo[segp->sides[side].tmap_num].flags & TMI_FORCE_FIELD)) {
						vms_vector pos;
						compute_center_point_on_side(&pos, segp, side );
						digi_link_sound_to_pos( SOUND_FORCEFIELD_OFF, segp-Segments, side, &pos, 0, F1_0 );
						Walls[segp->sides[side].wall_num].type = new_wall_type;
						digi_kill_sound_linked_to_segment(segp-Segments,side,SOUND_FORCEFIELD_HUM);
						if (cside > -1 && csegp->sides[cside].wall_num > -1)
						{
							Walls[csegp->sides[cside].wall_num].type = new_wall_type;
							digi_kill_sound_linked_to_segment(csegp-Segments, cside, SOUND_FORCEFIELD_HUM);
						}
					}
					else
						start_wall_cloak(segp,side);

					ret = 1;

					break;

				case TT_CLOSE_WALL:
					if ((TmapInfo[segp->sides[side].tmap_num].flags & TMI_FORCE_FIELD)) {
						vms_vector pos;
						compute_center_point_on_side(&pos, segp, side );
						digi_link_sound_to_pos(SOUND_FORCEFIELD_HUM,segp-Segments,side,&pos,1, F1_0/2);
						Walls[segp->sides[side].wall_num].type = new_wall_type;
						if (cside > -1 && csegp->sides[cside].wall_num > -1)
							Walls[csegp->sides[cside].wall_num].type = new_wall_type;
					}
					else
						start_wall_decloak(segp,side);
					break;

				case TT_ILLUSORY_WALL:
					Walls[segp->sides[side].wall_num].type = new_wall_type;
					if (cside > -1 && csegp->sides[cside].wall_num > -1)
						Walls[csegp->sides[cside].wall_num].type = new_wall_type;
					break;
			}


			kill_stuck_objects(segp->sides[side].wall_num);
			if (cside > -1 && csegp->sides[cside].wall_num > -1)
				kill_stuck_objects(csegp->sides[cside].wall_num);

  		}
  	}

	return ret;
}

void print_trigger_message (int pnum,int trig,int shot,char *message)
 {
	char *pl;		//points to 's' or nothing for plural word

   if (pnum!=Player_num)
		return;

	pl = (Triggers[trig].num_links>1)?"s":"";

    if (!(Triggers[trig].flags & TF_NO_MESSAGE) && shot)
     HUD_init_message(HM_DEFAULT, message,pl);
 }


void do_matcen(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			trigger_matcen(Triggers[trigger_num].seg[i] );
  		}
  	}
}


void do_il_on(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			wall_illusion_on(&Segments[Triggers[trigger_num].seg[i]], Triggers[trigger_num].side[i]);
  		}
  	}
}

void do_il_off(sbyte trigger_num)
{
	int i;

	if (trigger_num != -1) {
		for (i=0;i<Triggers[trigger_num].num_links;i++) {
			vms_vector	cp;
			segment		*seg = &Segments[Triggers[trigger_num].seg[i]];
			int			side = Triggers[trigger_num].side[i];

			wall_illusion_off(seg, side);

			compute_center_point_on_side(&cp, seg, side );
			digi_link_sound_to_pos( SOUND_WALL_REMOVED, seg-Segments, side, &cp, 0, F1_0 );

  		}
  	}
}

extern void EnterSecretLevel(void);
extern void ExitSecretLevel(void);
extern int p_secret_level_destroyed(void);

int wall_is_forcefield(trigger *trig)
{
	int i;

	for (i=0;i<trig->num_links;i++)
		if ((TmapInfo[Segments[trig->seg[i]].sides[trig->side[i]].tmap_num].flags & TMI_FORCE_FIELD))
			break;

	return (i<trig->num_links);
}

static void compute_seg_side_pos(int seg, int side, struct obj_position *out)
{
	vms_vector pos;
	compute_segment_center(&pos, &Segments[seg]);

	vms_matrix orient;
	vms_vector towards_point;
	vms_vector dir;
	compute_center_point_on_side(&towards_point, &Segments[seg], side);
	vm_vec_sub(&dir, &towards_point, &pos);
	vm_vector_2_matrix(&orient, &dir, NULL, NULL);

	out->pos = pos;
	out->orient = orient;
 	out->segnum = seg;
}

static void do_teleport_player(trigger *trig)
{
	int link = d_rand() % trig->num_links;

	struct obj_position pos;
	compute_seg_side_pos(trig->seg[link], trig->side[link], &pos);

	ConsoleObject->pos = pos.pos;
	ConsoleObject->orient = pos.orient;
 	obj_relink(ConsoleObject-Objects, pos.segnum);

	reset_player_object();
	reset_cruise();
	Do_appearance_effect = 1;
}

static void do_set_spawn(trigger *trig, int player_index)
{
	Assert(player_index >= 0 && player_index < trig->num_links);

	compute_seg_side_pos(trig->seg[player_index], trig->side[player_index],
						 &Player_init[player_index]);
}

static void do_master(trigger *trig, int player_index, int shot)
{
	for (int i = 0; i < trig->num_links; i++) {
		int wall_num = Segments[trig->seg[i]].sides[trig->side[i]].wall_num;
		int sub_trig = Walls[wall_num].trigger;
		printf("trigger %zd: sub trigger %d (%d) \n", trig-Triggers, sub_trig, Triggers[sub_trig].type);
		check_trigger_sub(sub_trig, player_index, shot);
	}
}

int check_trigger_sub(int trigger_num, int pnum,int shot)
{
	trigger *trig = &Triggers[trigger_num];

	if (trig->flags & TF_DISABLED)
		return 1;		//1 means don't send trigger hit to other players

	if (trig->flags & TF_ONE_SHOT)		//if this is a one-shot...
		trig->flags |= TF_DISABLED;		//..then don't let it happen again

	switch (trig->type) {

		case TT_EXIT:

			if (pnum!=Player_num)
			  break;

                        if (!EMULATING_D1)
			  digi_stop_digi_sounds();  //Sound shouldn't cut out when exiting a D1 lvl

			if (Current_level_num > 0) {
				start_endlevel_sequence();
			} else if (Current_level_num < 0) {
				if ((Players[Player_num].shields < 0) || Player_is_dead)
					break;
				// NMN 04/09/07 Do endlevel movie if we are
				//             playing a D1 secret level
				if (EMULATING_D1)
				{
					start_endlevel_sequence();
				} else {
					ExitSecretLevel();
				}
				return 1;
			} else {
				#ifdef EDITOR
					nm_messagebox( "Yo!", 1, "You have hit the exit trigger!", "" );
				#else
					Int3();		//level num == 0, but no editor!
				#endif
			}
			return 1;
			break;

		case TT_SECRET_EXIT: {
#ifndef SHAREWARE
			int	truth;
#endif

			if (pnum!=Player_num)
				break;

			if ((Players[Player_num].shields < 0) || Player_is_dead)
				break;

			if (Game_mode & GM_MULTI) {
				HUD_init_message(HM_DEFAULT, "Secret Level Teleporter disabled in multiplayer!");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			#ifndef SHAREWARE
			truth = p_secret_level_destroyed();

			if (Newdemo_state == ND_STATE_RECORDING)			// record whether we're really going to the secret level
				newdemo_record_secret_exit_blown(truth);

			if ((Newdemo_state != ND_STATE_PLAYBACK) && truth) {
				HUD_init_message(HM_DEFAULT, "Secret Level destroyed.  Exit disabled.");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			}
			#endif

			#ifdef SHAREWARE
				HUD_init_message(HM_DEFAULT, "Secret Level Teleporter disabled in Descent 2 Demo");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			#endif

			if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
				Newdemo_state = ND_STATE_PAUSED;

			digi_stop_digi_sounds();

			digi_play_sample( SOUND_SECRET_EXIT, F1_0 );

			EnterSecretLevel();
			Control_center_destroyed = 0;
			return 1;
			break;

		}

		case TT_OPEN_DOOR:
			do_link(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s opened!");

			break;

		case TT_CLOSE_DOOR:
			do_close_door(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s closed!");
			break;

		case TT_UNLOCK_DOOR:
			do_unlock_doors(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s unlocked!");

			break;

		case TT_LOCK_DOOR:
			do_lock_doors(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Door%s locked!");

			break;

		case TT_OPEN_WALL:
			if (do_change_walls(trigger_num))
			{
				if (wall_is_forcefield(trig))
					print_trigger_message (pnum,trigger_num,shot,"Force field%s deactivated!");
				else
					print_trigger_message (pnum,trigger_num,shot,"Wall%s opened!");
			}
			break;

		case TT_CLOSE_WALL:
			if (do_change_walls(trigger_num))
			{
				if (wall_is_forcefield(trig))
					print_trigger_message (pnum,trigger_num,shot,"Force field%s activated!");
				else
					print_trigger_message (pnum,trigger_num,shot,"Wall%s closed!");
			}
			break;

		case TT_ILLUSORY_WALL:
			//don't know what to say, so say nothing
			do_change_walls(trigger_num);
			break;

		case TT_MATCEN:
			if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS))
				do_matcen(trigger_num);
			break;

		case TT_ILLUSION_ON:
			do_il_on(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Illusion%s on!");
			break;

		case TT_ILLUSION_OFF:
			do_il_off(trigger_num);
			print_trigger_message (pnum,trigger_num,shot,"Illusion%s off!");
			break;

		case TT_LIGHT_OFF:
			if (do_light_off(trigger_num))
				print_trigger_message (pnum,trigger_num,shot,"Lights off!");
			break;

		case TT_LIGHT_ON:
			if (do_light_on(trigger_num))
				print_trigger_message (pnum,trigger_num,shot,"Lights on!");

			break;

		// D2X-XL
		case TT_TELEPORT:
			if (!Player_is_dead) {
				printf("D2X-XL: teleport\n");
				do_teleport_player(trig);
				print_trigger_message (pnum,trigger_num,shot,"Teleporting!");
			}
			break;
		case TT_SET_SPAWN:
			printf("D2X-XL: changing spawn\n");
			do_set_spawn(trig, Player_num); // disregarding multiplayer things
			break;
		case TT_MASTER:
			printf("D2X-XL: master\n");
			do_master(trig, Player_num, shot);
			break;
		case TT_SPEEDBOOST:
		case TT_CAMERA:
		case TT_SHIELD_DAMAGE:
		case TT_ENERGY_DRAIN:
		case TT_CHANGE_TEXTURE:
		case TT_SMOKE_LIFE:
		case TT_SMOKE_SPEED:
		case TT_SMOKE_DENS:
		case TT_SMOKE_SIZE:
		case TT_SMOKE_DRIFT:
		case TT_COUNTDOWN:
		case TT_SPAWN_BOT:
		case TT_SMOKE_BRIGHTNESS:
		case TT_MESSAGE:
		case TT_SOUND:
		case TT_ENABLE_TRIGGER:
		case TT_DISABLE_TRIGGER:
		case TT_DISARM_ROBOT:
		case TT_REPROGRAM_ROBOT:
		case TT_SHAKE_MINE:
			HUD_init_message(HM_DEFAULT, "D2X-XL: unimplemented trigger %d", trig->type);
			break;

		default:
			Int3();
			break;
	}

	return 0;
}

//-----------------------------------------------------------------
// Checks for a trigger whenever an object hits a trigger side.
void check_trigger(segment *seg, short side, short objnum,int shot)
{
	int wall_num, trigger_num;	//, ctrigger_num;
	//segment *csegp;
 	//short cside;

	if ((objnum == Players[Player_num].objnum) || ((Objects[objnum].type == OBJ_ROBOT) && (Robot_info[Objects[objnum].id].companion))) {

		if ( Newdemo_state == ND_STATE_RECORDING )
			newdemo_record_trigger( seg-Segments, side, objnum,shot);

		wall_num = seg->sides[side].wall_num;
		if ( wall_num == -1 ) return;

		trigger_num = Walls[wall_num].trigger;

		if (trigger_num == -1)
			return;

		if (check_trigger_sub(trigger_num, Player_num,shot))
			return;

#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_send_trigger(trigger_num);
#endif
	}
}

void triggers_frame_process()
{
	int i;

	for (i=0;i<Num_triggers;i++)
		if (Triggers[i].time >= 0)
			Triggers[i].time -= FrameTime;
}

/*
 * reads a v29_trigger structure from a CFILE
 */
extern void v29_trigger_read(v29_trigger *t, CFILE *fp)
{
	int i;

	t->type = cfile_read_byte(fp);
	t->flags = cfile_read_short(fp);
	t->value = cfile_read_fix(fp);
	t->time = cfile_read_fix(fp);
	t->link_num = cfile_read_byte(fp);
	t->num_links = cfile_read_short(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = cfile_read_short(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = cfile_read_short(fp);
}

/*
 * reads a v30_trigger structure from a CFILE
 */
extern void v30_trigger_read(v30_trigger *t, CFILE *fp)
{
	int i;

	t->flags = cfile_read_short(fp);
	t->num_links = cfile_read_byte(fp);
	t->pad = cfile_read_byte(fp);
	t->value = cfile_read_fix(fp);
	t->time = cfile_read_fix(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = cfile_read_short(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = cfile_read_short(fp);
}

/*
 * reads a trigger structure from a CFILE
 */
extern void trigger_read(trigger *t, CFILE *fp, bool obj_trigger)
{
	int i;

	t->type = cfile_read_byte(fp);
	if (obj_trigger) {
		short v = cfile_read_short(fp);
		if (v < 0 || v > 0xFF)
			printf("D2X-XL: warning: cutting off flags %d\n", v);
		t->flags = v;
	} else {
		t->flags = cfile_read_byte(fp);
	}
	t->num_links = cfile_read_byte(fp);
	t->pad = cfile_read_byte(fp);
	t->value = cfile_read_fix(fp);
	t->time = cfile_read_fix(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = cfile_read_short(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = cfile_read_short(fp);
}

void trigger_write(trigger *t, short version, PHYSFS_file *fp)
{
	int i;

	if (version != 30)
		PHYSFSX_writeU8(fp, t->type);

	if (version <= 29)
		switch (t->type)
		{
			case TT_OPEN_DOOR:
				PHYSFS_writeSLE16(fp, TRIGGER_CONTROL_DOORS);
				break;

			case TT_EXIT:
				PHYSFS_writeSLE16(fp, TRIGGER_EXIT);
				break;

			case TT_MATCEN:
				PHYSFS_writeSLE16(fp, TRIGGER_MATCEN);
				break;

			case TT_ILLUSION_OFF:
				PHYSFS_writeSLE16(fp, TRIGGER_ILLUSION_OFF);
				break;

			case TT_SECRET_EXIT:
				PHYSFS_writeSLE16(fp, TRIGGER_SECRET_EXIT);
				break;

			case TT_ILLUSION_ON:
				PHYSFS_writeSLE16(fp, TRIGGER_ILLUSION_ON);
				break;

			case TT_UNLOCK_DOOR:
				PHYSFS_writeSLE16(fp, TRIGGER_UNLOCK_DOORS);
				break;

			case TT_OPEN_WALL:
				PHYSFS_writeSLE16(fp, TRIGGER_OPEN_WALL);
				break;

			case TT_CLOSE_WALL:
				PHYSFS_writeSLE16(fp, TRIGGER_CLOSE_WALL);
				break;

			case TT_ILLUSORY_WALL:
				PHYSFS_writeSLE16(fp, TRIGGER_ILLUSORY_WALL);
				break;

			default:
				Int3();
				PHYSFS_writeSLE16(fp, 0);
				break;
		}
	else
		PHYSFSX_writeU8(fp, t->flags);

	if (version >= 30)
	{
		PHYSFSX_writeU8(fp, t->num_links);
		PHYSFSX_writeU8(fp, t->pad);
	}

	PHYSFSX_writeFix(fp, t->value);
	PHYSFSX_writeFix(fp, t->time);

	if (version <= 29)
	{
		PHYSFSX_writeU8(fp, -1);	//t->link_num
		PHYSFS_writeSLE16(fp, t->num_links);
	}

	for (i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->seg[i]);
	for (i = 0; i < MAX_WALLS_PER_LINK; i++)
		PHYSFS_writeSLE16(fp, t->side[i]);
}
