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
 * inferno.c: Entry point of program (main procedure)
 *
 * After main initializes everything, most of the time is spent in the loop
 * while (window_get_front())
 * In this loop, the main menu is brought up first.
 *
 * main() for Inferno
 *
 */

char copyright[] = "DESCENT II  COPYRIGHT (C) 1994-1996 PARALLAX SOFTWARE CORPORATION";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "gr.h"
#include "fix.h"
#include "vecmat.h"
#include "key.h"
#include "3d.h"
#include "bm.h"
#include "inferno.h"
#include "dxxerror.h"
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "u_mem.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "wall.h"
#include "polyobj.h"
#include "effects.h"
#include "digi.h"
#include "palette.h"
#include "args.h"
#include "sounds.h"
#include "titles.h"
#include "player.h"
#include "text.h"
#include "gauges.h"
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "multi.h"
#include "songs.h"
#include "cfile.h"
#include "gameseq.h"
#include "gamepal.h"
#include "mission.h"
#include "movie.h"
#include "playsave.h"
#include "collide.h"
#include "newdemo.h"
#include "joy.h"
#include "event.h"

#include <SDL.h>

#include "vers_id.h"

//Current version number

ubyte Version_major = 1;		//FULL VERSION
ubyte Version_minor = 2;

char desc_id_exit_num = 0;
int descent_critical_error = 0;
unsigned int descent_critical_deverror = 0;
unsigned int descent_critical_errcode = 0;

extern int Network_allow_socket_changes;
extern void piggy_init_pigfile(char *filename);
extern void arch_init(void);

#define LINE_LEN	100

//read help from a file & print to screen
void print_commandline_help()
{
	printf( "\n System Options:\n\n");
	printf( "  -nonicefps             %s\n", "Don't free CPU-cycles");
	printf( "  -maxfps <n>                   Set maximum framerate to <n>\n\t\t\t\t(default: %i, availble: 1-%i)\n", MAXIMUM_FPS, MAXIMUM_FPS);
	printf( "  -hogdir <s>            %s\n", "set shared data directory to <dir>");
	printf( "  -nohogdir              %s\n", "don't try to use shared data directory");
	printf( "  -use_players_dir       %s\n", "put player files and saved games in Players subdirectory");
	printf( "  -lowmem                %s\n", "Lowers animation detail for better performance with low memory");
	printf( "  -pilot <s>             %s\n", "Select this pilot automatically");
	printf( "  -autodemo              %s\n", "Start in demo mode");
	printf( "  -window                %s\n", "Run the game in a window");
	printf( "  -noborders             %s\n", "Do not show borders in window mode");
	printf( "  -nomovies              %s\n", "Don't play movies");

	printf( "\n Controls:\n\n");
	printf( "  -nomouse               %s\n", "Deactivate mouse");
	printf( "  -nojoystick            %s\n", "Deactivate joystick");
	printf( "  -nostickykeys          %s\n", "Make CapsLock and NumLock non-sticky so they can be used as normal keys");

	printf( "\n Sound:\n\n");
	printf( "  -nosound               %s\n", "Disables sound output");
	printf( "  -nomusic               %s\n", "Disables music output");
	printf( "  -sound11k              %s\n", "Use 11KHz sounds");
#ifdef    USE_SDLMIXER
	printf( "  -nosdlmixer            %s\n", "Disable Sound output via SDL_mixer");
#endif // USE SDLMIXER

	printf( "\n Graphics:\n\n");
	printf( "  -lowresfont            %s\n", "Force to use LowRes fonts");
	printf( "  -lowresgraphics        %s\n", "Force to use LowRes graphics");
	printf( "  -lowresmovies          %s\n", "Play low resolution movies if available (for slow machines)");
#ifdef    OGL
	printf( "  -gl_fixedfont          %s\n", "Do not scale fonts to current resolution");
#endif // OGL

#if defined(USE_UDP)
	printf( "\n Multiplayer:\n\n");
	printf( "  -udp_hostaddr <s>             Use IP address/Hostname <s> for manual game joining\n\t\t\t\t(default: %s)\n", "localhost");
#ifdef USE_TRACKER
	printf( "  -tracker_hostaddr <n>  %s\n", "Address of Tracker server to register/query games to/from (default: dxxtracker.reenigne.net)");
	printf( "  -tracker_hostport <n>  %s\n", "Port of Tracker server to register/query games to/from (default: 42420)");
#endif // USE_TRACKER
#endif // defined(USE_UDP)

	printf( "\n Debug (use only if you know what you're doing):\n\n");
	printf( "  -debug                 %s\n", "Enable very verbose output");
	printf( "  -verbose               %s\n", "Shows initialization steps for tech support");
	printf( "  -norun                 %s\n", "Bail out after initialization");
	printf( "  -renderstats           %s\n", "Enable renderstats info by default");
	printf( "  -text <s>              %s\n", "Specify alternate .tex file");
	printf( "  -tmap <s>              %s\n", "Select texmapper to use (c,fp,quad,i386)");
	printf( "  -showmeminfo           %s\n", "Show memory statistics");
	printf( "  -nodoublebuffer        %s\n", "Disable Doublebuffering");
	printf( "  -16bpp                 %s\n", "Use 16Bpp instead of 32Bpp");
#ifdef    OGL
	printf( "  -gl_oldtexmerge        %s\n", "Use old texmerge, uses more ram, but _might_ be a bit faster");
#else
	printf( "  -hwsurface             %s\n", "Use SDL HW Surface");
	printf( "  -asyncblit             %s\n", "Use queued blits over SDL. Can speed up rendering");
#endif // OGL

	printf( "\n Help:\n\n");
	printf( "  -help, -h, -?, ?       %s\n", "View this help screen");
	printf( "\n\n");
}

// Default event handler for everything
int standard_handler(d_event *event)
{
	int key;

	switch (event->type)
	{
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			// No window selecting
			// We stay with the current one until it's closed/hidden or another one is made
			break;

		case EVENT_KEY_COMMAND:
			key = event_key_get(event);

			switch (key)
			{
				case KEY_PRINT_SCREEN:
				{
					gr_set_current_canvas(NULL);
					save_screen_shot(0);
					return 1;
				}

				case KEY_ALTED+KEY_ENTER:
				case KEY_ALTED+KEY_PADENTER:
					gr_toggle_fullscreen();
					return 1;
			}
			break;

		case EVENT_WINDOW_DRAW:
		case EVENT_IDLE:
			return 1;

		default:
			break;
	}

	return 0;
}

#define PROGNAME argv[0]

//	DESCENT II by Parallax Software
//		Descent Main

int main(int argc, char *argv[])
{
	cfile_init_paths(argc, argv);
	InitArgs(argc, argv);

	if (GameArg.SysShowCmdHelp) {
		print_commandline_help();

		return(0);
	}

	printf("\nType %s -help' for a list of command-line options.\n\n", PROGNAME);

	if (! cfile_hog_add("descent2.hog", 1)) {
		if (! cfile_hog_add("d2demo.hog", 1))
		{
			Error("Could not find a valid hog file (descent2.hog or d2demo.hog)\nPossible locations are:\n"
			      "\t$HOME/.d2x-rebirth\n"
			      "\t" SHAREPATH "\n"
				  "Or use the -hogdir option to specify an alternate location.");
		}
	}

	load_text();

	//print out the banner title
	con_printf(CON_NORMAL, "DESCENT 2 %s v%d.%d",VERSION_TYPE,Version_major,Version_minor);
	#if 1	//def VERSION_NAME
	con_printf(CON_NORMAL, "  %s", DESCENT_VERSION);	// D2X version
	#endif
	if (cfexist(MISSION_DIR "d2x.hog")) {
		con_printf(CON_NORMAL, "  Vertigo Enhanced");
	}

	con_printf(CON_NORMAL, "  %s %s\n", __DATE__,__TIME__);
	con_printf(CON_NORMAL, "%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);
	con_printf(CON_NORMAL, "This is a MODIFIED version of Descent 2. Copyright (c) 1999 Peter Hawkins\n");
	con_printf(CON_NORMAL, "                                         Copyright (c) 2002 Bradley Bell\n");
	con_printf(CON_NORMAL, "                                         Copyright (c) 2005 Christian Beckhaeuser\n\n");

	if (GameArg.DbgVerbose)
		con_printf(CON_VERBOSE,"%s%s", TXT_VERBOSE_1, "\n");
	
	ReadConfigFile();

	arch_init();

	con_printf(CON_VERBOSE, "Going into graphics mode...\n");
	gr_create_window();

	// Load the palette stuff. Returns non-zero if error.
	con_printf(CON_DEBUG, "Initializing palette system...\n" );
	gr_use_palette_table(D2_DEFAULT_PALETTE );

	con_printf(CON_DEBUG, "Initializing font system...\n" );
	gamefont_init();	// must load after palette data loaded.

	set_default_handler(standard_handler);

	con_printf( CON_DEBUG, "Initializing movie libraries...\n" );
	init_movies();		//init movie libraries

	if (!GameArg.SysNoMovies && !GameCfg.SkipProgramIntro)
		show_titles();

	gr_set_current_canvas(NULL);

	con_printf( CON_DEBUG, "\nDoing gamedata_init..." );
	gamedata_init();

	if (GameArg.DbgNoRun)
		return(0);

	con_printf( CON_DEBUG, "\nInitializing texture caching system..." );
	texmerge_init( 10 );		// 10 cache bitmaps

	piggy_init_pigfile("groupa.pig");	//get correct pigfile

	con_printf( CON_DEBUG, "\nRunning game...\n" );
	init_game();

	Players[Player_num].callsign[0] = '\0';

	{
		if(GameArg.SysPilot)
		{
			char filename[32] = "";
			int j;

			if (GameArg.SysUsePlayersDir)
				strcpy(filename, "Players/");
			strncat(filename, GameArg.SysPilot, 12);
			filename[8 + 12] = '\0';	// unfortunately strncat doesn't put the terminating 0 on the end if it reaches 'n'
			for (j = GameArg.SysUsePlayersDir? 8 : 0; filename[j] != '\0'; j++) {
				switch (filename[j]) {
					case ' ':
						filename[j] = '\0';
				}
			}
			if(!strstr(filename,".plr")) // if player hasn't specified .plr extension in argument, add it
				strcat(filename,".plr");
			if(cfexist(filename))
			{
				strcpy(strstr(filename,".plr"),"\0");
				strcpy(Players[Player_num].callsign, GameArg.SysUsePlayersDir? &filename[8] : filename);
				read_player_file();
				WriteConfigFile();
			}
		}
	}

	{
		Game_mode = GM_GAME_OVER;
		DoMenu();
	}

	while (1) {
		window *win = window_get_front();
		if (!win)
			break;
		window_run_event_loop(win);
	}

	WriteConfigFile();
	show_order_form();

	con_printf( CON_DEBUG, "\nCleanup...\n" );
	close_game();
	texmerge_close();
	gamedata_close();
	gamefont_close();
	free_text();
	args_exit();
	newmenu_free_background();
	free_mission();

	return(0);		//presumably successful exit
}
