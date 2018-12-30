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
 * contains routine(s) to read in the configuration file which contains
 * game configuration stuff like detail level, sound card, etc
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "cfile.h"
#include "pstypes.h"
#include "game.h"
#include "menu.h"
#include "movie.h"
#include "digi.h"
#include "kconfig.h"
#include "palette.h"
#include "joy.h"
#include "songs.h"
#include "args.h"
#include "player.h"
#include "mission.h"
#include "physfsx.h"

struct Cfg GameCfg;

static char *DigiVolumeStr = "DigiVolume";
static char *MusicVolumeStr = "MidiVolume";
static char *SndEnableRedbookStr = "RedbookEnabled";
static char *ReverseStereoStr = "ReverseStereo";
static char *GammaLevelStr = "GammaLevel";
static char *LastPlayerStr = "LastPlayer";
static char *LastMissionStr = "LastMission";
static char *LastLevelStr = "LastLevel";
static char *ResolutionXStr="ResolutionX";
static char *ResolutionYStr="ResolutionY";
static char *AspectXStr="AspectX";
static char *AspectYStr="AspectY";
static char *WindowModeStr="WindowMode";
static char *TexFiltStr="TexFilt";
static char *VSyncStr="VSync";
static char *MultisampleStr="Multisample";
static char *JukeboxOnStr="JukeboxOn";
static char *JukeboxPathStr="JukeboxPath";
static char *IPHostAddrStr="IPHostAddr";

int ReadConfigFile()
{
	PHYSFS_file *infile;
	char line[PATH_MAX+50], *token, *value, *ptr;

	// set defaults
	GameCfg.DigiVolume = 8;
	GameCfg.MusicVolume = 8;
#if defined(__APPLE__) || defined(macintosh)	// MIDI is buggy
	GameCfg.SndEnableRedbook = 1;
#else
	GameCfg.SndEnableRedbook = 0;
#endif
	GameCfg.ReverseStereo = 0;
	GameCfg.GammaLevel = 0;
	memset(GameCfg.LastPlayer,0,CALLSIGN_LEN+1);
	memset(GameCfg.LastMission,0,MISSION_NAME_LEN+1);
        GameCfg.LastLevel = 1;
	GameCfg.ResolutionX = 640;
	GameCfg.ResolutionY = 480;
	GameCfg.AspectX = 3;
	GameCfg.AspectY = 4;
	GameCfg.WindowMode = 0;
	GameCfg.TexFilt = 0;
	GameCfg.VSync = 0;
	GameCfg.Multisample = 0;
	GameCfg.JukeboxOn = 0;
	memset(GameCfg.JukeboxPath,0,PATH_MAX+1);
#ifndef macintosh	// Mac OS 9 binary is in .app bundle
	strncpy(GameCfg.JukeboxPath, "Jukebox", PATH_MAX+1);	// maybe include this directory with the binary
#else
	strncpy(GameCfg.JukeboxPath, "::::Jukebox", PATH_MAX+1);
#endif
	memset(GameCfg.MplIpHostAddr, '\x0', sizeof(GameCfg.MplIpHostAddr));

	infile = PHYSFSX_openReadBuffered("descent.cfg");

	if (infile == NULL) {
		return 1;
	}

	while (!PHYSFS_eof(infile))
	{
		cfile_gets_nl(infile, line, sizeof(line));
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') {
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (!value)
				value = "";
			if (!strcmp(token, DigiVolumeStr))
				GameCfg.DigiVolume = strtol(value, NULL, 10);
			else if (!strcmp(token, MusicVolumeStr))
				GameCfg.MusicVolume = strtol(value, NULL, 10);
			else if (!strcmp(token, SndEnableRedbookStr))
				GameCfg.SndEnableRedbook = strtol(value, NULL, 10);
			else if (!strcmp(token, ReverseStereoStr))
				GameCfg.ReverseStereo = strtol(value, NULL, 10);
			else if (!strcmp(token, GammaLevelStr)) {
				GameCfg.GammaLevel = strtol(value, NULL, 10);
				gr_palette_set_gamma( GameCfg.GammaLevel );
			}
			else if (!strcmp(token, LastPlayerStr))	{
				char * p;
				strncpy( GameCfg.LastPlayer, value, CALLSIGN_LEN );
				p = strchr( GameCfg.LastPlayer, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, LastMissionStr))	{
				char * p;
				strncpy( GameCfg.LastMission, value, MISSION_NAME_LEN );
				p = strchr( GameCfg.LastMission, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, LastLevelStr))
				GameCfg.LastLevel = strtol(value, NULL, 10);
			else if (!strcmp(token, ResolutionXStr))
				GameCfg.ResolutionX = strtol(value, NULL, 10);
			else if (!strcmp(token, ResolutionYStr))
				GameCfg.ResolutionY = strtol(value, NULL, 10);
			else if (!strcmp(token, AspectXStr))
				GameCfg.AspectX = strtol(value, NULL, 10);
			else if (!strcmp(token, AspectYStr))
				GameCfg.AspectY = strtol(value, NULL, 10);
			else if (!strcmp(token, WindowModeStr))
				GameCfg.WindowMode = strtol(value, NULL, 10);
			else if (!strcmp(token, TexFiltStr))
				GameCfg.TexFilt = strtol(value, NULL, 10);
			else if (!strcmp(token, VSyncStr))
				GameCfg.VSync = strtol(value, NULL, 10);
			else if (!strcmp(token, MultisampleStr))
				GameCfg.Multisample = strtol(value, NULL, 10);
			else if (!strcmp(token, JukeboxOnStr))
				GameCfg.JukeboxOn = strtol(value, NULL, 10);
			else if (!strcmp(token, JukeboxPathStr))	{
				char * p;
				strncpy( GameCfg.JukeboxPath, value, PATH_MAX );
				p = strchr( GameCfg.JukeboxPath, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, IPHostAddrStr))	{
				char * p;
				strncpy( GameCfg.MplIpHostAddr, value, 128 );
				p = strchr( GameCfg.MplIpHostAddr, '\n');
				if ( p ) *p = 0;
			}
		}
	}

	PHYSFS_close(infile);

	if ( GameCfg.DigiVolume > 8 ) GameCfg.DigiVolume = 8;
	if ( GameCfg.MusicVolume > 8 ) GameCfg.MusicVolume = 8;

	digi_set_volume( (GameCfg.DigiVolume*32768)/8, (GameCfg.MusicVolume*128)/8 );

	if (GameCfg.ResolutionX >= 320 && GameCfg.ResolutionY >= 200)
		Game_screen_mode = SM(GameCfg.ResolutionX,GameCfg.ResolutionY);

	return 0;
}

int WriteConfigFile()
{
	PHYSFS_file *infile;

	GameCfg.GammaLevel = gr_palette_get_gamma();
	
	infile = PHYSFSX_openWriteBuffered("descent.cfg");

	if (infile == NULL) {
		return 1;
	}

	PHYSFSX_printf(infile, "%s=%d\n", DigiVolumeStr, GameCfg.DigiVolume);
	PHYSFSX_printf(infile, "%s=%d\n", MusicVolumeStr, GameCfg.MusicVolume);
	PHYSFSX_printf(infile, "%s=%d\n", SndEnableRedbookStr, GameCfg.SndEnableRedbook);
	PHYSFSX_printf(infile, "%s=%d\n", ReverseStereoStr, GameCfg.ReverseStereo);
	PHYSFSX_printf(infile, "%s=%d\n", GammaLevelStr, GameCfg.GammaLevel);
	PHYSFSX_printf(infile, "%s=%s\n", LastPlayerStr, Players[Player_num].callsign);
	PHYSFSX_printf(infile, "%s=%s\n", LastMissionStr, GameCfg.LastMission);
        PHYSFSX_printf(infile, "%s=%i\n", LastLevelStr, GameCfg.LastLevel);
	PHYSFSX_printf(infile, "%s=%i\n", ResolutionXStr, SM_W(Game_screen_mode));
	PHYSFSX_printf(infile, "%s=%i\n", ResolutionYStr, SM_H(Game_screen_mode));
	PHYSFSX_printf(infile, "%s=%i\n", AspectXStr, GameCfg.AspectX);
	PHYSFSX_printf(infile, "%s=%i\n", AspectYStr, GameCfg.AspectY);
	PHYSFSX_printf(infile, "%s=%i\n", WindowModeStr, GameCfg.WindowMode);
	PHYSFSX_printf(infile, "%s=%i\n", TexFiltStr, GameCfg.TexFilt);
	PHYSFSX_printf(infile, "%s=%i\n", VSyncStr, GameCfg.VSync);
	PHYSFSX_printf(infile, "%s=%i\n", MultisampleStr, GameCfg.Multisample);
	PHYSFSX_printf(infile, "%s=%i\n", JukeboxOnStr, GameCfg.JukeboxOn);
	PHYSFSX_printf(infile, "%s=%s\n", JukeboxPathStr, GameCfg.JukeboxPath);
	PHYSFSX_printf(infile, "%s=%s\n", IPHostAddrStr, GameCfg.MplIpHostAddr);

	PHYSFS_close(infile);

	return 0;
}
