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
 * Inferno main menu.
 *
 */

#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "key.h"
#include "iff.h"
#include "u_mem.h"
#include "error.h"
#include "bm.h"
#include "screens.h"
#include "joy.h"
#include "vecmat.h"
#include "effects.h"
#include "slew.h"
#include "gamemine.h"
#include "gamesave.h"
#include "palette.h"
#include "args.h"
#include "newdemo.h"
#include "timer.h"
#include "sounds.h"
#include "gameseq.h"
#include "text.h"
#include "gamefont.h"
#include "newmenu.h"
#include "scores.h"
#include "playsave.h"
#include "kconfig.h"
#include "titles.h"
#include "credits.h"
#include "texmap.h"
#include "polyobj.h"
#include "state.h"
#include "mission.h"
#include "songs.h"
#ifdef USE_SDLMIXER
#include "jukebox.h" // for jukebox_exts
#endif
#include "config.h"
#include "movie.h"
#include "gamepal.h"
#include "gauges.h"
#include "powerup.h"
#include "strutil.h"
#include "multi.h"
#ifdef USE_IPX
#include "net_ipx.h"
#endif
#ifdef USE_UDP
#include "net_udp.h"
#endif
#ifdef EDITOR
#include "editor/editor.h"
#endif


// Menu IDs...
enum MENUS
{
    MENU_NEW_GAME = 0,
    MENU_GAME,
    MENU_EDITOR,
    MENU_VIEW_SCORES,
    MENU_QUIT,
    MENU_LOAD_GAME,
    MENU_SAVE_GAME,
    MENU_DEMO_PLAY,
    MENU_LOAD_LEVEL,
    MENU_CONFIG,
    MENU_REJOIN_NETGAME,
    MENU_DIFFICULTY,
    MENU_HELP,
    MENU_NEW_PLAYER,
    #if defined(USE_UDP) || defined (USE_IPX)
        MENU_MULTIPLAYER,
    #endif

    MENU_SHOW_CREDITS,
    MENU_ORDER_INFO,

    #ifdef USE_UDP
    MENU_START_UDP_NETGAME,
    MENU_JOIN_MANUAL_UDP_NETGAME,
    MENU_JOIN_LIST_UDP_NETGAME,
    #endif
    #ifdef USE_IPX
    MENU_START_IPX_NETGAME,
    MENU_JOIN_IPX_NETGAME,
    MENU_START_KALI_NETGAME, // xKali support (not Windows Kali! Windows Kali is over IPX!)
    MENU_JOIN_KALI_NETGAME,
    #endif
};

//ADD_ITEM("Start netgame...", MENU_START_NETGAME, -1 );
//ADD_ITEM("Send net message...", MENU_SEND_NET_MESSAGE, -1 );

#define ADD_ITEM(t,value,key)  do { m[num_options].type=NM_TYPE_MENU; m[num_options].text=t; menu_choice[num_options]=value;num_options++; } while (0)

static window *menus[16] = { NULL };

// Function Prototypes added after LINTING
int do_option(int select);
int do_new_game_menu(void);
int do_load_level_menu(void);
void do_multi_player_menu();
extern void newmenu_free_background();
extern void ReorderPrimary();
extern void ReorderSecondary();

// Hide all menus
int hide_menus(void)
{
	window *wind;
	int i;

	if (menus[0])
		return 0;		// there are already hidden menus

	for (i = 0; (i < 15) && (wind = window_get_front()); i++)
	{
		menus[i] = wind;
		window_set_visible(wind, 0);
	}

	Assert(window_get_front() == NULL);
	menus[i] = NULL;

	return 1;
}

// Show all menus, with the front one shown first
// This makes sure EVENT_WINDOW_ACTIVATED is only sent to that window
void show_menus(void)
{
	int i;

	for (i = 0; (i < 16) && menus[i]; i++)
		if (window_exists(menus[i]))
			window_set_visible(menus[i], 1);

	menus[0] = NULL;
}

//pairs of chars describing ranges
char playername_allowed_chars[] = "azAZ09__--";

int MakeNewPlayerFile(int allow_abort)
{
	int x;
	char filename[14];
	newmenu_item m;
	char text[CALLSIGN_LEN+9]="";

	strncpy(text, Players[Player_num].callsign,CALLSIGN_LEN);

try_again:
	m.type=NM_TYPE_INPUT; m.text_len = CALLSIGN_LEN; m.text = text;

	Newmenu_allowed_chars = playername_allowed_chars;
	x = newmenu_do( NULL, TXT_ENTER_PILOT_NAME, 1, &m, NULL, NULL );
	Newmenu_allowed_chars = NULL;

	if ( x < 0 ) {
		if ( allow_abort ) return 0;
		goto try_again;
	}

	if (text[0]==0)	//null string
		goto try_again;

	strlwr(text);

	sprintf( filename, GameArg.SysUsePlayersDir? "Players/%s.plr" : "%s.plr", text );

	if (PHYSFS_exists(filename))
	{
		nm_messagebox(NULL, 1, TXT_OK, "%s '%s' %s", TXT_PLAYER, text, TXT_ALREADY_EXISTS );
		goto try_again;
	}

	if ( !new_player_config() )
		goto try_again;			// They hit Esc during New player config

	strncpy(Players[Player_num].callsign, text, CALLSIGN_LEN);
	strlwr(Players[Player_num].callsign);

	write_player_file();

	return 1;
}

void delete_player_saved_games(char * name);

int player_menu_keycommand( listbox *lb, d_event *event )
{
	char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	switch (((d_event_keycommand *)event)->keycode)
	{
		case KEY_CTRLED+KEY_D:
			if (citem > 0)
			{
				int x = 1;
				x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, items[citem]+((items[citem][0]=='$')?1:0) );
				if (x==0)	{
					char * p;
					char plxfile[PATH_MAX], efffile[PATH_MAX];
					int ret;
					char name[PATH_MAX];

					p = items[citem] + strlen(items[citem]);
					*p = '.';

					strcpy(name, GameArg.SysUsePlayersDir ? "Players/" : "");
					strcat(name, items[citem]);

					ret = !PHYSFS_delete(name);
					*p = 0;

					if (!ret)
					{
						delete_player_saved_games( items[citem] );
						// delete PLX file
						sprintf(plxfile, GameArg.SysUsePlayersDir? "Players/%.8s.plx" : "%.8s.plx", items[citem]);
						if (cfexist(plxfile))
							PHYSFS_delete(plxfile);
						// delete EFF file
						sprintf(efffile, GameArg.SysUsePlayersDir? "Players/%.8s.eff" : "%.8s.eff", items[citem]);
						if (cfexist(efffile))
							PHYSFS_delete(efffile);
					}

					if (ret)
						nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, items[citem]+((items[citem][0]=='$')?1:0) );
					else
						listbox_delete_item(lb, citem);
				}

				return 1;
			}
			break;
	}

	return 0;
}

int player_menu_handler( listbox *lb, d_event *event, char **list )
{
	char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			return player_menu_keycommand(lb, event);
			break;

		case EVENT_NEWMENU_SELECTED:
			if (citem < 0)
				return 0;		// shouldn't happen
			else if (citem == 0)
			{
				// They selected 'create new pilot'
				return !MakeNewPlayerFile(1);
			}
			else
			{
				strncpy(Players[Player_num].callsign,items[citem] + ((items[citem][0]=='$')?1:0), CALLSIGN_LEN);
				strlwr(Players[Player_num].callsign);
			}
			break;

		case EVENT_WINDOW_CLOSE:
			if (read_player_file() != EZERO)
				return 1;		// abort close!

			WriteConfigFile();		// Update lastplr

			PHYSFS_freeList(list);
			d_free(items);
			break;

		default:
			break;
	}

	return 0;
}

//Inputs the player's name, without putting up the background screen
int RegisterPlayer(int at_program_start)
{
	char **m;
	char **f;
	char **list;
	char *types[] = { ".plr", NULL };
	int i = 0, NumItems;
	int citem = 0;
	int allow_abort_flag = 1;

	if ( Players[Player_num].callsign[0] == 0 )
	{
		if (GameCfg.LastPlayer[0]==0)
		{
			strncpy( Players[Player_num].callsign, "player", CALLSIGN_LEN );
			allow_abort_flag = 0;
		}
		else
		{
			// Read the last player's name from config file, not lastplr.txt
			strncpy( Players[Player_num].callsign, GameCfg.LastPlayer, CALLSIGN_LEN );
		}
	}

	if (at_program_start && !(GameCfg.LastPlayer[0]==0)) {
		snprintf(Players[Player_num].callsign, CALLSIGN_LEN, "%s", GameCfg.LastPlayer);
		strlwr(Players[Player_num].callsign);
		return 1;
	}

	list = PHYSFSX_findFiles(GameArg.SysUsePlayersDir ? "Players/" : "", types);
	if (!list)
		return 0;	// memory error
	if (!*list)
	{
		MakeNewPlayerFile(0);	// make a new player without showing listbox
		PHYSFS_freeList(list);
		return 0;
	}


	for (NumItems = 0; list[NumItems] != NULL; NumItems++) {}
	NumItems++;		// for TXT_CREATE_NEW

	MALLOC(m, char *, NumItems);
	if (m == NULL)
	{
		PHYSFS_freeList(list);
		return 0;
	}

	m[i++] = TXT_CREATE_NEW;

	for (f = list; *f != NULL; f++)
	{
		char *p;

		m[i++] = *f;
		p = strchr(*f, '.');
		if (p)
			*p = '\0';		// chop the .plr
		if ((p - *f) > 8)
			*f[8] = 0;		// sorry guys, can only have up to eight chars for the player name
	}

	// Sort by name, except the <Create New Player> string
	qsort(&m[1], NumItems - 1, sizeof(char *), (int (*)( const void *, const void * ))string_array_sort_func);

	for ( i=0; i<NumItems; i++ )
		if (!stricmp(Players[Player_num].callsign, m[i]) )
			citem = i;

	newmenu_listbox1(TXT_SELECT_PILOT, NumItems, m, allow_abort_flag, citem, (int (*)(listbox *, d_event *, void *))player_menu_handler, list);

	return 1;
}

extern ubyte Version_major,Version_minor;

// Draw Copyright and Version strings
void draw_copyright()
{
	int w,h,aw;

	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(6,6,6),-1);
	gr_printf(0x8000,SHEIGHT-LINE_SPACING,TXT_COPYRIGHT);

	gr_get_string_size("V2.2", &w, &h, &aw );
	gr_printf(SWIDTH-w-FSPACX(1),SHEIGHT-LINE_SPACING,"V%d.%d",Version_major,Version_minor);

	gr_set_fontcolor( BM_XRGB(25,0,0), -1);
	gr_printf(0x8000,SHEIGHT-(LINE_SPACING*2),DESCENT_VERSION);
}

//returns the number of demo files on the disk
int newdemo_count_demos();

// ------------------------------------------------------------------------
int main_menu_handler(newmenu *menu, d_event *event, int *menu_choice )
{
	int curtime;
	newmenu_item *items = newmenu_get_items(menu);

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			load_palette(MENU_PALETTE,0,1);		//get correct palette

			if ( Players[Player_num].callsign[0]==0 )
				RegisterPlayer(1);
			else
				keyd_time_when_last_pressed = timer_get_fixed_seconds();		// .. 20 seconds from now!
			break;

		case EVENT_KEY_COMMAND:
			// Don't allow them to hit ESC in the main menu.
			if (((d_event_keycommand *)event)->keycode==KEY_ESC)
				return 1;
			break;

		case EVENT_IDLE:
			curtime = timer_get_fixed_seconds();
			if ( keyd_time_when_last_pressed+i2f(25) < curtime && GameArg.SysAutoDemo  )
			{
				int n_demos;
				n_demos = newdemo_count_demos();

			try_again:;
				if (((d_rand() % (n_demos+1)) == 0) && !GameArg.SysAutoDemo)
				{
#ifndef SHAREWARE
#ifdef OGL
					Screen_mode = -1;
#endif
					init_subtitles("intro.tex");
					PlayMovie("intro.mve",0);
					close_subtitles();
					songs_play_song(SONG_TITLE,1);
					set_screen_mode(SCREEN_MENU);
#endif // end of ifndef shareware
				}
				else
				{
					if (curtime < 0) curtime = 0;
					keyd_time_when_last_pressed = curtime;			// Reset timer so that disk won't thrash if no demos.
					newdemo_start_playback(NULL);		// Randomly pick a file, assume native endian (crashes if not)
					if (Newdemo_state == ND_STATE_PLAYBACK)
						return 0;
					else
						goto try_again;	//keep trying until we get a demo that works
				}
			}
			break;

		case EVENT_NEWMENU_DRAW:
			draw_copyright();
			break;

		case EVENT_NEWMENU_SELECTED:
			return do_option(menu_choice[newmenu_get_citem(menu)]);
			break;

		case EVENT_WINDOW_CLOSE:
			d_free(menu_choice);
			d_free(items);
			break;

		default:
			break;
	}

	return 0;
}

//	-----------------------------------------------------------------------------
//	Create the main menu.
void create_main_menu(newmenu_item *m, int *menu_choice, int *callers_num_options)
{
	int	num_options;

	#ifndef DEMO_ONLY
	num_options = 0;

	ADD_ITEM(TXT_NEW_GAME,MENU_NEW_GAME,KEY_N);

	ADD_ITEM(TXT_LOAD_GAME,MENU_LOAD_GAME,KEY_L);
#if defined(USE_UDP) || defined(USE_IPX)
	ADD_ITEM(TXT_MULTIPLAYER_,MENU_MULTIPLAYER,-1);
#endif

	ADD_ITEM(TXT_OPTIONS_, MENU_CONFIG, -1 );
	ADD_ITEM(TXT_CHANGE_PILOTS,MENU_NEW_PLAYER,unused);
	ADD_ITEM(TXT_VIEW_DEMO,MENU_DEMO_PLAY,0);
	ADD_ITEM(TXT_VIEW_SCORES,MENU_VIEW_SCORES,KEY_V);
	if (cfexist("orderd2.pcx")) /* SHAREWARE */
		ADD_ITEM(TXT_ORDERING_INFO,MENU_ORDER_INFO,-1);
	ADD_ITEM(TXT_CREDITS,MENU_SHOW_CREDITS,-1);
	#endif
	ADD_ITEM(TXT_QUIT,MENU_QUIT,KEY_Q);

	#ifndef RELEASE
	if (!(Game_mode & GM_MULTI ))	{
		//m[num_options].type=NM_TYPE_TEXT;
		//m[num_options++].text=" Debug options:";

		ADD_ITEM("  Load level...",MENU_LOAD_LEVEL ,KEY_N);
		#ifdef EDITOR
		ADD_ITEM("  Editor", MENU_EDITOR, KEY_E);
		#endif
	}

	#endif

	*callers_num_options = num_options;
}

//returns number of item chosen
int DoMenu()
{
	int *menu_choice;
	newmenu_item *m;
	int num_options = 0;

	MALLOC(menu_choice, int, 25);
	if (!menu_choice)
		return -1;
	MALLOC(m, newmenu_item, 25);
	if (!m)
	{
		d_free(menu_choice);
		return -1;
	}

	memset(menu_choice, 0, sizeof(int)*25);
	memset(m, 0, sizeof(newmenu_item)*25);

	create_main_menu(m, menu_choice, &num_options); // may have to change, eg, maybe selected pilot and no save games.

	newmenu_do3( "", NULL, num_options, m, (int (*)(newmenu *, d_event *, void *))main_menu_handler, menu_choice, 0, Menu_pcx_name);

	return 0;
}

extern void show_order_form(void);	// John didn't want this in inferno.h so I just externed it.

//returns flag, true means quit menu
int do_option ( int select)
{
	switch (select) {
		case MENU_NEW_GAME:
			select_mission(0, "New Game\n\nSelect mission", do_new_game_menu);
			break;
		case MENU_GAME:
			break;
		case MENU_DEMO_PLAY:
			select_demo();
			break;
		case MENU_LOAD_GAME:
			state_restore_all(0, 0, NULL);
			break;
		#ifdef EDITOR
		case MENU_EDITOR:
			Function_mode = FMODE_EDITOR;

			create_new_mine();
			SetPlayerFromCurseg();
			load_palette(NULL,1,0);

			keyd_editor_mode = 1;
			hide_menus();
			editor();
			if ( Function_mode == FMODE_GAME ) {
				Game_mode = GM_EDITOR;
				editor_reset_stuff_on_level();
				N_players = 1;
			}
			else
				show_menus();
			break;
		#endif
		case MENU_VIEW_SCORES:
			scores_view(NULL, -1);
			break;
#if 1 //def SHAREWARE
		case MENU_ORDER_INFO:
			show_order_form();
			break;
#endif
		case MENU_QUIT:
			#ifdef EDITOR
			if (! SafetyCheck()) break;
			#endif
			return 0;

		case MENU_NEW_PLAYER:
			RegisterPlayer(0);
			break;

#ifndef RELEASE
		case MENU_LOAD_LEVEL:
			select_mission(0, "Load Level\n\nSelect mission", do_load_level_menu);
			break;

#endif //ifndef RELEASE


#ifdef USE_UDP
		case MENU_START_UDP_NETGAME:
			multi_protocol = MULTI_PROTO_UDP;
			select_mission(1, TXT_MULTI_MISSION, net_udp_setup_game);
			break;
		case MENU_JOIN_MANUAL_UDP_NETGAME:
			multi_protocol = MULTI_PROTO_UDP;
			net_udp_manual_join_game();
			break;
		case MENU_JOIN_LIST_UDP_NETGAME:
			multi_protocol = MULTI_PROTO_UDP;
			net_udp_list_join_game();
			break;
#endif
#ifdef USE_IPX
		case MENU_START_IPX_NETGAME:
			multi_protocol = MULTI_PROTO_IPX;
			ipxdrv_set(NETPROTO_IPX);
			select_mission(1, TXT_MULTI_MISSION, net_ipx_setup_game);
			break;
		case MENU_JOIN_IPX_NETGAME:
			multi_protocol = MULTI_PROTO_IPX;
			ipxdrv_set(NETPROTO_IPX);
			net_ipx_join_game();
			break;
		case MENU_START_KALI_NETGAME:
			multi_protocol = MULTI_PROTO_IPX;
			ipxdrv_set(NETPROTO_KALINIX);
			select_mission(1, TXT_MULTI_MISSION, net_ipx_setup_game);
			break;
		case MENU_JOIN_KALI_NETGAME:
			multi_protocol = MULTI_PROTO_IPX;
			ipxdrv_set(NETPROTO_KALINIX);
			net_ipx_join_game();
			break;
#endif
#if defined(USE_UDP) || defined(USE_IPX)
		case MENU_MULTIPLAYER:
			do_multi_player_menu();
			break;
#endif
		case MENU_CONFIG:
			do_options_menu();
			break;
		case MENU_SHOW_CREDITS:
			songs_stop_all();
			credits_show(NULL);
			break;
		default:
			Error("Unknown option %d in do_option",select);
			break;
	}

	return 1;		// stay in main menu unless quitting
}

void delete_player_saved_games(char * name)
{
	int i;
	char filename[FILENAME_LEN + 9];

	for (i=0;i<10; i++)
	{
		sprintf( filename, GameArg.SysUsePlayersDir? "Players/%s.sg%x" : "%s.sg%x", name, i );

		PHYSFS_delete(filename);
	}
}

int demo_menu_keycommand( listbox *lb, d_event *event )
{
	char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	switch (((d_event_keycommand *)event)->keycode)
	{
		case KEY_CTRLED+KEY_D:
			if (citem >= 0)
			{
				int x = 1;
				x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, items[citem]+((items[citem][0]=='$')?1:0) );
				if (x==0)
				{
					int ret;
					char name[PATH_MAX];

					strcpy(name, DEMO_DIR);
					strcat(name,items[citem]);

					ret = !PHYSFS_delete(name);

					if (ret)
						nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, items[citem]+((items[citem][0]=='$')?1:0) );
					else
						listbox_delete_item(lb, citem);
				}

				return 1;
			}
			break;

		case KEY_CTRLED+KEY_C:
			{
				int x = 1;
				char bakname[PATH_MAX];

				// Get backup name
				change_filename_extension(bakname, items[citem]+((items[citem][0]=='$')?1:0), DEMO_BACKUP_EXT);
				x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO,	"Are you sure you want to\n"
								  "swap the endianness of\n"
								  "%s? If the file is\n"
								  "already endian native, D1X\n"
								  "will likely crash. A backup\n"
								  "%s will be created", items[citem]+((items[citem][0]=='$')?1:0), bakname );
				if (!x)
					newdemo_swap_endian(items[citem]);

				return 1;
			}
			break;
	}

	return 0;
}

int demo_menu_handler( listbox *lb, d_event *event, void *userdata )
{
	char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	userdata = userdata;

	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			return demo_menu_keycommand(lb, event);
			break;

		case EVENT_NEWMENU_SELECTED:
			if (citem < 0)
				return 0;		// shouldn't happen

			newdemo_start_playback(items[citem]);
			return 1;		// stay in demo selector

		case EVENT_WINDOW_CLOSE:
			PHYSFS_freeList(items);
			break;

		default:
			break;
	}

	return 0;
}

int select_demo(void)
{
	char **list;
	char *types[] = { DEMO_EXT, NULL };
	int NumItems;

	list = PHYSFSX_findFiles(DEMO_DIR, types);
	if (!list)
		return 0;	// memory error
	if ( !*list )
	{
		nm_messagebox( NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
		PHYSFS_freeList(list);
		return 0;
	}

	for (NumItems = 0; list[NumItems] != NULL; NumItems++) {}

	// Sort by name
	qsort(list, NumItems, sizeof(char *), (int (*)( const void *, const void * ))string_array_sort_func);

	newmenu_listbox1(TXT_SELECT_DEMO, NumItems, list, 1, 0, demo_menu_handler, NULL);

	return 1;
}

int do_difficulty_menu()
{
	int s;
	newmenu_item m[5];

	m[0].type=NM_TYPE_MENU; m[0].text=MENU_DIFFICULTY_TEXT(0);
	m[1].type=NM_TYPE_MENU; m[1].text=MENU_DIFFICULTY_TEXT(1);
	m[2].type=NM_TYPE_MENU; m[2].text=MENU_DIFFICULTY_TEXT(2);
	m[3].type=NM_TYPE_MENU; m[3].text=MENU_DIFFICULTY_TEXT(3);
	m[4].type=NM_TYPE_MENU; m[4].text=MENU_DIFFICULTY_TEXT(4);

	s = newmenu_do1( NULL, TXT_DIFFICULTY_LEVEL, NDL, m, NULL, NULL, Difficulty_level);

	if (s > -1 )	{
		if (s != Difficulty_level)
		{
			PlayerCfg.DefaultDifficulty = s;
			write_player_file();
		}
		Difficulty_level = s;
		return 1;
	}
	return 0;
}

int do_new_game_menu()
{
	int new_level_num,player_highest_level;

	new_level_num = 1;

	player_highest_level = get_highest_level();

	if (player_highest_level > Last_level)
		player_highest_level = Last_level;
	if (player_highest_level > 1) {
		newmenu_item m[4];
		char info_text[80];
		char num_text[10];
		int choice;
		int n_items;

try_again:
		sprintf(info_text,"%s %d",TXT_START_ANY_LEVEL, player_highest_level);

		m[0].type=NM_TYPE_TEXT; m[0].text = info_text;
		m[1].type=NM_TYPE_INPUT; m[1].text_len = 10; m[1].text = num_text;
		n_items = 2;

                snprintf(num_text, sizeof(num_text), "%d", GameCfg.LastLevel);

		choice = newmenu_do( NULL, TXT_SELECT_START_LEV, n_items, m, NULL, NULL );

		if (choice==-1 || m[1].text[0]==0)
			return 0;

		new_level_num = atoi(m[1].text);

		if (!(new_level_num>0 && new_level_num<=player_highest_level)) {
			m[0].text = TXT_ENTER_TO_CONT;
			nm_messagebox( NULL, 1, TXT_OK, TXT_INVALID_LEVEL);
			goto try_again;
		}
	}

	Difficulty_level = PlayerCfg.DefaultDifficulty;

	if (!do_difficulty_menu())
		return 0;

        GameCfg.LastLevel = new_level_num;
	StartNewGame(new_level_num);

	return 1;	// exit mission listbox
}

int do_load_level_menu(void)
{
	newmenu_item m;
	char text[10]="";
	int new_level_num;

	m.type=NM_TYPE_INPUT; m.text_len = 10; m.text = text;

	newmenu_do( NULL, "Enter level to load", 1, &m, NULL, NULL );

	new_level_num = atoi(m.text);

	if (new_level_num!=0 && new_level_num>=Last_secret_level && new_level_num<=Last_level)  {
		StartNewGame(new_level_num);
		return 1;
	}

	return 0;
}

void do_sound_menu();
void input_config();
void change_res();
void do_graphics_menu();
void do_misc_menu();

int options_menuset(newmenu *menu, d_event *event, void *userdata)
{
	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			if ( newmenu_get_citem(menu)==4)
			{
				gr_palette_set_gamma(newmenu_get_items(menu)[4].value);
			}
			break;

		case EVENT_NEWMENU_SELECTED:
			switch(newmenu_get_citem(menu))
			{
				case  0: do_sound_menu();		break;
				case  2: input_config();		break;
				case  5: change_res();			break;
				case  6: do_graphics_menu();		break;
				case  8: ReorderPrimary();		break;
				case  9: ReorderSecondary();		break;
				case 10: do_misc_menu();		break;
			}
			return 1;	// stay in menu until escape
			break;

		case EVENT_WINDOW_CLOSE:
		{
			newmenu_item *items = newmenu_get_items(menu);
			d_free(items);
			write_player_file();
			break;
		}

		default:
			break;
	}

	userdata = userdata;		//kill warning

	return 0;
}

int gcd(int a, int b)
{
	if (!b)
		return a;

	return gcd(b, a%b);
}

void change_res()
{
	u_int32_t modes[50], new_mode = 0;
	int i = 0, mc = 0, num_presets = 0, citem = -1, opt_cval = -1, opt_cres = -1, opt_casp = -1, opt_fullscr = -1;

	num_presets = gr_list_modes( modes );

	newmenu_item m[50+8];
	char restext[50][12], crestext[12], casptext[12];

	for (i = 0; i <= num_presets-1; i++)
	{
		snprintf(restext[mc], sizeof(restext[mc]), "%ix%i", SM_W(modes[i]), SM_H(modes[i]));
		m[mc].type = NM_TYPE_RADIO;
		m[mc].text = restext[mc];
		m[mc].value = ((citem == -1) && (Game_screen_mode == modes[i]) && GameCfg.AspectY == SM_W(modes[i])/gcd(SM_W(modes[i]),SM_H(modes[i])) && GameCfg.AspectX == SM_H(modes[i])/gcd(SM_W(modes[i]),SM_H(modes[i])));
		m[mc].group = 0;
		if (m[mc].value)
			citem = mc;
		mc++;
	}

	m[mc].type = NM_TYPE_TEXT; m[mc].text = ""; mc++; // little space for overview
	// the fields for custom resolution and aspect
	opt_cval = mc;
	m[mc].type = NM_TYPE_RADIO; m[mc].text = "use custom values"; m[mc].value = (citem == -1); m[mc].group = 0; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "resolution:"; mc++;
	snprintf(crestext, sizeof(crestext), "%ix%i", SM_W(Game_screen_mode), SM_H(Game_screen_mode));
	opt_cres = mc;
	m[mc].type = NM_TYPE_INPUT; m[mc].text = crestext; m[mc].text_len = 11; modes[mc] = 0; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "aspect:"; mc++;
	opt_casp = mc;
	snprintf(casptext, sizeof(casptext), "%ix%i", GameCfg.AspectY, GameCfg.AspectX);
	m[mc].type = NM_TYPE_INPUT; m[mc].text = casptext; m[mc].text_len = 11; modes[mc] = 0; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = ""; mc++; // little space for overview
	// fullscreen
	opt_fullscr = mc;
	m[mc].type = NM_TYPE_CHECK; m[mc].text = "Fullscreen"; m[mc].value = gr_check_fullscreen(); mc++;

	// create the menu
	newmenu_do1(NULL, "Screen Resolution", mc, m, NULL, NULL, 0);

	// menu is done, now do what we need to do

	// check which resolution field was selected
	for (i = 0; i <= mc; i++)
		if ((m[i].type == NM_TYPE_RADIO) && (m[i].group==0) && (m[i].value == 1))
			break;

	// now check for fullscreen toggle and apply if necessary
	if (m[opt_fullscr].value != gr_check_fullscreen())
		gr_toggle_fullscreen();

	if (i == opt_cval) // set custom resolution and aspect
	{
		u_int32_t cmode = Game_screen_mode, casp = Game_screen_mode;

		if (!strchr(crestext, 'x'))
			return;

		cmode = SM(atoi(crestext), atoi(strchr(crestext, 'x')+1));
		if (SM_W(cmode) < 320 || SM_H(cmode) < 200) // oh oh - the resolution is too small. Revert!
		{
			nm_messagebox( TXT_WARNING, 1, "OK", "Entered resolution is too small.\nReverting ..." );
			cmode = new_mode;
		}

		casp = cmode;
		if (strchr(casptext, 'x')) // we even have a custom aspect set up
		{
			casp = SM(atoi(casptext), atoi(strchr(casptext, 'x')+1));
		}
		GameCfg.AspectY = SM_W(casp)/gcd(SM_W(casp),SM_H(casp));
		GameCfg.AspectX = SM_H(casp)/gcd(SM_W(casp),SM_H(casp));
		new_mode = cmode;
	}
	else if (i >= 0 && i < num_presets) // set preset resolution
	{
		new_mode = modes[i];
		GameCfg.AspectY = SM_W(new_mode)/gcd(SM_W(new_mode),SM_H(new_mode));
		GameCfg.AspectX = SM_H(new_mode)/gcd(SM_W(new_mode),SM_H(new_mode));
	}

	// clean up and apply everything
	newmenu_free_background();
	set_screen_mode(SCREEN_MENU);
	if (new_mode != Game_screen_mode)
		gr_set_mode(new_mode);
	Game_screen_mode = new_mode;
}

int input_menuset(newmenu *menu, d_event *event, void *userdata)
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	userdata = userdata;

	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			switch (citem)
			{
				case 0:		(items[citem].value)?(PlayerCfg.ControlType|=CONTROL_USING_JOYSTICK):(PlayerCfg.ControlType&=~CONTROL_USING_JOYSTICK); break;
				case 1:		(items[citem].value)?(PlayerCfg.ControlType|=CONTROL_USING_MOUSE):(PlayerCfg.ControlType&=~CONTROL_USING_MOUSE); break;
				case 9:		PlayerCfg.JoystickSensitivityX = items[citem].value; break;
				case 10:	PlayerCfg.JoystickSensitivityY = items[citem].value; break;
				case 11:	PlayerCfg.JoystickDeadzone = items[citem].value; break;
				case 14:	PlayerCfg.MouseSensitivityX = items[citem].value; break;
				case 15:	PlayerCfg.MouseSensitivityY = items[citem].value; break;
				case 16:	PlayerCfg.MouseFilter = items[citem].value; break;
			}
			break;

		case EVENT_NEWMENU_SELECTED:
			switch (citem)
			{
				case 3:
					kconfig(0, "KEYBOARD");
					break;
				case 4:
					kconfig(1, "JOYSTICK");
					break;
				case 5:
					kconfig(2, "MOUSE");
					break;
				case 6:
					kconfig(3, "WEAPON KEYS");
					break;
				case 18:
					show_help();
					break;
				case 19:
					show_netgame_help();
					break;
				case 20:
					show_newdemo_help();
					break;
			}
			return 1;		// stay in menu
			break;

		default:
			break;
	}

	return 0;
}

void input_config()
{
	newmenu_item m[21];
	int nitems = 21;

	m[0].type = NM_TYPE_CHECK;  m[0].text = "USE JOYSTICK"; m[0].value = (PlayerCfg.ControlType&CONTROL_USING_JOYSTICK);
	m[1].type = NM_TYPE_CHECK;  m[1].text = "USE MOUSE"; m[1].value = (PlayerCfg.ControlType&CONTROL_USING_MOUSE);
	m[2].type = NM_TYPE_TEXT;   m[2].text = "";
	m[3].type = NM_TYPE_MENU;   m[3].text = "CUSTOMIZE KEYBOARD";
	m[4].type = NM_TYPE_MENU;   m[4].text = "CUSTOMIZE JOYSTICK";
	m[5].type = NM_TYPE_MENU;   m[5].text = "CUSTOMIZE MOUSE";
	m[6].type = NM_TYPE_MENU;   m[6].text = "CUSTOMIZE WEAPON KEYS";
	m[7].type = NM_TYPE_TEXT;   m[7].text = "";
	m[8].type = NM_TYPE_TEXT;   m[8].text = "Joystick";
	m[9].type = NM_TYPE_SLIDER; m[9].text="X Sensitivity"; m[9].value=PlayerCfg.JoystickSensitivityX; m[9].min_value = 0; m[9].max_value = 16;
	m[10].type = NM_TYPE_SLIDER; m[10].text="Y Sensitivity"; m[10].value=PlayerCfg.JoystickSensitivityY; m[10].min_value = 0; m[10].max_value = 16;
	m[11].type = NM_TYPE_SLIDER; m[11].text="Deadzone"; m[11].value=PlayerCfg.JoystickDeadzone; m[11].min_value=0; m[11].max_value = 16;
	m[12].type = NM_TYPE_TEXT;   m[12].text = "";
	m[13].type = NM_TYPE_TEXT;   m[13].text = "Mouse";
	m[14].type = NM_TYPE_SLIDER; m[14].text="X Sensitivity"; m[14].value=PlayerCfg.MouseSensitivityX; m[14].min_value = 0; m[14].max_value = 16;
	m[15].type = NM_TYPE_SLIDER; m[15].text="Y Sensitivity"; m[15].value=PlayerCfg.MouseSensitivityY; m[15].min_value = 0; m[15].max_value = 16;
	m[16].type = NM_TYPE_CHECK;  m[16].text="Mouse Smoothing/Filtering"; m[16].value=PlayerCfg.MouseFilter;
	m[17].type = NM_TYPE_TEXT;   m[17].text = "";
	m[18].type = NM_TYPE_MENU;   m[18].text = "GAME SYSTEM KEYS";
	m[19].type = NM_TYPE_MENU;   m[19].text = "NETGAME SYSTEM KEYS";
	m[20].type = NM_TYPE_MENU;   m[20].text = "DEMO SYSTEM KEYS";

	newmenu_do1(NULL, TXT_CONTROLS, nitems, m, input_menuset, NULL, 3);
}

void do_graphics_menu()
{
	newmenu_item m[10];
	int i = 0, j = 0;

	do {
		m[0].type = NM_TYPE_TEXT;   m[0].text="Texture Filtering (restart required):";
		m[1].type = NM_TYPE_RADIO;  m[1].text = "None (Classical)";       m[1].value = 0; m[1].group = 0;
		m[2].type = NM_TYPE_RADIO;  m[2].text = "Bilinear";               m[2].value = 0; m[2].group = 0;
		m[3].type = NM_TYPE_RADIO;  m[3].text = "Trilinear";              m[3].value = 0; m[3].group = 0;
		m[4].type = NM_TYPE_CHECK;  m[4].text="Movie Filter";             m[4].value = GameCfg.MovieTexFilt;
		m[5].type = NM_TYPE_TEXT;   m[5].text="";
		m[6].type = NM_TYPE_CHECK;  m[6].text="Transparency Effects";     m[6].value = PlayerCfg.OglAlphaEffects;
		m[7].type = NM_TYPE_CHECK;  m[7].text="Vectorial Reticle";        m[7].value = PlayerCfg.OglReticle;
		m[8].type = NM_TYPE_CHECK;  m[8].text="VSync";                    m[8].value = GameCfg.VSync;
		m[9].type = NM_TYPE_CHECK;  m[9].text="4x multisampling";         m[9].value = GameCfg.Multisample;

		m[GameCfg.TexFilt+1].value=1;

		i = newmenu_do1( NULL, "Graphics Options", sizeof(m)/sizeof(*m), m, NULL, NULL, i );

		if (GameCfg.VSync != m[8].value || GameCfg.Multisample != m[9].value)
			nm_messagebox( NULL, 1, TXT_OK, "To apply VSync or 4x Multisample\nyou need to restart the program");

		for (j = 0; j <= 2; j++)
			if (m[j+1].value)
				GameCfg.TexFilt = j;
		GameCfg.MovieTexFilt = m[4].value;
		PlayerCfg.OglAlphaEffects = m[6].value;
		PlayerCfg.OglReticle = m[7].value;
		GameCfg.VSync = m[8].value;
		GameCfg.Multisample = m[9].value;
		gr_set_attributes();
		gr_set_mode(Game_screen_mode);
	} while( i>-1 );
}

#if PHYSFS_VER_MAJOR >= 2
typedef struct browser
{
	char	*title;			// The title - needed for making another listbox when changing directory
	int		(*when_selected)(void *userdata, const char *filename);	// What to do when something chosen
	void	*userdata;		// Whatever you want passed to when_selected
	char	**list;			// All menu items
	char	*list_buf;		// Buffer for menu item text: hopefully reduces memory fragmentation this way
	char	**ext_list;		// List of file extensions we're looking for (if looking for a music file many types are possible)
	int		select_dir;		// Allow selecting the current directory (e.g. for Jukebox level song directory)
	int		num_files;		// Number of list items found (including parent directory and current directory if selectable)
	int		max_files;		// How many entries we can have before having to grow the array
	int		max_buf;		// How much text we can have before having to grow the buffer
	char	view_path[PATH_MAX];	// The absolute path we're currently looking at
	int		new_path;		// Whether the view_path is a new searchpath, if so, remove it when finished
} browser;

void list_dir_el(browser *b, const char *origdir, const char *fname)
{
	char *ext;
	char **i = NULL;
#if 0
	
	ext = strrchr(fname, '.');
	if (ext)
		for (i = b->ext_list; *i != NULL && stricmp(ext, *i); i++) {}	// see if the file is of a type we want
	
	if ((!strcmp((PHYSFS_getRealDir(fname)==NULL?"":PHYSFS_getRealDir(fname)), b->view_path)) && (PHYSFS_isDirectory(fname) || (ext && *i))
#if defined(__MACH__) && defined(__APPLE__)
		&& stricmp(fname, "Volumes")	// this messes things up, use '..' instead
#endif
		)
		string_array_add(&b->list, &b->list_buf, &b->num_files, &b->max_files, &b->max_buf, fname);
#endif
}

int list_directory(browser *b)
{
	if (!string_array_new(&b->list, &b->list_buf, &b->num_files, &b->max_files, &b->max_buf))
		return 0;
	
	strcpy(b->list_buf, "..");		// go to parent directory
	b->list[b->num_files++] = b->list_buf;
	
	if (b->select_dir)
	{
		b->list[b->num_files] = b->list[b->num_files - 1] + strlen(b->list[b->num_files - 1]) + 1;
		strcpy(b->list[b->num_files++], "<this directory>");	// choose the directory being viewed
	}
	
	//PHYSFS_enumerateFilesCallback("", (PHYSFS_EnumFilesCallback) list_dir_el, b);
	string_array_tidy(&b->list, &b->list_buf, &b->num_files, &b->max_files, &b->max_buf, 1 + (b->select_dir ? 1 : 0),
#ifdef __LINUX__
					  strcmp
#elif defined(_WIN32) || defined(macintosh)
					  stricmp
#else
					  strcasecmp
#endif
					  );
					  
	return 1;
}

int select_file_recursive(char *title, const char *orig_path, char **ext_list, int select_dir, int (*when_selected)(void *userdata, const char *filename), void *userdata);

int select_file_handler(listbox *menu, d_event *event, browser *b)
{
	char newpath[PATH_MAX];
	char **list = listbox_get_items(menu);
	int citem = listbox_get_citem(menu);
	const char *sep = "/";

	memset(newpath, 0, sizeof(char)*PATH_MAX);
	switch (event->type)
	{
#ifdef _WIN32
		case EVENT_KEY_COMMAND:
		{
			if (((d_event_keycommand *)event)->keycode == KEY_CTRLED + KEY_D)
			{
				newmenu_item *m;
				char *text = NULL;
				int rval = 0;

				MALLOC(text, char, 2);
				MALLOC(m, newmenu_item, 1);
				snprintf(text, sizeof(char)*PATH_MAX, "c");
				m->type=NM_TYPE_INPUT; m->text_len = 3; m->text = text;
				rval = newmenu_do( NULL, "Enter drive letter", 1, m, NULL, NULL );
				text[1] = '\0'; 
				snprintf(newpath, sizeof(char)*PATH_MAX, "%s:%s", text, sep);
				if (!rval && strlen(text))
				{
					select_file_recursive(b->title, newpath, b->ext_list, b->select_dir, b->when_selected, b->userdata);
					// close old box.
					event->type = EVENT_WINDOW_CLOSED;
					window_close(listbox_get_window(menu));
				}
				d_free(text);
				d_free(m);
				return 0;
			}
			break;
		}
#endif
		case EVENT_NEWMENU_SELECTED:
			strcpy(newpath, b->view_path);

			if (citem == 0)		// go to parent dir
			{
				char *p;
				
				if ((p = strstr(&newpath[strlen(newpath) - strlen(sep)], sep)))
					if (p != strstr(newpath, sep))	// if this isn't the only separator (i.e. it's not about to look at the root)
						*p = 0;
				
				p = newpath + strlen(newpath) - 1;
				while ((p > newpath) && strncmp(p, sep, strlen(sep)))	// make sure full separator string is matched (typically is)
					p--;
				
				if (p == strstr(newpath, sep))	// Look at root directory next, if not already
				{
#if defined(__MACH__) && defined(__APPLE__)
					if (!stricmp(p, "/Volumes"))
						return 1;
#endif
					if (p[strlen(sep)] != '\0')
						p[strlen(sep)] = '\0';
					else
					{
#if defined(__MACH__) && defined(__APPLE__)
						// For Mac OS X, list all active volumes if we leave the root
						strcpy(newpath, "/Volumes");
#else
						return 1;
#endif
					}
				}
				else
					*p = '\0';
			}
			else if (citem == 1 && b->select_dir)
				return !(*b->when_selected)(b->userdata, "");
			else
			{
				if (strncmp(&newpath[strlen(newpath) - strlen(sep)], sep, strlen(sep)))
				{
					strncat(newpath, sep, PATH_MAX - 1 - strlen(newpath));
					newpath[PATH_MAX - 1] = '\0';
				}
				strncat(newpath, list[citem], PATH_MAX - 1 - strlen(newpath));
				newpath[PATH_MAX - 1] = '\0';
			}
			
			if ((citem == 0) || PHYSFS_isDirectory(list[citem]))
			{
				// If it fails, stay in this one
				return !select_file_recursive(b->title, newpath, b->ext_list, b->select_dir, b->when_selected, b->userdata);
			}
			
			return !(*b->when_selected)(b->userdata, list[citem]);
			break;
			
		case EVENT_WINDOW_CLOSE:
			//if (b->new_path)
			//	PHYSFS_removeFromSearchPath(b->view_path);

			if (list)
				d_free(list);
			if (b->list_buf)
				d_free(b->list_buf);
			d_free(b);
			break;
			
		default:
			break;
	}
	
	return 0;
}

int select_file_recursive(char *title, const char *orig_path, char **ext_list, int select_dir, int (*when_selected)(void *userdata, const char *filename), void *userdata)
{
#if 0
	browser *b;
	const char *sep = PHYSFS_getDirSeparator();
	char *p;
	
	MALLOC(b, browser, 1);
	if (!b)
		return 0;
	
	b->title = title;
	b->when_selected = when_selected;
	b->userdata = userdata;
	b->ext_list = ext_list;
	b->select_dir = select_dir;
	b->num_files = b->max_files = 0;
	b->view_path[0] = '\0';
	b->new_path = 1;
	
	// Set the viewing directory to orig_path, or some parent of it
	if (orig_path)
	{
		// Must make this an absolute path for directory browsing to work properly
#ifdef _WIN32
		if (!(isalpha(orig_path[0]) && (orig_path[1] == ':')))	// drive letter prompt (e.g. "C:"
#elif defined(macintosh)
		if (orig_path[0] == ':')
#else
		if (orig_path[0] != '/')
#endif
		{
			strncpy(b->view_path, PHYSFS_getBaseDir(), PATH_MAX - 1);		// current write directory must be set to base directory
			b->view_path[PATH_MAX - 1] = '\0';
#ifdef macintosh
			orig_path++;	// go past ':'
#endif
			strncat(b->view_path, orig_path, PATH_MAX - 1 - strlen(b->view_path));
			b->view_path[PATH_MAX - 1] = '\0';
		}
		else
		{
			strncpy(b->view_path, orig_path, PATH_MAX - 1);
			b->view_path[PATH_MAX - 1] = '\0';
		}

		p = b->view_path + strlen(b->view_path) - 1;
		b->new_path = PHYSFSX_isNewPath(b->view_path);
		
		while (!PHYSFS_addToSearchPath(b->view_path, 0))
		{
			while ((p > b->view_path) && strncmp(p, sep, strlen(sep)))
				p--;
			*p = '\0';
			
			if (p == b->view_path)
				break;
			
			b->new_path = PHYSFSX_isNewPath(b->view_path);
		}
	}
	
	// Set to user directory if we couldn't find a searchpath
	if (!b->view_path[0])
	{
		strncpy(b->view_path, PHYSFS_getUserDir(), PATH_MAX - 1);
		b->view_path[PATH_MAX - 1] = '\0';
		b->new_path = PHYSFSX_isNewPath(b->view_path);
		if (!PHYSFS_addToSearchPath(b->view_path, 0))
		{
			d_free(b);
			return 0;
		}
	}
	
	if (!list_directory(b))
	{
		d_free(b);
		return 0;
	}
	
	return newmenu_listbox1(title, b->num_files, b->list, 1, 0, (int (*)(listbox *, d_event *, void *))select_file_handler, b) >= 0;
#endif
	return 0;
}

#define PATH_HEADER_TYPE NM_TYPE_MENU
#define BROWSE_TXT " (browse...)"

#else

int select_file_recursive(char *title, const char *orig_path, char **ext_list, int select_dir, int (*when_selected)(void *userdata, const char *filename), void *userdata)
{
	return 0;
}

#define PATH_HEADER_TYPE NM_TYPE_TEXT
#define BROWSE_TXT

#endif

int opt_sm_digivol = -1, opt_sm_musicvol = -1, opt_sm_revstereo = -1, opt_sm_mtype0 = -1, opt_sm_mtype1 = -1, opt_sm_mtype2 = -1, opt_sm_mtype3 = -1, opt_sm_redbook_playorder = -1, opt_sm_mtype3_lmpath = -1, opt_sm_mtype3_lmplayorder1 = -1, opt_sm_mtype3_lmplayorder2 = -1, opt_sm_cm_mtype3_file1_b = -1, opt_sm_cm_mtype3_file1 = -1, opt_sm_cm_mtype3_file2_b = -1, opt_sm_cm_mtype3_file2 = -1, opt_sm_cm_mtype3_file3_b = -1, opt_sm_cm_mtype3_file3 = -1, opt_sm_cm_mtype3_file4_b = -1, opt_sm_cm_mtype3_file4 = -1, opt_sm_cm_mtype3_file5_b = -1, opt_sm_cm_mtype3_file5 = -1;

void set_extmusic_volume(int volume);

#if 0
int get_absolute_path(char *full_path, const char *rel_path)
{
	PHYSFSX_getRealPath(rel_path, full_path);
	return 1;
}

#ifdef USE_SDLMIXER
#define SELECT_SONG(t, s)	select_file_recursive(t, GameCfg.CMMiscMusic[s], jukebox_exts, 0, (int (*)(void *, const char *))get_absolute_path, GameCfg.CMMiscMusic[s])
#endif

int sound_menuset(newmenu *menu, d_event *event, void *userdata)
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	//int nitems = newmenu_get_nitems(menu);
	int replay = 0;
	int rval = 0;

	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			if (citem == opt_sm_digivol)
			{
				GameCfg.DigiVolume = items[citem].value;
				digi_set_digi_volume( (GameCfg.DigiVolume*32768)/8 );
				digi_play_sample_once( SOUND_DROP_BOMB, F1_0 );
			}
			else if (citem == opt_sm_musicvol)
			{
				GameCfg.MusicVolume = items[citem].value;
				songs_set_volume(GameCfg.MusicVolume);
			}
			else if (citem == opt_sm_revstereo)
			{
				GameCfg.ReverseStereo = items[citem].value;
			}
			else if (citem == opt_sm_mtype0)
			{
				GameCfg.MusicType = MUSIC_TYPE_NONE;
				replay = 1;
			}
			else if (citem == opt_sm_mtype1)
			{
				GameCfg.MusicType = MUSIC_TYPE_BUILTIN;
				replay = 1;
			}
			else if (citem == opt_sm_mtype2)
			{
				GameCfg.MusicType = MUSIC_TYPE_REDBOOK;
				replay = 1;
			}
#ifdef USE_SDLMIXER
			else if (citem == opt_sm_mtype3)
			{
				GameCfg.MusicType = MUSIC_TYPE_CUSTOM;
				replay = 1;
			}
#endif
			else if (citem == opt_sm_redbook_playorder)
			{
				GameCfg.OrigTrackOrder = items[citem].value;
				replay = (Game_wind != NULL);
			}
#ifdef USE_SDLMIXER
			else if (citem == opt_sm_mtype3_lmplayorder1)
			{
				GameCfg.CMLevelMusicPlayOrder = MUSIC_CM_PLAYORDER_CONT;
				replay = (Game_wind != NULL);
			}
			else if (citem == opt_sm_mtype3_lmplayorder2)
			{
				GameCfg.CMLevelMusicPlayOrder = MUSIC_CM_PLAYORDER_LEVEL;
				replay = (Game_wind != NULL);
			}
#endif
			break;

		case EVENT_NEWMENU_SELECTED:
#ifdef USE_SDLMIXER
			if (citem == opt_sm_mtype3_lmpath)
			{
				static char *ext_list[] = { ".m3u", NULL };		// select a directory or M3U playlist
				select_file_recursive(
#ifndef _WIN32
					"Select directory or\nM3U playlist to\n play level music from",
#else
					"Select directory or\nM3U playlist to\n play level music from.\n CTRL-D to change drive",
#endif
									  GameCfg.CMLevelMusicPath, ext_list, 1,	// look in current music path for ext_list files and allow directory selection
									  (int (*)(void *, const char *))get_absolute_path, GameCfg.CMLevelMusicPath);	// just copy the absolute path
			}
			else if (citem == opt_sm_cm_mtype3_file1_b)
#ifndef _WIN32
				SELECT_SONG("Select main menu music", SONG_TITLE);
#else
				SELECT_SONG("Select main menu music.\nCTRL-D to change drive", SONG_TITLE);
#endif
			else if (citem == opt_sm_cm_mtype3_file2_b)
#ifndef _WIN32
				SELECT_SONG("Select briefing music", SONG_BRIEFING);
#else
				SELECT_SONG("Select briefing music.\nCTRL-D to change drive", SONG_BRIEFING);
#endif
			else if (citem == opt_sm_cm_mtype3_file3_b)
#ifndef _WIN32
				SELECT_SONG("Select credits music", SONG_CREDITS);
#else
				SELECT_SONG("Select credits music.\nCTRL-D to change drive", SONG_CREDITS);
#endif
			else if (citem == opt_sm_cm_mtype3_file4_b)
#ifndef _WIN32
				SELECT_SONG("Select escape sequence music", SONG_ENDLEVEL);
#else
				SELECT_SONG("Select escape sequence music.\nCTRL-D to change drive", SONG_ENDLEVEL);
#endif
			else if (citem == opt_sm_cm_mtype3_file5_b)
#ifndef _WIN32
				SELECT_SONG("Select game ending music", SONG_ENDGAME);
#else
				SELECT_SONG("Select game ending music.\nCTRL-D to change drive", SONG_ENDGAME);
#endif
#endif
			rval = 1;	// stay in menu
			break;

		case EVENT_WINDOW_CLOSE:
			d_free(items);
			break;

		default:
			break;
	}

	if (replay)
	{
		songs_uninit();

		if (Game_wind)
			songs_play_level_song( Current_level_num, 0 );
		else
			songs_play_song(SONG_TITLE, 1);
	}

	userdata = userdata;

	return rval;
}
#endif

#ifdef USE_SDLMIXER
#define SOUND_MENU_NITEMS 32
#else
#ifdef _WIN32
#define SOUND_MENU_NITEMS 11
#else
#define SOUND_MENU_NITEMS 10
#endif
#endif

void do_sound_menu()
{
	newmenu_item *m;
	int nitems = 0;
	char old_CMLevelMusicPath[PATH_MAX+1], old_CMMiscMusic0[PATH_MAX+1];

	memset(old_CMLevelMusicPath, 0, sizeof(char)*(PATH_MAX+1));
	snprintf(old_CMLevelMusicPath, strlen(GameCfg.CMLevelMusicPath)+1, "%s", GameCfg.CMLevelMusicPath);
	memset(old_CMMiscMusic0, 0, sizeof(char)*(PATH_MAX+1));
	snprintf(old_CMMiscMusic0, strlen(GameCfg.CMMiscMusic[SONG_TITLE])+1, "%s", GameCfg.CMMiscMusic[SONG_TITLE]);

	MALLOC(m, newmenu_item, SOUND_MENU_NITEMS);
	if (!m)
		return;

	opt_sm_digivol = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_FX_VOLUME; m[nitems].value = GameCfg.DigiVolume; m[nitems].min_value = 0; m[nitems++].max_value = 8;

	opt_sm_musicvol = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = "music volume"; m[nitems].value = GameCfg.MusicVolume; m[nitems].min_value = 0; m[nitems++].max_value = 8;

	opt_sm_revstereo = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text = TXT_REVERSE_STEREO; m[nitems++].value = GameCfg.ReverseStereo;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "music type:";

	opt_sm_mtype0 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "no music"; m[nitems].value = (GameCfg.MusicType == MUSIC_TYPE_NONE); m[nitems].group = 0; nitems++;

#if defined(USE_SDLMIXER) || defined(_WIN32)
	opt_sm_mtype1 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "built-in/addon music"; m[nitems].value = (GameCfg.MusicType == MUSIC_TYPE_BUILTIN); m[nitems].group = 0; nitems++;
#endif

	opt_sm_mtype2 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "cd music"; m[nitems].value = (GameCfg.MusicType == MUSIC_TYPE_REDBOOK); m[nitems].group = 0; nitems++;

#ifdef USE_SDLMIXER
	opt_sm_mtype3 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "jukebox"; m[nitems].value = (GameCfg.MusicType == MUSIC_TYPE_CUSTOM); m[nitems].group = 0; nitems++;

#endif

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
#ifdef USE_SDLMIXER
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "cd music / jukebox options:";
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "cd music options:";
#endif

	opt_sm_redbook_playorder = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text = "force descent ][ cd track order"; m[nitems++].value = GameCfg.OrigTrackOrder;

#ifdef USE_SDLMIXER
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "jukebox options:";

	opt_sm_mtype3_lmpath = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "path for level music" BROWSE_TXT;

	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMLevelMusicPath; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "level music play order:";

	opt_sm_mtype3_lmplayorder1 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "continuously"; m[nitems].value = (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_CONT); m[nitems].group = 1; nitems++;

	opt_sm_mtype3_lmplayorder2 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "one track per level"; m[nitems].value = (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVEL); m[nitems].group = 1; nitems++;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "non-level music:";

	opt_sm_cm_mtype3_file1_b = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "main menu" BROWSE_TXT;

	opt_sm_cm_mtype3_file1 = nitems;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMMiscMusic[SONG_TITLE]; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;

	opt_sm_cm_mtype3_file2_b = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "briefing" BROWSE_TXT;

	opt_sm_cm_mtype3_file2 = nitems;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMMiscMusic[SONG_BRIEFING]; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;

	opt_sm_cm_mtype3_file3_b = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "credits" BROWSE_TXT;

	opt_sm_cm_mtype3_file3 = nitems;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMMiscMusic[SONG_CREDITS]; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;

	opt_sm_cm_mtype3_file4_b = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "escape sequence" BROWSE_TXT;

	opt_sm_cm_mtype3_file4 = nitems;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMMiscMusic[SONG_ENDLEVEL]; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;

	opt_sm_cm_mtype3_file5_b = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "game ending" BROWSE_TXT;

	opt_sm_cm_mtype3_file5 = nitems;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMMiscMusic[SONG_ENDGAME]; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;
#endif

	Assert(nitems == SOUND_MENU_NITEMS);

	//newmenu_do1( NULL, "Sound Effects & Music", nitems, m, sound_menuset, NULL, 0 );

#ifdef USE_SDLMIXER
	if ( ((Game_wind != NULL) && strcmp(old_CMLevelMusicPath, GameCfg.CMLevelMusicPath)) || ((Game_wind == NULL) && strcmp(old_CMMiscMusic0, GameCfg.CMMiscMusic[SONG_TITLE])) )
	{
		songs_uninit();

		if (Game_wind)
			songs_play_level_song( Current_level_num, 0 );
		else
			songs_play_song(SONG_TITLE, 1);
	}
#endif
}

#define ADD_CHECK(n,txt,v)  do { m[n].type=NM_TYPE_CHECK; m[n].text=txt; m[n].value=v;} while (0)

void do_misc_menu()
{
	newmenu_item m[9];
	int i = 0;

	do {
		ADD_CHECK(0, "Ship auto-leveling", PlayerCfg.AutoLeveling);
		ADD_CHECK(1, "Show reticle", PlayerCfg.ReticleOn);
		ADD_CHECK(2, "Missile view", PlayerCfg.MissileViewEnabled);
		ADD_CHECK(3, "Headlight on when picked up", PlayerCfg.HeadlightActiveDefault );
		ADD_CHECK(4, "Show guided missile in main display", PlayerCfg.GuidedInBigWindow );
		ADD_CHECK(5, "Escort robot hot keys",PlayerCfg.EscortHotKeys);
		ADD_CHECK(6, "Persistent Debris",PlayerCfg.PersistentDebris);
		ADD_CHECK(7, "Screenshots w/o HUD",PlayerCfg.PRShot);
		ADD_CHECK(8, "Movie Subtitles",GameCfg.MovieSubtitles);

		i = newmenu_do1( NULL, "Misc Options", sizeof(m)/sizeof(*m), m, NULL, NULL, i );

		PlayerCfg.AutoLeveling		= m[0].value;
		PlayerCfg.ReticleOn		= m[1].value;
		PlayerCfg.MissileViewEnabled   	= m[2].value;
		PlayerCfg.HeadlightActiveDefault= m[3].value;
		PlayerCfg.GuidedInBigWindow	= m[4].value;
		PlayerCfg.EscortHotKeys		= m[5].value;
		PlayerCfg.PersistentDebris	= m[6].value;
		PlayerCfg.PRShot 		= m[7].value;
		GameCfg.MovieSubtitles 		= m[8].value;

	} while( i>-1 );

}

#if defined(USE_UDP) || defined(USE_IPX)
static int multi_player_menu_handler(newmenu *menu, d_event *event, int *menu_choice)
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event->type)
	{
		case EVENT_NEWMENU_SELECTED:
			// stay in multiplayer menu, even after having played a game
			return do_option(menu_choice[newmenu_get_citem(menu)]);

		case EVENT_WINDOW_CLOSE:
			d_free(menu_choice);
			d_free(items);
			break;

		default:
			break;
	}

	return 0;
}

void do_multi_player_menu()
{
	int *menu_choice;
	newmenu_item *m;
	int num_options = 0;

	MALLOC(menu_choice, int, 12);
	if (!menu_choice)
		return;

	MALLOC(m, newmenu_item, 12);
	if (!m)
	{
		d_free(menu_choice);
		return;
	}

#ifdef USE_UDP
	m[num_options].type=NM_TYPE_TEXT; m[num_options].text="UDP:"; num_options++;
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="HOST GAME"; menu_choice[num_options]=MENU_START_UDP_NETGAME; num_options++;
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="FIND LAN GAMES"; menu_choice[num_options]=MENU_JOIN_LIST_UDP_NETGAME; num_options++;
	//m[num_options].type=NM_TYPE_MENU; m[num_options].text="FIND LAN/ONLINE GAMES"; menu_choice[num_options]=MENU_JOIN_LIST_UDP_NETGAME; num_options++;
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="JOIN GAME MANUALLY"; menu_choice[num_options]=MENU_JOIN_MANUAL_UDP_NETGAME; num_options++;
#endif

#ifdef USE_IPX
	m[num_options].type=NM_TYPE_TEXT; m[num_options].text=""; num_options++;
	m[num_options].type=NM_TYPE_TEXT; m[num_options].text="IPX:"; num_options++;
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="HOST GAME"; menu_choice[num_options]=MENU_START_IPX_NETGAME; num_options++;
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="JOIN GAME"; menu_choice[num_options]=MENU_JOIN_IPX_NETGAME; num_options++;
#ifdef __LINUX__
	m[num_options].type=NM_TYPE_TEXT; m[num_options].text=""; num_options++;
	m[num_options].type=NM_TYPE_TEXT; m[num_options].text="XKALI:"; num_options++;
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="HOST GAME"; menu_choice[num_options]=MENU_START_KALI_NETGAME; num_options++;
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="JOIN GAME"; menu_choice[num_options]=MENU_JOIN_KALI_NETGAME; num_options++;
#endif
#endif

	newmenu_do3( NULL, TXT_MULTIPLAYER, num_options, m, (int (*)(newmenu *, d_event *, void *))multi_player_menu_handler, menu_choice, 0, NULL );
}
#endif

void do_options_menu()
{
	newmenu_item *m;

	MALLOC(m, newmenu_item, 11);
	if (!m)
		return;

	m[ 0].type = NM_TYPE_MENU;   m[ 0].text="Sound effects & music...";
	m[ 1].type = NM_TYPE_TEXT;   m[ 1].text="";
	m[ 2].type = NM_TYPE_MENU;   m[ 2].text=TXT_CONTROLS_;
	m[ 3].type = NM_TYPE_TEXT;   m[ 3].text="";

	m[ 4].type = NM_TYPE_SLIDER;
	m[ 4].text = TXT_BRIGHTNESS;
	m[ 4].value = gr_palette_get_gamma();
	m[ 4].min_value = 0;
	m[ 4].max_value = 16;

	m[ 5].type = NM_TYPE_MENU;   m[ 5].text="Screen resolution...";
#ifdef OGL
	m[ 6].type = NM_TYPE_MENU;   m[ 6].text="Graphics Options...";
#else
	m[ 6].type = NM_TYPE_TEXT;   m[ 6].text="";
#endif

	m[ 7].type = NM_TYPE_TEXT;   m[ 7].text="";
	m[ 8].type = NM_TYPE_MENU;   m[ 8].text="Primary autoselect ordering...";
	m[ 9].type = NM_TYPE_MENU;   m[ 9].text="Secondary autoselect ordering...";
	m[10].type = NM_TYPE_MENU;   m[10].text="Misc Options...";

	// Fall back to main event loop
	// Allows clean closing and re-opening when resolution changes
	newmenu_do3( NULL, TXT_OPTIONS, 11, m, options_menuset, NULL, 0, NULL );
}
