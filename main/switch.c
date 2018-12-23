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
#include "dxxerror.h"
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

trigger Triggers[MAX_ALL_TRIGGERS];
int Num_triggers;
int Num_object_triggers;

extern int Do_appearance_effect;

static int do_trigger(int trigger_num, int pnum, int shot);

//turns lighting on or off.  returns true if lights were actually changed. (they
//would not be if they had previously been shot out etc.).
static int do_change_light(trigger *trigger, bool on)
{
	int i,ret=0;

		for (i=0;i<trigger->num_links;i++) {
			int segnum,sidenum;
			segnum = trigger->seg[i];
			sidenum = trigger->side[i];

			//check if tmap2 casts light before turning the light on.  This
			//is to keep us from turning on blown-out lights
			if (TmapInfo[Segments[segnum].sides[sidenum].tmap_num2 & 0x3fff].lighting) {
				if (on) {
					ret |= add_light(segnum, sidenum);
					enable_flicker(segnum, sidenum);
				} else {
					ret |= subtract_light(segnum, sidenum);
					disable_flicker(segnum, sidenum);
				}
			}
		}

	return ret;
}

// Changes walls pointed to by a trigger. returns true if any walls changed
static int do_change_walls(trigger *trigger)
{
	int i,ret=0;

		for (i=0;i<trigger->num_links;i++) {
			segment *segp,*csegp;
			short side,cside;
			int new_wall_type;

			segp = &Segments[trigger->seg[i]];
			side = trigger->side[i];

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

			switch (trigger->type) {
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

			switch (trigger->type) {

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

	return ret;
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
	return do_trigger(trigger_num, pnum, shot);
}

static int do_trigger(int trigger_num, int pnum, int shot)
{
	trigger *trig = &Triggers[trigger_num];

	if (pnum < 0 || pnum > MAX_PLAYERS)
		return 1;
	if ((Game_mode & GM_MULTI) && (Players[pnum].connected != CONNECT_PLAYING)) // as a host we may want to handle triggers for our clients. to do that properly we must check wether we (host) or client is actually playing.
		return 1;

	if (trig->flags & TF_DISABLED)
		return 1;		//1 means don't send trigger hit to other players

	if (trig->flags & TF_ONE_SHOT)		//if this is a one-shot...
		trig->flags |= TF_DISABLED;		//..then don't let it happen again

	bool show_msg = pnum == Player_num && !(trig->flags & TF_NO_MESSAGE) && shot;
	const char *pl = trig->num_links > 1 ? "s" : "";

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
				Int3();		//level num == 0, but no editor!
			}
			return 1;
			break;

		case TT_SECRET_EXIT: {
			int	truth;

			if (pnum!=Player_num)
				break;

			if ((Players[Player_num].shields < 0) || Player_is_dead)
				break;

			if (is_SHAREWARE || is_MAC_SHARE) {
				HUD_init_message(HM_DEFAULT, "Secret Level Teleporter disabled in Descent 2 Demo");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			if (Game_mode & GM_MULTI) {
				HUD_init_message(HM_DEFAULT, "Secret Level Teleporter disabled in multiplayer!");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			truth = p_secret_level_destroyed();

			if (Newdemo_state == ND_STATE_RECORDING)			// record whether we're really going to the secret level
				newdemo_record_secret_exit_blown(truth);

			if ((Newdemo_state != ND_STATE_PLAYBACK) && truth) {
				HUD_init_message(HM_DEFAULT, "Secret Level destroyed.  Exit disabled.");
				digi_play_sample( SOUND_BAD_SELECTION, F1_0 );
				break;
			}

			if (Newdemo_state == ND_STATE_RECORDING)		// stop demo recording
				Newdemo_state = ND_STATE_PAUSED;

			digi_stop_digi_sounds();

			EnterSecretLevel();
			Control_center_destroyed = 0;
			return 1;
			break;

		}

		case TT_OPEN_DOOR:
			for (int i = 0; i < trig->num_links; i++)
				wall_toggle(trig->seg[i], trig->side[i]);

			if (show_msg)
				HUD_init_message(HM_DEFAULT, "Door%s opened!", pl);
			break;

		case TT_CLOSE_DOOR:
			for (int i = 0; i < trig->num_links; i++)
				wall_close_door(&Segments[trig->seg[i]], trig->side[i]);

			if (show_msg)
				HUD_init_message(HM_DEFAULT, "Door%s closed!", pl);
			break;

		case TT_UNLOCK_DOOR:
			for (int i = 0; i < trig->num_links; i++) {
				Walls[Segments[trig->seg[i]].sides[trig->side[i]].wall_num].flags &= ~WALL_DOOR_LOCKED;
				Walls[Segments[trig->seg[i]].sides[trig->side[i]].wall_num].keys = KEY_NONE;
			}

			if (show_msg)
				HUD_init_message(HM_DEFAULT, "Door%s unlocked!", pl);
			break;

		case TT_LOCK_DOOR:
			for (int i = 0; i < trig->num_links; i++)
				Walls[Segments[trig->seg[i]].sides[trig->side[i]].wall_num].flags |= WALL_DOOR_LOCKED;

			if (show_msg)
				HUD_init_message(HM_DEFAULT, "Door%s locked!", pl);
			break;

		case TT_OPEN_WALL:
			if (do_change_walls(trig) && show_msg) {
				if (wall_is_forcefield(trig))
					HUD_init_message(HM_DEFAULT, "Force field%s deactivated!", pl);
				else
					HUD_init_message(HM_DEFAULT, "Wall%s opened!", pl);
			}
			break;

		case TT_CLOSE_WALL:
			if (do_change_walls(trig) && show_msg) {
				if (wall_is_forcefield(trig))
					HUD_init_message(HM_DEFAULT, "Force field%s activated!", pl);
				else
					HUD_init_message(HM_DEFAULT, "Wall%s closed!", pl);
			}
			break;

		case TT_ILLUSORY_WALL:
			//don't know what to say, so say nothing
			do_change_walls(trig);
			break;

		case TT_MATCEN:
			if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_ROBOTS)) {
				for (int i = 0; i < trig->num_links; i++)
					trigger_matcen(trig->seg[i]);
			}
			break;

		case TT_ILLUSION_ON:
			for (int i = 0; i < trig->num_links; i++)
				wall_illusion_on(&Segments[trig->seg[i]], trig->side[i]);

			if (show_msg)
				HUD_init_message(HM_DEFAULT, "Illusion%s on!", pl);
			break;

		case TT_ILLUSION_OFF:
			for (int i = 0; i < trig->num_links; i++) {
				vms_vector	cp;
				segment		*seg = &Segments[trig->seg[i]];
				int			side = trig->side[i];

				wall_illusion_off(seg, side);

				compute_center_point_on_side(&cp, seg, side );
				digi_link_sound_to_pos( SOUND_WALL_REMOVED, seg-Segments, side, &cp, 0, F1_0 );
			}

			if (show_msg)
				HUD_init_message(HM_DEFAULT, "Illusion%s off!", pl);
			break;

		case TT_LIGHT_OFF:
			if (do_change_light(trig, false) && show_msg)
				HUD_init_message(HM_DEFAULT, "Lights off!");
			break;

		case TT_LIGHT_ON:
			if (do_change_light(trig, true) && show_msg)
				HUD_init_message(HM_DEFAULT, "Lights on!");
			break;

		// D2X-XL
		case TT_TELEPORT:
			if (!Player_is_dead) {
				printf("D2X-XL: teleport\n");
				do_teleport_player(trig);
				if (show_msg)
					HUD_init_message(HM_DEFAULT, "Teleporting!");
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

	if ((Game_mode & GM_MULTI) && (Players[Player_num].connected != CONNECT_PLAYING)) // as a host we may want to handle triggers for our clients. so this function may be called when we are not playing.
		return;

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
}

void trigger_delete_object(int objnum)
{
	object *obj = &Objects[objnum];

	if (!(obj->flags & OF_HAS_TRIGGERS))
		return;

	for (int n = 0; n < Num_object_triggers; n++) {
		trigger *trig = &ObjectTriggers[n];
		if (trig->object_id == objnum) {
			// Coincidentally destroying the object also triggers the trigger.
			do_trigger(OBJECT_TRIGGER_INDEX(n), Player_num, 1);
			trig->object_id = -1;
		}
	}
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
		t->flags = (uint16_t)cfile_read_short(fp);
	} else {
		t->flags = cfile_read_byte(fp);
	}
	t->num_links = cfile_read_byte(fp);
	cfile_read_byte(fp);
	t->value = cfile_read_fix(fp);
	t->time = cfile_read_fix(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->seg[i] = cfile_read_short(fp);
	for (i=0; i<MAX_WALLS_PER_LINK; i++ )
		t->side[i] = cfile_read_short(fp);
}
