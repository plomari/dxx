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
#include "songs.h"
#include "kconfig.h"
#include "palette.h"
#include "args.h"
#include "player.h"
#include "mission.h"
#include "physfsx.h"

struct Cfg GameCfg;

static const char *DigiVolumeStr = "DigiVolume";
static const char *MusicVolumeStr = "MusicVolume";
static const char *ReverseStereoStr = "ReverseStereo";
static const char *OrigTrackOrderStr = "OrigTrackOrder";
static const char *MusicTypeStr = "MusicType";
static const char *CMLevelMusicPlayOrderStr = "CMLevelMusicPlayOrder";
static const char *CMLevelMusicTrack0Str = "CMLevelMusicTrack0";
static const char *CMLevelMusicTrack1Str = "CMLevelMusicTrack1";
static const char *CMLevelMusicPathStr = "CMLevelMusicPath";
static const char *CMMiscMusic0Str = "CMMiscMusic0";
static const char *CMMiscMusic1Str = "CMMiscMusic1";
static const char *CMMiscMusic2Str="CMMiscMusic2";
static const char *CMMiscMusic3Str = "CMMiscMusic3";
static const char *CMMiscMusic4Str = "CMMiscMusic4";
static const char *GammaLevelStr = "GammaLevel";
static const char *LastPlayerStr = "LastPlayer";
static const char *LastMissionStr = "LastMission";
static const char *LastLevelStr = "LastLevel";
static const char *ResolutionXStr="ResolutionX";
static const char *ResolutionYStr="ResolutionY";
static const char *WindowModeStr="WindowMode";
static const char *TexFiltStr="TexFilt";
static const char *MovieSubtitlesStr="MovieSubtitles";
static const char *VSyncStr="VSync";
static const char *MultisampleStr="Multisample";
static const char *GrabinputStr="GrabInput";
static const char *SkipProgramIntroStr="SkipProgramIntro";

int ReadConfigFile()
{
	PHYSFS_file *infile;
	char line[PATH_MAX+50], *token, *value, *ptr;

	// set defaults
	GameCfg = (struct Cfg) {
		.DigiVolume = 8,
		.MusicVolume = 8,
		.MusicType = MUSIC_TYPE_BUILTIN,
		.CMLevelMusicPlayOrder = MUSIC_CM_PLAYORDER_CONT,
		.CMLevelMusicTrack[0] = -1,
		.CMLevelMusicTrack[1] = -1,
		.ResolutionX = 640,
		.ResolutionY = 480,
		.Grabinput = 1,
	};

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
			else if (!strcmp(token, ReverseStereoStr))
				GameCfg.ReverseStereo = strtol(value, NULL, 10);
			else if (!strcmp(token, OrigTrackOrderStr))
				GameCfg.OrigTrackOrder = strtol(value, NULL, 10);
			else if (!strcmp(token, MusicTypeStr))
				GameCfg.MusicType = strtol(value, NULL, 10);
			else if (!strcmp(token, CMLevelMusicPlayOrderStr))
				GameCfg.CMLevelMusicPlayOrder = strtol(value, NULL, 10);
			else if (!strcmp(token, CMLevelMusicTrack0Str))
				GameCfg.CMLevelMusicTrack[0] = strtol(value, NULL, 10);
			else if (!strcmp(token, CMLevelMusicTrack1Str))
				GameCfg.CMLevelMusicTrack[1] = strtol(value, NULL, 10);
			else if (!strcmp(token, CMLevelMusicPathStr))	{
				char * p;
				strncpy( GameCfg.CMLevelMusicPath, value, PATH_MAX );
				p = strchr( GameCfg.CMLevelMusicPath, '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, CMMiscMusic0Str))	{
				char * p;
				strncpy( GameCfg.CMMiscMusic[SONG_TITLE], value, PATH_MAX );
				p = strchr( GameCfg.CMMiscMusic[SONG_TITLE], '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, CMMiscMusic1Str))	{
				char * p;
				strncpy( GameCfg.CMMiscMusic[SONG_BRIEFING], value, PATH_MAX );
				p = strchr( GameCfg.CMMiscMusic[SONG_BRIEFING], '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, CMMiscMusic2Str))	{
				char * p;
				strncpy( GameCfg.CMMiscMusic[SONG_ENDLEVEL], value, PATH_MAX );
				p = strchr( GameCfg.CMMiscMusic[SONG_ENDLEVEL], '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, CMMiscMusic3Str))	{
				char * p;
				strncpy( GameCfg.CMMiscMusic[SONG_ENDGAME], value, PATH_MAX );
				p = strchr( GameCfg.CMMiscMusic[SONG_ENDGAME], '\n');
				if ( p ) *p = 0;
			}
			else if (!strcmp(token, CMMiscMusic4Str))	{
				char * p;
				strncpy( GameCfg.CMMiscMusic[SONG_CREDITS], value, PATH_MAX );
				p = strchr( GameCfg.CMMiscMusic[SONG_CREDITS], '\n');
				if ( p ) *p = 0;
			}
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
			else if (!strcmp(token, WindowModeStr))
				GameCfg.WindowMode = strtol(value, NULL, 10);
			else if (!strcmp(token, TexFiltStr))
				GameCfg.TexFilt = strtol(value, NULL, 10);
			else if (!strcmp(token, MovieSubtitlesStr))
				GameCfg.MovieSubtitles = strtol(value, NULL, 10);
			else if (!strcmp(token, VSyncStr))
				GameCfg.VSync = strtol(value, NULL, 10);
			else if (!strcmp(token, MultisampleStr))
				GameCfg.Multisample = strtol(value, NULL, 10);
			else if (!strcmp(token, GrabinputStr))
				GameCfg.Grabinput = strtol(value, NULL, 10);
			else if (!strcmp(token, SkipProgramIntroStr))
				GameCfg.SkipProgramIntro = strtol(value, NULL, 10);
		}
	}

	PHYSFS_close(infile);

	if ( GameCfg.DigiVolume > 8 ) GameCfg.DigiVolume = 8;
	if ( GameCfg.MusicVolume > 8 ) GameCfg.MusicVolume = 8;

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
	PHYSFSX_printf(infile, "%s=%d\n", ReverseStereoStr, GameCfg.ReverseStereo);
	PHYSFSX_printf(infile, "%s=%d\n", OrigTrackOrderStr, GameCfg.OrigTrackOrder);
	PHYSFSX_printf(infile, "%s=%d\n", MusicTypeStr, GameCfg.MusicType);
	PHYSFSX_printf(infile, "%s=%d\n", CMLevelMusicPlayOrderStr, GameCfg.CMLevelMusicPlayOrder);
	PHYSFSX_printf(infile, "%s=%d\n", CMLevelMusicTrack0Str, GameCfg.CMLevelMusicTrack[0]);
	PHYSFSX_printf(infile, "%s=%d\n", CMLevelMusicTrack1Str, GameCfg.CMLevelMusicTrack[1]);
	PHYSFSX_printf(infile, "%s=%s\n", CMLevelMusicPathStr, GameCfg.CMLevelMusicPath);
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic0Str, GameCfg.CMMiscMusic[SONG_TITLE]);
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic1Str, GameCfg.CMMiscMusic[SONG_BRIEFING]);
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic2Str, GameCfg.CMMiscMusic[SONG_ENDLEVEL]);
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic3Str, GameCfg.CMMiscMusic[SONG_ENDGAME]);
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic4Str, GameCfg.CMMiscMusic[SONG_CREDITS]);
	PHYSFSX_printf(infile, "%s=%d\n", GammaLevelStr, GameCfg.GammaLevel);
	PHYSFSX_printf(infile, "%s=%s\n", LastPlayerStr, Players[Player_num].callsign);
	PHYSFSX_printf(infile, "%s=%s\n", LastMissionStr, GameCfg.LastMission);
        PHYSFSX_printf(infile, "%s=%i\n", LastLevelStr, GameCfg.LastLevel);
	PHYSFSX_printf(infile, "%s=%i\n", ResolutionXStr, GameCfg.ResolutionX);
	PHYSFSX_printf(infile, "%s=%i\n", ResolutionYStr, GameCfg.ResolutionY);
	PHYSFSX_printf(infile, "%s=%i\n", WindowModeStr, GameCfg.WindowMode);
	PHYSFSX_printf(infile, "%s=%i\n", TexFiltStr, GameCfg.TexFilt);
	PHYSFSX_printf(infile, "%s=%i\n", MovieSubtitlesStr, GameCfg.MovieSubtitles);
	PHYSFSX_printf(infile, "%s=%i\n", VSyncStr, GameCfg.VSync);
	PHYSFSX_printf(infile, "%s=%i\n", MultisampleStr, GameCfg.Multisample);
	PHYSFSX_printf(infile, "%s=%i\n", GrabinputStr, GameCfg.Grabinput);
	PHYSFSX_printf(infile, "%s=%i\n", SkipProgramIntroStr, GameCfg.SkipProgramIntro);

	PHYSFS_close(infile);

	return 0;
}
