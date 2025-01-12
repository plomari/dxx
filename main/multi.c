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
 * Multiplayer code for network play.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "u_mem.h"
#include "strutil.h"
#include "game.h"
#include "multi.h"
#include "object.h"
#include "laser.h"
#include "fuelcen.h"
#include "scores.h"
#include "gauges.h"
#include "collide.h"
#include "dxxerror.h"
#include "fireball.h"
#include "newmenu.h"
#include "console.h"
#include "wall.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "polyobj.h"
#include "bm.h"
#include "endlevel.h"
#include "key.h"
#include "playsave.h"
#include "timer.h"
#include "digi.h"
#include "sounds.h"
#include "kconfig.h"
#include "newdemo.h"
#include "text.h"
#include "kmatrix.h"
#include "multibot.h"
#include "gameseq.h"
#include "physics.h"
#include "config.h"
#include "ai.h"
#include "switch.h"
#include "textures.h"
#include "byteswap.h"
#include "sounds.h"
#include "args.h"
#include "cfile.h"
#include "effects.h"
#include "iff.h"
#include "state.h"
#include "automap.h"
#ifdef USE_UDP
#include "net_udp.h"
#endif
#include "automap.h"

void multi_reset_player_object(object *objp);
void multi_reset_object_texture(object *objp);
void multi_add_lifetime_killed();
void multi_add_lifetime_kills();
void multi_send_play_by_play(int num,int spnum,int dpnum);
void multi_send_heartbeat();
void multi_powcap_cap_objects();
void multi_powcap_adjust_remote_cap(int pnum);
void multi_set_robot_ai(void);
void multi_send_powcap_update();
void bash_to_shield(int i,char *s);
void init_hoard_data();
void multi_apply_goal_textures();
int  find_goal_texture(ubyte t);
void multi_do_capture_bonus(char *buf);
void multi_do_orb_bonus(char *buf);
void multi_send_drop_flag(int objnum,int seed);
void multi_send_ranking();
void multi_do_play_by_play(char *buf);
void multi_new_bounty_target( int pnum );
void multi_do_bounty( char *buf );

//
// Local macros and prototypes
//

// LOCALIZE ME!!

#define vm_angvec_zero(v) (v)->p=(v)->b=(v)->h=0

void drop_player_eggs(object *player); // from collide.c

//
// Global variables
//

int multi_protocol=0; // set and determinate used protocol
int imulti_new_game=0; // to prep stuff for level only when starting new game
int who_killed_controlcen = -1;  // -1 = noone

//do we draw the kill list on the HUD?
int Show_kill_list = 1;
int Show_reticle_name = 1;
fix Show_kill_list_timer = 0;

char Multi_is_guided=0;
char PKilledFlags[MAX_NUM_NET_PLAYERS];
int Bounty_target = 0; // Target for bounty mode netgame

int multi_sending_message = 0;
int multi_defining_message = 0;
int multi_message_index = 0;

char multibuf[MAX_MULTI_MESSAGE_LEN+4];            // This is where multiplayer message are built

short remote_to_local[MAX_NUM_NET_PLAYERS][MAX_OBJECTS];  // Remote object number for each local object
short local_to_remote[MAX_OBJECTS];
sbyte object_owner[MAX_OBJECTS];   // Who created each object in my universe, -1 = loaded at start

int   Net_create_objnums[MAX_NET_CREATE_OBJECTS]; // For tracking object creation that will be sent to remote
int   Net_create_loc = 0;       // pointer into previous array
int   Network_status = 0;
char  Network_message[MAX_MESSAGE_LEN];
int   Network_message_reciever=-1;
int   sorted_kills[MAX_NUM_NET_PLAYERS];
short kill_matrix[MAX_NUM_NET_PLAYERS][MAX_NUM_NET_PLAYERS];
int   multi_goto_secret = 0;
short team_kills[2];
int   multi_quit_game = 0;
char *GMNames[9]={"Anarchy","Team Anarchy","Robo Anarchy","Cooperative","Capture the Flag","Hoard","Team Hoard","Bounty","Unknown"};
char *GMNamesShrt[9]={"ANRCHY","TEAM","ROBO","COOP","FLAG","HOARD","TMHOARD","BOUNTY","UNKNOWN"};

// For rejoin object syncing (used here and all protocols - globally)

int	Network_send_objects = 0;  // Are we in the process of sending objects to a player?
int	Network_send_object_mode = 0; // What type of objects are we sending, static or dynamic?
int 	Network_send_objnum = -1;   // What object are we sending next?
int     Network_rejoined = 0;       // Did WE rejoin this game?
int     Network_new_game = 0;            // Is this the first level of a new game?
int     Network_sending_extras=0;
int     VerifyPlayerJoined=-1;      // Player (num) to enter game before any ingame/extra stuff is being sent
int     Player_joining_extras=-1;  // This is so we know who to send 'latecomer' packets to.
int     Network_player_added = 0;   // Is this a new player or a returning player?

ushort          my_segments_checksum = 0;

netgame_info Netgame;

bitmap_index multi_player_textures[MAX_NUM_NET_PLAYERS][N_PLAYER_SHIP_TEXTURES];

// Globals for protocol-bound Refuse-functions
char RefuseThisPlayer=0,WaitForRefuseAnswer=0,RefuseTeam,RefusePlayerName[12];
fix64 RefuseTimeLimit=0;

static const int message_length[MULTI_MAX_TYPE+1] = {
	25, // POSITION
	3,  // REAPPEAR
	8,  // FIRE
	5,  // KILL
	4,  // REMOVE_OBJECT
	97+9, // PLAYER_EXPLODE
	37, // MESSAGE (MAX_MESSAGE_LENGTH = 40)
	2,  // QUIT
	4,  // PLAY_SOUND
	41, // BEGIN_SYNC
	4,  // CONTROLCEN
	5,  // CLAIM ROBOT
	4,  // END_SYNC
	2,  // CLOAK
	3,  // ENDLEVEL_START
	5,  // DOOR_OPEN
	2,  // CREATE_EXPLOSION
	16, // CONTROLCEN_FIRE
	97+9, // PLAYER_DROP
	19, // CREATE_POWERUP
	9,  // MISSILE_TRACK
	2,  // DE-CLOAK
	2,  // MENU_CHOICE
	28, // ROBOT_POSITION  (shortpos_length (23) + 5 = 28)
	9,  // ROBOT_EXPLODE
	5,  // ROBOT_RELEASE
	18, // ROBOT_FIRE
	6,  // SCORE
	6,  // CREATE_ROBOT
	3,  // TRIGGER
	10, // BOSS_ACTIONS
	27, // ROBOT_POWERUPS
	7,  // HOSTAGE_DOOR
	2+24, // SAVE_GAME      (ubyte slot, uint id, char name[20]) // obsolete
	2+4,  // RESTORE_GAME   (ubyte slot, uint id) // obsolete
	-1,  // MULTI_REQ_PLAYER - NEVER USED
	-1, // MULTI_SEND_PLAYER - NEVER USED
	55, // MULTI_MARKER
	12, // MULTI_DROP_WEAPON
	3+sizeof(shortpos), // MULTI_GUIDED, IF SHORTPOS CHANGES, CHANGE MAC VALUE BELOW
	11, // MULTI_STOLEN_ITEMS
	6,  // MULTI_WALL_STATUS
	5,  // MULTI_HEARTBEAT
	9,  // MULTI_KILLGOALS
	9,  // MULTI_SEISMIC
	18, // MULTI_LIGHT
	2,  // MULTI_START_TRIGGER
	6,  // MULTI_FLAGS
	2,  // MULTI_DROP_BLOB
	MAX_POWERUP_TYPES+1, // MULTI_POWCAP_UPDATE
	sizeof(active_door)+3, // MULTI_ACTIVE_DOOR
	4,  // MULTI_SOUND_FUNCTION
	2,  // MULTI_CAPTURE_BONUS
	2,  // MULTI_GOT_FLAG
	12, // MULTI_DROP_FLAG
	1,	 // MULTI_ROBOT_CONTROLS - UNUSED
	2,  // MULTI_FINISH_GAME
	3,  // MULTI_RANK
	1,  // MULTI_MODEM_PING
	1,  // MULTI_MODEM_PING_RETURN
	3,  // MULTI_ORB_BONUS
	2,  // MULTI_GOT_ORB
	12, // MULTI_DROP_ORB
	4,  // MULTI_PLAY_BY_PLAY
	2,  // MULTI_DO_BOUNTY
};

char PowerupsInMine[MAX_POWERUP_TYPES],MaxPowerupsAllowed[MAX_POWERUP_TYPES];
extern fix ThisLevelTime;

char *RankStrings[]={"(unpatched) ","Cadet ","Ensign ","Lieutenant ","Lt.Commander ",
                     "Commander ","Captain ","Vice Admiral ","Admiral ","Demigod "};

char *multi_allow_powerup_text[MULTI_ALLOW_POWERUP_MAX] =
{ "Laser upgrade", "Super lasers", "Quad Lasers", "Vulcan cannon", "Gauss cannon", "Spreadfire cannon", 
"Helix cannon", "Plasma cannon", "Phoenix cannon", "Fusion cannon", "Omega cannon",
"Flash Missiles", "Homing Missiles", "Guided Missiles", "Proximity Bombs", "Smart Mines",
"Smart Missiles", "Mercury Missiles", "Mega Missiles", "EarthShaker Missiles", 
"Cloaking", "Invulnerability", "Afterburners", "Ammo rack", "Energy Converter", "Headlight" };

int GetMyNetRanking()
{
	int rank, eff;

	if (PlayerCfg.NetlifeKills+PlayerCfg.NetlifeKilled==0)
		return (1);

	rank=(int) (((float)PlayerCfg.NetlifeKills/3000.0)*8.0);

	eff=(int)((float)((float)PlayerCfg.NetlifeKills/((float)PlayerCfg.NetlifeKilled+(float)PlayerCfg.NetlifeKills))*100.0);

	if (rank>8)
		rank=8;

	if (eff<0)
		eff=0;

	if (eff<60)
		rank-=((59-eff)/10);

	if (rank<0)
		rank=0;
	if (rank>8)
		rank=8;

	return (rank+1);
}

void ClipRank (ubyte *rank)
{
	// This function insures no crashes when dealing with D2 1.0
	if (*rank > 9)
		*rank = 0;
}

//
//  Functions that replace what used to be macros
//

int objnum_remote_to_local(int remote_objnum, int owner)
{
	// Map a remote object number from owner to a local object number

	int result;

	if ((owner >= N_players) || (owner < -1)) {
		Int3(); // Illegal!
		return(remote_objnum);
	}

	if (owner == -1)
		return(remote_objnum);

	if ((remote_objnum < 0) || (remote_objnum >= MAX_OBJECTS))
		return(-1);

	result = remote_to_local[owner][remote_objnum];

	if (result < 0)
	{
		return(-1);
	}

	return(result);
}

int objnum_local_to_remote(int local_objnum, sbyte *owner)
{
	// Map a local object number to a remote + owner

	int result;

	if ((local_objnum < 0) || (local_objnum > Highest_object_index)) {
		*owner = -1;
		return(-1);
	}

	*owner = object_owner[local_objnum];

	if (*owner == -1)
		return(local_objnum);

	if ((*owner >= N_players) || (*owner < -1)) {
		Int3(); // Illegal!
		*owner = -1;
		return local_objnum;
	}

	result = local_to_remote[local_objnum];

	if (result < 0)
	{
		Int3(); // See Rob, object has no remote number!
	}

	return(result);
}

void
map_objnum_local_to_remote(int local_objnum, int remote_objnum, int owner)
{
	// Add a mapping from a network remote object number to a local one

	Assert(local_objnum > -1);
	Assert(local_objnum < MAX_OBJECTS);
	Assert(remote_objnum > -1);
	Assert(remote_objnum < MAX_OBJECTS);
	Assert(owner > -1);
	Assert(owner != Player_num);

	object_owner[local_objnum] = owner;

	remote_to_local[owner][remote_objnum] = local_objnum;
	local_to_remote[local_objnum] = remote_objnum;

	return;
}

void
map_objnum_local_to_local(int local_objnum)
{
	// Add a mapping for our locally created objects

	Assert(local_objnum > -1);
	Assert(local_objnum < MAX_OBJECTS);

	object_owner[local_objnum] = Player_num;
	remote_to_local[Player_num][local_objnum] = local_objnum;
	local_to_remote[local_objnum] = local_objnum;

	return;
}

void reset_network_objects()
{
	memset(local_to_remote, -1, MAX_OBJECTS*sizeof(short));
	memset(remote_to_local, -1, MAX_NUM_NET_PLAYERS*MAX_OBJECTS*sizeof(short));
	memset(object_owner, -1, MAX_OBJECTS);
}

int multi_objnum_is_past(int objnum)
{
	switch (multi_protocol)
	{
		case MULTI_PROTO_UDP:
#ifdef USE_UDP
			return net_udp_objnum_is_past(objnum);
			break;
#endif
		default:
			Error("Protocol handling missing in multi_objnum_is_past\n");
			break;
	}
}

//
// Part 1 : functions whose main purpose in life is to divert the flow
//          of execution to either network  specific code based
//          on the curretn Game_mode value.
//

void
multi_endlevel_score(void)
{
	int i, old_connect=0;

	// Show a score list to end of net players

	// Save connect state and change to new connect state
#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		old_connect = Players[Player_num].connected;
		if (Players[Player_num].connected!=CONNECT_DIED_IN_MINE)
			Players[Player_num].connected = CONNECT_END_MENU;
		Network_status = NETSTAT_ENDLEVEL;
	}
#endif

	// Do the actual screen we wish to show
	kmatrix_view(Game_mode & GM_NETWORK);

	// Restore connect state

	if (Game_mode & GM_NETWORK)
	{
		Players[Player_num].connected = old_connect;
	}

	if (Game_mode & GM_MULTI_COOP)
	{
		for (i = 0; i < MaxNumNetPlayers; i++)
			// Reset keys
			Players[i].flags &= ~(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY);
	}

	for (i = 0; i < MaxNumNetPlayers; i++)
		Players[i].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag

	for (i=0;i<MAX_PLAYERS;i++)
		Players[i].KillGoalCount=0;

	for (i=0;i<MAX_POWERUP_TYPES;i++)
	{
		MaxPowerupsAllowed[i]=0;
		PowerupsInMine[i]=0;
	}
}

int
get_team(int pnum)
{
	if (Netgame.team_vector & (1 << pnum))
		return 1;
	else
		return 0;
}

extern void game_disable_cheats();

void
multi_new_game(void)
{
	int i, save_pnum = Player_num;
	extern int Final_boss_is_dead;

	// Reset variables for a new net game

	for (Player_num = 0; Player_num < MAX_NUM_NET_PLAYERS; Player_num++)
		init_player_stats_game();
	Player_num = save_pnum;

	memset(kill_matrix, 0, MAX_NUM_NET_PLAYERS*MAX_NUM_NET_PLAYERS*2); // Clear kill matrix

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	{
		sorted_kills[i] = i;
		Players[i].connected = CONNECT_DISCONNECTED;
		Players[i].net_killed_total = 0;
		Players[i].net_kills_total = 0;
		Players[i].flags = 0;
		Players[i].KillGoalCount=0;
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		robot_controlled[i] = -1;
		robot_agitation[i] = 0;
		robot_fired[i] = 0;
	}

	for (i=0;i<MAX_POWERUP_TYPES;i++)
	{
		MaxPowerupsAllowed[i]=0;
		PowerupsInMine[i]=0;
	}

	team_kills[0] = team_kills[1] = 0;
	Final_boss_is_dead=0;
	Endlevel_sequence = 0;
	Control_center_destroyed = 0;
	Player_is_dead = 0;
	multi_quit_game = 0;
	Show_kill_list = 1;
	game_disable_cheats();
	Player_exploded = 0;
	Dead_player_camera = 0;
	Network_new_game = 1;
}

void
multi_make_player_ghost(int playernum)
{
	object *obj;

	if ((playernum == Player_num) || (playernum >= MAX_NUM_NET_PLAYERS) || (playernum < 0))
	{
		Int3(); // Non-terminal, see Rob
		return;
	}

	obj = &Objects[Players[playernum].objnum];

	obj->type = OBJ_GHOST;
	obj->render_type = RT_NONE;
	obj->movement_type = MT_NONE;
	multi_reset_player_object(obj);

	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(playernum);
}

void
multi_make_ghost_player(int playernum)
{
	object *obj;

	if ((playernum == Player_num) || (playernum >= MAX_NUM_NET_PLAYERS))
	{
		Int3(); // Non-terminal, see rob
		return;
	}

	obj = &Objects[Players[playernum].objnum];

	obj->type = OBJ_PLAYER;
	obj->movement_type = MT_PHYSICS;
	multi_reset_player_object(obj);
}

int multi_get_kill_list(int *plist)
{
	// Returns the number of active net players and their
	// sorted order of kills
	int i;
	int n = 0;

	for (i = 0; i < N_players; i++)
		//if (Players[sorted_kills[i]].connected)
		plist[n++] = sorted_kills[i];

	if (n == 0)
		Int3(); // SEE ROB OR MATT

	//memcpy(plist, sorted_kills, N_players*sizeof(int));

	return(n);
}

void
multi_sort_kill_list(void)
{
	// Sort the kills list each time a new kill is added

	int kills[MAX_NUM_NET_PLAYERS];
	int i;
	int changed = 1;

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	{
		if (Game_mode & GM_MULTI_COOP)
			kills[i] = Players[i].score;
		else
		if (Show_kill_list==2)
		{
			if (Players[i].net_killed_total+Players[i].net_kills_total==0)
				kills[i]=-1;  // always draw the ones without any ratio last
			else
				kills[i]=(int)((float)((float)Players[i].net_kills_total/((float)Players[i].net_killed_total+(float)Players[i].net_kills_total))*100.0);
		}
		else
			kills[i] = Players[i].net_kills_total;
	}

	while (changed)
	{
		changed = 0;
		for (i = 0; i < N_players-1; i++)
		{
			if (kills[sorted_kills[i]] < kills[sorted_kills[i+1]])
			{
				changed = sorted_kills[i];
				sorted_kills[i] = sorted_kills[i+1];
				sorted_kills[i+1] = changed;
				changed = 1;
			}
		}
	}
}

extern object *obj_find_first_of_type (int);

void multi_compute_kill(int killer, int killed)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int killed_pnum, killed_type;
	int killer_pnum, killer_type,killer_id;
	int TheGoal;
	char killed_name[(CALLSIGN_LEN*2)+4];
	char killer_name[(CALLSIGN_LEN*2)+4];

	// Both object numbers are localized already!

	if ((killed < 0) || (killed > Highest_object_index) || (killer < 0) || (killer > Highest_object_index))
	{
		Int3(); // See Rob, illegal value passed to compute_kill;
		return;
	}

	killed_type = Objects[killed].type;
	killer_type = Objects[killer].type;
	killer_id = Objects[killer].id;

	if ((killed_type != OBJ_PLAYER) && (killed_type != OBJ_GHOST))
	{
		Int3(); // compute_kill passed non-player object!
		return;
	}

	killed_pnum = Objects[killed].id;

	PKilledFlags[killed_pnum]=1;

	Assert ((killed_pnum >= 0) && (killed_pnum < N_players));

	if (Game_mode & GM_TEAM)
		sprintf(killed_name, "%s (%s)", Players[killed_pnum].callsign, Netgame.team_name[get_team(killed_pnum)]);
	else
		sprintf(killed_name, "%s", Players[killed_pnum].callsign);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_death(killed_pnum);

	digi_play_sample( SOUND_HUD_KILL, F3_0 );

	if (Control_center_destroyed)
		Players[killed_pnum].connected=CONNECT_DIED_IN_MINE;

	if (killer_type == OBJ_CNTRLCEN)
	{
		Players[killed_pnum].net_killed_total++;
		Players[killed_pnum].net_kills_total--;

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_kill(killed_pnum, -1);

		if (killed_pnum == Player_num)
		{
			HUD_init_message(HM_MULTI, "%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_NONPLAY);
			multi_add_lifetime_killed ();
		}
		else
			HUD_init_message(HM_MULTI, "%s %s %s.", killed_name, TXT_WAS, TXT_KILLED_BY_NONPLAY );
		return;
	}

	else if ((killer_type != OBJ_PLAYER) && (killer_type != OBJ_GHOST))
	{
		if (killer_id==PMINE_ID && killer_type!=OBJ_ROBOT)
		{
			if (killed_pnum == Player_num)
				HUD_init_message(HM_MULTI, "You were killed by a mine!");
			else
				HUD_init_message(HM_MULTI, "%s was killed by a mine!",killed_name);
		}
		else
		{
			if (killed_pnum == Player_num)
			{
				HUD_init_message(HM_MULTI, "%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_ROBOT);
				multi_add_lifetime_killed();
			}
			else
				HUD_init_message(HM_MULTI, "%s %s %s.", killed_name, TXT_WAS, TXT_KILLED_BY_ROBOT );
		}
		Players[killed_pnum].net_killed_total++;
		return;
	}

	killer_pnum = Objects[killer].id;

	if (Game_mode & GM_TEAM)
		sprintf(killer_name, "%s (%s)", Players[killer_pnum].callsign, Netgame.team_name[get_team(killer_pnum)]);
	else
		sprintf(killer_name, "%s", Players[killer_pnum].callsign);

	// Beyond this point, it was definitely a player-player kill situation

	if ((killer_pnum < 0) || (killer_pnum >= N_players))
		Int3(); // See rob, tracking down bug with kill HUD messages
	if ((killed_pnum < 0) || (killed_pnum >= N_players))
		Int3(); // See rob, tracking down bug with kill HUD messages

	if (killer_pnum == killed_pnum)
	{
		if (!(Game_mode & GM_HOARD))
		{
			if (Game_mode & GM_TEAM)
				team_kills[get_team(killed_pnum)] -= 1;

			Players[killed_pnum].net_killed_total += 1;
			Players[killed_pnum].net_kills_total -= 1;

			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_multi_kill(killed_pnum, -1);
		}
		kill_matrix[killed_pnum][killed_pnum] += 1; // # of suicides

		if (killer_pnum == Player_num)
		{
			HUD_init_message(HM_MULTI, "%s %s %s!", TXT_YOU, TXT_KILLED, TXT_YOURSELF );
			multi_add_lifetime_killed();
		}
		else
			HUD_init_message(HM_MULTI, "%s %s", killed_name, TXT_SUICIDE);

		/* Bounty mode needs some lovin' */
		if( Game_mode & GM_BOUNTY && killed_pnum == Bounty_target && multi_i_am_master() )
		{
			/* Select a random number */
			int new_bounty_target = d_rand() % MAX_NUM_NET_PLAYERS;
			
			/* Make sure they're valid: Don't check against kill flags,
			 * just in case everyone's dead! */
			while( !Players[new_bounty_target].connected )
				new_bounty_target = d_rand() % MAX_NUM_NET_PLAYERS;
			
			/* Select new target */
			multi_new_bounty_target( new_bounty_target );
			
			/* Send this new data */
			multi_send_bounty();
		}
	}

	else
	{
		if (!(Game_mode & GM_HOARD))
		{
			if (Game_mode & GM_TEAM)
			{
				if (get_team(killed_pnum) == get_team(killer_pnum))
				{
					team_kills[get_team(killed_pnum)] -= 1;
					Players[killer_pnum].net_kills_total -= 1;
				}
				else
				{
					team_kills[get_team(killer_pnum)] += 1;
					Players[killer_pnum].net_kills_total += 1;
					Players[killer_pnum].KillGoalCount +=1;
				}
			}
			else if( Game_mode & GM_BOUNTY )
			{
				/* Did the target die?  Did the target get a kill? */
				if( killed_pnum == Bounty_target || killer_pnum == Bounty_target )
				{
					/* Increment kill counts */
					Players[killer_pnum].net_kills_total++;
					Players[killer_pnum].KillGoalCount++;
					
					/* Record the kill in a demo */
					if( Newdemo_state == ND_STATE_RECORDING )
						newdemo_record_multi_kill( killer_pnum, 1 );
					
					/* If the target died, the new one is set! */
					if( killed_pnum == Bounty_target )
						multi_new_bounty_target( killer_pnum );
				}
			}
			else
			{
				Players[killer_pnum].net_kills_total += 1;
				Players[killer_pnum].KillGoalCount+=1;
			}
			
			if (Newdemo_state == ND_STATE_RECORDING && !( Game_mode & GM_BOUNTY ) )
				newdemo_record_multi_kill(killer_pnum, 1);
		}

		kill_matrix[killer_pnum][killed_pnum] += 1;
		Players[killed_pnum].net_killed_total += 1;
		if (killer_pnum == Player_num) {
			HUD_init_message(HM_MULTI, "%s %s %s!", TXT_YOU, TXT_KILLED, killed_name);
			multi_add_lifetime_kills();
			if ((Game_mode & GM_MULTI_COOP) && (Players[Player_num].score >= 1000))
				add_points_to_score(-1000);
		}
		else if (killed_pnum == Player_num)
		{
			HUD_init_message(HM_MULTI, "%s %s %s!", killer_name, TXT_KILLED, TXT_YOU);
			multi_add_lifetime_killed();
			if (Game_mode & GM_HOARD)
			{
				if (Players[Player_num].secondary_ammo[PROXIMITY_INDEX]>3)
					multi_send_play_by_play (1,killer_pnum,Player_num);
				else if (Players[Player_num].secondary_ammo[PROXIMITY_INDEX]>0)
					multi_send_play_by_play (0,killer_pnum,Player_num);
			}
		}
		else
			HUD_init_message(HM_MULTI, "%s %s %s!", killer_name, TXT_KILLED, killed_name);
	}

	TheGoal=Netgame.KillGoal*5;

	if (Netgame.KillGoal>0)
	{
		if (Players[killer_pnum].KillGoalCount>=TheGoal)
		{
			if (killer_pnum==Player_num)
			{
				HUD_init_message(HM_MULTI, "You reached the kill goal!");
				Players[Player_num].shields=i2f(200);
			}
			else
				HUD_init_message(HM_MULTI, "%s has reached the kill goal!",Players[killer_pnum].callsign);

			HUD_init_message(HM_MULTI, "The control center has been destroyed!");
			net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
		}
	}

	multi_sort_kill_list();
	multi_show_player_list();
	Players[killed_pnum].flags&=(~(PLAYER_FLAGS_HEADLIGHT_ON));  // clear the killed guys flags/headlights
}

void multi_do_protocol_frame(int force, int listen)
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_do_frame(force, listen);
			break;
#endif
		default:
			Error("Protocol handling missing in multi_do_protocol_frame\n");
			break;
	}
}

void multi_do_frame(void)
{
	static int lasttime=0;
	int i;

	if (!(Game_mode & GM_MULTI) || Newdemo_state == ND_STATE_PLAYBACK)
	{
		Int3();
		return;
	}

	if ((Game_mode & GM_NETWORK) && Netgame.PlayTimeAllowed && lasttime!=f2i (ThisLevelTime))
	{
		for (i=0;i<N_players;i++)
			if (Players[i].connected)
			{
				if (i==Player_num)
				{
					multi_send_heartbeat();
					lasttime=f2i(ThisLevelTime);
				}
				break;
			}
	}

	multi_send_message(); // Send any waiting messages

	if (Game_mode & GM_MULTI_ROBOTS)
	{
		multi_check_robot_timeout();
	}

	multi_do_protocol_frame(0, 1);

	if (multi_quit_game)
	{
		multi_quit_game = 0;
		if (Game_wind)
			window_close(Game_wind);
	}
}

void
multi_send_data(char *buf, int len, int priority)
{
	if (len != message_length[(int)buf[0]])
		Error("multi_send_data: Packet type %i length: %i, expected: %i\n", buf[0], len, message_length[(int)buf[0]]);
	if (buf[0] < 0 || buf[0] > MULTI_MAX_TYPE)
		Error("multi_send_data: Illegal packet type %i\n", buf[0]);

	if (Game_mode & GM_NETWORK)
	{
		switch (multi_protocol)
		{
#ifdef USE_UDP
			case MULTI_PROTO_UDP:
				net_udp_send_data((unsigned char *)buf, len, priority);
				break;
#endif
			default:
				Error("Protocol handling missing in multi_send_data_real\n");
				break;
		}
	}
}

void multi_send_data_direct(unsigned char *buf, int len, int pnum, int priority)
{
	if (len != message_length[(int)buf[0]])
		Error("multi_send_data_direct: Packet type %i length: %i, expected: %i\n", buf[0], len, message_length[(int)buf[0]]);
	if (buf[0] < 0 || buf[0] > MULTI_MAX_TYPE)
		Error("multi_send_data_direct: Illegal packet type %i\n", buf[0]);
	if (pnum < 0 && pnum > MAX_NUM_NET_PLAYERS)
		Error("multi_send_data_direct: Illegal player num: %i\n", pnum);

	net_udp_send_mdata_direct((ubyte *)multibuf, len, pnum, priority);
}

void
multi_leave_game(void)
{

	if (!(Game_mode & GM_MULTI))
		return;

	if (Game_mode & GM_NETWORK)
	{
		Net_create_loc = 0;
		multi_send_position(Players[Player_num].objnum);
		multi_powcap_cap_objects();
		if (!Player_eggs_dropped)
		{
			drop_player_eggs(ConsoleObject);
			Player_eggs_dropped = 1;
		}
		multi_send_player_explode(MULTI_PLAYER_DROP);
	}

	multi_send_quit(MULTI_QUIT);

	if (Game_mode & GM_NETWORK)
	{
		switch (multi_protocol)
		{
#ifdef USE_UDP
			case MULTI_PROTO_UDP:
				net_udp_leave_game();
				break;
#endif
			default:
				Error("Protocol handling missing in multi_leave_game\n");
				break;
		}
	}
}

void
multi_show_player_list()
{
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_COOP))
		return;

	if (Show_kill_list)
		return;

	Show_kill_list_timer = F1_0*5; // 5 second timer
	Show_kill_list = 1;
}

int
multi_endlevel(int *secret)
{
	int result = 0;

	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			result = net_udp_endlevel(secret);
			break;
#endif
		default:
			Error("Protocol handling missing in multi_endlevel\n");
			break;
	}

	return(result);
}

int multi_endlevel_poll1( newmenu *menu, d_event *event, void *userdata )
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			return net_udp_kmatrix_poll1( menu, event, userdata );
			break;
#endif
		default:
			Error("Protocol handling missing in multi_endlevel_poll1\n");
			break;
	}
	
	return 0;	// kill warning
}

int multi_endlevel_poll2( newmenu *menu, d_event *event, void *userdata )
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			return net_udp_kmatrix_poll2( menu, event, userdata );
			break;
#endif
		default:
			Error("Protocol handling missing in multi_endlevel_poll2\n");
			break;
	}
	
	return 0;
}

void multi_send_endlevel_packet()
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_send_endlevel_packet();
			break;
#endif
		default:
			Error("Protocol handling missing in multi_send_endlevel_packet\n");
			break;
	}
}

//
// Part 2 : functions that act on network messages and change the
//          the state of the game in some way.
//

void
multi_define_macro(int key)
{
	if (!(Game_mode & GM_MULTI))
		return;

	key &= (~KEY_SHIFTED);

	switch(key)
	{
		case KEY_F9:
			multi_defining_message = 1; break;
		case KEY_F10:
			multi_defining_message = 2; break;
		case KEY_F11:
			multi_defining_message = 3; break;
		case KEY_F12:
			multi_defining_message = 4; break;
		default:
			Int3();
	}

	if (multi_defining_message)     {
		multi_message_index = 0;
		Network_message[multi_message_index] = 0;
	}

}

char feedback_result[200];

void
multi_message_feedback(void)
{
	char *colon;
	int found = 0;
	int i;

	if (!( ((colon = strrchr(Network_message, ':')) == NULL) || (colon-Network_message < 1) || (colon-Network_message > CALLSIGN_LEN) ))
	{
		sprintf(feedback_result, "%s ", TXT_MESSAGE_SENT_TO);
		if ((Game_mode & GM_TEAM) && (atoi(Network_message) > 0) && (atoi(Network_message) < 3))
		{
			sprintf(feedback_result+strlen(feedback_result), "%s '%s'", TXT_TEAM, Netgame.team_name[atoi(Network_message)-1]);
			found = 1;
		}
		if (Game_mode & GM_TEAM)
		{
			for (i = 0; i < N_players; i++)
			{
				if (!strnicmp(Netgame.team_name[i], Network_message, colon-Network_message))
				{
					if (found)
						strcat(feedback_result, ", ");
					found++;
					if (!(found % 4))
						strcat(feedback_result, "\n");
					sprintf(feedback_result+strlen(feedback_result), "%s '%s'", TXT_TEAM, Netgame.team_name[i]);
				}
			}
		}
		for (i = 0; i < N_players; i++)
		{
			if ((!strnicmp(Players[i].callsign, Network_message, colon-Network_message)) && (i != Player_num) && (Players[i].connected))
			{
				if (found)
					strcat(feedback_result, ", ");
				found++;
				if (!(found % 4))
					strcat(feedback_result, "\n");
				sprintf(feedback_result+strlen(feedback_result), "%s", Players[i].callsign);
			}
		}
		if (!found)
			strcat(feedback_result, TXT_NOBODY);
		else
			strcat(feedback_result, ".");

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		Assert(strlen(feedback_result) < 200);

		HUD_init_message(HM_MULTI, feedback_result);
		//sprintf (temp,"%s",colon);
		//sprintf (Network_message,"%s",temp);

	}
}

void
multi_send_macro(int key)
{
	if (! (Game_mode & GM_MULTI) )
		return;

	switch(key)
	{
		case KEY_F9:
			key = 0; break;
		case KEY_F10:
			key = 1; break;
		case KEY_F11:
			key = 2; break;
		case KEY_F12:
			key = 3; break;
		default:
			Int3();
	}

	if (!PlayerCfg.NetworkMessageMacro[key][0])
	{
		HUD_init_message(HM_MULTI, TXT_NO_MACRO);
		return;
	}

	strcpy(Network_message, PlayerCfg.NetworkMessageMacro[key]);
	Network_message_reciever = 100;

	HUD_init_message(HM_MULTI, "%s '%s'", TXT_SENDING, Network_message);
	multi_message_feedback();
}


void
multi_send_message_start()
{
	if (Game_mode&GM_MULTI) {
		multi_sending_message = 1;
		multi_message_index = 0;
		Network_message[multi_message_index] = 0;
	}
}

extern fix StartingShields;

extern int multi_who_is_master();
extern char NameReturning;

void multi_send_message_end()
{
	char *mytempbuf;
	int i,t;

	Network_message_reciever = 100;

	if (!strnicmp (Network_message,"Handicap:",9))
	{
		mytempbuf=&Network_message[9];
		StartingShields=atol (mytempbuf);
		if (StartingShields<10)
			StartingShields=10;
		if (StartingShields>100)
		{
			sprintf (Network_message,"%s has tried to cheat!",Players[Player_num].callsign);
			StartingShields=100;
		}
		else
			sprintf (Network_message,"%s handicap is now %d",Players[Player_num].callsign,StartingShields);

		HUD_init_message(HM_MULTI, "Telling others of your handicap of %d!",StartingShields);
		StartingShields=i2f(StartingShields);
	}
	else if (!strnicmp (Network_message,"move:",5))
	{
		if ((Game_mode & GM_NETWORK) && (Game_mode & GM_TEAM))
		{
			int name_index=5;
			if (strlen(Network_message) > 5)
				while (Network_message[name_index] == ' ')
					name_index++;

			if (!multi_i_am_master())
			{
				HUD_init_message(HM_MULTI, "Only %s can move players!",Players[multi_who_is_master()].callsign);
				return;
			}

			if (strlen(Network_message)<=name_index)
			{
				HUD_init_message(HM_MULTI, "You must specify a name to move");
				return;
			}

			for (i = 0; i < N_players; i++)
				if ((!strnicmp(Players[i].callsign, &Network_message[name_index], strlen(Network_message)-name_index)) && (Players[i].connected))
				{
					if ((Game_mode & GM_CAPTURE) && (Players[i].flags & PLAYER_FLAGS_FLAG))
					{
						HUD_init_message(HM_MULTI, "Can't move player because s/he has a flag!");
						return;
					}

					if (Netgame.team_vector & (1<<i))
						Netgame.team_vector&=(~(1<<i));
					else
						Netgame.team_vector|=(1<<i);

					for (t=0;t<N_players;t++)
						if (Players[t].connected)
							multi_reset_object_texture (&Objects[Players[t].objnum]);

					switch (multi_protocol)
					{
#ifdef USE_UDP
						case MULTI_PROTO_UDP:
							net_udp_send_netgame_update();
							break;
#endif
						default:
							Error("Protocol handling missing in multi_send_message_end\n");
							break;
					}
					
					sprintf (Network_message,"%s has changed teams!",Players[i].callsign);
					if (i==Player_num)
					{
						HUD_init_message(HM_MULTI, "You have changed teams!");
						reset_cockpit();
					}
					else
						HUD_init_message(HM_MULTI, "Moving %s to other team.",Players[i].callsign);
					break;
				}
		}
	}

	else if (!strnicmp (Network_message,"kick:",5) && (Game_mode & GM_NETWORK))
	{
		int name_index=5;
		if (strlen(Network_message) > 5)
			while (Network_message[name_index] == ' ')
				name_index++;

		if (!multi_i_am_master())
		{
			HUD_init_message(HM_MULTI, "Only %s can kick others out!",Players[multi_who_is_master()].callsign);
			multi_message_index = 0;
			multi_sending_message = 0;
			return;
		}
		if (strlen(Network_message)<=name_index)
		{
			HUD_init_message(HM_MULTI, "You must specify a name to kick");
			multi_message_index = 0;
			multi_sending_message = 0;
			return;
		}

		if (Network_message[name_index] == '#' && isdigit(Network_message[name_index+1])) {
			int players[MAX_NUM_NET_PLAYERS];
			int listpos = Network_message[name_index+1] - '0';

			if (Show_kill_list==1 || Show_kill_list==2) {
				if (listpos == 0 || listpos >= N_players) {
					HUD_init_message(HM_MULTI, "Invalid player number for kick.");
					multi_message_index = 0;
					multi_sending_message = 0;
					return;
				}
				multi_get_kill_list(players);
				i = players[listpos];
				if ((i != Player_num) && (Players[i].connected))
					goto kick_player;
			}
			else HUD_init_message(HM_MULTI, "You cannot use # kicking with in team display.");


		    multi_message_index = 0;
		    multi_sending_message = 0;
			return;
		}


		for (i = 0; i < N_players; i++)
		if ((!strnicmp(Players[i].callsign, &Network_message[name_index], strlen(Network_message)-name_index)) && (i != Player_num) && (Players[i].connected)) {
			kick_player:;
				switch (multi_protocol)
				{
#ifdef USE_UDP
					case MULTI_PROTO_UDP:
						net_udp_dump_player(Netgame.players[i].protocol.udp.addr, DUMP_KICKED);
						break;
#endif
					default:
						Error("Protocol handling missing in multi_send_message_end\n");
						break;
				}

				HUD_init_message(HM_MULTI, "Dumping %s...",Players[i].callsign);
				multi_message_index = 0;
				multi_sending_message = 0;
				return;
			}
	}
	
	else if (!strnicmp (Network_message,"killreactor",11) && (Game_mode & GM_NETWORK) && !Control_center_destroyed)
	{
		if (!multi_i_am_master())
			HUD_init_message(HM_MULTI, "Only %s can kill the reactor this way!",Players[multi_who_is_master()].callsign);
		else
		{
			net_destroy_controlcen(NULL);
			multi_send_destroy_controlcen(-1,Player_num);
		}
		multi_message_index = 0;
		multi_sending_message = 0;
		return;
	}

	else
		HUD_init_message(HM_MULTI, "%s '%s'", TXT_SENDING, Network_message);

	multi_send_message();
	multi_message_feedback();

	multi_message_index = 0;
	multi_sending_message = 0;
	game_flush_inputs();
}

void multi_define_macro_end()
{
	Assert( multi_defining_message > 0 );

	strcpy( PlayerCfg.NetworkMessageMacro[multi_defining_message-1], Network_message );
	write_player_file();

	multi_message_index = 0;
	multi_defining_message = 0;
	game_flush_inputs();
}

int multi_message_input_sub(int key)
{
	switch( key )
	{
		case KEY_F8:
		case KEY_ESC:
			multi_sending_message = 0;
			multi_defining_message = 0;
			game_flush_inputs();
			return 1;
		case KEY_LEFT:
		case KEY_BACKSP:
		case KEY_PAD4:
			if (multi_message_index > 0)
				multi_message_index--;
			Network_message[multi_message_index] = 0;
			return 1;
		case KEY_ENTER:
			if ( multi_sending_message )
				multi_send_message_end();
			else if ( multi_defining_message )
				multi_define_macro_end();
			game_flush_inputs();
			return 1;
		default:
		{
			int ascii = key_ascii();
			if ((ascii < 255 ) && (ascii != 37))     {
				if (multi_message_index < MAX_MESSAGE_LEN-2 )   {
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
				} else if ( multi_sending_message )     {
					int i;
					char * ptext, * pcolon;
					ptext = NULL;
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
					for (i=multi_message_index-1; i>=0; i-- )       {
						if ( Network_message[i]==32 )   {
							ptext = &Network_message[i+1];
							Network_message[i] = 0;
							break;
						}
					}
					multi_send_message_end();
					if ( ptext )    {
						multi_sending_message = 1;
						pcolon = strchr( Network_message, ':' );
						if ( pcolon )
							strcpy( pcolon+1, ptext );
						else
							strcpy( Network_message, ptext );
						multi_message_index = strlen( Network_message );
					}
				}
			}
		}
	}
	
	return 0;
}

void
multi_send_message_dialog(void)
{
	newmenu_item m[1];
	int choice;

	if (!(Game_mode&GM_MULTI))
		return;

	Network_message[0] = 0;             // Get rid of old contents

	nm_set_item_input(&m[0], MAX_MESSAGE_LEN-1, Network_message);
	choice = newmenu_do( NULL, TXT_SEND_MESSAGE, 1, m, NULL, NULL );

	if ((choice > -1) && (strlen(Network_message) > 0)) {
		Network_message_reciever = 100;
		HUD_init_message(HM_MULTI, "%s '%s'", TXT_SENDING, Network_message);
		multi_message_feedback();
	}
}



void
multi_do_death(int objnum)
{
	// Do any miscellaneous stuff for a new network player after death

	objnum = objnum;

	if (!(Game_mode & GM_MULTI_COOP))
	{
		Players[Player_num].flags |= (PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY);
	}
}

void
multi_do_fire(char *buf)
{
	ubyte weapon;
	char pnum;
	sbyte flags;
	//static dum=0;

	// Act out the actual shooting
	pnum = buf[1];

	weapon = (int)buf[2];

	flags = buf[4];
	Network_laser_track = GET_INTEL_SHORT(buf + 6);

	Assert (pnum < N_players);

	if (Objects[Players[(int)pnum].objnum].type == OBJ_GHOST)
		multi_make_ghost_player(pnum);

	if (weapon == FLARE_ADJUST)
		Laser_player_fire( Objects+Players[(int)pnum].objnum, FLARE_ID, 6, 1, 0);
	else if (weapon >= MISSILE_ADJUST) {
		int weapon_id,weapon_gun;

		weapon_id = Secondary_weapon_to_weapon_info[weapon-MISSILE_ADJUST];
		weapon_gun = Secondary_weapon_to_gun_num[weapon-MISSILE_ADJUST] + (flags & 1);

		if (weapon-MISSILE_ADJUST==GUIDED_INDEX)
		{
			Multi_is_guided=1;
		}

		Laser_player_fire( Objects+Players[(int)pnum].objnum, weapon_id, weapon_gun, 1, 0 );
	}
	else {
		fix save_charge = Fusion_charge;

		if (weapon == FUSION_INDEX) {
			Fusion_charge = flags << 12;
		}
		if (weapon == LASER_ID) {
			if (flags & LASER_QUAD)
				Players[(int)pnum].flags |= PLAYER_FLAGS_QUAD_LASERS;
			else
				Players[(int)pnum].flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		}

		do_laser_firing(Players[(int)pnum].objnum, weapon, (int)buf[3], flags, (int)buf[5]);

		if (weapon == FUSION_INDEX)
			Fusion_charge = save_charge;
	}
}

void
multi_do_message(char *buf)
{
	char *colon;
	char *tilde,mesbuf[100];
	char dollarbuf[100];
	int tloc,t;

	int loc = 0;
	buf += 2;

	if ((tilde=strchr (buf+loc,'$')))  // do that stupid name stuff
	{											// why'd I put this in?  Probably for the
		tloc=tilde-(buf+loc);				// same reason you can name your guidebot
		snprintf(dollarbuf, sizeof(dollarbuf), "%.*s%s%s", tloc, buf, Players[Player_num].callsign, buf+tloc+1);
		buf = dollarbuf;
	}

	if (((colon = strrchr(buf+loc, ':')) == NULL) || (colon-(buf+loc) < 1) || (colon-(buf+loc) > CALLSIGN_LEN))
	{
		int color = 0;
		mesbuf[0] = CC_COLOR;
		if (Game_mode & GM_TEAM)
			color = get_team((int)buf[1]);
		else
			color = (int)buf[1];
		mesbuf[1] = BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b);
		strcpy(&mesbuf[2], Players[(int)buf[1]].callsign);
		t = strlen(mesbuf);
		mesbuf[t] = ':';
		mesbuf[t+1] = CC_COLOR;
		mesbuf[t+2] = BM_XRGB(0, 31, 0);
		mesbuf[t+3] = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		HUD_init_message(HM_MULTI, "%s %s", mesbuf, buf+2);
	}
	else
	{
		if ( (!strnicmp(Players[Player_num].callsign, buf+loc, colon-(buf+loc))) ||
			 ((Game_mode & GM_TEAM) && ( (get_team(Player_num) == atoi(buf+loc)-1) || !strnicmp(Netgame.team_name[get_team(Player_num)], buf+loc, colon-(buf+loc)))) )
		{
			int color = 0;
			mesbuf[0] = CC_COLOR;
			if (Game_mode & GM_TEAM)
				color = get_team((int)buf[1]);
			else
				color = (int)buf[1];
			mesbuf[1] = BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b);
			strcpy(&mesbuf[2], Players[(int)buf[1]].callsign);
			t = strlen(mesbuf);
			mesbuf[t] = ':';
			mesbuf[t+1] = CC_COLOR;
			mesbuf[t+2] = BM_XRGB(0, 31, 0);
			mesbuf[t+3] = 0;

			digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
			HUD_init_message(HM_MULTI, "%s %s", mesbuf, colon+1);
		}
	}
}

void
multi_do_position(char *buf)
{
	ubyte pnum = 0;
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

	pnum = buf[1];

#ifndef WORDS_BIGENDIAN
	extract_shortpos(&Objects[Players[pnum].objnum], (shortpos *)(buf + 2),0);
#else
	memcpy((ubyte *)(sp.bytemat), (ubyte *)(buf + 2), 9);
	memcpy((ubyte *)&(sp.xo), (ubyte *)(buf + 11), 14);
	extract_shortpos(&Objects[Players[pnum].objnum], &sp, 1);
#endif

	if (Objects[Players[pnum].objnum].movement_type == MT_PHYSICS)
		set_thrust_from_velocity(&Objects[Players[pnum].objnum]);
}

void
multi_do_reappear(char *buf)
{
	short objnum;

	objnum = GET_INTEL_SHORT(buf + 1);

	Assert(objnum >= 0);
	//      Assert(Players[Objects[objnum].id]].objnum == objnum);

	multi_make_ghost_player(Objects[objnum].id);
	create_player_appearance_effect(&Objects[objnum]);
	PKilledFlags[Objects[objnum].id]=0;
}

void
multi_do_player_explode(char *buf)
{
	// Only call this for players, not robots.  pnum is player number, not
	// Object number.

	object *objp;
	int count;
	int pnum;
	int i;
	char remote_created;

	pnum = buf[1];

	if ((pnum < 0) || (pnum >= N_players))
		return;

#ifdef NETWORK
	// If we are in the process of sending objects to a new player, reset that process
	if (Network_send_objects)
	{
		Network_send_objnum = -1;
	}
#endif

	// Stuff the Players structure to prepare for the explosion

	count = 2;
	Players[pnum].primary_weapon_flags = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].secondary_weapon_flags = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].laser_level = buf[count];                                                 count++;
	Players[pnum].secondary_ammo[HOMING_INDEX] = buf[count];                count++;
	Players[pnum].secondary_ammo[CONCUSSION_INDEX] = buf[count];count++;
	Players[pnum].secondary_ammo[SMART_INDEX] = buf[count];         count++;
	Players[pnum].secondary_ammo[MEGA_INDEX] = buf[count];          count++;
	Players[pnum].secondary_ammo[PROXIMITY_INDEX] = buf[count]; count++;

	Players[pnum].secondary_ammo[SMISSILE1_INDEX] = buf[count]; count++;
	Players[pnum].secondary_ammo[GUIDED_INDEX]    = buf[count]; count++;
	Players[pnum].secondary_ammo[SMART_MINE_INDEX]= buf[count]; count++;
	Players[pnum].secondary_ammo[SMISSILE4_INDEX] = buf[count]; count++;
	Players[pnum].secondary_ammo[SMISSILE5_INDEX] = buf[count]; count++;

	Players[pnum].primary_ammo[VULCAN_INDEX] = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].primary_ammo[GAUSS_INDEX] = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].flags = GET_INTEL_INT(buf + count);               count += 4;

	multi_powcap_adjust_remote_cap (pnum);

	objp = Objects+Players[pnum].objnum;

	//      objp->phys_info.velocity = *(vms_vector *)(buf+16); // 12 bytes
	//      objp->pos = *(vms_vector *)(buf+28);                // 12 bytes

	remote_created = buf[count++]; // How many did the other guy create?

	Net_create_loc = 0;

	drop_player_eggs(objp);

	// Create mapping from remote to local numbering system

	// We now handle this situation gracefully, Int3 not required
	//      if (Net_create_loc != remote_created)
	//              Int3(); // Probably out of object array space, see Rob

	for (i = 0; i < remote_created; i++)
	{
		short s;

		s = GET_INTEL_SHORT(buf + count);

		if ((i < Net_create_loc) && (s > 0))
			map_objnum_local_to_remote((short)Net_create_objnums[i], s, pnum);
		count += 2;
	}
	for (i = remote_created; i < Net_create_loc; i++) {
		Objects[Net_create_objnums[i]].flags |= OF_SHOULD_BE_DEAD;
	}

	if (buf[0] == MULTI_PLAYER_EXPLODE)
	{
		explode_badass_player(objp);

		objp->flags &= ~OF_SHOULD_BE_DEAD;              //don't really kill player
		multi_make_player_ghost(pnum);
	}
	else
	{
		create_player_appearance_effect(objp);
	}

	Players[pnum].flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_FLAG);
	Players[pnum].cloak_time = 0;
}

void
multi_do_kill(char *buf)
{
	int killer, killed;
	int count = 1;
	int pnum;

	pnum = (int)(buf[count]);
	if ((pnum < 0) || (pnum >= N_players))
	{
		Int3(); // Invalid player number killed
		return;
	}

	killed = Players[pnum].objnum;
	count += 1;

	killer = GET_INTEL_SHORT(buf + count);
	if (killer > 0)
		killer = objnum_remote_to_local(killer, (sbyte)buf[count+2]);

	multi_compute_kill(killer, killed);
}

void multi_do_controlcen_destroy(char *buf)
{
	sbyte who;
	short objnum;

	objnum = GET_INTEL_SHORT(buf + 1);
	who = buf[3];

	if (Control_center_destroyed != 1)
	{
		if ((who < N_players) && (who != Player_num)) {
			HUD_init_message(HM_MULTI, "%s %s", Players[who].callsign, TXT_HAS_DEST_CONTROL);
		}
		else if (who == Player_num)
			HUD_init_message(HM_MULTI, TXT_YOU_DEST_CONTROL);
		else
			HUD_init_message(HM_MULTI, TXT_CONTROL_DESTROYED);

		if (objnum != -1)
			net_destroy_controlcen(Objects+objnum);
		else
			net_destroy_controlcen(NULL);
	}
}

void
multi_do_escape(char *buf)
{
	int objnum;

	objnum = Players[(int)buf[1]].objnum;

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
	digi_kill_sound_linked_to_object (objnum);

	if (buf[2] == 0)
	{
		HUD_init_message(HM_MULTI, "%s %s", Players[(int)buf[1]].callsign, TXT_HAS_ESCAPED);
		if (Game_mode & GM_NETWORK)
			Players[(int)buf[1]].connected = CONNECT_ESCAPE_TUNNEL;
		if (!multi_goto_secret)
			multi_goto_secret = 2;
	}
	else if (buf[2] == 1)
	{
		HUD_init_message(HM_MULTI, "%s %s", Players[(int)buf[1]].callsign, TXT_HAS_FOUND_SECRET);
		if (Game_mode & GM_NETWORK)
			Players[(int)buf[1]].connected = CONNECT_FOUND_SECRET;
		if (!multi_goto_secret)
			multi_goto_secret = 1;
	}
	create_player_appearance_effect(&Objects[objnum]);
	multi_make_player_ghost(buf[1]);
}

void
multi_do_remobj(char *buf)
{
	short objnum; // which object to remove
	short local_objnum;
	sbyte obj_owner; // which remote list is it entered in

	objnum = GET_INTEL_SHORT(buf + 1);
	obj_owner = buf[3];

	Assert(objnum >= 0);

	if (objnum < 1)
		return;

	local_objnum = objnum_remote_to_local(objnum, obj_owner); // translate to local objnum

	if (local_objnum < 0)
	{
		return;
	}

	if ((Objects[local_objnum].type != OBJ_POWERUP) && (Objects[local_objnum].type != OBJ_HOSTAGE))
	{
		return;
	}

	if (Network_send_objects && multi_objnum_is_past(local_objnum))
	{
		Network_send_objnum = -1;
	}
	if (Objects[local_objnum].type==OBJ_POWERUP)
		if (Game_mode & GM_NETWORK)
		{
			if (multi_powerup_is_4pack (Objects[local_objnum].id))
			{
				if (PowerupsInMine[Objects[local_objnum].id-1]-4<0)
					PowerupsInMine[Objects[local_objnum].id-1]=0;
				else
					PowerupsInMine[Objects[local_objnum].id-1]-=4;
			}
			else
			{
				if (PowerupsInMine[Objects[local_objnum].id]>0)
					PowerupsInMine[Objects[local_objnum].id]--;
			}
		}

	Objects[local_objnum].flags |= OF_SHOULD_BE_DEAD; // quick and painless

}

void
multi_do_quit(char *buf)
{

	if (Game_mode & GM_NETWORK)
	{
		int i, n = 0;

		digi_play_sample( SOUND_HUD_MESSAGE, F1_0 );

		HUD_init_message(HM_MULTI,  "%s %s", Players[(int)buf[1]].callsign, TXT_HAS_LEFT_THE_GAME);

		switch (multi_protocol)
		{
#ifdef USE_UDP
			case MULTI_PROTO_UDP:
				net_udp_disconnect_player(buf[1]);
				break;
#endif
			default:
				Error("Protocol handling missing in multi_do_quit\n");
				break;
		}

		for (i = 0; i < N_players; i++)
			if (Players[i].connected) n++;
		if (n == 1)
		{
			HUD_init_message(HM_MULTI, "You are the only person remaining in this netgame");
		}

		// Bounty target left - select a new one
		if( Game_mode & GM_BOUNTY && buf[1] == Bounty_target && multi_i_am_master() )
		{
			/* Select a random number */
			int new_bounty_target = d_rand() % MAX_NUM_NET_PLAYERS;
			
			/* Make sure they're valid: Don't check against kill flags,
			 * just in case everyone's dead! */
			while( !Players[new_bounty_target].connected )
				new_bounty_target = d_rand() % MAX_NUM_NET_PLAYERS;
			
			/* Select new target */
			multi_new_bounty_target( new_bounty_target );
			
			/* Send this new data */
			multi_send_bounty();
		}
	}

	return;
}

void
multi_do_cloak(char *buf)
{
	int pnum;

	pnum = (int)(buf[1]);

	Assert(pnum < N_players);

	Players[pnum].flags |= PLAYER_FLAGS_CLOAKED;
	Players[pnum].cloak_time = GameTime;
	ai_do_cloak_stuff();

	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(pnum);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_cloak(pnum);
}

void
multi_do_decloak(char *buf)
{
	int pnum;

	pnum = (int)(buf[1]);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_decloak(pnum);

}

void
multi_do_door_open(char *buf)
{
	int segnum;
	sbyte side;
	segment *seg;
	wall *w;
	ubyte flag;

	segnum = GET_INTEL_SHORT(buf + 1);
	side = buf[3];
	flag= buf[4];

	if ((segnum < 0) || (segnum > Highest_segment_index) || (side < 0) || (side > 5))
	{
		Int3();
		return;
	}

	seg = &Segments[segnum];

	if (seg->sides[side].wall_num == -1) {  //Opening door on illegal wall
		Int3();
		return;
	}

	w = &Walls[seg->sides[side].wall_num];

	if (w->type == WALL_BLASTABLE)
	{
		if (!(w->flags & WALL_BLASTED))
		{
			wall_destroy(seg, side);
		}
		return;
	}
	else if (w->state != WALL_DOOR_OPENING)
	{
		wall_open_door(seg, side);
		w->flags=flag;
	}
	else
		w->flags=flag;

}

void
multi_do_create_explosion(char *buf)
{
	int pnum;
	int count = 1;

	pnum = buf[count++];

	create_small_fireball_on_object(&Objects[Players[pnum].objnum], F1_0, 1);
}

void
multi_do_controlcen_fire(char *buf)
{
	vms_vector to_target;
	char gun_num;
	short objnum;
	int count = 1;

	memcpy(&to_target, buf+count, 12);          count += 12;
#ifdef WORDS_BIGENDIAN  // swap the vector to_target
	to_target.x = (fix)INTEL_INT((int)to_target.x);
	to_target.y = (fix)INTEL_INT((int)to_target.y);
	to_target.z = (fix)INTEL_INT((int)to_target.z);
#endif
	gun_num = buf[count];                       count += 1;
	objnum = GET_INTEL_SHORT(buf + count);      count += 2;

	struct controlcen_gun_data guns = {0};
	controlcen_calc_guns(&Objects[objnum], &guns);

	Laser_create_new_easy(&to_target, &guns.pos[(int)gun_num], objnum, CONTROLCEN_WEAPON_NUM, 1);
}

void
multi_do_create_powerup(char *buf)
{
	short segnum;
	short objnum;
	int my_objnum;
	char pnum;
	int count = 1;
	vms_vector new_pos;
	char powerup_type;

	if (Endlevel_sequence || Control_center_destroyed)
		return;

	pnum = buf[count++];
	powerup_type = buf[count++];
	segnum = GET_INTEL_SHORT(buf + count); count += 2;
	objnum = GET_INTEL_SHORT(buf + count); count += 2;

	if ((segnum < 0) || (segnum > Highest_segment_index)) {
		Int3();
		return;
	}

	memcpy(&new_pos, buf+count, sizeof(vms_vector)); count+=sizeof(vms_vector);
#ifdef WORDS_BIGENDIAN
	new_pos.x = (fix)SWAPINT((int)new_pos.x);
	new_pos.y = (fix)SWAPINT((int)new_pos.y);
	new_pos.z = (fix)SWAPINT((int)new_pos.z);
#endif

	Net_create_loc = 0;
	my_objnum = call_object_create_egg(&Objects[Players[(int)pnum].objnum], 1, OBJ_POWERUP, powerup_type);

	if (my_objnum < 0) {
		return;
	}

	if (Network_send_objects && multi_objnum_is_past(my_objnum))
	{
		Network_send_objnum = -1;
	}

	Objects[my_objnum].pos = new_pos;

	vm_vec_zero(&Objects[my_objnum].mtype.phys_info.velocity);

	obj_relink(my_objnum, segnum);

	map_objnum_local_to_remote(my_objnum, objnum, pnum);

	object_create_explosion(segnum, &new_pos, i2f(5), VCLIP_POWERUP_DISAPPEARANCE);

	if (Game_mode & GM_NETWORK)
	{
		if (multi_powerup_is_4pack((int)powerup_type))
			PowerupsInMine[(int)(powerup_type-1)]+=4;
		else
			PowerupsInMine[(int)powerup_type]++;
	}
}

void
multi_do_play_sound(char *buf)
{
	int pnum = (int)(buf[1]);
	int sound_num = (int)(buf[2]);
	fix volume = (int)(buf[3]) << 12;

	if (!Players[pnum].connected)
		return;

	Assert(Players[pnum].objnum >= 0);
	Assert(Players[pnum].objnum <= Highest_object_index);

	digi_link_sound_to_object( sound_num, Players[pnum].objnum, 0, volume);
}

void
multi_do_score(char *buf)
{
	int pnum = (int)(buf[1]);

	if ((pnum < 0) || (pnum >= N_players))
	{
		Int3(); // Non-terminal, see rob
		return;
	}

	if (Newdemo_state == ND_STATE_RECORDING) {
		int score;
		score = GET_INTEL_INT(buf + 2);
		newdemo_record_multi_score(pnum, score);
	}

	Players[pnum].score = GET_INTEL_INT(buf + 2);

	multi_sort_kill_list();
}

void
multi_do_trigger(char *buf)
{
	int pnum = (int)(buf[1]);
	int trigger = (int)(buf[2]);

	if ((pnum < 0) || (pnum >= N_players) || (pnum == Player_num))
	{
		Int3(); // Got trigger from illegal playernum
		return;
	}
	if ((trigger < 0) || (trigger >= Num_triggers))
	{
		Int3(); // Illegal trigger number in multiplayer
		return;
	}
	check_trigger_sub(trigger, pnum,0);
}

void multi_do_drop_marker (char *buf)
{
	int i;
	int pnum=(int)(buf[1]);
	int mesnum=(int)(buf[2]);
	vms_vector position;

	if (pnum==Player_num)  // my marker? don't set it down cuz it might screw up the orientation
		return;

	position.x = GET_INTEL_INT(buf + 3);
	position.y = GET_INTEL_INT(buf + 7);
	position.z = GET_INTEL_INT(buf + 11);

	for (i=0;i<40;i++)
		MarkerMessage[(pnum*2)+mesnum][i]=buf[15+i];

	MarkerPoint[(pnum*2)+mesnum]=position;

	if (MarkerObject[(pnum*2)+mesnum] !=-1 && Objects[MarkerObject[(pnum*2)+mesnum]].type!=OBJ_NONE && MarkerObject[(pnum*2)+mesnum] !=0)
		obj_delete(MarkerObject[(pnum*2)+mesnum]);

	MarkerObject[(pnum*2)+mesnum] = drop_marker_object(&position,Objects[Players[pnum].objnum].segnum,&Objects[Players[pnum].objnum].orient,(pnum*2)+mesnum);
}


void multi_do_hostage_door_status(char *buf)
{
	// Update hit point status of a door

	int count = 1;
	int wallnum;
	fix hps;

	wallnum = GET_INTEL_SHORT(buf + count);     count += 2;
	hps = GET_INTEL_INT(buf + count);           count += 4;

	if ((wallnum < 0) || (wallnum > Num_walls) || (hps < 0) || (Walls[wallnum].type != WALL_BLASTABLE))
	{
		Int3(); // Non-terminal, see Rob
		return;
	}

	if (hps < Walls[wallnum].hps)
		wall_damage(&Segments[Walls[wallnum].segnum], Walls[wallnum].sidenum, Walls[wallnum].hps - hps);
}

void
multi_reset_stuff(void)
{
	// A generic, emergency function to solve problems that crop up
	// when a player exits quick-out from the game because of a
	// connection loss.  Fixes several weird bugs!

	dead_player_end();

	Players[Player_num].homing_object_dist = -F1_0; // Turn off homing sound.

	Dead_player_camera = 0;
	Endlevel_sequence = 0;
	reset_rear_view();
}

void
multi_reset_player_object(object *objp)
{
	int i;

	//Init physics for a non-console player

	Assert(objp >= Objects);
	Assert(objp <= Objects+Highest_object_index);
	Assert((objp->type == OBJ_PLAYER) || (objp->type == OBJ_GHOST));

	vm_vec_zero(&objp->mtype.phys_info.velocity);
	vm_vec_zero(&objp->mtype.phys_info.thrust);
	vm_vec_zero(&objp->mtype.phys_info.rotvel);
	vm_vec_zero(&objp->mtype.phys_info.rotthrust);
	objp->mtype.phys_info.brakes = objp->mtype.phys_info.turnroll = 0;
	objp->mtype.phys_info.mass = Player_ship->mass;
	objp->mtype.phys_info.drag = Player_ship->drag;
	//      objp->mtype.phys_info.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST);
	objp->mtype.phys_info.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE);

	//Init render info

	objp->render_type = RT_POLYOBJ;
	objp->rtype.pobj_info.model_num = Player_ship->model_num;               //what model is this?
	objp->rtype.pobj_info.subobj_flags = 0;         //zero the flags
	for (i=0;i<MAX_SUBMODELS;i++)
		vm_angvec_zero(&objp->rtype.pobj_info.anim_angles[i]);

	//reset textures for this, if not player 0

	multi_reset_object_texture (objp);

	// Clear misc

	objp->flags = 0;

	if (objp->type == OBJ_GHOST)
		objp->render_type = RT_NONE;

}

void multi_reset_object_texture (object *objp)
{
	int id,i;

	if (Game_mode & GM_TEAM)
		id = get_team(objp->id);
	else
		id = objp->id;

	if (id == 0)
		objp->rtype.pobj_info.alt_textures=0;
	else {
		if (N_PLAYER_SHIP_TEXTURES < Polygon_models[objp->rtype.pobj_info.model_num].n_textures)
			Error("Too many player ship textures!\n");

		for (i=0;i<Polygon_models[objp->rtype.pobj_info.model_num].n_textures;i++)
			multi_player_textures[id-1][i] = ObjBitmaps[ObjBitmapPtrs[Polygon_models[objp->rtype.pobj_info.model_num].first_texture+i]];

		multi_player_textures[id-1][4] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(id-1)*2]];
		multi_player_textures[id-1][5] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(id-1)*2+1]];

		objp->rtype.pobj_info.alt_textures = id;
	}
}

void
multi_process_bigdata(char *buf, int len)
{
	// Takes a bunch of messages, check them for validity,
	// and pass them to multi_process_data.

	int type, sub_len, bytes_processed = 0;

	while( bytes_processed < len )  {
		type = buf[bytes_processed];

		if ( (type<0) || (type>MULTI_MAX_TYPE))
		{
			con_printf( CON_DEBUG,"multi_process_bigdata: Invalid packet type %d!\n", type );
			return;
		}
		sub_len = message_length[type];

		Assert(sub_len > 0);

		if ( (bytes_processed+sub_len) > len )  {
			con_printf(CON_DEBUG, "multi_process_bigdata: packet type %d too short (%d>%d)!\n", type, (bytes_processed+sub_len), len );
			Int3();
			return;
		}

		multi_process_data(&buf[bytes_processed], sub_len);
		bytes_processed += sub_len;
	}
}

//
// Part 2 : Functions that send communication messages to inform the other
//          players of something we did.
//

void multi_send_fire(int laser_gun, int laser_level, int laser_flags, int laser_fired, short laser_track)
{
	multi_do_protocol_frame(1, 0); // provoke positional update if possible

	multibuf[0] = (char)MULTI_FIRE;
	multibuf[1] = (char)Player_num;
	multibuf[2] = (char)laser_gun;
	multibuf[3] = (char)laser_level;
	multibuf[4] = (char)laser_flags;
	multibuf[5] = (char)laser_fired;
	PUT_INTEL_SHORT(multibuf+6, laser_track);

	multi_send_data(multibuf, 8, 1);
}

void
multi_send_destroy_controlcen(int objnum, int player)
{
	if (player == Player_num)
		HUD_init_message(HM_MULTI, TXT_YOU_DEST_CONTROL);
	else if ((player > 0) && (player < N_players))
		HUD_init_message(HM_MULTI, "%s %s", Players[player].callsign, TXT_HAS_DEST_CONTROL);
	else
		HUD_init_message(HM_MULTI, TXT_CONTROL_DESTROYED);

	multibuf[0] = (char)MULTI_CONTROLCEN;
	PUT_INTEL_SHORT(multibuf+1, objnum);
	multibuf[3] = player;
	multi_send_data(multibuf, 4, 2);
}

void multi_send_drop_marker (int player,vms_vector position,char messagenum,char text[])
{
	int i;

	if (player<N_players)
	{
		multibuf[0]=(char)MULTI_MARKER;
		multibuf[1]=(char)player;
		multibuf[2]=messagenum;
		PUT_INTEL_INT(multibuf+3, position.x);
		PUT_INTEL_INT(multibuf+7, position.y);
		PUT_INTEL_INT(multibuf+11, position.z);
		for (i=0;i<40;i++)
			multibuf[15+i]=text[i];
	}
	multi_send_data(multibuf, 55, 2);
}

void multi_send_markers()
{
	// send marker positions/text to new player
	int i;

	for (i = 0; i < N_players; i++)
	{
		if (MarkerObject[(i*2)]!=-1)
			multi_send_drop_marker (i,MarkerPoint[(i*2)],0,MarkerMessage[i*2]);
		if (MarkerObject[(i*2)+1]!=-1)
			multi_send_drop_marker (i,MarkerPoint[(i*2)+1],1,MarkerMessage[(i*2)+1]);
	}
}

void
multi_send_endlevel_start(int secret)
{
	multibuf[0] = (char)MULTI_ENDLEVEL_START;
	multibuf[1] = Player_num;
	multibuf[2] = (char)secret;

	if ((secret) && !multi_goto_secret)
		multi_goto_secret = 1;
	else if (!multi_goto_secret)
		multi_goto_secret = 2;

	multi_send_data(multibuf, 3, 2);
	if (Game_mode & GM_NETWORK)
	{
		Players[Player_num].connected = CONNECT_ESCAPE_TUNNEL;
		switch (multi_protocol)
		{
#ifdef USE_UDP
			case MULTI_PROTO_UDP:
				net_udp_send_endlevel_packet();
				break;
#endif
			default:
				Error("Protocol handling missing in multi_send_endlevel_start\n");
				break;
		}
	}
}

void
multi_send_player_explode(char type)
{
	int count = 0;
	int i;

	Assert( (type == MULTI_PLAYER_DROP) || (type == MULTI_PLAYER_EXPLODE) );

	if (Network_send_objects)
	{
		Network_send_objnum = -1;
	}

	multi_send_position(Players[Player_num].objnum);

	multibuf[count++] = type;
	multibuf[count++] = Player_num;

	PUT_INTEL_SHORT(multibuf+count, Players[Player_num].primary_weapon_flags);
	count += 2;
	PUT_INTEL_SHORT(multibuf+count, Players[Player_num].secondary_weapon_flags);
	count += 2;
	multibuf[count++] = (char)Players[Player_num].laser_level;

	multibuf[count++] = (char)Players[Player_num].secondary_ammo[HOMING_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[CONCUSSION_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMART_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[MEGA_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[PROXIMITY_INDEX];

	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMISSILE1_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[GUIDED_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMART_MINE_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMISSILE4_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMISSILE5_INDEX];

	PUT_INTEL_SHORT(multibuf+count, Players[Player_num].primary_ammo[VULCAN_INDEX] );
	count += 2;
	PUT_INTEL_SHORT(multibuf+count, Players[Player_num].primary_ammo[GAUSS_INDEX] );
	count += 2;
	PUT_INTEL_INT(multibuf+count, Players[Player_num].flags );
	count += 4;

	multibuf[count++] = Net_create_loc;

	Assert(Net_create_loc <= MAX_NET_CREATE_OBJECTS);

	memset(multibuf+count, -1, MAX_NET_CREATE_OBJECTS*sizeof(short));

	for (i = 0; i < Net_create_loc; i++)
	{
		if (Net_create_objnums[i] <= 0) {
			Int3(); // Illegal value in created egg object numbers
			count +=2;
			continue;
		}

		PUT_INTEL_SHORT(multibuf+count, Net_create_objnums[i]); count += 2;

		// We created these objs so our local number = the network number
		map_objnum_local_to_local((short)Net_create_objnums[i]);
	}

	Net_create_loc = 0;

	if (count > message_length[MULTI_PLAYER_EXPLODE])
	{
		Int3(); // See Rob
	}

	multi_send_data(multibuf, message_length[MULTI_PLAYER_EXPLODE], 2);
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		multi_send_decloak();
	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(Player_num);
}

extern int Proximity_dropped, Smartmines_dropped;

/*
 * Powerup capping: Keep track of how many powerups are in level and kill these which would exceed initial limit.
 * NOTE: code encapsuled by OLDPOWCAP define is original and buggy Descent2 code. 
 */

// Count the initial amount of Powerups in the level
void multi_powcap_count_powerups_in_mine(void)
{
	int i;

	for (i=0;i<MAX_POWERUP_TYPES;i++)
		PowerupsInMine[i]=0;
		
	for (i=0;i<=Highest_object_index;i++) 
	{
		if (Objects[i].type==OBJ_POWERUP)
		{
			if (multi_powerup_is_4pack(Objects[i].id))
				PowerupsInMine[Objects[i].id-1]+=4;
			else
				PowerupsInMine[Objects[i].id]++;
		}
	}
}

// We want to drop something. Kill every Powerup which exceeds the level limit
void multi_powcap_cap_objects()
{
	char type,flagtype;
	int index;

	if (!(Game_mode & GM_NETWORK))
		return;

	if (!(Game_mode & GM_HOARD))
	  	Players[Player_num].secondary_ammo[PROXIMITY_INDEX]+=Proximity_dropped;
	Players[Player_num].secondary_ammo[SMART_MINE_INDEX]+=Smartmines_dropped;
	Proximity_dropped=0;
	Smartmines_dropped=0;

	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		type=Primary_weapon_to_powerup[index];
		if (PowerupsInMine[(int)type]>=MaxPowerupsAllowed[(int)type])
			if(Players[Player_num].primary_weapon_flags & (1 << index))
			{
				con_printf(CON_VERBOSE,"PIM=%d MPA=%d\n",PowerupsInMine[(int)type],MaxPowerupsAllowed[(int)type]);
				con_printf(CON_VERBOSE,"Killing a primary cuz there's too many! (%d)\n",type);
				Players[Player_num].primary_weapon_flags&=(~(1 << index));
			}
	}


	// Don't do the adjustment stuff for Hoard mode
	if (!(Game_mode & GM_HOARD))
		Players[Player_num].secondary_ammo[2]/=4;

	Players[Player_num].secondary_ammo[7]/=4;

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		if ((Game_mode & GM_HOARD) && index==PROXIMITY_INDEX)
			continue;

		type=Secondary_weapon_to_powerup[index];

		if ((Players[Player_num].secondary_ammo[index]+PowerupsInMine[(int)type])>MaxPowerupsAllowed[(int)type])
		{
			if (MaxPowerupsAllowed[(int)type]-PowerupsInMine[(int)type]<0)
				Players[Player_num].secondary_ammo[index]=0;
			else
				Players[Player_num].secondary_ammo[index]=(MaxPowerupsAllowed[(int)type]-PowerupsInMine[(int)type]);
			con_printf(CON_VERBOSE,"Hey! I killed secondary type %d because PIM=%d MPA=%d\n",type,PowerupsInMine[(int)type],MaxPowerupsAllowed[(int)type]);
		}
	}

	if (!(Game_mode & GM_HOARD))
		Players[Player_num].secondary_ammo[2]*=4;
	Players[Player_num].secondary_ammo[7]*=4;

	if (Players[Player_num].laser_level > MAX_LASER_LEVEL)
		if (PowerupsInMine[POW_SUPER_LASER]+1 > MaxPowerupsAllowed[POW_SUPER_LASER])
			Players[Player_num].laser_level=0;

	if (Players[Player_num].flags & PLAYER_FLAGS_QUAD_LASERS)
		if (PowerupsInMine[POW_QUAD_FIRE]+1 > MaxPowerupsAllowed[POW_QUAD_FIRE])
			Players[Player_num].flags&=(~PLAYER_FLAGS_QUAD_LASERS);

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		if (PowerupsInMine[POW_CLOAK]+1 > MaxPowerupsAllowed[POW_CLOAK])
			Players[Player_num].flags&=(~PLAYER_FLAGS_CLOAKED);

	if (Players[Player_num].flags & PLAYER_FLAGS_MAP_ALL)
		if (PowerupsInMine[POW_FULL_MAP]+1 > MaxPowerupsAllowed[POW_FULL_MAP])
			Players[Player_num].flags&=(~PLAYER_FLAGS_MAP_ALL);

	if (Players[Player_num].flags & PLAYER_FLAGS_AFTERBURNER)
		if (PowerupsInMine[POW_AFTERBURNER]+1 > MaxPowerupsAllowed[POW_AFTERBURNER])
			Players[Player_num].flags&=(~PLAYER_FLAGS_AFTERBURNER);

	if (Players[Player_num].flags & PLAYER_FLAGS_AMMO_RACK)
		if (PowerupsInMine[POW_AMMO_RACK]+1 > MaxPowerupsAllowed[POW_AMMO_RACK])
			Players[Player_num].flags&=(~PLAYER_FLAGS_AMMO_RACK);

	if (Players[Player_num].flags & PLAYER_FLAGS_CONVERTER)
		if (PowerupsInMine[POW_CONVERTER]+1 > MaxPowerupsAllowed[POW_CONVERTER])
			Players[Player_num].flags&=(~PLAYER_FLAGS_CONVERTER);

	if (Players[Player_num].flags & PLAYER_FLAGS_HEADLIGHT)
		if (PowerupsInMine[POW_HEADLIGHT]+1 > MaxPowerupsAllowed[POW_HEADLIGHT])
			Players[Player_num].flags&=(~PLAYER_FLAGS_HEADLIGHT);

	if (Game_mode & GM_CAPTURE)
	{
		if (Players[Player_num].flags & PLAYER_FLAGS_FLAG)
		{
			if (get_team(Player_num)==TEAM_RED)
				flagtype=POW_FLAG_BLUE;
			else
				flagtype=POW_FLAG_RED;

			if (PowerupsInMine[(int)flagtype]+1 > MaxPowerupsAllowed[(int)flagtype])
				Players[Player_num].flags&=(~PLAYER_FLAGS_FLAG);
		}
	}

}

// Adds players inventory to multi cap
void multi_powcap_adjust_cap_for_player(int pnum)
{
	char type;

	int index;

	if (!(Game_mode & GM_NETWORK))
		return;

	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		type=Primary_weapon_to_powerup[index];
		if (Players[pnum].primary_weapon_flags & (1 << index))
		    MaxPowerupsAllowed[(int)type]++;
	}

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		type=Secondary_weapon_to_powerup[index];
		MaxPowerupsAllowed[(int)type]+=Players[pnum].secondary_ammo[index];
	}

	if (Players[pnum].laser_level > MAX_LASER_LEVEL)
		MaxPowerupsAllowed[POW_SUPER_LASER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_QUAD_LASERS)
		MaxPowerupsAllowed[POW_QUAD_FIRE]++;

	if (Players[pnum].flags & PLAYER_FLAGS_CLOAKED)
		MaxPowerupsAllowed[POW_CLOAK]++;

	if (Players[pnum].flags & PLAYER_FLAGS_MAP_ALL)
		MaxPowerupsAllowed[POW_FULL_MAP]++;

	if (Players[pnum].flags & PLAYER_FLAGS_AFTERBURNER)
		MaxPowerupsAllowed[POW_AFTERBURNER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_AMMO_RACK)
		MaxPowerupsAllowed[POW_AMMO_RACK]++;

	if (Players[pnum].flags & PLAYER_FLAGS_CONVERTER)
		MaxPowerupsAllowed[POW_CONVERTER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_HEADLIGHT)
		MaxPowerupsAllowed[POW_HEADLIGHT]++;
}

void multi_powcap_adjust_remote_cap(int pnum)
{
	char type;

	int index;

	if (!(Game_mode & GM_NETWORK))
		return;

	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		type=Primary_weapon_to_powerup[index];
		if (Players[pnum].primary_weapon_flags & (1 << index))
		    PowerupsInMine[(int)type]++;
	}

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		type=Secondary_weapon_to_powerup[index];

		if ((Game_mode & GM_HOARD) && index==2)
			continue;

		if (index==2 || index==7) // PROX or SMARTMINES? Those bastards...
			PowerupsInMine[(int)type]+=(Players[pnum].secondary_ammo[index]/4);
		else
			PowerupsInMine[(int)type]+=Players[pnum].secondary_ammo[index];

	}

	if (Players[pnum].laser_level > MAX_LASER_LEVEL)
		PowerupsInMine[POW_SUPER_LASER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_QUAD_LASERS)
		PowerupsInMine[POW_QUAD_FIRE]++;

	if (Players[pnum].flags & PLAYER_FLAGS_CLOAKED)
		PowerupsInMine[POW_CLOAK]++;

	if (Players[pnum].flags & PLAYER_FLAGS_MAP_ALL)
		PowerupsInMine[POW_FULL_MAP]++;

	if (Players[pnum].flags & PLAYER_FLAGS_AFTERBURNER)
		PowerupsInMine[POW_AFTERBURNER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_AMMO_RACK)
		PowerupsInMine[POW_AMMO_RACK]++;

	if (Players[pnum].flags & PLAYER_FLAGS_CONVERTER)
		PowerupsInMine[POW_CONVERTER]++;

	if (Players[pnum].flags & PLAYER_FLAGS_HEADLIGHT)
		PowerupsInMine[POW_HEADLIGHT]++;

}

void
multi_send_message(void)
{
	int loc = 0;
	if (Network_message_reciever != -1)
	{
		multibuf[loc] = (char)MULTI_MESSAGE;            loc += 1;
		multibuf[loc] = (char)Player_num;                       loc += 1;
		strncpy(multibuf+loc, Network_message, MAX_MESSAGE_LEN); loc += MAX_MESSAGE_LEN;
		multibuf[loc-1] = '\0';
		multi_send_data(multibuf, loc, 0);
		Network_message_reciever = -1;
	}
}

void
multi_send_reappear()
{
	multi_send_position(Players[Player_num].objnum);
	
	multibuf[0] = (char)MULTI_REAPPEAR;
	PUT_INTEL_SHORT(multibuf+1, Players[Player_num].objnum);

	multi_send_data(multibuf, 3, 2);
	PKilledFlags[Player_num]=0;
}

void
multi_send_position(int objnum)
{
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif
	int count=0;

	multibuf[count++] = (char)MULTI_POSITION;
	multibuf[count++] = (char)Player_num;
#ifndef WORDS_BIGENDIAN
	create_shortpos((shortpos *)(multibuf+count), Objects+objnum,0);
	count += sizeof(shortpos);
#else
	create_shortpos(&sp, Objects+objnum, 1);
	memcpy(&(multibuf[count]), (ubyte *)(sp.bytemat), 9);
	count += 9;
	memcpy(&(multibuf[count]), (ubyte *)&(sp.xo), 14);
	count += 14;
#endif
	// send twice while first has priority so the next one will be attached to the next bigdata packet
	multi_send_data(multibuf, count, 2);
	multi_send_data(multibuf, count, 0);
}

void
multi_send_kill(int objnum)
{
	// I died, tell the world.

	int killer_objnum;
	int count = 0;

	Assert(Objects[objnum].id == Player_num);
	killer_objnum = Players[Player_num].killer_objnum;

	multi_compute_kill(killer_objnum, objnum);

	multibuf[0] = (char)MULTI_KILL;     count += 1;
	multibuf[1] = Player_num;           count += 1;
	if (killer_objnum > -1) {
		short s;		// do it with variable since INTEL_SHORT won't work on return val from function.

		s = (short)objnum_local_to_remote(killer_objnum, (sbyte *)&multibuf[count+2]);
		PUT_INTEL_SHORT(multibuf+count, s);
	}
	else
	{
		PUT_INTEL_SHORT(multibuf+count, -1);
		multibuf[count+2] = (char)-1;
	}
	count += 3;
	// I am host - I know what's going on so attach game_mode related info which might be vital for correct kill computation
	if (multi_i_am_master())
	{
		multibuf[count] = Netgame.team_vector;	count += 1;
		multibuf[count] = Bounty_target;	count += 1;
	}

	if (multi_i_am_master())
	{
		multi_compute_kill(killer_objnum, objnum);
		multi_send_data(multibuf, count, 2);
	}
	else
		multi_send_data_direct((ubyte*)multibuf, count, multi_who_is_master(), 2); // I am just a client so I'll only send my kill but not compute it, yet. I'll get response from host so I can compute it correctly

	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(Player_num);
}

void
multi_send_remobj(int objnum)
{
	// Tell the other guy to remove an object from his list

	sbyte obj_owner;
	short remote_objnum;

	if (Objects[objnum].type==OBJ_POWERUP && (Game_mode & GM_NETWORK))
	{
		if (multi_powerup_is_4pack (Objects[objnum].id))
		{
			if (PowerupsInMine[Objects[objnum].id-1]-4<0)
				PowerupsInMine[Objects[objnum].id-1]=0;
			else
				PowerupsInMine[Objects[objnum].id-1]-=4;
		}
		else
		{
			if (PowerupsInMine[Objects[objnum].id]>0)
				PowerupsInMine[Objects[objnum].id]--;
		}
	}

	multibuf[0] = (char)MULTI_REMOVE_OBJECT;

	remote_objnum = objnum_local_to_remote((short)objnum, &obj_owner);

	PUT_INTEL_SHORT(multibuf+1, remote_objnum); // Map to network objnums

	multibuf[3] = obj_owner;

	multi_send_data(multibuf, 4, 2);

	if (Network_send_objects && multi_objnum_is_past(objnum))
	{
		Network_send_objnum = -1;
	}
}

void
multi_send_quit(int why)
{
	// I am quitting the game, tell the other guy the bad news.

	Assert (why == MULTI_QUIT);

	multibuf[0] = (char)why;
	multibuf[1] = Player_num;
	multi_send_data(multibuf, 2, 2);

}

void
multi_send_cloak(void)
{
	// Broadcast a change in our pflags (made to support cloaking)

	multibuf[0] = MULTI_CLOAK;
	multibuf[1] = (char)Player_num;

	multi_send_data(multibuf, 2, 2);

	if (Game_mode & GM_MULTI_ROBOTS)
		multi_strip_robots(Player_num);
}

void
multi_send_decloak(void)
{
	// Broadcast a change in our pflags (made to support cloaking)

	multibuf[0] = MULTI_DECLOAK;
	multibuf[1] = (char)Player_num;

	multi_send_data(multibuf, 2, 2);
}

void
multi_send_door_open(int segnum, int side,ubyte flag)
{
	// When we open a door make sure everyone else opens that door

	multibuf[0] = MULTI_DOOR_OPEN;
	PUT_INTEL_SHORT(multibuf+1, segnum );
	multibuf[3] = (sbyte)side;
	multibuf[4] = flag;

	multi_send_data(multibuf, 5, 2);
}

extern void net_ipx_send_naked_packet (char *,short,int);

void multi_send_door_open_specific(int pnum,int segnum, int side,ubyte flag)
{
	// For sending doors only to a specific person (usually when they're joining)

	Assert (Game_mode & GM_NETWORK);
	//   Assert (pnum>-1 && pnum<N_players);

	multibuf[0] = MULTI_DOOR_OPEN;
	PUT_INTEL_SHORT(multibuf+1, segnum);
	multibuf[3] = (sbyte)side;
	multibuf[4] = flag;

	multi_send_data_direct((ubyte *)multibuf, 5, pnum, 2);
}

//
// Part 3 : Functions that change or prepare the game for multiplayer use.
//          Not including functions needed to syncronize or start the
//          particular type of multiplayer game.  Includes preparing the
//                      mines, player structures, etc.

void
multi_send_create_explosion(int pnum)
{
	// Send all data needed to create a remote explosion

	int count = 0;

	multibuf[count] = MULTI_CREATE_EXPLOSION;       count += 1;
	multibuf[count] = (sbyte)pnum;                  count += 1;
	//                                                                                                      -----------
	//                                                                                                      Total size = 2

	multi_send_data(multibuf, count, 0);
}

void
multi_send_controlcen_fire(vms_vector *to_goal, int best_gun_num, int objnum)
{
#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif
	int count = 0;

	multibuf[count] = MULTI_CONTROLCEN_FIRE;                count +=  1;
#ifndef WORDS_BIGENDIAN
	memcpy(multibuf+count, to_goal, 12);                    count += 12;
#else
	swapped_vec.x = (fix)INTEL_INT( (int)to_goal->x );
	swapped_vec.y = (fix)INTEL_INT( (int)to_goal->y );
	swapped_vec.z = (fix)INTEL_INT( (int)to_goal->z );
	memcpy(multibuf+count, &swapped_vec, 12);				count += 12;
#endif
	multibuf[count] = (char)best_gun_num;                   count +=  1;
	PUT_INTEL_SHORT(multibuf+count, objnum );     count +=  2;
	//                                                                                                                      ------------
	//                                                                                                                      Total  = 16
	multi_send_data(multibuf, count, 0);
}

void
multi_send_create_powerup(int powerup_type, int segnum, int objnum, vms_vector *pos)
{
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

#ifdef WORDS_BIGENDIAN
	vms_vector swapped_vec;
#endif
	int count = 0;

	multi_send_position(Players[Player_num].objnum);

	if (Game_mode & GM_NETWORK)
	{
		if (multi_powerup_is_4pack(powerup_type))
			PowerupsInMine[powerup_type-1]+=4;
		else
			PowerupsInMine[powerup_type]++;
	}

	multibuf[count] = MULTI_CREATE_POWERUP;         count += 1;
	multibuf[count] = Player_num;                                      count += 1;
	multibuf[count] = powerup_type;                                 count += 1;
	PUT_INTEL_SHORT(multibuf+count, segnum );     count += 2;
	PUT_INTEL_SHORT(multibuf+count, objnum );     count += 2;
#ifndef WORDS_BIGENDIAN
	memcpy(multibuf+count, pos, sizeof(vms_vector));  count += sizeof(vms_vector);
#else
	swapped_vec.x = (fix)INTEL_INT( (int)pos->x );
	swapped_vec.y = (fix)INTEL_INT( (int)pos->y );
	swapped_vec.z = (fix)INTEL_INT( (int)pos->z );
	memcpy(multibuf+count, &swapped_vec, 12);				count += 12;
#endif
	//                                                                                                            -----------
	//                                                                                                            Total =  19
	multi_send_data(multibuf, count, 2);

	if (Network_send_objects && multi_objnum_is_past(objnum))
	{
		Network_send_objnum = -1;
	}

	map_objnum_local_to_local(objnum);
}

void
multi_send_play_sound(int sound_num, fix volume)
{
	int count = 0;
	multibuf[count] = MULTI_PLAY_SOUND;                     count += 1;
	multibuf[count] = Player_num;                                   count += 1;
	multibuf[count] = (char)sound_num;                      count += 1;
	multibuf[count] = (char)(volume >> 12); count += 1;
	//                                                                                                         -----------
	//                                                                                                         Total = 4
	multi_send_data(multibuf, count, 0);
}

void
multi_send_audio_taunt(int taunt_num)
{
	return; // Taken out, awaiting sounds..

#if 0
	int audio_taunts[4] = {
		SOUND_CONTROL_CENTER_WARNING_SIREN,
		SOUND_HOSTAGE_RESCUED,
		SOUND_REFUEL_STATION_GIVING_FUEL,
		SOUND_BAD_SELECTION
	};


	Assert(taunt_num >= 0);
	Assert(taunt_num < 4);

	digi_play_sample( audio_taunts[taunt_num], F1_0 );
	multi_send_play_sound(audio_taunts[taunt_num], F1_0);
#endif
}

void
multi_send_score(void)
{
	// Send my current score to all other players so it will remain
	// synced.
	int count = 0;

	if (Game_mode & GM_MULTI_COOP) {
		multi_sort_kill_list();
		multibuf[count] = MULTI_SCORE;                  count += 1;
		multibuf[count] = Player_num;                           count += 1;
		PUT_INTEL_INT(multibuf+count, Players[Player_num].score);  count += 4;
		multi_send_data(multibuf, count, 0);
	}
}

void
multi_send_trigger(int triggernum)
{
	// Send an even to trigger something in the mine

	int count = 0;

	multibuf[count] = MULTI_TRIGGER;                                count += 1;
	multibuf[count] = Player_num;                                   count += 1;
	multibuf[count] = (ubyte)triggernum;            count += 1;

	multi_send_data(multibuf, count, 2);
}

void
multi_send_hostage_door_status(int wallnum)
{
	// Tell the other player what the hit point status of a hostage door
	// should be

	int count = 0;

	Assert(Walls[wallnum].type == WALL_BLASTABLE);

	multibuf[count] = MULTI_HOSTAGE_DOOR;           count += 1;
	PUT_INTEL_SHORT(multibuf+count, wallnum );           count += 2;
	PUT_INTEL_INT(multibuf+count, Walls[wallnum].hps );  count += 4;

	multi_send_data(multibuf, count, 0);
}

extern int Drop_afterburner_blob_flag;
int PhallicLimit=0;
int PhallicMan=-1;

void multi_consistency_error(int reset)
{
	static int count = 0;

	if (reset)
		count = 0;

	if (++count < 10)
		return;

	nm_messagebox(NULL, TXT_CONSISTENCY_ERROR, TXT_OK);
	count = 0;
	multi_quit_game = 1;
	game_leave_menus();
	multi_reset_stuff();
}

void multi_prep_level(void)
{
	// Do any special stuff to the level required for games
	// before we begin playing in it.

	// Player_num MUST be set before calling this procedure.

	// This function must be called before checksuming the Object array,
	// since the resulting checksum with depend on the value of Player_num
	// at the time this is called.

	int i;
	int     cloak_count, inv_count;

	Assert(Game_mode & GM_MULTI);

	Assert(NumNetPlayerPositions > 0);

	PhallicLimit=0;
	PhallicMan=-1;
	Drop_afterburner_blob_flag=0;
	Bounty_target = 0;
	multi_consistency_error(1);

	for (i=0;i<MAX_NUM_NET_PLAYERS;i++)
		PKilledFlags[i]=0;

	for (i = 0; i < NumNetPlayerPositions; i++)
	{
		if (i != Player_num)
			Objects[Players[i].objnum].control_type = CT_REMOTE;
		Objects[Players[i].objnum].movement_type = MT_PHYSICS;
		multi_reset_player_object(&Objects[Players[i].objnum]);
		Netgame.players[i].LastPacketTime = 0;
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		robot_controlled[i] = -1;
		robot_agitation[i] = 0;
		robot_fired[i] = 0;
	}

	Viewer = ConsoleObject = &Objects[Players[Player_num].objnum];

	if (!(Game_mode & GM_MULTI_COOP))
	{
		multi_delete_extra_objects(); // Removes monsters from level
	}

	if (Game_mode & GM_MULTI_ROBOTS)
	{
		multi_set_robot_ai(); // Set all Robot AI to types we can cope with
	}

	if (Game_mode & GM_NETWORK)
	{
		multi_powcap_adjust_cap_for_player(Player_num);
		multi_send_powcap_update();
	}

	inv_count = 0;
	cloak_count = 0;
	for (i=0; i<=Highest_object_index; i++)
	{
		int objnum;

		if ((Objects[i].type == OBJ_HOSTAGE) && !(Game_mode & GM_MULTI_COOP))
		{
			objnum = obj_create(OBJ_POWERUP, POW_SHIELD_BOOST, Objects[i].segnum, &Objects[i].pos, &vmd_identity_matrix, Powerup_info[POW_SHIELD_BOOST].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);
			obj_delete(i);
			if (objnum != -1)
			{
				Objects[objnum].rtype.vclip_info.vclip_num = Powerup_info[POW_SHIELD_BOOST].vclip_num;
				Objects[objnum].rtype.vclip_info.frametime = Vclip[Objects[objnum].rtype.vclip_info.vclip_num].frame_time;
				Objects[objnum].rtype.vclip_info.framenum = 0;
				Objects[objnum].mtype.phys_info.drag = 512;     //1024;
				Objects[objnum].mtype.phys_info.mass = F1_0;
				vm_vec_zero(&Objects[objnum].mtype.phys_info.velocity);
			}
			continue;
		}

		if (Objects[i].type == OBJ_POWERUP)
		{
			if (Objects[i].id == POW_EXTRA_LIFE)
			{
				if (!(Netgame.AllowedItems & NETFLAG_DOINVUL))
				{
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				}
				else
				{
					Objects[i].id = POW_INVULNERABILITY;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				}

			}

			if (!(Game_mode & GM_MULTI_COOP))
				if ((Objects[i].id >= POW_KEY_BLUE) && (Objects[i].id <= POW_KEY_GOLD))
				{
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				}

			if (Objects[i].id == POW_INVULNERABILITY) {
				if (inv_count >= 3 || (!(Netgame.AllowedItems & NETFLAG_DOINVUL))) {
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				} else
					inv_count++;
			}

			if (Objects[i].id == POW_CLOAK) {
				if (cloak_count >= 3 || (!(Netgame.AllowedItems & NETFLAG_DOCLOAK))) {
					Objects[i].id = POW_SHIELD_BOOST;
					Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
					Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
				} else
					cloak_count++;
			}

			if (Objects[i].id == POW_AFTERBURNER && !(Netgame.AllowedItems & NETFLAG_DOAFTERBURNER))
				bash_to_shield (i,"afterburner");
			if (Objects[i].id == POW_FUSION_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOFUSION))
				bash_to_shield (i,"fusion");
			if (Objects[i].id == POW_PHOENIX_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOPHOENIX))
				bash_to_shield (i,"phoenix");

			if (Objects[i].id == POW_HELIX_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOHELIX))
				bash_to_shield (i,"helix");

			if (Objects[i].id == POW_MEGA_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOMEGA))
				bash_to_shield (i,"mega");

			if (Objects[i].id == POW_SMARTBOMB_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOSMART))
				bash_to_shield (i,"smartmissile");

			if (Objects[i].id == POW_GAUSS_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOGAUSS))
				bash_to_shield (i,"gauss");

			if (Objects[i].id == POW_VULCAN_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOVULCAN))
				bash_to_shield (i,"vulcan");

			if (Objects[i].id == POW_PLASMA_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOPLASMA))
				bash_to_shield (i,"plasma");

			if (Objects[i].id == POW_OMEGA_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOOMEGA))
				bash_to_shield (i,"omega");

			if (Objects[i].id == POW_SUPER_LASER && !(Netgame.AllowedItems & NETFLAG_DOSUPERLASER))
				bash_to_shield (i,"superlaser");

			if (Objects[i].id == POW_PROXIMITY_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOPROXIM))
				bash_to_shield (i,"proximity");

			// Special: Make all proximity bombs into shields if in
			// hoard mode because we use the proximity slot in the
			// player struct to signify how many orbs the player has.

			if (Objects[i].id == POW_PROXIMITY_WEAPON && (Game_mode & GM_HOARD))
				bash_to_shield (i,"proximity");

			if (Objects[i].id==POW_VULCAN_AMMO && (!(Netgame.AllowedItems & NETFLAG_DOVULCAN) && !(Netgame.AllowedItems & NETFLAG_DOGAUSS)))
				bash_to_shield(i,"vulcan ammo");

			if (Objects[i].id == POW_SPREADFIRE_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOSPREAD))
				bash_to_shield (i,"spread");
			if (Objects[i].id == POW_SMART_MINE && !(Netgame.AllowedItems & NETFLAG_DOSMARTMINE))
				bash_to_shield (i,"smartmine");
			if (Objects[i].id == POW_SMISSILE1_1 && !(Netgame.AllowedItems & NETFLAG_DOFLASH))
				bash_to_shield (i,"flash");
			if (Objects[i].id == POW_SMISSILE1_4 && !(Netgame.AllowedItems & NETFLAG_DOFLASH))
				bash_to_shield (i,"flash");
			if (Objects[i].id == POW_GUIDED_MISSILE_1 && !(Netgame.AllowedItems & NETFLAG_DOGUIDED))
				bash_to_shield (i,"guided");
			if (Objects[i].id == POW_GUIDED_MISSILE_4 && !(Netgame.AllowedItems & NETFLAG_DOGUIDED))
				bash_to_shield (i,"guided");
			if (Objects[i].id == POW_EARTHSHAKER_MISSILE && !(Netgame.AllowedItems & NETFLAG_DOSHAKER))
				bash_to_shield (i,"earth");
			if (Objects[i].id == POW_MERCURY_MISSILE_1 && !(Netgame.AllowedItems & NETFLAG_DOMERCURY))
				bash_to_shield (i,"Mercury");
			if (Objects[i].id == POW_MERCURY_MISSILE_4 && !(Netgame.AllowedItems & NETFLAG_DOMERCURY))
				bash_to_shield (i,"Mercury");
			if (Objects[i].id == POW_CONVERTER && !(Netgame.AllowedItems & NETFLAG_DOCONVERTER))
				bash_to_shield (i,"Converter");
			if (Objects[i].id == POW_AMMO_RACK && !(Netgame.AllowedItems & NETFLAG_DOAMMORACK))
				bash_to_shield (i,"Ammo rack");
			if (Objects[i].id == POW_HEADLIGHT && !(Netgame.AllowedItems & NETFLAG_DOHEADLIGHT))
				bash_to_shield (i,"Headlight");
			if (Objects[i].id == POW_LASER && !(Netgame.AllowedItems & NETFLAG_DOLASER))
				bash_to_shield (i,"Laser powerup");
			if (Objects[i].id == POW_HOMING_AMMO_1 && !(Netgame.AllowedItems & NETFLAG_DOHOMING))
				bash_to_shield (i,"Homing");
			if (Objects[i].id == POW_HOMING_AMMO_4 && !(Netgame.AllowedItems & NETFLAG_DOHOMING))
				bash_to_shield (i,"Homing");
			if (Objects[i].id == POW_QUAD_FIRE && !(Netgame.AllowedItems & NETFLAG_DOQUAD))
				bash_to_shield (i,"Quad Lasers");
			if (Objects[i].id == POW_FLAG_BLUE && !(Game_mode & GM_CAPTURE))
				bash_to_shield (i,"Blue flag");
			if (Objects[i].id == POW_FLAG_RED && !(Game_mode & GM_CAPTURE))
				bash_to_shield (i,"Red flag");
		}
	}

	if (Game_mode & GM_HOARD)
		init_hoard_data();

	if ((Game_mode & GM_CAPTURE) || (Game_mode & GM_HOARD))
		multi_apply_goal_textures();

	multi_sort_kill_list();

	multi_show_player_list();

	ConsoleObject->control_type = CT_FLYING;

	reset_player_object();

}

int multi_level_sync(void)
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			return net_udp_level_sync();
			break;
#endif
		default:
			Error("Protocol handling missing in multi_level_sync\n");
			break;
	}
}

int Goal_blue_segnum,Goal_red_segnum;

void multi_apply_goal_textures()
{
	int		i,j,tex;
	segment	*seg;

	for (i=0; i <= Highest_segment_index; i++)
	{
		seg = &Segments[i];

		if (seg->special==SEGMENT_IS_GOAL_BLUE)
		{

			Goal_blue_segnum = i;

			if (Game_mode & GM_HOARD)
				tex=find_goal_texture (TMI_GOAL_HOARD);
			else
				tex=find_goal_texture (TMI_GOAL_BLUE);

			if (tex>-1)
				for (j = 0; j < 6; j++) {
					int v;
					seg->sides[j].tmap_num=tex;
					for (v=0;v<4;v++)
						seg->sides[j].uvls[v].l = i2f(100);		//max out
				}

			seg->static_light = i2f(100);	//make static light bright

		}

		if (seg->special==SEGMENT_IS_GOAL_RED)
		{
			Goal_red_segnum = i;

			// Make both textures the same if Hoard mode

			if (Game_mode & GM_HOARD)
				tex=find_goal_texture (TMI_GOAL_HOARD);
			else
				tex=find_goal_texture (TMI_GOAL_RED);

			if (tex>-1)
				for (j = 0; j < 6; j++) {
					int v;
					seg->sides[j].tmap_num=tex;
					for (v=0;v<4;v++)
						seg->sides[j].uvls[v].l = i2f(1000);		//max out
				}

			seg->static_light = i2f(100);	//make static light bright
		}
	}
}
int find_goal_texture (ubyte t)
{
	int i;

	for (i=0;i<NumTextures;i++)
		if (TmapInfo[i].flags & t)
			return i;

	Int3(); // Hey, there is no goal texture for this PIG!!!!
	// Edit bitmaps.tbl and designate two textures to be RED and BLUE
	// goal textures
	return (-1);
}


void multi_set_robot_ai(void)
{
	// Go through the objects array looking for robots and setting
	// them to certain supported types of NET AI behavior.

	//      int i;
	//
	//      for (i = 0; i <= Highest_object_index; i++)
	//      {
	//              if (Objects[i].type == OBJ_ROBOT) {
	//                      Objects[i].ai_info.REMOTE_OWNER = -1;
	//                      if (Objects[i].ai_info.behavior == AIB_STATION)
	//                              Objects[i].ai_info.behavior = AIB_NORMAL;
	//              }
	//      }
}

int multi_delete_extra_objects()
{
	int i;
	int nnp=0;
	object *objp;

	// Go through the object list and remove any objects not used in
	// 'Anarchy!' games.

	// This function also prints the total number of available multiplayer
	// positions in this level, even though this should always be 8 or more!

	objp = Objects;
	for (i=0;i<=Highest_object_index;i++) {
		if ((objp->type==OBJ_PLAYER) || (objp->type==OBJ_GHOST))
			nnp++;
		else if ((objp->type==OBJ_ROBOT) && (Game_mode & GM_MULTI_ROBOTS))
			;
		else if ( (objp->type!=OBJ_NONE) && (objp->type!=OBJ_PLAYER) && (objp->type!=OBJ_POWERUP) && (objp->type!=OBJ_CNTRLCEN) && (objp->type!=OBJ_HOSTAGE) && !(objp->type==OBJ_WEAPON && objp->id==PMINE_ID) ) {
			// Before deleting object, if it's a robot, drop it's special powerup, if any
			if (objp->type == OBJ_ROBOT)
				if (objp->contains_count && (objp->contains_type == OBJ_POWERUP))
					object_create_egg(objp);
			obj_delete(i);
		}
		objp++;
	}

	return nnp;
}

// Returns 1 if player is Master/Host of this game
int multi_i_am_master(void)
{
	return (Player_num == 0);
}

// Returns the Player_num of Master/Host of this game
int multi_who_is_master(void)
{
	return 0;
}

void change_playernum_to( int new_Player_num )
{
// 	if (Player_num > -1)
// 		memcpy( Players[new_Player_num].callsign, Players[Player_num].callsign, CALLSIGN_LEN+1 );
	if (Player_num > -1)
	{
		char *buf;
		MALLOC(buf,char,CALLSIGN_LEN+1);
		memcpy( buf, Players[Player_num].callsign, CALLSIGN_LEN+1 );
		strcpy(Players[new_Player_num].callsign,buf);
		d_free(buf);
	}

	Player_num = new_Player_num;
}

int multi_all_players_alive()
{
	int i;
	for (i=0;i<N_players;i++)
	{
		if (PKilledFlags[i] && Players[i].connected)
			return (0);
	}
	return (1);
}

void multi_send_drop_weapon (int objnum,int seed)
{
	object *objp;
	int count=0;
	int ammo_count;

	multi_send_position(Players[Player_num].objnum);

	objp = &Objects[objnum];

	ammo_count = objp->ctype.powerup_info.count;

	if (objp->id == POW_OMEGA_WEAPON && ammo_count == F1_0)
		ammo_count = F1_0 - 1; //make fit in short

	Assert(ammo_count < F1_0); //make sure fits in short

	multibuf[count++]=(char)MULTI_DROP_WEAPON;
	multibuf[count++]=(char)objp->id;

	PUT_INTEL_SHORT(multibuf+count, Player_num); count += 2;
	PUT_INTEL_SHORT(multibuf+count, objnum); count += 2;
	PUT_INTEL_SHORT(multibuf+count, ammo_count); count += 2;
	PUT_INTEL_INT(multibuf+count, seed);

	map_objnum_local_to_local(objnum);

	if (Game_mode & GM_NETWORK)
	{
		if (multi_powerup_is_4pack(objp->id))
			PowerupsInMine[objp->id-1]+=4;
		else
			PowerupsInMine[objp->id]++;
	}

	multi_send_data(multibuf, 12, 2);
}

void multi_do_drop_weapon (char *buf)
{
	int pnum,ammo,objnum,remote_objnum,seed;
	object *objp;
	int powerup_id;

	powerup_id=(int)(buf[1]);
	pnum = GET_INTEL_SHORT(buf + 2);
	remote_objnum = GET_INTEL_SHORT(buf + 4);
	ammo = GET_INTEL_SHORT(buf + 6);
	seed = GET_INTEL_INT(buf + 8);

	objp = &Objects[Players[pnum].objnum];

	objnum = spit_powerup(objp, powerup_id, seed);

	map_objnum_local_to_remote(objnum, remote_objnum, pnum);

	if (objnum!=-1)
		Objects[objnum].ctype.powerup_info.count = ammo;

	if (Game_mode & GM_NETWORK)
	{
		if (multi_powerup_is_4pack(powerup_id))
			PowerupsInMine[powerup_id-1]+=4;
		else
			PowerupsInMine[powerup_id]++;
	}

}

void multi_send_guided_info (object *miss,char done)
{
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif
	int count=0;

	multibuf[count++]=(char)MULTI_GUIDED;
	multibuf[count++]=(char)Player_num;
	multibuf[count++]=done;

#ifndef WORDS_BIGENDIAN
	create_shortpos((shortpos *)(multibuf+count), miss,0);
	count+=sizeof(shortpos);
#else
	create_shortpos(&sp, miss, 1);
	memcpy(&(multibuf[count]), (ubyte *)(sp.bytemat), 9);
	count += 9;
	memcpy(&(multibuf[count]), (ubyte *)&(sp.xo), 14);
	count += 14;
#endif

	multi_send_data(multibuf, count, 0);
}

void multi_do_guided (char *buf)
{
	char pnum=buf[1];
	int count=3;
	static int fun=200;
#ifdef WORDS_BIGENDIAN
	shortpos sp;
#endif

	if (Guided_missile[(int)pnum]==NULL)
	{
		if (++fun>=50)
		{
			fun=0;
		}
		return;
	}
	else if (++fun>=50)
	{
		fun=0;
	}

	if (buf[2])
	{
		release_guided_missile(pnum);
		return;
	}


	if (Guided_missile[(int)pnum]-Objects<0 || Guided_missile[(int)pnum]-Objects > Highest_object_index)
	{
		Int3();  // Get Jason immediately!
		return;
	}

#ifndef WORDS_BIGENDIAN
	extract_shortpos(Guided_missile[(int)pnum], (shortpos *)(buf+count),0);
#else
	memcpy((ubyte *)(sp.bytemat), (ubyte *)(buf + count), 9);
	memcpy((ubyte *)&(sp.xo), (ubyte *)(buf + count + 9), 14);
	extract_shortpos(Guided_missile[(int)pnum], &sp, 1);
#endif

	count+=sizeof (shortpos);

	update_object_seg(Guided_missile[(int)pnum]);
}

void multi_send_stolen_items ()
{
	int i,count=1;
	multibuf[0]=MULTI_STOLEN_ITEMS;

	for (i=0;i<MAX_STOLEN_ITEMS;i++)
	{
		multibuf[i+1]=Stolen_items[i];
		count++;      // So I like to break my stuff into smaller chunks, so what?
	}
	multi_send_data(multibuf, count, 2);
}

void multi_do_stolen_items (char *buf)
{
	int i;

	for (i=0;i<MAX_STOLEN_ITEMS;i++)
	{
		Stolen_items[i]=buf[i+1];
	}
}

void multi_send_wall_status_specific (int pnum,int wallnum,ubyte type,ubyte flags,ubyte state)
{
	// Send wall states a specific rejoining player

	int count=0;

	Assert (Game_mode & GM_NETWORK);
	//Assert (pnum>-1 && pnum<N_players);

	multibuf[count]=MULTI_WALL_STATUS;        count++;
	PUT_INTEL_SHORT(multibuf+count, wallnum);  count+=2;
	multibuf[count]=type;                 count++;
	multibuf[count]=flags;                count++;
	multibuf[count]=state;                count++;

	multi_send_data_direct((ubyte *)multibuf, count, pnum, 2);
}

void multi_do_wall_status (char *buf)
{
	short wallnum;
	ubyte flag,type,state;

	wallnum = GET_INTEL_SHORT(buf + 1);
	type=buf[3];
	flag=buf[4];
	state=buf[5];

	Assert (wallnum>=0);
	Walls[wallnum].type=type;
	Walls[wallnum].flags=flag;
	//Assert(state <= 4);
	Walls[wallnum].state=state;

	if (Walls[wallnum].type==WALL_OPEN)
	{
		digi_kill_sound_linked_to_segment(Walls[wallnum].segnum,Walls[wallnum].sidenum,SOUND_FORCEFIELD_HUM);
		//digi_kill_sound_linked_to_segment(csegp-Segments,cside,SOUND_FORCEFIELD_HUM);
	}
}

void multi_send_jason_cheat (int num)
{
	num=num;
	return;
}

void multi_send_kill_goal_counts()
{
	int i,count=1;
	multibuf[0]=MULTI_KILLGOALS;

	for (i=0;i<MAX_PLAYERS;i++)
	{
		*(char *)(multibuf+count)=(char)Players[i].KillGoalCount;
		count++;
	}

	multi_send_data(multibuf, count, 2);
}

void multi_do_kill_goal_counts(char *buf)
{
	int i,count=1;

	for (i=0;i<MAX_PLAYERS;i++)
	{
		Players[i].KillGoalCount=*(char *)(buf+count);
		count++;
	}

}

void multi_send_heartbeat ()
{
	if (!Netgame.PlayTimeAllowed)
		return;

	multibuf[0]=MULTI_HEARTBEAT;
	PUT_INTEL_INT(multibuf+1, ThisLevelTime);
	multi_send_data(multibuf, 5, 0);
}

void multi_do_heartbeat (char *buf)
{
	fix num;

	num = GET_INTEL_INT(buf + 1);

	ThisLevelTime=num;
}

void multi_check_for_killgoal_winner ()
{
	int i,best=0,bestnum=0;
	object *objp;

	if (Control_center_destroyed)
		return;

	for (i=0;i<N_players;i++)
	{
		if (Players[i].KillGoalCount>best)
		{
			best=Players[i].KillGoalCount;
			bestnum=i;
		}
	}

	if (bestnum==Player_num)
	{
		HUD_init_message(HM_MULTI, "You have the best score at %d kills!",best);
		//Players[Player_num].shields=i2f(200);
	}
	else

		HUD_init_message(HM_MULTI, "%s has the best score with %d kills!",Players[bestnum].callsign,best);

	HUD_init_message(HM_MULTI, "The control center has been destroyed!");

	objp=obj_find_first_of_type (OBJ_CNTRLCEN);
	net_destroy_controlcen (objp);
}

extern fix64 Seismic_disturbance_start_time;
extern fix64 Seismic_disturbance_end_time;

// Sync our seismic time with other players
void multi_send_seismic (fix64 t1,fix64 t2)
{
	int count=1;

	multibuf[0]=MULTI_SEISMIC;
	PUT_INTEL_INT(multibuf+count, t1); count+=(sizeof(fix));
	PUT_INTEL_INT(multibuf+count, t2); count+=(sizeof(fix));
	multi_send_data(multibuf, count, 2);
}

void multi_do_seismic (char *buf)
{
	fix duration = GET_INTEL_INT(buf + 5);
	Seismic_disturbance_start_time = GameTime;
	Seismic_disturbance_end_time = GameTime + duration;
}

void multi_send_light_specific (int pnum,int segnum,ubyte val)
{
	int count=1,i;

	Assert (Game_mode & GM_NETWORK);
	//  Assert (pnum>-1 && pnum<N_players);

	multibuf[0]=MULTI_LIGHT;
	PUT_INTEL_INT(multibuf+count, segnum); count+=(sizeof(int));
	*(char *)(multibuf+count)=val; count++;

	for (i=0;i<6;i++)
	{
		PUT_INTEL_SHORT(multibuf+count, Segments[segnum].sides[i].tmap_num2); count+=2;
	}

	multi_send_data_direct((ubyte *)multibuf, count, pnum, 2);
}

void multi_do_light (char *buf)
{
	int i, seg;
	ubyte sides=*(char *)(buf+5);

	seg = GET_INTEL_INT(buf + 1);
	for (i=0;i<6;i++)
	{
		if ((sides & (1<<i)))
		{
			subtract_light (seg,i);
			Segments[seg].sides[i].tmap_num2 = GET_INTEL_SHORT(buf + (6 + (2 * i)));
		}
	}
}

void multi_do_flags (char *buf)
{
	char pnum=buf[1];
	uint flags;

	flags = GET_INTEL_INT(buf + 2);
	if (pnum!=Player_num)
		Players[(int)pnum].flags=flags;
}

void multi_send_flags (char pnum)
{
	multibuf[0]=MULTI_FLAGS;
	multibuf[1]=pnum;
	PUT_INTEL_INT(multibuf+2, Players[(int)pnum].flags);
 
	multi_send_data(multibuf, 6, 2);
}

void multi_send_drop_blobs (char pnum)
{
	multibuf[0]=MULTI_DROP_BLOB;
	multibuf[1]=pnum;

	multi_send_data(multibuf, 2, 0);
}

void multi_do_drop_blob (char *buf)
{
	char pnum=buf[1];
	drop_afterburner_blobs (&Objects[Players[(int)pnum].objnum], 2, i2f(5)/2, -1);
}

void multi_send_powcap_update ()
{
	int i;


	multibuf[0]=MULTI_POWCAP_UPDATE;
	for (i=0;i<MAX_POWERUP_TYPES;i++)
		multibuf[i+1]=MaxPowerupsAllowed[i];

	multi_send_data(multibuf, MAX_POWERUP_TYPES+1, 2);
}
void multi_do_powcap_update (char *buf)
{
	int i;

	for (i=0;i<MAX_POWERUP_TYPES;i++)
		if (buf[i+1]>MaxPowerupsAllowed[i])
			MaxPowerupsAllowed[i]=buf[i+1];
}

extern active_door ActiveDoors[];
extern int Num_open_doors;          // Number of open doors


#if 0 // never used...
void multi_send_active_door (int i)
{
	int count;

	multibuf[0]=MULTI_ACTIVE_DOOR;
	multibuf[1]=i;
	multibuf[2]=Num_open_doors;
	count = 3;
#ifndef WORDS_BIGENDIAN
	memcpy ((char *)(&multibuf[3]),&ActiveDoors[(int)i],sizeof(struct active_door));
	count += sizeof(active_door);
#else
	PUT_INTEL_INT(multibuf + count, ActiveDoors[i].n_parts);                 count += 4;
	PUT_INTEL_SHORT(multibuf + count, ActiveDoors[i].front_wallnum[0]);    count += 2;
	PUT_INTEL_SHORT(multibuf + count, ActiveDoors[i].front_wallnum[1]);    count += 2;
	PUT_INTEL_SHORT(multibuf + count, ActiveDoors[i].back_wallnum[0]);     count += 2;
	PUT_INTEL_SHORT(multibuf + count, ActiveDoors[i].back_wallnum[1]);     count += 2;
	PUT_INTEL_INT(multibuf + count, ActiveDoors[i].time);                    count += 4;
#endif
	multi_send_data (multibuf,count,2);
}
#endif // 0 (never used)

void multi_do_active_door (char *buf)
{
	char i = multibuf[1];
	Num_open_doors = buf[2];

	memcpy(&ActiveDoors[(int)i], buf+3, sizeof(struct active_door));
#ifdef WORDS_BIGENDIAN
	{
		active_door *ad = &ActiveDoors[(int)i];
		ad->n_parts = INTEL_INT(ad->n_parts);
		ad->front_wallnum[0] = INTEL_SHORT(ad->front_wallnum[0]);
		ad->front_wallnum[1] = INTEL_SHORT(ad->front_wallnum[1]);
		ad->back_wallnum[0] = INTEL_SHORT(ad->back_wallnum[0]);
		ad->back_wallnum[1] = INTEL_SHORT(ad->back_wallnum[1]);
		ad->time = INTEL_INT(ad->time);
	}
#endif //WORDS_BIGENDIAN
}

void multi_send_sound_function (char whichfunc, char sound)
{
	int count=0;

	multibuf[0]=MULTI_SOUND_FUNCTION;   count++;
	multibuf[1]=Player_num;             count++;
	multibuf[2]=whichfunc;              count++;
#ifndef WORDS_BIGENDIAN
	*(uint *)(multibuf+count)=sound;    count++;
#else
	multibuf[3] = sound; count++;       // this would probably work on the PC as well.  Jason?
#endif
	multi_send_data (multibuf,4,2);
}

#define AFTERBURNER_LOOP_START  20098
#define AFTERBURNER_LOOP_END    25776

void multi_do_sound_function (char *buf)
{
	// for afterburner

	char pnum,whichfunc;
	int sound;

	if (Players[Player_num].connected!=CONNECT_PLAYING)
		return;

	pnum=buf[1];
	whichfunc=buf[2];
	sound=buf[3];

	if (whichfunc==0)
		digi_kill_sound_linked_to_object (Players[(int)pnum].objnum);
	else if (whichfunc==3)
		digi_link_sound_to_object3( sound, Players[(int)pnum].objnum, 1,F1_0, i2f(256), AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
}

void multi_send_capture_bonus (char pnum)
{
	Assert (Game_mode & GM_CAPTURE);

	multibuf[0]=MULTI_CAPTURE_BONUS;
	multibuf[1]=pnum;

	multi_send_data (multibuf,2,2);
	multi_do_capture_bonus (multibuf);
}
void multi_send_orb_bonus (char pnum)
{
	Assert (Game_mode & GM_HOARD);

	multibuf[0]=MULTI_ORB_BONUS;
	multibuf[1]=pnum;
	multibuf[2]=Players[Player_num].secondary_ammo[PROXIMITY_INDEX];

	multi_send_data (multibuf,3,2);
	multi_do_orb_bonus (multibuf);
}
void multi_do_capture_bonus(char *buf)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	char pnum=buf[1];
	int TheGoal;

	if (pnum==Player_num)
		HUD_init_message(HM_MULTI, "You have Scored!");
	else
		HUD_init_message(HM_MULTI, "%s has Scored!",Players[(int)pnum].callsign);

	if (pnum==Player_num)
		digi_play_sample (SOUND_HUD_YOU_GOT_GOAL,F1_0*2);
	else if (get_team(pnum)==TEAM_RED)
		digi_play_sample (SOUND_HUD_RED_GOT_GOAL,F1_0*2);
	else
		digi_play_sample (SOUND_HUD_BLUE_GOT_GOAL,F1_0*2);

	Players[(int)pnum].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag

	team_kills[get_team(pnum)] += 5;
	Players[(int)pnum].net_kills_total += 5;
	Players[(int)pnum].KillGoalCount+=5;

	if (Netgame.KillGoal>0)
	{
		TheGoal=Netgame.KillGoal*5;

		if (Players[(int)pnum].KillGoalCount>=TheGoal)
		{
			if (pnum==Player_num)
			{
				HUD_init_message(HM_MULTI, "You reached the kill goal!");
				Players[Player_num].shields=i2f(200);
			}
			else
				HUD_init_message(HM_MULTI, "%s has reached the kill goal!",Players[(int)pnum].callsign);

			HUD_init_message(HM_MULTI, "The control center has been destroyed!");
			net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
		}
	}

	multi_sort_kill_list();
	multi_show_player_list();
}

int GetOrbBonus (char num)
{
	int bonus;

	bonus=num*(num+1)/2;
	return (bonus);
}

void multi_do_orb_bonus(char *buf)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	char pnum=buf[1];
	int TheGoal;
	int bonus=GetOrbBonus (buf[2]);

	if (pnum==Player_num)
		HUD_init_message(HM_MULTI, "You have scored %d points!",bonus);
	else
		HUD_init_message(HM_MULTI, "%s has scored with %d orbs!",Players[(int)pnum].callsign,buf[2]);

	if (pnum==Player_num)
		digi_start_sound_queued (SOUND_HUD_YOU_GOT_GOAL,F1_0*2);
	else if (Game_mode & GM_TEAM)
	{
		if (get_team(pnum)==TEAM_RED)
			digi_play_sample (SOUND_HUD_RED_GOT_GOAL,F1_0*2);
		else
			digi_play_sample (SOUND_HUD_BLUE_GOT_GOAL,F1_0*2);
	}
	else
		digi_play_sample (SOUND_OPPONENT_HAS_SCORED,F1_0*2);

	if (bonus>PhallicLimit)
	{
		if (pnum==Player_num)
			HUD_init_message(HM_MULTI, "You have the record with %d points!",bonus);
		else
			HUD_init_message(HM_MULTI, "%s has the record with %d points!",Players[(int)pnum].callsign,bonus);
		digi_play_sample (SOUND_BUDDY_MET_GOAL,F1_0*2);
		PhallicMan=pnum;
		PhallicLimit=bonus;
	}

	Players[(int)pnum].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear orb flag

	team_kills[get_team(pnum)] += bonus;
	Players[(int)pnum].net_kills_total += bonus;
	Players[(int)pnum].KillGoalCount+=bonus;

	team_kills[get_team(pnum)]%=1000;
	Players[(int)pnum].net_kills_total%=1000;
	Players[(int)pnum].KillGoalCount%=1000;

	if (Netgame.KillGoal>0)
	{
		TheGoal=Netgame.KillGoal*5;

		if (Players[(int)pnum].KillGoalCount>=TheGoal)
		{
			if (pnum==Player_num)
			{
				HUD_init_message(HM_MULTI, "You reached the kill goal!");
				Players[Player_num].shields=i2f(200);
			}
			else
				HUD_init_message(HM_MULTI, "%s has reached the kill goal!",Players[(int)pnum].callsign);

			HUD_init_message(HM_MULTI, "The control center has been destroyed!");
			net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
		}
	}
	multi_sort_kill_list();
	multi_show_player_list();
}

void multi_send_got_flag (char pnum)
{
	multibuf[0]=MULTI_GOT_FLAG;
	multibuf[1]=pnum;

	digi_start_sound_queued (SOUND_HUD_YOU_GOT_FLAG,F1_0*2);

	multi_send_data (multibuf,2,2);
	multi_send_flags (Player_num);
}

int SoundHacked=0;
digi_sound ReversedSound;

void multi_send_got_orb (char pnum)
{
	multibuf[0]=MULTI_GOT_ORB;
	multibuf[1]=pnum;

	digi_play_sample (SOUND_YOU_GOT_ORB,F1_0*2);

	multi_send_data (multibuf,2,2);
	multi_send_flags (Player_num);
}

void multi_do_got_flag (char *buf)
{
	char pnum=buf[1];

	if (pnum==Player_num)
		digi_start_sound_queued (SOUND_HUD_YOU_GOT_FLAG,F1_0*2);
	else if (get_team(pnum)==TEAM_RED)
		digi_start_sound_queued (SOUND_HUD_RED_GOT_FLAG,F1_0*2);
	else
		digi_start_sound_queued (SOUND_HUD_BLUE_GOT_FLAG,F1_0*2);
	Players[(int)pnum].flags|=PLAYER_FLAGS_FLAG;
	HUD_init_message(HM_MULTI, "%s picked up a flag!",Players[(int)pnum].callsign);
}
void multi_do_got_orb (char *buf)
{
	char pnum=buf[1];

	Assert (Game_mode & GM_HOARD);

	if (Game_mode & GM_TEAM)
	{
		if (get_team(pnum)==get_team(Player_num))
			digi_play_sample (SOUND_FRIEND_GOT_ORB,F1_0*2);
		else
			digi_play_sample (SOUND_OPPONENT_GOT_ORB,F1_0*2);
    }
	else
		digi_play_sample (SOUND_OPPONENT_GOT_ORB,F1_0*2);

	Players[(int)pnum].flags|=PLAYER_FLAGS_FLAG;
	HUD_init_message(HM_MULTI, "%s picked up an orb!",Players[(int)pnum].callsign);
}


void DropOrb ()
{
	int objnum,seed;

	if (!(Game_mode & GM_HOARD))
		Int3(); // How did we get here? Get Leighton!

	if (!Players[Player_num].secondary_ammo[PROXIMITY_INDEX])
	{
		HUD_init_message(HM_MULTI, "No orbs to drop!");
		return;
	}

	seed = d_rand();

	objnum = spit_powerup(ConsoleObject,POW_HOARD_ORB,seed);

	if (objnum<0)
		return;

	HUD_init_message(HM_MULTI, "Orb dropped!");
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	if ((Game_mode & GM_HOARD) && objnum>-1)
		multi_send_drop_flag(objnum,seed);

	Players[Player_num].secondary_ammo[PROXIMITY_INDEX]--;

	// If empty, tell everyone to stop drawing the box around me
	if (Players[Player_num].secondary_ammo[PROXIMITY_INDEX]==0)
	{
		Players[Player_num].flags &=~(PLAYER_FLAGS_FLAG);
		multi_send_flags (Player_num);
	}
}

void DropFlag ()
{
	int objnum,seed;

	if (!(Game_mode & GM_CAPTURE) && !(Game_mode & GM_HOARD))
		return;
	if (Game_mode & GM_HOARD)
	{
		DropOrb();
		return;
	}

	if (!(Players[Player_num].flags & PLAYER_FLAGS_FLAG))
	{
		HUD_init_message(HM_MULTI, "No flag to drop!");
		return;
	}


	HUD_init_message(HM_MULTI, "Flag dropped!");
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	seed = d_rand();

	if (get_team (Player_num)==TEAM_RED)
		objnum = spit_powerup(ConsoleObject,POW_FLAG_BLUE,seed);
	else
		objnum = spit_powerup(ConsoleObject,POW_FLAG_RED,seed);

	if (objnum<0)
		return;

	if ((Game_mode & GM_CAPTURE) && objnum>-1)
		multi_send_drop_flag(objnum,seed);

	Players[Player_num].flags &=~(PLAYER_FLAGS_FLAG);
}


void multi_send_drop_flag (int objnum,int seed)
{
	object *objp;
	int count=0;

	objp = &Objects[objnum];

	multibuf[count++]=(char)MULTI_DROP_FLAG;
	multibuf[count++]=(char)objp->id;

	PUT_INTEL_SHORT(multibuf+count, Player_num); count += 2;
	PUT_INTEL_SHORT(multibuf+count, objnum); count += 2;
	PUT_INTEL_SHORT(multibuf+count, objp->ctype.powerup_info.count); count += 2;
	PUT_INTEL_INT(multibuf+count, seed);

	map_objnum_local_to_local(objnum);

	if (!(Game_mode & GM_HOARD))
		if (Game_mode & GM_NETWORK)
			PowerupsInMine[objp->id]++;

	multi_send_data(multibuf, 12, 2);
}

void multi_do_drop_flag (char *buf)
{
	int pnum,ammo,objnum,remote_objnum,seed;
	object *objp;
	int powerup_id;

	powerup_id=buf[1];
	pnum = GET_INTEL_SHORT(buf + 2);
	remote_objnum = GET_INTEL_SHORT(buf + 4);
	ammo = GET_INTEL_SHORT(buf + 6);
	seed = GET_INTEL_INT(buf + 8);

	objp = &Objects[Players[pnum].objnum];

	objnum = spit_powerup(objp, powerup_id, seed);

	map_objnum_local_to_remote(objnum, remote_objnum, pnum);

	if (objnum!=-1)
		Objects[objnum].ctype.powerup_info.count = ammo;

	if (!(Game_mode & GM_HOARD))
	{
		if (Game_mode & GM_NETWORK)
			PowerupsInMine[powerup_id]++;
		Players[pnum].flags &= ~(PLAYER_FLAGS_FLAG);
	}
}

#define POWERUPADJUSTS 5
int PowerupAdjustMapping[]={11,19,39,41,44};

int multi_powerup_is_4pack (int id)
{
	int i;

	for (i=0;i<POWERUPADJUSTS;i++)
		if (id==PowerupAdjustMapping[i])
			return (1);
	return (0);
}

int multi_powerup_is_allowed(int id)
{
	if (id == POW_INVULNERABILITY && !(Netgame.AllowedItems & NETFLAG_DOINVUL))
		return (0);
	if (id == POW_CLOAK && !(Netgame.AllowedItems & NETFLAG_DOCLOAK))
		return (0);
	if (id == POW_AFTERBURNER && !(Netgame.AllowedItems & NETFLAG_DOAFTERBURNER))
		return (0);
	if (id == POW_FUSION_WEAPON &&  !(Netgame.AllowedItems & NETFLAG_DOFUSION))
		return (0);
	if (id == POW_PHOENIX_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOPHOENIX))
		return (0);
	if (id == POW_HELIX_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOHELIX))
		return (0);
	if (id == POW_MEGA_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOMEGA))
		return (0);
	if (id == POW_SMARTBOMB_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOSMART))
		return (0);
	if (id == POW_GAUSS_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOGAUSS))
		return (0);
	if (id == POW_VULCAN_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOVULCAN))
		return (0);
	if (id == POW_PLASMA_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOPLASMA))
		return (0);
	if (id == POW_OMEGA_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOOMEGA))
		return (0);
	if (id == POW_SUPER_LASER && !(Netgame.AllowedItems & NETFLAG_DOSUPERLASER))
		return (0);
	if (id == POW_PROXIMITY_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOPROXIM))
		return (0);
	if (id==POW_VULCAN_AMMO && (!(Netgame.AllowedItems & NETFLAG_DOVULCAN) && !(Netgame.AllowedItems & NETFLAG_DOGAUSS)))
		return (0);
	if (id == POW_SPREADFIRE_WEAPON && !(Netgame.AllowedItems & NETFLAG_DOSPREAD))
		return (0);
	if (id == POW_SMART_MINE && !(Netgame.AllowedItems & NETFLAG_DOSMARTMINE))
		return (0);
	if (id == POW_SMISSILE1_1 &&  !(Netgame.AllowedItems & NETFLAG_DOFLASH))
		return (0);
	if (id == POW_SMISSILE1_4 &&  !(Netgame.AllowedItems & NETFLAG_DOFLASH))
		return (0);
	if (id == POW_GUIDED_MISSILE_1 &&  !(Netgame.AllowedItems & NETFLAG_DOGUIDED))
		return (0);
	if (id == POW_GUIDED_MISSILE_4 &&  !(Netgame.AllowedItems & NETFLAG_DOGUIDED))
		return (0);
	if (id == POW_EARTHSHAKER_MISSILE &&  !(Netgame.AllowedItems & NETFLAG_DOSHAKER))
		return (0);
	if (id == POW_MERCURY_MISSILE_1 &&  !(Netgame.AllowedItems & NETFLAG_DOMERCURY))
		return (0);
	if (id == POW_MERCURY_MISSILE_4 &&  !(Netgame.AllowedItems & NETFLAG_DOMERCURY))
		return (0);
	if (id == POW_CONVERTER &&  !(Netgame.AllowedItems & NETFLAG_DOCONVERTER))
		return (0);
	if (id == POW_AMMO_RACK &&  !(Netgame.AllowedItems & NETFLAG_DOAMMORACK))
		return (0);
	if (id == POW_HEADLIGHT &&  !(Netgame.AllowedItems & NETFLAG_DOHEADLIGHT))
		return (0);
	if (id == POW_LASER &&  !(Netgame.AllowedItems & NETFLAG_DOLASER))
		return (0);
	if (id == POW_HOMING_AMMO_1 &&  !(Netgame.AllowedItems & NETFLAG_DOHOMING))
		return (0);
	if (id == POW_HOMING_AMMO_4 &&  !(Netgame.AllowedItems & NETFLAG_DOHOMING))
		return (0);
	if (id == POW_QUAD_FIRE &&  !(Netgame.AllowedItems & NETFLAG_DOQUAD))
		return (0);
	if (id == POW_FLAG_BLUE && !(Game_mode & GM_CAPTURE))
		return (0);
	if (id == POW_FLAG_RED && !(Game_mode & GM_CAPTURE))
		return (0);

	return (1);
}

void multi_send_finish_game ()
{
	multibuf[0]=MULTI_FINISH_GAME;
	multibuf[1]=Player_num;

	multi_send_data (multibuf,2,2);
}


extern void do_final_boss_hacks();
void multi_do_finish_game (char *buf)
{
	if (buf[0]!=MULTI_FINISH_GAME)
		return;

	if (Current_level_num!=Last_level)
		return;

	do_final_boss_hacks();
}

void multi_send_trigger_specific (char pnum,char trig)
{
	multibuf[0] = MULTI_START_TRIGGER;
	multibuf[1] = trig;

	multi_send_data_direct((ubyte *)multibuf, 2, pnum, 2);
}
void multi_do_start_trigger (char *buf)
{
	Triggers[(int)buf[1]].flags |=TF_DISABLED;
}

extern char *RankStrings[];

void multi_add_lifetime_kills ()
{
	// This function adds a kill to lifetime stats of this player, and possibly
	// gives a promotion.  If so, it will tell everyone else

	int oldrank;

	if (!(Game_mode & GM_NETWORK))
		return;

	oldrank=GetMyNetRanking();

	PlayerCfg.NetlifeKills++;

	if (oldrank!=GetMyNetRanking())
	{
		multi_send_ranking();
		if (!PlayerCfg.NoRankings)
		{
			HUD_init_message(HM_MULTI, "You have been promoted to %s!",RankStrings[GetMyNetRanking()]);
			digi_play_sample (SOUND_BUDDY_MET_GOAL,F1_0*2);
			Netgame.players[Player_num].rank=GetMyNetRanking();
		}
	}
}

void multi_add_lifetime_killed ()
{
	// This function adds a "killed" to lifetime stats of this player, and possibly
	// gives a demotion.  If so, it will tell everyone else

	int oldrank;

	if (!(Game_mode & GM_NETWORK))
		return;

	oldrank=GetMyNetRanking();

	PlayerCfg.NetlifeKilled++;

	if (oldrank!=GetMyNetRanking())
	{
		multi_send_ranking();
		Netgame.players[Player_num].rank=GetMyNetRanking();

		if (!PlayerCfg.NoRankings)
			HUD_init_message(HM_MULTI, "You have been demoted to %s!",RankStrings[GetMyNetRanking()]);

	}
}

void multi_send_ranking ()
{
	multibuf[0]=(char)MULTI_RANK;
	multibuf[1]=(char)Player_num;
	multibuf[2]=(char)GetMyNetRanking();

	multi_send_data (multibuf,3,2);
}

void multi_do_ranking (char *buf)
{
	char rankstr[20];
	char pnum=buf[1];
	char rank=buf[2];

	if (Netgame.players[(int)pnum].rank<rank)
		strcpy (rankstr,"promoted");
	else if (Netgame.players[(int)pnum].rank>rank)
		strcpy (rankstr,"demoted");
	else
		return;

	Netgame.players[(int)pnum].rank=rank;

	if (!PlayerCfg.NoRankings)
		HUD_init_message(HM_MULTI, "%s has been %s to %s!",Players[(int)pnum].callsign,rankstr,RankStrings[(int)rank]);
}

void multi_send_play_by_play (int num,int spnum,int dpnum)
{
	if (!(Game_mode & GM_HOARD))
		return;

	return;
	multibuf[0]=MULTI_PLAY_BY_PLAY;
	multibuf[1]=(char)num;
	multibuf[2]=(char)spnum;
	multibuf[3]=(char)dpnum;
	multi_send_data (multibuf,4,2);
	multi_do_play_by_play (multibuf);
}

void multi_do_play_by_play (char *buf)
{
	int whichplay=buf[1];
	int spnum=buf[2];
	int dpnum=buf[3];

	if (!(Game_mode & GM_HOARD))
	{
		Int3(); // Get Leighton, something bad has happened.
		return;
	}

	switch (whichplay)
	{
	case 0: // Smacked!
		HUD_init_message(HM_MULTI, "Ouch! %s has been smacked by %s!",Players[dpnum].callsign,Players[spnum].callsign);
		break;
	case 1: // Spanked!
		HUD_init_message(HM_MULTI, "Haha! %s has been spanked by %s!",Players[dpnum].callsign,Players[spnum].callsign);
		break;
	default:
		Int3();
	}
}

// Decide if fire from "killer" is friendly. If yes return 1 (no harm to me) otherwise 0 (damage me)
int multi_maybe_disable_friendly_fire(object *killer)
{
	if (!(Game_mode & GM_NETWORK)) // no Multiplayer game -> always harm me!
		return 0;
	if (!Netgame.NoFriendlyFire) // friendly fire is activated -> harm me!
		return 0;
	if (killer == NULL) // no actual killer -> harm me!
		return 0;
	if (killer->type != OBJ_PLAYER) // not a player -> harm me!
		return 0;
	if (Game_mode & GM_MULTI_COOP) // coop mode -> don't harm me!
		return 1;
	else if (Game_mode & GM_TEAM) // team mode - find out if killer is in my team
	{
		if (get_team(Player_num) == get_team(killer->id)) // in my team -> don't harm me!
			return 1;
		else // opposite team -> harm me!
			return 0;
	}
	return 0; // all other cases -> harm me!
}

/* Bounty packer sender and handler */
void multi_send_bounty( void )
{
	/* Test game mode */
	if( !( Game_mode & GM_BOUNTY ) )
		return;
	
	/* Add opcode and target ID */
	multibuf[0] = MULTI_DO_BOUNTY;
	multibuf[1] = (char)Bounty_target;
	
	/* Send data */
	multi_send_data( multibuf, 2, 2 );
}

void multi_do_bounty( char *buf )
{
	/* New target! */
	multi_new_bounty_target( buf[1] );
}

void multi_new_bounty_target( int pnum )
{
	/* If it's already the same, don't do it */
	if( Bounty_target == pnum )
		return;
	
	/* Set the target */
	Bounty_target = pnum;
	
	/* Send a message */
	HUD_init_message( HM_MULTI, "%c%c%s is the new target!", CC_COLOR,
		BM_XRGB( player_rgb[Bounty_target].r, player_rgb[Bounty_target].g, player_rgb[Bounty_target].b ),
		Players[Bounty_target].callsign );

	digi_play_sample( SOUND_BUDDY_MET_GOAL, F1_0 * 2 );
}

///
/// CODE TO LOAD HOARD DATA
///

int HoardEquipped()
{
	static int checked=-1;

	if (checked==-1)
	{
		if (cfexist("hoard.ham"))
			checked=1;
		else
			checked=0;
	}
	return (checked);
}

grs_bitmap Orb_icons[2];
int Hoard_goal_eclip, Hoard_bm_idx, Hoard_snd_idx;

void free_hoard_data()
{
	int i;

	d_free(GameBitmaps[Hoard_bm_idx].bm_data);
	for (i = Hoard_snd_idx; i < Hoard_snd_idx+4; i++)
		d_free(GameSounds[i].data);
	for (i = 0; i < 2; i++)
		d_free(Orb_icons[i].bm_data);
}

void init_hoard_data()
{
	static int first_time=1;
	static int orb_vclip;
	int n_orb_frames,n_goal_frames;
	int orb_w,orb_h;
	int icon_w,icon_h;
	ubyte palette[256*3];
	CFILE *ifile;
	int i,save_pos;
	extern int Num_bitmap_files,Num_effects,Num_sound_files;

	ifile = PHYSFSX_openReadBuffered("hoard.ham");
	if (ifile == NULL)
		Error("can't open <hoard.ham>");

	n_orb_frames = cfile_read_short(ifile);
	orb_w = cfile_read_short(ifile);
	orb_h = cfile_read_short(ifile);
	save_pos = cftell(ifile);
	cfseek(ifile,sizeof(palette)+n_orb_frames*orb_w*orb_h,SEEK_CUR);
	n_goal_frames = cfile_read_short(ifile);
	cfseek(ifile,save_pos,SEEK_SET);

	if (first_time) {
		ubyte *bitmap_data;
		int bitmap_num=Hoard_bm_idx=Num_bitmap_files;

		//Allocate memory for bitmaps
		MALLOC( bitmap_data, ubyte, n_orb_frames*orb_w*orb_h + n_goal_frames*64*64 );

		//Create orb vclip
		orb_vclip = Num_vclips++;
		Assert(Num_vclips <= VCLIP_MAXNUM);
		Vclip[orb_vclip].play_time = F1_0/2;
		Vclip[orb_vclip].num_frames = n_orb_frames;
		Vclip[orb_vclip].frame_time = Vclip[orb_vclip].play_time / Vclip[orb_vclip].num_frames;
		Vclip[orb_vclip].flags = 0;
		Vclip[orb_vclip].sound_num = -1;
		Vclip[orb_vclip].light_value = F1_0;
		for (i=0;i<n_orb_frames;i++) {
			Vclip[orb_vclip].frames[i].index = bitmap_num;
			gr_init_bitmap(&GameBitmaps[bitmap_num],0,0,orb_w,orb_h,orb_w,bitmap_data);
			gr_set_transparent (&GameBitmaps[bitmap_num], 1);
			bitmap_data += orb_w*orb_h;
			bitmap_num++;
			Assert(bitmap_num < MAX_BITMAP_FILES);
		}

		//Create obj powerup
		Powerup_info[POW_HOARD_ORB].vclip_num = orb_vclip;
		Powerup_info[POW_HOARD_ORB].hit_sound = -1; //Powerup_info[POW_SHIELD_BOOST].hit_sound;
		Powerup_info[POW_HOARD_ORB].size = Powerup_info[POW_SHIELD_BOOST].size;
		Powerup_info[POW_HOARD_ORB].light = Powerup_info[POW_SHIELD_BOOST].light;

		//Create orb goal wall effect
		Hoard_goal_eclip = Num_effects++;
		Assert(Num_effects < MAX_EFFECTS);
		Effects[Hoard_goal_eclip] = Effects[94];        //copy from blue goal
		Effects[Hoard_goal_eclip].changing_wall_texture = NumTextures;
		Effects[Hoard_goal_eclip].vc.num_frames=n_goal_frames;

		TmapInfo[NumTextures] = TmapInfo[find_goal_texture(TMI_GOAL_BLUE)];
		TmapInfo[NumTextures].eclip_num = Hoard_goal_eclip;
		TmapInfo[NumTextures].flags = TMI_GOAL_HOARD;
		NumTextures++;
		Assert(NumTextures < MAX_TEXTURES);
		for (i=0;i<n_goal_frames;i++) {
			Effects[Hoard_goal_eclip].vc.frames[i].index = bitmap_num;
			gr_init_bitmap(&GameBitmaps[bitmap_num],0,0,64,64,64,bitmap_data);
			bitmap_data += 64*64;
			bitmap_num++;
			Assert(bitmap_num < MAX_BITMAP_FILES);
		}
	}

	//Load and remap bitmap data for orb
	cfread(palette,3,256,ifile);
	for (i=0;i<n_orb_frames;i++) {
		grs_bitmap *bm = &GameBitmaps[Vclip[orb_vclip].frames[i].index];
		cfread(bm->bm_data,1,orb_w*orb_h,ifile);
		gr_remap_bitmap_good( bm, palette, 255, -1 );
	}

	//Load and remap bitmap data for goal texture
	cfile_read_short(ifile);        //skip frame count
	cfread(palette,3,256,ifile);
	for (i=0;i<n_goal_frames;i++) {
		grs_bitmap *bm = &GameBitmaps[Effects[Hoard_goal_eclip].vc.frames[i].index];
		cfread(bm->bm_data,1,64*64,ifile);
		gr_remap_bitmap_good( bm, palette, 255, -1 );
	}

	//Load and remap bitmap data for HUD icons
	for (i=0;i<2;i++) {
		icon_w = cfile_read_short(ifile);
		icon_h = cfile_read_short(ifile);
		if (first_time) {
			ubyte *bitmap_data;
			MALLOC( bitmap_data, ubyte, icon_w*icon_h );
			gr_init_bitmap(&Orb_icons[i],0,0,icon_w,icon_h,icon_w,bitmap_data);
			gr_set_transparent (&Orb_icons[i], 1);
		}
		cfread(palette,3,256,ifile);
		cfread(Orb_icons[i].bm_data,1,icon_w*icon_h,ifile);
		gr_remap_bitmap_good( &Orb_icons[i], palette, 255, -1 );
	}

	if (first_time) {

		//Load sounds for orb game
		Hoard_snd_idx = Num_sound_files;
		for (i=0;i<4;i++) {
			int len;

			len = cfile_read_int(ifile);        //get 11k len

			if (GameArg.SndDigiSampleRate == SAMPLE_RATE_22K) {
				cfseek(ifile,len,SEEK_CUR);     //skip over 11k sample
				len = cfile_read_int(ifile);    //get 22k len
			}

			GameSounds[Num_sound_files+i].length = len;
			GameSounds[Num_sound_files+i].data = d_malloc(len);
			cfread(GameSounds[Num_sound_files+i].data,1,len,ifile);

			if (GameArg.SndDigiSampleRate == SAMPLE_RATE_11K) {
				len = cfile_read_int(ifile);    //get 22k len
				cfseek(ifile,len,SEEK_CUR);     //skip over 22k sample
			}

			Sounds[SOUND_YOU_GOT_ORB+i] = Num_sound_files+i;
			AltSounds[SOUND_YOU_GOT_ORB+i] = Sounds[SOUND_YOU_GOT_ORB+i];
		}
	}

	cfclose(ifile);
	if (first_time)
		atexit(free_hoard_data);

	first_time = 0;
}

void
multi_process_data(char *buf, int len)
{
	// Take an entire message (that has already been checked for validity,
	// if necessary) and act on it.

	int type;
	len = len;

	type = buf[0];

	if (type > MULTI_MAX_TYPE)
	{
		Int3();
		return;
	}

	switch(type)
	{
	case MULTI_POSITION:
		if (!Endlevel_sequence) multi_do_position(buf); break;
	case MULTI_REAPPEAR:
		if (!Endlevel_sequence) multi_do_reappear(buf); break;
	case MULTI_FIRE:
		if (!Endlevel_sequence) multi_do_fire(buf); break;
	case MULTI_KILL:
		multi_do_kill(buf); break;
	case MULTI_REMOVE_OBJECT:
		if (!Endlevel_sequence) multi_do_remobj(buf); break;
	case MULTI_PLAYER_DROP:
	case MULTI_PLAYER_EXPLODE:
		if (!Endlevel_sequence) multi_do_player_explode(buf); break;
	case MULTI_MESSAGE:
		if (!Endlevel_sequence) multi_do_message(buf); break;
	case MULTI_QUIT:
		if (!Endlevel_sequence) multi_do_quit(buf); break;
	case MULTI_BEGIN_SYNC:
		break;
	case MULTI_CONTROLCEN:
		if (!Endlevel_sequence) multi_do_controlcen_destroy(buf); break;
	case MULTI_POWCAP_UPDATE:
		if (!Endlevel_sequence) multi_do_powcap_update(buf); break;
	case MULTI_SOUND_FUNCTION:
		multi_do_sound_function(buf); break;
	case MULTI_MARKER:
		if (!Endlevel_sequence) multi_do_drop_marker (buf); break;
	case MULTI_DROP_WEAPON:
		if (!Endlevel_sequence) multi_do_drop_weapon(buf); break;
	case MULTI_DROP_FLAG:
		if (!Endlevel_sequence) multi_do_drop_flag(buf); break;
	case MULTI_GUIDED:
		if (!Endlevel_sequence) multi_do_guided (buf); break;
	case MULTI_STOLEN_ITEMS:
		if (!Endlevel_sequence) multi_do_stolen_items(buf); break;
	case MULTI_WALL_STATUS:
		if (!Endlevel_sequence) multi_do_wall_status(buf); break;
	case MULTI_HEARTBEAT:
		if (!Endlevel_sequence) multi_do_heartbeat (buf); break;
	case MULTI_SEISMIC:
		if (!Endlevel_sequence) multi_do_seismic (buf); break;
	case MULTI_LIGHT:
		if (!Endlevel_sequence) multi_do_light (buf); break;
	case MULTI_KILLGOALS:

		if (!Endlevel_sequence) multi_do_kill_goal_counts (buf); break;
	case MULTI_ENDLEVEL_START:
		if (!Endlevel_sequence) multi_do_escape(buf); break;
	case MULTI_END_SYNC:
		break;
	case MULTI_CLOAK:
		if (!Endlevel_sequence) multi_do_cloak(buf); break;
	case MULTI_DECLOAK:
		if (!Endlevel_sequence) multi_do_decloak(buf); break;
	case MULTI_DOOR_OPEN:
		if (!Endlevel_sequence) multi_do_door_open(buf); break;
	case MULTI_CREATE_EXPLOSION:
		if (!Endlevel_sequence) multi_do_create_explosion(buf); break;
	case MULTI_CONTROLCEN_FIRE:
		if (!Endlevel_sequence) multi_do_controlcen_fire(buf); break;
	case MULTI_CREATE_POWERUP:
		if (!Endlevel_sequence) multi_do_create_powerup(buf); break;
	case MULTI_PLAY_SOUND:
		if (!Endlevel_sequence) multi_do_play_sound(buf); break;
	case MULTI_CAPTURE_BONUS:
		if (!Endlevel_sequence) multi_do_capture_bonus(buf); break;
	case MULTI_ORB_BONUS:
		if (!Endlevel_sequence) multi_do_orb_bonus(buf); break;
	case MULTI_GOT_FLAG:
		if (!Endlevel_sequence) multi_do_got_flag(buf); break;
	case MULTI_GOT_ORB:
		if (!Endlevel_sequence) multi_do_got_orb(buf); break;
	case MULTI_PLAY_BY_PLAY:
		if (!Endlevel_sequence) multi_do_play_by_play(buf); break;
	case MULTI_RANK:
		if (!Endlevel_sequence) multi_do_ranking (buf); break;
	case MULTI_FINISH_GAME:
		multi_do_finish_game(buf); break;  // do this one regardless of endsequence
	case MULTI_ROBOT_CONTROLS:
		break;
	case MULTI_ROBOT_CLAIM:
		if (!Endlevel_sequence) multi_do_claim_robot(buf); break;
	case MULTI_ROBOT_POSITION:
		if (!Endlevel_sequence) multi_do_robot_position(buf); break;
	case MULTI_ROBOT_EXPLODE:
		if (!Endlevel_sequence) multi_do_robot_explode(buf); break;
	case MULTI_ROBOT_RELEASE:
		if (!Endlevel_sequence) multi_do_release_robot(buf); break;
	case MULTI_ROBOT_FIRE:
		if (!Endlevel_sequence) multi_do_robot_fire(buf); break;
	case MULTI_SCORE:
		if (!Endlevel_sequence) multi_do_score(buf); break;
	case MULTI_CREATE_ROBOT:
		if (!Endlevel_sequence) multi_do_create_robot(buf); break;
	case MULTI_TRIGGER:
		if (!Endlevel_sequence) multi_do_trigger(buf); break;
	case MULTI_START_TRIGGER:
		if (!Endlevel_sequence) multi_do_start_trigger(buf); break;
	case MULTI_FLAGS:
		if (!Endlevel_sequence) multi_do_flags(buf); break;
	case MULTI_DROP_BLOB:
		if (!Endlevel_sequence) multi_do_drop_blob(buf); break;
	case MULTI_ACTIVE_DOOR:
		if (!Endlevel_sequence) multi_do_active_door(buf); break;
	case MULTI_BOSS_ACTIONS:
		if (!Endlevel_sequence) multi_do_boss_actions(buf); break;
	case MULTI_CREATE_ROBOT_POWERUPS:
		if (!Endlevel_sequence) multi_do_create_robot_powerups(buf); break;
	case MULTI_HOSTAGE_DOOR:
		if (!Endlevel_sequence) multi_do_hostage_door_status(buf); break;
	case MULTI_DO_BOUNTY:
		if( !Endlevel_sequence ) multi_do_bounty( buf ); break;
	default:
		Int3();
	}
}

// Following functions convert object to object_rw and back.
// turn object to object_rw for sending
void multi_object_to_object_rw(object *obj, object_rw *obj_rw)
{
	obj_rw->signature     = obj->signature;
	obj_rw->type          = obj->type;
	obj_rw->id            = obj->id;
	obj_rw->next          = obj->next;
	obj_rw->prev          = obj->prev;
	obj_rw->control_type  = obj->control_type;
	obj_rw->movement_type = obj->movement_type;
	obj_rw->render_type   = obj->render_type;
	obj_rw->flags         = obj->flags;
	obj_rw->segnum        = obj->segnum;
	obj_rw->attached_obj  = obj->attached_obj;
	obj_rw->pos.x         = obj->pos.x;
	obj_rw->pos.y         = obj->pos.y;
	obj_rw->pos.z         = obj->pos.z;
	obj_rw->orient.rvec.x = obj->orient.rvec.x;
	obj_rw->orient.rvec.y = obj->orient.rvec.y;
	obj_rw->orient.rvec.z = obj->orient.rvec.z;
	obj_rw->orient.fvec.x = obj->orient.fvec.x;
	obj_rw->orient.fvec.y = obj->orient.fvec.y;
	obj_rw->orient.fvec.z = obj->orient.fvec.z;
	obj_rw->orient.uvec.x = obj->orient.uvec.x;
	obj_rw->orient.uvec.y = obj->orient.uvec.y;
	obj_rw->orient.uvec.z = obj->orient.uvec.z;
	obj_rw->size          = obj->size;
	obj_rw->shields       = obj->shields;
	obj_rw->last_pos.x    = obj->last_pos.x;
	obj_rw->last_pos.y    = obj->last_pos.y;
	obj_rw->last_pos.z    = obj->last_pos.z;
	obj_rw->contains_type = obj->contains_type;
	obj_rw->contains_id   = obj->contains_id;
	obj_rw->contains_count= obj->contains_count;
	obj_rw->matcen_creator= obj->matcen_creator;
	obj_rw->lifeleft      = obj->lifeleft;
	
	switch (obj_rw->movement_type)
	{
		case MT_PHYSICS:
			obj_rw->mtype.phys_info.velocity.x  = obj->mtype.phys_info.velocity.x;
			obj_rw->mtype.phys_info.velocity.y  = obj->mtype.phys_info.velocity.y;
			obj_rw->mtype.phys_info.velocity.z  = obj->mtype.phys_info.velocity.z;
			obj_rw->mtype.phys_info.thrust.x    = obj->mtype.phys_info.thrust.x;
			obj_rw->mtype.phys_info.thrust.y    = obj->mtype.phys_info.thrust.y;
			obj_rw->mtype.phys_info.thrust.z    = obj->mtype.phys_info.thrust.z;
			obj_rw->mtype.phys_info.mass        = obj->mtype.phys_info.mass;
			obj_rw->mtype.phys_info.drag        = obj->mtype.phys_info.drag;
			obj_rw->mtype.phys_info.brakes      = obj->mtype.phys_info.brakes;
			obj_rw->mtype.phys_info.rotvel.x    = obj->mtype.phys_info.rotvel.x;
			obj_rw->mtype.phys_info.rotvel.y    = obj->mtype.phys_info.rotvel.y;
			obj_rw->mtype.phys_info.rotvel.z    = obj->mtype.phys_info.rotvel.z;
			obj_rw->mtype.phys_info.rotthrust.x = obj->mtype.phys_info.rotthrust.x;
			obj_rw->mtype.phys_info.rotthrust.y = obj->mtype.phys_info.rotthrust.y;
			obj_rw->mtype.phys_info.rotthrust.z = obj->mtype.phys_info.rotthrust.z;
			obj_rw->mtype.phys_info.turnroll    = obj->mtype.phys_info.turnroll;
			obj_rw->mtype.phys_info.flags       = obj->mtype.phys_info.flags;
			break;
			
		case MT_SPINNING:
			obj_rw->mtype.spin_rate.x = obj->mtype.spin_rate.x;
			obj_rw->mtype.spin_rate.y = obj->mtype.spin_rate.y;
			obj_rw->mtype.spin_rate.z = obj->mtype.spin_rate.z;
			break;
	}
	
	switch (obj_rw->control_type)
	{
		case CT_WEAPON:
			obj_rw->ctype.laser_info.parent_type      = obj->ctype.laser_info.parent_type;
			obj_rw->ctype.laser_info.parent_num       = obj->ctype.laser_info.parent_num;
			obj_rw->ctype.laser_info.parent_signature = obj->ctype.laser_info.parent_signature;
			if (obj->ctype.laser_info.creation_time - GameTime < F1_0*(-18000))
				obj_rw->ctype.laser_info.creation_time = F1_0*(-18000);
			else
				obj_rw->ctype.laser_info.creation_time = obj->ctype.laser_info.creation_time - GameTime;
			obj_rw->ctype.laser_info.last_hitobj      = obj->ctype.laser_info.last_hitobj;
			obj_rw->ctype.laser_info.track_goal       = obj->ctype.laser_info.track_goal;
			obj_rw->ctype.laser_info.multiplier       = obj->ctype.laser_info.multiplier;
			break;
			
		case CT_EXPLOSION:
			obj_rw->ctype.expl_info.spawn_time    = obj->ctype.expl_info.spawn_time;
			obj_rw->ctype.expl_info.delete_time   = obj->ctype.expl_info.delete_time;
			obj_rw->ctype.expl_info.delete_objnum = obj->ctype.expl_info.delete_objnum;
			obj_rw->ctype.expl_info.attach_parent = obj->ctype.expl_info.attach_parent;
			obj_rw->ctype.expl_info.prev_attach   = obj->ctype.expl_info.prev_attach;
			obj_rw->ctype.expl_info.next_attach   = obj->ctype.expl_info.next_attach;
			break;
			
		case CT_AI:
		{
			int i;
			obj_rw->ctype.ai_info.behavior               = obj->ctype.ai_info.behavior; 
			for (i = 0; i < MAX_AI_FLAGS; i++)
				obj_rw->ctype.ai_info.flags[i]       = obj->ctype.ai_info.flags[i]; 
			obj_rw->ctype.ai_info.hide_segment           = obj->ctype.ai_info.hide_segment;
			obj_rw->ctype.ai_info.hide_index             = obj->ctype.ai_info.hide_index;
			obj_rw->ctype.ai_info.path_length            = obj->ctype.ai_info.path_length;
			obj_rw->ctype.ai_info.cur_path_index         = obj->ctype.ai_info.cur_path_index;
			obj_rw->ctype.ai_info.dying_sound_playing    = obj->ctype.ai_info.dying_sound_playing;
			obj_rw->ctype.ai_info.danger_laser_num       = obj->ctype.ai_info.danger_laser_num;
			obj_rw->ctype.ai_info.danger_laser_signature = obj->ctype.ai_info.danger_laser_signature;
			if (obj->ctype.ai_info.dying_start_time == 0) // if bot not dead, anything but 0 will kill it
				obj_rw->ctype.ai_info.dying_start_time = 0;
			else
				obj_rw->ctype.ai_info.dying_start_time = obj->ctype.ai_info.dying_start_time - GameTime;
			break;
		}
			
		case CT_LIGHT:
			obj_rw->ctype.light_info.intensity = obj->ctype.light_info.intensity;
			break;
			
		case CT_POWERUP:
			obj_rw->ctype.powerup_info.count         = obj->ctype.powerup_info.count;
			if (obj->ctype.powerup_info.creation_time - GameTime < F1_0*(-18000))
				obj_rw->ctype.powerup_info.creation_time = F1_0*(-18000);
			else
				obj_rw->ctype.powerup_info.creation_time = obj->ctype.powerup_info.creation_time - GameTime;
			obj_rw->ctype.powerup_info.flags         = obj->ctype.powerup_info.flags;
			break;
	}
	
	switch (obj_rw->render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj->render_type == RT_NONE && obj->type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time.
				break;
			obj_rw->rtype.pobj_info.model_num                = obj->rtype.pobj_info.model_num;
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj_rw->rtype.pobj_info.anim_angles[i].p = obj->rtype.pobj_info.anim_angles[i].p;
				obj_rw->rtype.pobj_info.anim_angles[i].b = obj->rtype.pobj_info.anim_angles[i].b;
				obj_rw->rtype.pobj_info.anim_angles[i].h = obj->rtype.pobj_info.anim_angles[i].h;
			}
			obj_rw->rtype.pobj_info.subobj_flags             = obj->rtype.pobj_info.subobj_flags;
			obj_rw->rtype.pobj_info.tmap_override            = obj->rtype.pobj_info.tmap_override;
			obj_rw->rtype.pobj_info.alt_textures             = obj->rtype.pobj_info.alt_textures;
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj_rw->rtype.vclip_info.vclip_num = obj->rtype.vclip_info.vclip_num;
			obj_rw->rtype.vclip_info.frametime = obj->rtype.vclip_info.frametime;
			obj_rw->rtype.vclip_info.framenum  = obj->rtype.vclip_info.framenum;
			break;
			
		case RT_LASER:
			break;
			
	}
}

// turn object_rw to object after receiving
void multi_object_rw_to_object(object_rw *obj_rw, object *obj)
{
	obj->signature     = obj_rw->signature;
	obj->type          = obj_rw->type;
	obj->id            = obj_rw->id;
	obj->next          = obj_rw->next;
	obj->prev          = obj_rw->prev;
	obj->control_type  = obj_rw->control_type;
	obj->movement_type = obj_rw->movement_type;
	obj->render_type   = obj_rw->render_type;
	obj->flags         = obj_rw->flags;
	obj->segnum        = obj_rw->segnum;
	obj->attached_obj  = obj_rw->attached_obj;
	obj->pos.x         = obj_rw->pos.x;
	obj->pos.y         = obj_rw->pos.y;
	obj->pos.z         = obj_rw->pos.z;
	obj->orient.rvec.x = obj_rw->orient.rvec.x;
	obj->orient.rvec.y = obj_rw->orient.rvec.y;
	obj->orient.rvec.z = obj_rw->orient.rvec.z;
	obj->orient.fvec.x = obj_rw->orient.fvec.x;
	obj->orient.fvec.y = obj_rw->orient.fvec.y;
	obj->orient.fvec.z = obj_rw->orient.fvec.z;
	obj->orient.uvec.x = obj_rw->orient.uvec.x;
	obj->orient.uvec.y = obj_rw->orient.uvec.y;
	obj->orient.uvec.z = obj_rw->orient.uvec.z;
	obj->size          = obj_rw->size;
	obj->shields       = obj_rw->shields;
	obj->last_pos.x    = obj_rw->last_pos.x;
	obj->last_pos.y    = obj_rw->last_pos.y;
	obj->last_pos.z    = obj_rw->last_pos.z;
	obj->contains_type = obj_rw->contains_type;
	obj->contains_id   = obj_rw->contains_id;
	obj->contains_count= obj_rw->contains_count;
	obj->matcen_creator= obj_rw->matcen_creator;
	obj->lifeleft      = obj_rw->lifeleft;
	
	switch (obj->movement_type)
	{
		case MT_PHYSICS:
			obj->mtype.phys_info.velocity.x  = obj_rw->mtype.phys_info.velocity.x;
			obj->mtype.phys_info.velocity.y  = obj_rw->mtype.phys_info.velocity.y;
			obj->mtype.phys_info.velocity.z  = obj_rw->mtype.phys_info.velocity.z;
			obj->mtype.phys_info.thrust.x    = obj_rw->mtype.phys_info.thrust.x;
			obj->mtype.phys_info.thrust.y    = obj_rw->mtype.phys_info.thrust.y;
			obj->mtype.phys_info.thrust.z    = obj_rw->mtype.phys_info.thrust.z;
			obj->mtype.phys_info.mass        = obj_rw->mtype.phys_info.mass;
			obj->mtype.phys_info.drag        = obj_rw->mtype.phys_info.drag;
			obj->mtype.phys_info.brakes      = obj_rw->mtype.phys_info.brakes;
			obj->mtype.phys_info.rotvel.x    = obj_rw->mtype.phys_info.rotvel.x;
			obj->mtype.phys_info.rotvel.y    = obj_rw->mtype.phys_info.rotvel.y;
			obj->mtype.phys_info.rotvel.z    = obj_rw->mtype.phys_info.rotvel.z;
			obj->mtype.phys_info.rotthrust.x = obj_rw->mtype.phys_info.rotthrust.x;
			obj->mtype.phys_info.rotthrust.y = obj_rw->mtype.phys_info.rotthrust.y;
			obj->mtype.phys_info.rotthrust.z = obj_rw->mtype.phys_info.rotthrust.z;
			obj->mtype.phys_info.turnroll    = obj_rw->mtype.phys_info.turnroll;
			obj->mtype.phys_info.flags       = obj_rw->mtype.phys_info.flags;
			break;
			
		case MT_SPINNING:
			obj->mtype.spin_rate.x = obj_rw->mtype.spin_rate.x;
			obj->mtype.spin_rate.y = obj_rw->mtype.spin_rate.y;
			obj->mtype.spin_rate.z = obj_rw->mtype.spin_rate.z;
			break;
	}
	
	switch (obj->control_type)
	{
		case CT_WEAPON:
			obj->ctype.laser_info.parent_type      = obj_rw->ctype.laser_info.parent_type;
			obj->ctype.laser_info.parent_num       = obj_rw->ctype.laser_info.parent_num;
			obj->ctype.laser_info.parent_signature = obj_rw->ctype.laser_info.parent_signature;
			obj->ctype.laser_info.creation_time    = obj_rw->ctype.laser_info.creation_time;
			obj->ctype.laser_info.last_hitobj      = obj_rw->ctype.laser_info.last_hitobj;
			obj->ctype.laser_info.track_goal       = obj_rw->ctype.laser_info.track_goal;
			obj->ctype.laser_info.multiplier       = obj_rw->ctype.laser_info.multiplier;
			break;
			
		case CT_EXPLOSION:
			obj->ctype.expl_info.spawn_time    = obj_rw->ctype.expl_info.spawn_time;
			obj->ctype.expl_info.delete_time   = obj_rw->ctype.expl_info.delete_time;
			obj->ctype.expl_info.delete_objnum = obj_rw->ctype.expl_info.delete_objnum;
			obj->ctype.expl_info.attach_parent = obj_rw->ctype.expl_info.attach_parent;
			obj->ctype.expl_info.prev_attach   = obj_rw->ctype.expl_info.prev_attach;
			obj->ctype.expl_info.next_attach   = obj_rw->ctype.expl_info.next_attach;
			break;
			
		case CT_AI:
		{
			int i;
			obj->ctype.ai_info.behavior               = obj_rw->ctype.ai_info.behavior; 
			for (i = 0; i < MAX_AI_FLAGS; i++)
				obj->ctype.ai_info.flags[i]       = obj_rw->ctype.ai_info.flags[i]; 
			obj->ctype.ai_info.hide_segment           = obj_rw->ctype.ai_info.hide_segment;
			obj->ctype.ai_info.hide_index             = obj_rw->ctype.ai_info.hide_index;
			obj->ctype.ai_info.path_length            = obj_rw->ctype.ai_info.path_length;
			obj->ctype.ai_info.cur_path_index         = obj_rw->ctype.ai_info.cur_path_index;
			obj->ctype.ai_info.dying_sound_playing    = obj_rw->ctype.ai_info.dying_sound_playing;
			obj->ctype.ai_info.danger_laser_num       = obj_rw->ctype.ai_info.danger_laser_num;
			obj->ctype.ai_info.danger_laser_signature = obj_rw->ctype.ai_info.danger_laser_signature;
			obj->ctype.ai_info.dying_start_time       = obj_rw->ctype.ai_info.dying_start_time;
			break;
		}
			
		case CT_LIGHT:
			obj->ctype.light_info.intensity = obj_rw->ctype.light_info.intensity;
			break;
			
		case CT_POWERUP:
			obj->ctype.powerup_info.count         = obj_rw->ctype.powerup_info.count;
			obj->ctype.powerup_info.creation_time = obj_rw->ctype.powerup_info.creation_time;
			obj->ctype.powerup_info.flags         = obj_rw->ctype.powerup_info.flags;
			break;
	}
	
	switch (obj->render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj->render_type == RT_NONE && obj->type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time.
				break;
			obj->rtype.pobj_info.model_num                = obj_rw->rtype.pobj_info.model_num;
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj->rtype.pobj_info.anim_angles[i].p = obj_rw->rtype.pobj_info.anim_angles[i].p;
				obj->rtype.pobj_info.anim_angles[i].b = obj_rw->rtype.pobj_info.anim_angles[i].b;
				obj->rtype.pobj_info.anim_angles[i].h = obj_rw->rtype.pobj_info.anim_angles[i].h;
			}
			obj->rtype.pobj_info.subobj_flags             = obj_rw->rtype.pobj_info.subobj_flags;
			obj->rtype.pobj_info.tmap_override            = obj_rw->rtype.pobj_info.tmap_override;
			obj->rtype.pobj_info.alt_textures             = obj_rw->rtype.pobj_info.alt_textures;
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj->rtype.vclip_info.vclip_num = obj_rw->rtype.vclip_info.vclip_num;
			obj->rtype.vclip_info.frametime = obj_rw->rtype.vclip_info.frametime;
			obj->rtype.vclip_info.framenum  = obj_rw->rtype.vclip_info.framenum;
			break;
			
		case RT_LASER:
			break;
			
	}
}

