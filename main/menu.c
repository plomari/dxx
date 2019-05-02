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
#include "mouse.h"
#include "iff.h"
#include "u_mem.h"
#include "dxxerror.h"
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
#include "config.h"
#include "movie.h"
#include "gamepal.h"
#include "gauges.h"
#include "powerup.h"
#include "strutil.h"
#include "multi.h"
#ifdef USE_UDP
#include "net_udp.h"
#endif
#include "ogl.h"


// Menu IDs...
enum MENUS
{
    MENU_NEW_GAME = 0,
    MENU_GAME,
    MENU_VIEW_SCORES,
    MENU_QUIT,
    MENU_LOAD_GAME,
    MENU_SAVE_GAME,
    MENU_DEMO_PLAY,
    MENU_CONFIG,
    MENU_REJOIN_NETGAME,
    MENU_DIFFICULTY,
    MENU_HELP,
    MENU_NEW_PLAYER,
    #if defined(USE_UDP)
        MENU_MULTIPLAYER,
    #endif

    MENU_SHOW_CREDITS,
    MENU_ORDER_INFO,

    #ifdef USE_UDP
    MENU_START_UDP_NETGAME,
    MENU_JOIN_MANUAL_UDP_NETGAME,
    MENU_JOIN_LIST_UDP_NETGAME,
    #endif
    MENU_SANDBOX
};

//ADD_ITEM("Start netgame...", MENU_START_NETGAME, -1 );
//ADD_ITEM("Send net message...", MENU_SEND_NET_MESSAGE, -1 );

#define ADD_ITEM(t,value,key)  do { m[num_options].type=NM_TYPE_MENU; m[num_options].text=t; menu_choice[num_options]=value;num_options++; } while (0)

// Function Prototypes added after LINTING
int do_option(int select);
int do_new_game_menu(void);
void do_multi_player_menu();
void do_sandbox_menu();
extern void newmenu_free_background();
extern void ReorderPrimary();
extern void ReorderSecondary();

//pairs of chars describing ranges
char playername_allowed_chars[] = "azAZ09__--";

int MakeNewPlayerFile(int allow_abort)
{
	int x;
	char filename[PATH_MAX];
	newmenu_item m;
	char text[CALLSIGN_LEN+9]="";

	strncpy(text, Players[Player_num].callsign,CALLSIGN_LEN);

try_again:
	nm_set_item_input(&m, CALLSIGN_LEN, text);

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

	memset(filename, '\0', PATH_MAX);
	snprintf( filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%s.plr" : "%s.plr", text );

	if (PHYSFS_exists(filename))
	{
		nm_messagebox(NULL, tprintf(40, "%s '%s' %s", TXT_PLAYER, text, TXT_ALREADY_EXISTS), TXT_OK);
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

	switch (event_key_get(event))
	{
		case KEY_CTRLED+KEY_D:
			if (citem > 0)
			{
				int x = 1;
				x = nm_messagebox( NULL, tprintf(80, "%s %s?", TXT_DELETE_PILOT, items[citem]+((items[citem][0]=='$')?1:0)), TXT_YES, TXT_NO);
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
						nm_messagebox( NULL, tprintf(80, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, items[citem]+((items[citem][0]=='$')?1:0)), TXT_OK);
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
			if (read_player_file() != EZERO) {
				printf("could not read player file, exiting\n");
				exit(0);
			}

			WriteConfigFile();		// Update lastplr

			PHYSFS_freeList(list);
			d_free(items);
			break;

		default:
			break;
	}

	return 0;
}

static int string_array_sort_func(const void *p0, const void *p1)
{
	const char *const *e0 = p0;
	const char *const *e1 = p1;
	return stricmp(*e0, *e1);
}

//Inputs the player's name, without putting up the background screen
int RegisterPlayer(int at_program_start)
{
	char **m;
	char **f;
	char **list;
	static const char *const types[] = { ".plr", NULL };
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
		snprintf(Players[Player_num].callsign, sizeof(Players[0].callsign), "%s", GameCfg.LastPlayer);
		strlwr(Players[Player_num].callsign);
		if (read_player_file() == EZERO)
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

		if (strlen(*f) > FILENAME_LEN-1 || strlen(*f) < 5) // sorry guys, can only have up to eight chars for the player name
		{
			NumItems--;
			continue;
		}
		m[i++] = *f;
		p = strchr(*f, '.');
		if (p)
			*p = '\0';		// chop the .plr
	}

	if (NumItems <= 1) // so it seems all plr files we found were too long. funny. let's make a real player
	{
		MakeNewPlayerFile(0);	// make a new player without showing listbox
		PHYSFS_freeList(list);
		return 0;
	}

	// Sort by name, except the <Create New Player> string
	qsort(&m[1], NumItems - 1, sizeof(char *), string_array_sort_func);

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
	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(6,6,6),-1);
	gr_printf(0x8000,SHEIGHT-LINE_SPACING,TXT_COPYRIGHT);
	gr_set_fontcolor( BM_XRGB(25,0,0), -1);
	gr_printf(0x8000,SHEIGHT-(LINE_SPACING*2),DESCENT_VERSION);
}

//returns the number of demo files on the disk
int newdemo_count_demos();

// ------------------------------------------------------------------------
int main_menu_handler(newmenu *menu, d_event *event, int *menu_choice )
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			load_palette(MENU_PALETTE,0,1);		//get correct palette

			if ( Players[Player_num].callsign[0]==0 )
				RegisterPlayer(1);
			else
				keyd_time_when_last_pressed = timer_query();		// .. 20 seconds from now!
			break;

		case EVENT_KEY_COMMAND:
			// Don't allow them to hit ESC in the main menu.
			if (event_key_get(event)==KEY_ESC)
				return 1;
			break;

		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			// Don't allow mousebutton-closing in main menu.
			if (event_mouse_get_button(event) == MBTN_RIGHT)
				return 1;
			break;

		case EVENT_IDLE:
			if ( keyd_time_when_last_pressed+i2f(25) < timer_query() && GameArg.SysAutoDemo  )
			{
				int n_demos;
				n_demos = newdemo_count_demos();
				keyd_time_when_last_pressed = timer_query();			// Reset timer so that disk won't thrash if no demos.

				if (((d_rand() % (n_demos+1)) == 0) && !GameArg.SysAutoDemo)
				{
					init_subtitles("intro.tex");
					PlayMovie("intro.mve",0);
					close_subtitles();
					songs_play_song(SONG_TITLE,1);
					gr_set_current_canvas(NULL);
				}
				else
				{
					newdemo_start_playback(NULL);		// Randomly pick a file, assume native endian (crashes if not)
					if (Newdemo_state == ND_STATE_PLAYBACK)
						return 0;
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

	num_options = 0;

	ADD_ITEM(TXT_NEW_GAME,MENU_NEW_GAME,KEY_N);

	ADD_ITEM(TXT_LOAD_GAME,MENU_LOAD_GAME,KEY_L);
#if defined(USE_UDP)
	ADD_ITEM(TXT_MULTIPLAYER_,MENU_MULTIPLAYER,-1);
#endif

	ADD_ITEM(TXT_OPTIONS_, MENU_CONFIG, -1 );
	ADD_ITEM(TXT_CHANGE_PILOTS,MENU_NEW_PLAYER,unused);
	ADD_ITEM(TXT_VIEW_DEMO,MENU_DEMO_PLAY,0);
	ADD_ITEM(TXT_VIEW_SCORES,MENU_VIEW_SCORES,KEY_V);
	if (cfexist("orderd2.pcx")) /* SHAREWARE */
		ADD_ITEM(TXT_ORDERING_INFO,MENU_ORDER_INFO,-1);
	ADD_ITEM(TXT_CREDITS,MENU_SHOW_CREDITS,-1);

	ADD_ITEM("SANDBOX", MENU_SANDBOX, -1);
	ADD_ITEM(TXT_QUIT,MENU_QUIT,KEY_Q);

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
		case MENU_VIEW_SCORES:
			scores_view(NULL, -1);
			break;
#if 1 //def SHAREWARE
		case MENU_ORDER_INFO:
			show_order_form();
			break;
#endif
		case MENU_QUIT:
			return 0;

		case MENU_NEW_PLAYER:
			RegisterPlayer(0);
			break;

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
#if defined(USE_UDP)
		case MENU_MULTIPLAYER:
			do_multi_player_menu();
			break;
#endif
		case MENU_CONFIG:
			do_options_menu();
			break;
		case MENU_SHOW_CREDITS:
			credits_show(NULL);
			break;
		case MENU_SANDBOX:
			do_sandbox_menu();
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

	switch (event_key_get(event))
	{
		case KEY_CTRLED+KEY_D:
			if (citem >= 0)
			{
				int x = 1;
				x = nm_messagebox( NULL, tprintf(80, "%s %s?", TXT_DELETE_DEMO, items[citem]+((items[citem][0]=='$')?1:0)), TXT_YES, TXT_NO);
				if (x==0)
				{
					int ret;
					char name[PATH_MAX];

					strcpy(name, DEMO_DIR);
					strcat(name,items[citem]);

					ret = !PHYSFS_delete(name);

					if (ret)
						nm_messagebox( NULL, tprintf(80, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, items[citem]+((items[citem][0]=='$')?1:0)), TXT_OK);
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
				x = nm_messagebox( NULL, tprintf(512, "Are you sure you want to\n"
								  "swap the endianness of\n"
								  "%s? If the file is\n"
								  "already endian native, D1X\n"
								  "will likely crash. A backup\n"
								  "%s will be created", items[citem]+((items[citem][0]=='$')?1:0), bakname),
								  TXT_YES, TXT_NO);
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
	static const char *const types[] = { DEMO_EXT, NULL };
	int NumItems;

	list = PHYSFSX_findFiles(DEMO_DIR, types);
	if (!list)
		return 0;	// memory error
	if ( !*list )
	{
		nm_messagebox( NULL, tprintf(40, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE), TXT_OK);
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

			nm_set_item_text(& m[0], info_text);
			nm_set_item_input(&m[1], 10, num_text);
			n_items = 2;

                snprintf(num_text, sizeof(num_text), "%d", max(GameCfg.LastLevel, 1));

		choice = newmenu_do( NULL, TXT_SELECT_START_LEV, n_items, m, NULL, NULL );

		if (choice==-1 || m[1].text[0]==0)
			return 0;

		new_level_num = atoi(m[1].text);

		if (!(new_level_num>0 && new_level_num<=player_highest_level)) {
			m[0].text = TXT_ENTER_TO_CONT;
			nm_messagebox( NULL, TXT_INVALID_LEVEL, TXT_OK);
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

void do_sound_menu();
void input_config();
void graphics_config();
void do_misc_menu();

int options_menuset(newmenu *menu, d_event *event, void *userdata)
{
	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			break;

		case EVENT_NEWMENU_SELECTED:
			switch(newmenu_get_citem(menu))
			{
				case  0: do_sound_menu();		break;
				case  2: input_config();		break;
				case  5: graphics_config();		break;
				case  7: ReorderPrimary();		break;
				case  8: ReorderSecondary();		break;
				case  9: do_misc_menu();		break;
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

void input_config_sensitivity()
{
	newmenu_item m[34];
	int i = 0, nitems = 0, keysens = 0, joysens = 0, joydead = 0, mousesens = 0, mousefsdead;

	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "Keyboard Sensitivity:"; nitems++;
	keysens = nitems;
	nm_set_item_slider(&m[nitems], TXT_TURN_LR, PlayerCfg.KeyboardSens[0], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_PITCH_UD, PlayerCfg.KeyboardSens[1], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_LR, PlayerCfg.KeyboardSens[2], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_UD, PlayerCfg.KeyboardSens[3], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_BANK_LR, PlayerCfg.KeyboardSens[4], 0, 16); nitems++;
	nm_set_item_checkbox(&m[nitems], "Old behavior (ignores sensitivity)", PlayerCfg.OldKeyboardRamping); nitems++;
	nm_set_item_text(& m[nitems++], "");
	nm_set_item_text(& m[nitems++], "Joystick Sensitivity:");
	joysens = nitems;
	nm_set_item_slider(&m[nitems], TXT_TURN_LR, PlayerCfg.JoystickSens[0], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_PITCH_UD, PlayerCfg.JoystickSens[1], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_LR, PlayerCfg.JoystickSens[2], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_UD, PlayerCfg.JoystickSens[3], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_BANK_LR, PlayerCfg.JoystickSens[4], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_THROTTLE, PlayerCfg.JoystickSens[5], 0, 16); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	nm_set_item_text(& m[nitems], "Joystick Deadzone:"); nitems++;
	joydead = nitems;
	nm_set_item_slider(&m[nitems], TXT_TURN_LR, PlayerCfg.JoystickDead[0], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_PITCH_UD, PlayerCfg.JoystickDead[1], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_LR, PlayerCfg.JoystickDead[2], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_UD, PlayerCfg.JoystickDead[3], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_BANK_LR, PlayerCfg.JoystickDead[4], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_THROTTLE, PlayerCfg.JoystickDead[5], 0, 16); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	nm_set_item_text(& m[nitems], "Mouse Sensitivity:"); nitems++;
	mousesens = nitems;
	nm_set_item_slider(&m[nitems], TXT_TURN_LR, PlayerCfg.MouseSens[0], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_PITCH_UD, PlayerCfg.MouseSens[1], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_LR, PlayerCfg.MouseSens[2], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_UD, PlayerCfg.MouseSens[3], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_BANK_LR, PlayerCfg.MouseSens[4], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_THROTTLE, PlayerCfg.MouseSens[5], 0, 16); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	nm_set_item_text(& m[nitems], "Mouse FlightSim Deadzone:"); nitems++;
	mousefsdead = nitems;
	nm_set_item_slider(&m[nitems], "X/Y", PlayerCfg.MouseFSDead, 0, 16); nitems++;

	newmenu_do1(NULL, "SENSITIVITY & DEADZONE", nitems, m, NULL, NULL, 1);

	for (i = 0; i <= 5; i++)
	{
		if (i < 5)
			PlayerCfg.KeyboardSens[i] = m[keysens+i].value;
		PlayerCfg.JoystickSens[i] = m[joysens+i].value;
		PlayerCfg.JoystickDead[i] = m[joydead+i].value;
		PlayerCfg.MouseSens[i] = m[mousesens+i].value;
	}
	PlayerCfg.MouseFSDead = m[mousefsdead].value;
	PlayerCfg.OldKeyboardRamping = m[keysens+5].value;
}

static void input_config_stick(enum key_stick_type *field, char *name)
{
	newmenu_item m[3];

	// Nobody said life isn't terrible
	static_assert(KEY_STICK_NORMAL == 0, "");
	static_assert(KEY_STICK_TOGGLE == 1, "");
	static_assert(KEY_STICK_STICKY == 2, "");

	if ((unsigned)*field >= 3)
		*field = KEY_STICK_NORMAL;

	nm_set_item_radio(&m[0], "Active while key held", *field == 0, 0);
	nm_set_item_radio(&m[1], "Toggle active on press", *field == 1, 0);
	nm_set_item_radio(&m[2], "Toggle on short press,\ntemporary on long press", *field == 2, 0);

	newmenu_do1(NULL, name, 3, m, NULL, NULL, 0);

	for (int n = 0; n < 3; n++) {
		if (m[n].value)
			*field = n;
	}
}

static int opt_ic_usejoy = 0, opt_ic_usemouse = 0, opt_ic_confkey = 0, opt_ic_confjoy = 0, opt_ic_confmouse = 0, opt_ic_confweap = 0, opt_ic_mouseflightsim = 0, opt_ic_joymousesens = 0, opt_ic_grabinput = 0, opt_ic_mousefsgauge = 0, opt_ic_help0 = 0, opt_ic_help1 = 0, opt_ic_help2 = 0, opt_ic_conf_stick_rear, opt_ic_conf_stick_conv;

int input_config_menuset(newmenu *menu, d_event *event, void *userdata)
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	userdata = userdata;

	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			if (citem == opt_ic_usejoy)
				(items[citem].value)?(PlayerCfg.ControlType|=CONTROL_USING_JOYSTICK):(PlayerCfg.ControlType&=~CONTROL_USING_JOYSTICK);
			if (citem == opt_ic_usemouse)
				(items[citem].value)?(PlayerCfg.ControlType|=CONTROL_USING_MOUSE):(PlayerCfg.ControlType&=~CONTROL_USING_MOUSE);
			if (citem == opt_ic_mouseflightsim)
				PlayerCfg.MouseFlightSim = 0;
			if (citem == opt_ic_mouseflightsim+1)
				PlayerCfg.MouseFlightSim = 1;
			if (citem == opt_ic_grabinput)
				GameCfg.Grabinput = items[citem].value;
			if (citem == opt_ic_mousefsgauge)
				PlayerCfg.MouseFSIndicator = items[citem].value;
			break;

		case EVENT_NEWMENU_SELECTED:
			if (citem == opt_ic_confkey)
				kconfig(0, "KEYBOARD");
			if (citem == opt_ic_confjoy)
				kconfig(1, "JOYSTICK");
			if (citem == opt_ic_confmouse)
				kconfig(2, "MOUSE");
			if (citem == opt_ic_confweap)
				kconfig(3, "WEAPON KEYS");
			if (citem == opt_ic_joymousesens)
				input_config_sensitivity();
			if (citem == opt_ic_conf_stick_rear)
				input_config_stick(&PlayerCfg.KeyStickRearView, "Rear view");
			if (citem == opt_ic_conf_stick_conv)
				input_config_stick(&PlayerCfg.KeyStickEnergyConvert, "Energy->shield");
			if (citem == opt_ic_help0)
				show_help();
			if (citem == opt_ic_help1)
				show_netgame_help();
			if (citem == opt_ic_help2)
				show_newdemo_help();
			return 1;		// stay in menu
			break;

		default:
			break;
	}

	return 0;
}

void input_config()
{
	newmenu_item m[30];
	int nitems = 0;

	opt_ic_usejoy = nitems;
	nm_set_item_checkbox(&m[nitems], "USE JOYSTICK", (PlayerCfg.ControlType&CONTROL_USING_JOYSTICK)); nitems++;
	opt_ic_usemouse = nitems;
	nm_set_item_checkbox(&m[nitems], "USE MOUSE", (PlayerCfg.ControlType&CONTROL_USING_MOUSE)); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	opt_ic_confkey = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "CUSTOMIZE KEYBOARD"; nitems++;
	opt_ic_confjoy = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "CUSTOMIZE JOYSTICK"; nitems++;
	opt_ic_confmouse = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "CUSTOMIZE MOUSE"; nitems++;
	opt_ic_confweap = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "CUSTOMIZE WEAPON KEYS"; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "MOUSE CONTROL TYPE:"; nitems++;
	opt_ic_mouseflightsim = nitems;
	nm_set_item_radio(&m[nitems], "normal", !PlayerCfg.MouseFlightSim, 0); nitems++;
	nm_set_item_radio(&m[nitems], "FlightSim", PlayerCfg.MouseFlightSim, 0); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	opt_ic_joymousesens = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "SENSITIVITY & DEADZONE"; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;

	opt_ic_conf_stick_rear = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "Rear view key stickiness"; nitems++;
	opt_ic_conf_stick_conv = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "Energy->shield key stickiness"; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;

	opt_ic_grabinput = nitems;
	nm_set_item_checkbox(&m[nitems], "Keep Keyboard/Mouse focus", GameCfg.Grabinput); nitems++;
	opt_ic_mousefsgauge = nitems;
	nm_set_item_checkbox(&m[nitems], "Mouse FlightSim Indicator", PlayerCfg.MouseFSIndicator); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	opt_ic_help0 = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "GAME SYSTEM KEYS"; nitems++;
	opt_ic_help1 = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "NETGAME SYSTEM KEYS"; nitems++;
	opt_ic_help2 = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "DEMO SYSTEM KEYS"; nitems++;

	newmenu_do1(NULL, TXT_CONTROLS, nitems, m, input_config_menuset, NULL, 3);
}

void reticle_config()
{
	newmenu_item m[18];
	int nitems = 0, i, opt_ret_type, opt_ret_rgba, opt_ret_size;
	
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "Reticle Type:"; nitems++;
	opt_ret_type = nitems;
	nm_set_item_radio(&m[nitems], "Classic", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Classic Reboot", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "None", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "X", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Dot", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Circle", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Cross V1", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Cross V2", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Angle", 0, 0); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	nm_set_item_text(& m[nitems], "Reticle Color:"); nitems++;
	opt_ret_rgba = nitems;
	nm_set_item_slider(&m[nitems], "Red", (PlayerCfg.ReticleRGBA[0]/2), 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], "Green", (PlayerCfg.ReticleRGBA[1]/2), 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], "Blue", (PlayerCfg.ReticleRGBA[2]/2), 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], "Alpha", (PlayerCfg.ReticleRGBA[3]/2), 0, 16); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	opt_ret_size = nitems;
	nm_set_item_slider(&m[nitems], "Reticle Size:", PlayerCfg.ReticleSize, 0, 4); nitems++;

	i = PlayerCfg.ReticleType;
	m[opt_ret_type+i].value=1;

	newmenu_do1( NULL, "Reticle Options", nitems, m, NULL, NULL, 1 );

	for (i = 0; i < 9; i++)
		if (m[i+opt_ret_type].value)
			PlayerCfg.ReticleType = i;
	for (i = 0; i < 4; i++)
		PlayerCfg.ReticleRGBA[i] = (m[i+opt_ret_rgba].value*2);
	PlayerCfg.ReticleSize = m[opt_ret_size].value;
}

int opt_gr_texfilt, opt_gr_brightness, opt_gr_reticlemenu, opt_gr_alphafx, opt_gr_dynlightcolor, opt_gr_vsync, opt_gr_multisample, opt_gr_fpsindi;

int graphics_config_menuset(newmenu *menu, d_event *event, void *userdata)
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	userdata = userdata;

	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			if (citem == opt_gr_texfilt + 3 && ogl_maxanisotropy <= 1.0) {
				nm_messagebox( TXT_ERROR, "Anisotropic Filtering not\nsupported by your hardware/driver.", TXT_OK);
				items[opt_gr_texfilt + 3].value = 0;
				items[opt_gr_texfilt + 2].value = 1;
			}
			if ( citem == opt_gr_brightness)
				gr_palette_set_gamma(items[citem].value);
			break;

		case EVENT_NEWMENU_SELECTED:
			if (citem == opt_gr_reticlemenu)
				reticle_config();
			return 1;		// stay in menu
			break;

		default:
			break;
	}

	return 0;
}

void graphics_config()
{
	newmenu_item m[15];
	int i = 0;
	int nitems = 0;
	int opt_fullscr;

	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "Texture Filtering:"; nitems++;
	opt_gr_texfilt = nitems;
	nm_set_item_radio(&m[nitems++], "None (Classical)", 0, 0);
	nm_set_item_radio(&m[nitems++], "Bilinear", 0, 0);
	nm_set_item_radio(&m[nitems++], "Trilinear", 0, 0);
	nm_set_item_radio(&m[nitems++], "Anisotropic", 0, 0);
	nm_set_item_text(& m[nitems], ""); nitems++;
	opt_gr_brightness = nitems;
	nm_set_item_slider(&m[nitems], TXT_BRIGHTNESS, gr_palette_get_gamma(), 0, 16); nitems++;
	opt_gr_reticlemenu = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "Reticle Options"; nitems++;
	opt_gr_alphafx = nitems;
	nm_set_item_checkbox(&m[nitems], "Transparency Effects", PlayerCfg.AlphaEffects); nitems++;
	opt_gr_dynlightcolor = nitems;
	nm_set_item_checkbox(&m[nitems], "Colored Dynamic Light", PlayerCfg.DynLightColor); nitems++;
	opt_gr_vsync = nitems;
	nm_set_item_checkbox(&m[nitems],"VSync", GameCfg.VSync); nitems++;
	opt_gr_multisample = nitems;
	nm_set_item_checkbox(&m[nitems],"4x multisampling", GameCfg.Multisample); nitems++;
	opt_gr_fpsindi = nitems;
	nm_set_item_checkbox(&m[nitems],"FPS Counter", GameArg.SysFPSIndicator); nitems++;
	m[opt_gr_texfilt+GameCfg.TexFilt].value=1;
	opt_fullscr = nitems;
	nm_set_item_checkbox(&m[nitems], "Fullscreen", gr_check_fullscreen()); nitems++;

	newmenu_do1( NULL, "Graphics Options", nitems, m, graphics_config_menuset, NULL, 1 );

	if (m[opt_fullscr].value != gr_check_fullscreen())
		gr_toggle_fullscreen();

	if (GameCfg.VSync != m[opt_gr_vsync].value || GameCfg.Multisample != m[opt_gr_multisample].value)
		nm_messagebox( NULL, "Setting VSync or 4x Multisample\nrequires restart on some systems.", TXT_OK);

	for (i = 0; i <= 3; i++)
		if (m[i+opt_gr_texfilt].value)
			GameCfg.TexFilt = i;
	PlayerCfg.AlphaEffects = m[opt_gr_alphafx].value;
	PlayerCfg.DynLightColor = m[opt_gr_dynlightcolor].value;
	GameCfg.VSync = m[opt_gr_vsync].value;
	GameCfg.Multisample = m[opt_gr_multisample].value;
	GameCfg.GammaLevel = m[opt_gr_brightness].value;
	gr_set_attributes();
}

#define PATH_HEADER_TYPE NM_TYPE_TEXT
#define BROWSE_TXT

int opt_sm_digivol = -1, opt_sm_musicvol = -1, opt_sm_revstereo = -1, opt_sm_mtype0 = -1, opt_sm_mtype1 = -1;

void set_extmusic_volume(int volume);

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
			break;

		case EVENT_NEWMENU_SELECTED:
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

#define SOUND_MENU_NITEMS 33

void do_sound_menu()
{
	newmenu_item *m;
	int nitems = 0;
	char old_CMLevelMusicPath[PATH_MAX+1], old_CMMiscMusic0[PATH_MAX+1];

	memset(old_CMLevelMusicPath, 0, (PATH_MAX+1));
	snprintf(old_CMLevelMusicPath, sizeof(old_CMLevelMusicPath), "%s", GameCfg.CMLevelMusicPath);
	memset(old_CMMiscMusic0, 0, PATH_MAX+1);
	snprintf(old_CMMiscMusic0, sizeof(old_CMMiscMusic0), "%s", GameCfg.CMMiscMusic[SONG_TITLE]);

	MALLOC(m, newmenu_item, SOUND_MENU_NITEMS);
	if (!m)
		return;

	opt_sm_digivol = nitems;
	nm_set_item_slider(&m[nitems++], TXT_FX_VOLUME, GameCfg.DigiVolume, 0, 8);

	opt_sm_musicvol = nitems;
	nm_set_item_slider(&m[nitems++], "music volume", GameCfg.MusicVolume, 0, 8);

	opt_sm_revstereo = nitems;
	nm_set_item_checkbox(&m[nitems++], TXT_REVERSE_STEREO, GameCfg.ReverseStereo);

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "music type:";

	opt_sm_mtype0 = nitems;
	nm_set_item_radio(&m[nitems], "no music", (GameCfg.MusicType == MUSIC_TYPE_NONE), 0); nitems++;

#if defined(USE_SDLMIXER) || defined(_WIN32)
	opt_sm_mtype1 = nitems;
	nm_set_item_radio(&m[nitems], "built-in/addon music", (GameCfg.MusicType == MUSIC_TYPE_BUILTIN), 0); nitems++;
#endif

	Assert(nitems <= SOUND_MENU_NITEMS);

	newmenu_do1( NULL, "Sound Effects & Music", nitems, m, sound_menuset, NULL, 0 );

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
	newmenu_item m[18];
	int i = 0;

	do {
		ADD_CHECK(0, "Ship auto-leveling", PlayerCfg.AutoLeveling);
		ADD_CHECK(1, "Missile view", PlayerCfg.MissileViewEnabled);
		ADD_CHECK(2, "Headlight on when picked up", PlayerCfg.HeadlightActiveDefault );
		ADD_CHECK(3, "Show guided missile in main display", PlayerCfg.GuidedInBigWindow );
		ADD_CHECK(4, "Escort robot hot keys",PlayerCfg.EscortHotKeys);
		ADD_CHECK(5, "Persistent Debris",PlayerCfg.PersistentDebris);
		ADD_CHECK(6, "Screenshots w/o HUD",PlayerCfg.PRShot);
		ADD_CHECK(7, "Movie Subtitles",GameCfg.MovieSubtitles);
		ADD_CHECK(8, "No redundant pickup messages",PlayerCfg.NoRedundancy);
		ADD_CHECK(9, "Show Player chat only (Multi)",PlayerCfg.MultiMessages);
		ADD_CHECK(10, "No Rankings (Multi)",PlayerCfg.NoRankings);
		ADD_CHECK(11, "Free Flight controls in Automap",PlayerCfg.AutomapFreeFlight);
		ADD_CHECK(12, "No Weapon Autoselect when firing",PlayerCfg.NoFireAutoselect);
		ADD_CHECK(13, "Extended ammo rack mode", PlayerCfg.ExtendedAmmoRack);
		ADD_CHECK(14, "Skip game startup movie/delay", GameCfg.SkipProgramIntro);
		ADD_CHECK(15, "Skip level briefing", PlayerCfg.SkipLevelBriefing);
		ADD_CHECK(16, "Skip level movies", PlayerCfg.SkipLevelMovies);
		ADD_CHECK(17, "Debug mode", Debug_mode);

		i = newmenu_do1( NULL, "Misc Options", sizeof(m)/sizeof(*m), m, NULL, NULL, i );

		PlayerCfg.AutoLeveling			= m[0].value;
		PlayerCfg.MissileViewEnabled   		= m[1].value;
		PlayerCfg.HeadlightActiveDefault	= m[2].value;
		PlayerCfg.GuidedInBigWindow		= m[3].value;
		PlayerCfg.EscortHotKeys			= m[4].value;
		PlayerCfg.PersistentDebris		= m[5].value;
		PlayerCfg.PRShot 			= m[6].value;
		GameCfg.MovieSubtitles 			= m[7].value;
		PlayerCfg.NoRedundancy 			= m[8].value;
		PlayerCfg.MultiMessages 		= m[9].value;
		PlayerCfg.NoRankings 			= m[10].value;
		PlayerCfg.AutomapFreeFlight		= m[11].value;
		PlayerCfg.NoFireAutoselect		= m[12].value;
		PlayerCfg.ExtendedAmmoRack		= m[13].value;
		GameCfg.SkipProgramIntro		= m[14].value;
		PlayerCfg.SkipLevelBriefing		= m[15].value;
		PlayerCfg.SkipLevelMovies		= m[16].value;
		Debug_mode						= m[17].value;

	} while( i>-1 );

}

#if defined(USE_UDP)
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

	MALLOC(menu_choice, int, 3);
	if (!menu_choice)
		return;

	MALLOC(m, newmenu_item, 3);
	if (!m)
	{
		d_free(menu_choice);
		return;
	}

#ifdef USE_UDP
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="HOST GAME"; menu_choice[num_options]=MENU_START_UDP_NETGAME; num_options++;
#ifdef USE_TRACKER
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="FIND LAN/ONLINE GAMES"; menu_choice[num_options]=MENU_JOIN_LIST_UDP_NETGAME; num_options++;
#else
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="FIND LAN GAMES"; menu_choice[num_options]=MENU_JOIN_LIST_UDP_NETGAME; num_options++;
#endif
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="JOIN GAME MANUALLY"; menu_choice[num_options]=MENU_JOIN_MANUAL_UDP_NETGAME; num_options++;
#endif

	newmenu_do3( NULL, TXT_MULTIPLAYER, num_options, m, (int (*)(newmenu *, d_event *, void *))multi_player_menu_handler, menu_choice, 0, NULL );
}
#endif

void do_options_menu()
{
	newmenu_item *m;

	MALLOC(m, newmenu_item, 10);
	if (!m)
		return;

	m[ 0].type = NM_TYPE_MENU;   m[ 0].text="Sound effects & music...";
	m[ 1].type = NM_TYPE_TEXT;   m[ 1].text="";
	m[ 2].type = NM_TYPE_MENU;   m[ 2].text=TXT_CONTROLS_;
	m[ 3].type = NM_TYPE_TEXT;   m[ 3].text="";
	m[ 4].type = NM_TYPE_TEXT;   m[ 4].text="";
	m[ 5].type = NM_TYPE_MENU;   m[ 5].text="Graphics Options...";
	m[ 6].type = NM_TYPE_TEXT;   m[ 6].text="";
	m[ 7].type = NM_TYPE_MENU;   m[ 7].text="Primary autoselect ordering...";
	m[ 8].type = NM_TYPE_MENU;   m[ 8].text="Secondary autoselect ordering...";
	m[ 9].type = NM_TYPE_MENU;   m[ 9].text="Misc Options...";

	// Fall back to main event loop
	// Allows clean closing and re-opening when resolution changes
	newmenu_do3( NULL, TXT_OPTIONS, 10, m, options_menuset, NULL, 0, NULL );
}

int polygon_models_viewer_handler(window *wind, d_event *event)
{
	static int view_idx = 0;
	int key = 0;
	static vms_angvec ang;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			gr_use_palette_table("groupa.256");
			view_idx = 0;
			ang.p = ang.b = 0;
			ang.h = F1_0/2;
			break;
		case EVENT_KEY_COMMAND:
			key = event_key_get(event);
			switch (key)
			{
				case KEY_ESC:
					window_close(wind);
					break;
				case KEY_SPACEBAR:
					view_idx ++;
					if (view_idx >= N_polygon_models) view_idx = 0;
					break;
				case KEY_BACKSP:
					view_idx --;
					if (view_idx < 0 ) view_idx = N_polygon_models - 1;
					break;
				case KEY_A:
					ang.h -= 100;
					break;
				case KEY_D:
					ang.h += 100;
					break;
				case KEY_W:
					ang.p -= 100;
					break;
				case KEY_S:
					ang.p += 100;
					break;
				case KEY_Q:
					ang.b -= 100;
					break;
				case KEY_E:
					ang.b += 100;
					break;
				case KEY_R:
					ang.p = ang.b = 0;
					ang.h = F1_0/2;
					break;
				default:
					break;
			}
			return 1;
		case EVENT_WINDOW_DRAW:
			timer_delay(F1_0/60);
			draw_model_picture(view_idx, &ang);
			gr_set_curfont(GAME_FONT);
			gr_set_fontcolor(BM_XRGB(255,255,255), -1);
			gr_printf(FSPACX(1), FSPACY(1), "ESC: leave\nSPACE/BACKSP: next/prev model (%i/%i)\nA/D: rotate y\nW/S: rotate x\nQ/E: rotate z\nR: reset orientation",view_idx,N_polygon_models-1);
			break;
		case EVENT_WINDOW_CLOSE:
			load_palette(MENU_PALETTE,0,1);
			break;
		default:
			break;
	}
	
	return 0;
}

void polygon_models_viewer()
{
	window *wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))polygon_models_viewer_handler, NULL);
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		polygon_models_viewer_handler(NULL, &event);
		return;
	}

	window_run_event_loop(wind);
}

int gamebitmaps_viewer_handler(window *wind, d_event *event)
{
	static int view_idx = 0;
	int key = 0;
	float scale = 1.0;
	bitmap_index bi;
	grs_bitmap *bm;
	extern int Num_bitmap_files;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			gr_use_palette_table("groupa.256");
			view_idx = 0;
			break;
		case EVENT_KEY_COMMAND:
			key = event_key_get(event);
			switch (key)
			{
				case KEY_ESC:
					window_close(wind);
					break;
				case KEY_SPACEBAR:
					view_idx ++;
					if (view_idx >= Num_bitmap_files) view_idx = 0;
					break;
				case KEY_BACKSP:
					view_idx --;
					if (view_idx < 0 ) view_idx = Num_bitmap_files - 1;
					break;
				default:
					break;
			}
			return 1;
		case EVENT_WINDOW_DRAW:
			bi.index = view_idx;
			bm = &GameBitmaps[view_idx];
			timer_delay(F1_0/60);
			PIGGY_PAGE_IN(bi);
			gr_clear_canvas( BM_XRGB(0,0,0) );
			scale = (bm->bm_w > bm->bm_h)?(SHEIGHT/bm->bm_w)*0.8:(SHEIGHT/bm->bm_h)*0.8;
			ogl_ubitmapm_cs((SWIDTH/2)-(bm->bm_w*scale/2),(SHEIGHT/2)-(bm->bm_h*scale/2),bm->bm_w*scale,bm->bm_h*scale,bm,-1,F1_0);
			gr_set_curfont(GAME_FONT);
			gr_set_fontcolor(BM_XRGB(255,255,255), -1);
			gr_printf(FSPACX(1), FSPACY(1), "ESC: leave\nSPACE/BACKSP: next/prev bitmap (%i/%i)",view_idx,Num_bitmap_files-1);
			break;
		case EVENT_WINDOW_CLOSE:
			load_palette(MENU_PALETTE,0,1);
			break;
		default:
			break;
	}
	
	return 0;
}

void gamebitmaps_viewer()
{
	window *wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))gamebitmaps_viewer_handler, NULL);
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		gamebitmaps_viewer_handler(NULL, &event);
		return;
	}

	window_run_event_loop(wind);
}

int sandbox_menuset(newmenu *menu, d_event *event, void *userdata)
{
	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			break;

		case EVENT_NEWMENU_SELECTED:
			switch(newmenu_get_citem(menu))
			{
				case  0: polygon_models_viewer(); break;
				case  1: gamebitmaps_viewer(); break;
			}
			return 1; // stay in menu until escape
			break;

		case EVENT_WINDOW_CLOSE:
		{
			newmenu_item *items = newmenu_get_items(menu);
			d_free(items);
			break;
		}

		default:
			break;
	}

	userdata = userdata; //kill warning

	return 0;
}

void do_sandbox_menu()
{
	newmenu_item *m;

	MALLOC(m, newmenu_item, 2);
	if (!m)
		return;

	m[ 0].type = NM_TYPE_MENU;   m[ 0].text="Polygon_models viewer";
	m[ 1].type = NM_TYPE_MENU;   m[ 1].text="GameBitmaps viewer";

	newmenu_do3( NULL, "Coder's sandbox", 2, m, sandbox_menuset, NULL, 0, NULL );
}
