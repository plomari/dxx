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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Functions for accessing arguments.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "physfsx.h"
#include "args.h"
#include "u_mem.h"
#include "strio.h"
#include "strutil.h"
#include "digi.h"
#include "game.h"
#include "gauges.h"
#include "console.h"

#define MAX_ARGS 1000
#define INI_FILENAME "d2x.ini"

int Num_args=0;
char * Args[MAX_ARGS];

struct Arg GameArg;

void ReadCmdArgs(void);

static int FindArg(const char *const s)
{
	int i;

	for (i=0; i<Num_args; i++ )
		if (! stricmp( Args[i], s))
			return i;

	return 0;
}

void AppendIniArgs(void)
{
	PHYSFS_file *f;
	char *line, *token;
	char separator[] = " ";

	f = PHYSFSX_openReadBuffered(INI_FILENAME);

	if(f) {
		while(!PHYSFS_eof(f) && Num_args < MAX_ARGS)
		{
			line=fgets_unlimited(f);

			token = strtok(line, separator);        /* first token in current line */
			if (token)
				Args[Num_args++] = d_strdup(token);
			while( token != NULL )
			{
				token = strtok(NULL, separator);        /* next tokens in current line */
				if (token)
					Args[Num_args++] = d_strdup(token);
			}

			d_free(line);
		}
		PHYSFS_close(f);
	}
}

// Utility function to get an integer provided as argument
int get_int_arg(char *arg_name, int default_value) {
	int t;
	return ((t = FindArg(arg_name)) ? atoi(Args[t+1]) : default_value);

}
// Utility function to get a string provided as argument
char *get_str_arg(char *arg_name, char *default_value) {
	int t;
	return ((t = FindArg(arg_name)) ? Args[t+1] : default_value);
}

// All FindArg calls should be here to keep the code clean
void ReadCmdArgs(void)
{
	// System Options

	GameArg.SysShowCmdHelp 		= (FindArg( "-help" ) || FindArg( "-h" ) || FindArg( "-?" ) || FindArg( "?" ));
	GameArg.SysFPSIndicator 	= FindArg("-fps");
	GameArg.SysUseNiceFPS 		= !FindArg("-nonicefps");

	GameArg.SysMaxFPS = get_int_arg("-maxfps", MAXIMUM_FPS);
	if (GameArg.SysMaxFPS <= MINIMUM_FPS)
		GameArg.SysMaxFPS = MINIMUM_FPS;
	else if (GameArg.SysMaxFPS > MAXIMUM_FPS)
		GameArg.SysMaxFPS = MAXIMUM_FPS;

	GameArg.SysHogDir = get_str_arg("-hogdir", NULL);
	if (GameArg.SysHogDir == NULL)
		GameArg.SysNoHogDir = FindArg("-nohogdir");

	GameArg.SysUsePlayersDir 	= FindArg("-use_players_dir");
	GameArg.SysLowMem 		= FindArg("-lowmem");
	GameArg.SysPilot 		= get_str_arg("-pilot", NULL);
	GameArg.SysWindow 		= FindArg("-window");
	GameArg.SysNoMovies 		= FindArg("-nomovies");
	GameArg.SysAutoDemo 		= FindArg("-autodemo");

	// Control Options

	GameArg.CtlNoMouse 		= FindArg("-nomouse");
	GameArg.CtlNoJoystick 		= FindArg("-nojoystick");
	GameArg.CtlNoStickyKeys		= FindArg("-nostickykeys");

	// Sound Options

	GameArg.SndNoSound 		= FindArg("-nosound");
	GameArg.SndNoMusic 		= FindArg("-nomusic");
	GameArg.SndDigiSampleRate 	= (FindArg("-sound11k") ? SAMPLE_RATE_11K : SAMPLE_RATE_22K);

#ifdef USE_SDLMIXER
	GameArg.SndDisableSdlMixer 	= FindArg("-nosdlmixer");
#else
	GameArg.SndDisableSdlMixer	= 1;
#endif


	// Graphics Options

	GameArg.GfxHiresGFXAvailable	= !FindArg("-lowresgraphics");
	GameArg.GfxHiresFNTAvailable	= !FindArg("-lowresfont");
	GameArg.GfxMovieHires 		= !FindArg( "-lowresmovies" );

	// OpenGL Options

	GameArg.OglFixedFont 		= FindArg("-gl_fixedfont");

	// Multiplayer Options

#ifdef USE_UDP
	GameArg.MplUdpHostAddr		= get_str_arg("-udp_hostaddr", "localhost");
#ifdef USE_TRACKER
	GameArg.MplTrackerAddr		= get_str_arg("-tracker_hostaddr", "dxxtracker.reenigne.net");
	GameArg.MplTrackerPort		= get_int_arg("-tracker_hostport", 42420);
#endif
#endif

	// Debug Options

	if (FindArg("-debug"))		GameArg.DbgVerbose = CON_DEBUG;
	else if (FindArg("-verbose"))	GameArg.DbgVerbose = CON_VERBOSE;
	else				GameArg.DbgVerbose = CON_NORMAL;

	GameArg.DbgNoRun 		= FindArg("-norun");
	GameArg.DbgRenderStats 		= FindArg("-renderstats");
	GameArg.DbgAltTex 		= get_str_arg("-text", NULL);
	GameArg.DbgTexMap 		= get_str_arg("-tmap", NULL);
	GameArg.DbgShowMemInfo 		= FindArg("-showmeminfo");
	GameArg.DbgUseDoubleBuffer 	= !FindArg("-nodoublebuffer");

	GameArg.DbgAltTexMerge 		= !FindArg("-gl_oldtexmerge");
}

void args_exit(void)
{
	int i;
	for (i=0; i< Num_args; i++ )
		d_free(Args[i]);
}

void InitArgs( int argc,char **argv )
{
	int i;

	Num_args=0;

	for (i=0; i<argc; i++ )
		Args[Num_args++] = d_strdup( argv[i] );


	for (i=0; i< Num_args; i++ ) {
		if ( Args[i][0] == '-' )
			strlwr( Args[i]  );  // Convert all args to lowercase
	}

	AppendIniArgs();
	ReadCmdArgs();
}
