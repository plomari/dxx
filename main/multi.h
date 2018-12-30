/* $Id: multi.h,v 1.1.1.1 2006/03/17 19:56:09 zicodxx Exp $ */
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
 * Defines and exported variables for multi.c
 *
 */

#ifndef _MULTI_H
#define _MULTI_H

#include "gameseq.h"
#include "piggy.h"
#include "vers_id.h"
#include "newmenu.h"

#ifdef USE_UDP
#ifdef _WIN32
#include <winsock.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#endif

#ifdef IPv6
#define _sockaddr sockaddr_in6
#define _af AF_INET6
#define _pf PF_INET6
#else
#define _sockaddr sockaddr_in
#define _af AF_INET
#define _pf PF_INET
#endif
#endif

// PROTOCOL VARIABLES AND DEFINES
extern int multi_protocol; // set and determinate used protocol
#define MULTI_PROTO_IPX 1 // IPX-type protocol (IPX, KALI)
#define MULTI_PROTO_UDP 2 // UDP protocol

// What version of the multiplayer protocol is this?
// Protocol versions:
//   1 Descent Shareware
//   2 Descent Registered/Commercial
//   3 Descent II Shareware
//   4 Descent II Commercial
#define MULTI_PROTO_VERSION 4
// this define should become obsolete due to strict version checking in UDP
#define MULTI_PROTO_D2X_VER	7  // Increment everytime we change networking features - based on ubyte, must never be > 255
// PROTOCOL VARIABLES AND DEFINES - END


#define MAX_MESSAGE_LEN 35
#define MAX_NUM_NET_PLAYERS     8 // How many simultaneous network players do we support?

#define MULTI_POSITION          0
#define MULTI_REAPPEAR          1
#define MULTI_FIRE              2
#define MULTI_KILL              3
#define MULTI_REMOVE_OBJECT     4
#define MULTI_PLAYER_EXPLODE    5
#define MULTI_MESSAGE           6
#define MULTI_QUIT              7
#define MULTI_PLAY_SOUND        8
#define MULTI_BEGIN_SYNC        9
#define MULTI_CONTROLCEN        10
#define MULTI_ROBOT_CLAIM       11
#define MULTI_END_SYNC          12
#define MULTI_CLOAK             13
#define MULTI_ENDLEVEL_START    14
#define MULTI_DOOR_OPEN         15
#define MULTI_CREATE_EXPLOSION  16
#define MULTI_CONTROLCEN_FIRE   17
#define MULTI_PLAYER_DROP       18
#define MULTI_CREATE_POWERUP    19
#define MULTI_CONSISTENCY       20
#define MULTI_DECLOAK           21
#define MULTI_MENU_CHOICE       22
#define MULTI_ROBOT_POSITION    23
#define MULTI_ROBOT_EXPLODE     24
#define MULTI_ROBOT_RELEASE     25
#define MULTI_ROBOT_FIRE        26
#define MULTI_SCORE             27
#define MULTI_CREATE_ROBOT      28
#define MULTI_TRIGGER           29
#define MULTI_BOSS_ACTIONS      30
#define MULTI_CREATE_ROBOT_POWERUPS 31
#define MULTI_HOSTAGE_DOOR      32

#define MULTI_SAVE_GAME         33 // obsolete
#define MULTI_RESTORE_GAME      34 // obsolete

#define MULTI_REQ_PLAYER        35  // Someone requests my player structure
#define MULTI_SEND_PLAYER       36  // Sending someone my player structure
#define MULTI_MARKER            37
#define MULTI_DROP_WEAPON       38
#define MULTI_GUIDED            39
#define MULTI_STOLEN_ITEMS      40
#define MULTI_WALL_STATUS       41  // send to new players
#define MULTI_HEARTBEAT         42
#define MULTI_KILLGOALS         43
#define MULTI_SEISMIC           44
#define MULTI_LIGHT             45
#define MULTI_START_TRIGGER     46
#define MULTI_FLAGS             47
#define MULTI_DROP_BLOB         48
#define MULTI_POWERUP_UPDATE    49
#define MULTI_ACTIVE_DOOR       50
#define MULTI_SOUND_FUNCTION    51
#define MULTI_CAPTURE_BONUS     52
#define MULTI_GOT_FLAG          53
#define MULTI_DROP_FLAG         54
#define MULTI_ROBOT_CONTROLS    55 // unused!
#define MULTI_FINISH_GAME       56
#define MULTI_RANK              57
#define MULTI_MODEM_PING        58
#define MULTI_MODEM_PING_RETURN 59
#define MULTI_ORB_BONUS         60
#define MULTI_GOT_ORB           61
#define MULTI_DROP_ORB          62
#define MULTI_PLAY_BY_PLAY      63

#define MULTI_MAX_TYPE          63

#define MAX_NET_CREATE_OBJECTS  40

#define MAX_MULTI_MESSAGE_LEN   120

#define NETGAME_ANARCHY         0
#define NETGAME_TEAM_ANARCHY    1
#define NETGAME_ROBOT_ANARCHY   2
#define NETGAME_COOPERATIVE     3
#define NETGAME_CAPTURE_FLAG    4
#define NETGAME_HOARD           5
#define NETGAME_TEAM_HOARD      6

#define NETSTAT_MENU                0
#define NETSTAT_PLAYING             1
#define NETSTAT_BROWSING            2
#define NETSTAT_WAITING             3
#define NETSTAT_STARTING            4
#define NETSTAT_ENDLEVEL            5

#define CONNECT_DISCONNECTED        0
#define CONNECT_PLAYING             1
#define CONNECT_WAITING             2
#define CONNECT_DIED_IN_MINE        3
#define CONNECT_FOUND_SECRET        4
#define CONNECT_ESCAPE_TUNNEL       5
#define CONNECT_END_MENU            6
#define CONNECT_KMATRIX_WAITING     7 // Like CONNECT_WAITING but used especially in kmatrix.c to seperate "escaped" and "waiting"

// reasons for a packet with type PID_DUMP
#define DUMP_CLOSED     0 // no new players allowed after game started
#define DUMP_FULL       1 // player cound maxed out
#define DUMP_ENDLEVEL   2
#define DUMP_DORK       3
#define DUMP_ABORTED    4
#define DUMP_CONNECTED  5 // never used
#define DUMP_LEVEL      6
#define DUMP_KICKED     7

// Bitmask for netgame_info->AllowedItems to set allowed items in Netgame
#define NETFLAG_DOLASER   		1
#define NETFLAG_DOSUPERLASER	2
#define NETFLAG_DOQUAD    		4
#define NETFLAG_DOVULCAN		8
#define NETFLAG_DOGAUSS			16
#define NETFLAG_DOSPREAD		32
#define NETFLAG_DOHELIX			64
#define NETFLAG_DOPLASMA		128
#define NETFLAG_DOPHOENIX		256
#define NETFLAG_DOFUSION		512
#define NETFLAG_DOOMEGA			1024
#define NETFLAG_DOFLASH			2048
#define NETFLAG_DOHOMING		4096
#define NETFLAG_DOGUIDED		8192
#define NETFLAG_DOPROXIM		16384
#define NETFLAG_DOSMARTMINE		32768
#define NETFLAG_DOSMART			65536
#define NETFLAG_DOMERCURY		131072
#define NETFLAG_DOMEGA			262144
#define NETFLAG_DOSHAKER		524288
#define NETFLAG_DOCLOAK			1048576
#define NETFLAG_DOINVUL			2097152
#define NETFLAG_DOAFTERBURNER	4194304
#define NETFLAG_DOAMMORACK		8388608
#define NETFLAG_DOCONVERTER		16777216
#define NETFLAG_DOHEADLIGHT		33554432
#define NETFLAG_DOPOWERUP		67108863  // mask for all powerup flags

#define MULTI_ALLOW_POWERUP_MAX 26
extern char *multi_allow_powerup_text[MULTI_ALLOW_POWERUP_MAX];

// Exported functions

extern int GetMyNetRanking();
extern void ClipRank (ubyte *rank);
int objnum_remote_to_local(int remote_obj, int owner);
int objnum_local_to_remote(int local_obj, sbyte *owner);
void map_objnum_local_to_remote(int local, int remote, int owner);
void map_objnum_local_to_local(int objnum);
void reset_network_objects();
int multi_objnum_is_past(int objnum);
void multi_do_ping_frame();

void multi_init_objects(void);
void multi_show_player_list(void);
void multi_do_protocol_frame(int force, int listen);
void multi_do_frame(void);


void multi_send_flags(char);
void multi_send_fire(void);
void multi_send_destroy_controlcen(int objnum, int player);
void multi_send_endlevel_start(int);
void multi_send_player_explode(char type);
void multi_send_message(void);
void multi_send_position(int objnum);
void multi_send_reappear();
void multi_send_kill(int objnum);
void multi_send_remobj(int objnum);
void multi_send_quit(int why);
void multi_send_door_open(int segnum, int side,ubyte flag);
void multi_send_create_explosion(int player_num);
void multi_send_controlcen_fire(vms_vector *to_target, int gun_num, int objnum);
void multi_send_cloak(void);
void multi_send_decloak(void);
void multi_send_create_powerup(int powerup_type, int segnum, int objnum, vms_vector *pos);
void multi_send_play_sound(int sound_num, fix volume);
void multi_send_audio_taunt(int taunt_num);
void multi_send_score(void);
void multi_send_trigger(int trigger);
void multi_send_hostage_door_status(int wallnum);
void multi_send_netplayer_stats_request(ubyte player_num);
void multi_send_drop_weapon (int objnum,int seed);
void multi_send_drop_marker (int player,vms_vector position,char messagenum,char text[]);
void multi_send_guided_info (object *miss,char);


void multi_endlevel_score(void);
void multi_consistency_error(int reset);
void multi_prep_level(void);
int multi_level_sync(void);
int multi_endlevel(int *secret);
int multi_endlevel_poll1(newmenu *menu, d_event *event, void *userdata);
int multi_endlevel_poll2( newmenu *menu, d_event *event, void *userdata );
void multi_send_endlevel_packet();
void multi_leave_game(void);
void multi_process_data(char *dat, int len);
void multi_process_bigdata(char *buf, int len);
void multi_do_death(int objnum);
void multi_send_message_dialog(void);
int multi_delete_extra_objects(void);
void multi_make_ghost_player(int objnum);
void multi_make_player_ghost(int objnum);
void multi_define_macro(int key);
void multi_send_macro(int key);
int multi_get_kill_list(int *plist);
void multi_new_game(void);
void multi_sort_kill_list(void);
void multi_reset_stuff(void);
void multi_send_data(char *buf, int len, int priority);
int get_team(int pnum);


// Exported variables

extern int PacketUrgent;
extern int Network_status;
extern int Network_laser_gun;
extern int Network_laser_fired;
extern int Network_laser_level;
extern int Network_laser_flags;

// IMPORTANT: These variables needed for player rejoining done by protocol-specific code
extern int Network_send_objects;
extern int Network_send_object_mode;
extern int Network_send_objnum;
extern int Network_rejoined;
extern int Network_new_game;
extern int Network_sending_extras;
extern int Player_joining_extras;
extern int Network_player_added;

extern int message_length[MULTI_MAX_TYPE+1];
extern char multibuf[MAX_MULTI_MESSAGE_LEN+4];
extern short Network_laser_track;

extern int who_killed_controlcen;

extern int Net_create_objnums[MAX_NET_CREATE_OBJECTS];
extern int Net_create_loc;

extern short kill_matrix[MAX_NUM_NET_PLAYERS][MAX_NUM_NET_PLAYERS];
extern short team_kills[2];

extern int multi_goto_secret;

extern ushort my_segments_checksum;

//do we draw the kill list on the HUD?
extern int Show_kill_list;
extern int Show_reticle_name;
extern fix Show_kill_list_timer;

// Used to send network messages

extern char Network_message[MAX_MESSAGE_LEN];
extern int Network_message_reciever;

// Which player 'owns' each local object for network purposes
extern sbyte object_owner[MAX_OBJECTS];

extern int multi_quit_game;

extern int multi_sending_message;
extern int multi_defining_message;
extern int multi_message_input_sub(int key);
extern void multi_send_message_start();

extern int multi_powerup_is_4pack(int );
extern void multi_send_orb_bonus( char pnum );
extern void multi_send_got_orb( char pnum );
extern void multi_add_lifetime_kills(void);

extern int PhallicLimit,PhallicMan;

#define N_PLAYER_SHIP_TEXTURES 6

extern bitmap_index multi_player_textures[MAX_NUM_NET_PLAYERS][N_PLAYER_SHIP_TEXTURES];

extern char *RankStrings[];

// Globals for protocol-bound Refuse-functions
extern char RefuseThisPlayer,WaitForRefuseAnswer,RefuseTeam,RefusePlayerName[12];
extern fix RefuseTimeLimit;
#define REFUSE_INTERVAL (F1_0*8)

#define NETGAME_FLAG_CLOSED             1
#define NETGAME_FLAG_SHOW_ID            2
#define NETGAME_FLAG_SHOW_MAP           4
#define NETGAME_FLAG_HOARD              8
#define NETGAME_FLAG_TEAM_HOARD         16
#define NETGAME_FLAG_REALLY_ENDLEVEL    32
#define NETGAME_FLAG_REALLY_FORMING     64

#define NETGAME_NAME_LEN                15
#define NETGAME_AUX_SIZE                20  // Amount of extra data for the network protocol to store in the netgame packet

extern struct netgame_info Netgame;
extern struct AllNetPlayers_info NetPlayers;

int multi_i_am_master(void);
int multi_who_is_master(void);
void change_playernum_to(int new_pnum);

//how to encode missiles & flares in weapon packets
#define MISSILE_ADJUST  100
#define FLARE_ADJUST    127

int HoardEquipped();
#ifdef EDITOR
void save_hoard_data(void);
#endif

/*
 * The Network Players structure
 * Contains both IPX- and UDP-specific data with designated prefixes and general player-related data.
 * Note that not all of these infos will be sent to other users - some are used and/or set locally, only.
 * Even if some variables are not UDP-specific, they might be used in IPX as well to maintain backwards compability.
 */
typedef struct netplayer_info
{
#if defined(USE_UDP) || defined(USE_IPX)
	union
	{
#ifdef USE_IPX
		struct
		{
			ubyte				server[4];
			ubyte				node[6];
			ushort				socket;
			ubyte				computer_type; // {DOS,WIN_32,WIN_95,MAC}
		} ipx;
#endif
#ifdef USE_UDP
		struct
		{
			struct _sockaddr	addr; // IP address of this peer
			ubyte				isyou; // This flag is set true while sending info to tell player his designated (re)join position
		} udp;
#endif
	} protocol;	
#endif
	char						callsign[CALLSIGN_LEN+1];
	ubyte						version_major;
	ubyte						version_minor;
	sbyte						connected;
	ubyte						rank;
	fix							ping;
	fix							LastPacketTime;
} __pack__ netplayer_info;

/*
 * The Network Game structure
 * Contains both IPX- and UDP-specific data with designated prefixes and general game-related data.
 * Note that not all of these infos will be sent to clients - some are used and/or set locally, only.
 * Even if some variables are not UDP-specific, they might be used in IPX as well to maintain backwards compability.
 */
typedef struct netgame_info
{
#if defined(USE_UDP) || defined(USE_IPX)
	union
	{
#ifdef USE_IPX
		struct
		{
			ubyte				Game_pkt_type;
			int     			Game_Security;
			ubyte				Player_pkt_type;
			int					Player_Security;
			ubyte   			protocol_version;
			ubyte   			ShortPackets;
		} ipx;
#endif
#ifdef USE_UDP
		struct
		{
			struct _sockaddr	addr; // IP address of this netgame's host
			short				program_iver[3]; // IVER of program for version checking
			sbyte				valid; // Status of Netgame info: -1 = Failed, Wrong version; 0 = No info, yet; 1 = Success
		} udp;
#endif
	} protocol;	
#endif
	ubyte			   			version_major; // Game content data version major
	ubyte   					version_minor; // Game content data version minor
	struct netplayer_info 		players[MAX_PLAYERS+4];
	char    					game_name[NETGAME_NAME_LEN+1];
	char    					mission_title[MISSION_NAME_LEN+1];
	char    					mission_name[9];
	int     					levelnum;
	ubyte   					gamemode;
	ubyte   					RefusePlayers;
	ubyte   					difficulty;
	ubyte   					game_status;
	ubyte   					numplayers;
	ubyte   					max_numplayers;
	ubyte   					numconnected;
	ubyte   					game_flags;
	ubyte   					team_vector;
	u_int32_t					AllowedItems;
	short						Allow_marker_view;
	short						AlwaysLighting;
	short						ShowAllNames;
	short						BrightPlayers;
	short						InvulAppear;
	char						team_name[2][CALLSIGN_LEN+1];
	int							locations[MAX_PLAYERS];
	short						kills[MAX_PLAYERS][MAX_PLAYERS];
	ushort						segments_checksum;
	short						team_kills[2];
	short						killed[MAX_PLAYERS];
	short						player_kills[MAX_PLAYERS];
	int							KillGoal;
	fix							PlayTimeAllowed;
	fix							level_time;
	int							control_invul_time;
	int							monitor_vector;
	int							player_score[MAX_PLAYERS];
	ubyte						player_flags[MAX_PLAYERS];
	short						PacketsPerSec;
	ubyte						PacketLossPrevention;
} __pack__ netgame_info;
#endif /* _MULTI_H */
