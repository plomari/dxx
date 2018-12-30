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
 * Header for playsave.c
 *
 */


#ifndef _PLAYSAVE_H
#define _PLAYSAVE_H

#include "kconfig.h"
#include "mission.h"
#include "weapon.h"
#include "multi.h"
#include "escort.h"

#define GAME_NAME_LEN   25      // +1 for terminating zero = 26

typedef struct hli {
	char	Shortname[9];
	ubyte	LevelNum;
} hli;


// (values are saved as raw integers in user config files)
enum key_stick_type {
	KEY_STICK_NORMAL,	// state is active while key is held down
	KEY_STICK_TOGGLE,	// key down toggles the state
	KEY_STICK_STICKY,	// short press is like TOGGLE, long press like NORMAL
};

typedef struct player_config
{
	ubyte ControlType;
	ubyte PrimaryOrder[MAX_PRIMARY_WEAPONS+1];
	ubyte SecondaryOrder[MAX_SECONDARY_WEAPONS+1];
	ubyte KeySettings[3][MAX_CONTROLS];
	ubyte KeySettingsD2X[MAX_D2X_CONTROLS];
	int DefaultDifficulty;
	int AutoLeveling;
	short NHighestLevels;
	hli HighestLevels[MAX_MISSIONS];
	int KeyboardSens[5];
	int JoystickSens[6];
	int JoystickDead[6];
	ubyte MouseFlightSim;
	int MouseSens[6];
	int MouseFSDead;
	int MouseFSIndicator;
	int CockpitMode[2]; // 0 saves the "real" cockpit, 1 also saves letterbox and rear. Used to properly switch between modes and restore the real one.
	int Cockpit3DView[2];
	char NetworkMessageMacro[4][MAX_MESSAGE_LEN];
	int NetlifeKills;
	int NetlifeKilled;
	ubyte ReticleType;
	int ReticleRGBA[4];
	int ReticleSize;
	int MissileViewEnabled;
	int HeadlightActiveDefault;
	int GuidedInBigWindow;
	char GuidebotName[GUIDEBOT_NAME_LEN+1];
	char GuidebotNameReal[GUIDEBOT_NAME_LEN+1];
	int HudMode;
	int EscortHotKeys;
	int PersistentDebris;
	int PRShot;
	ubyte NoRedundancy;
	ubyte MultiMessages;
	ubyte NoRankings;
	ubyte AutomapFreeFlight;
	ubyte AutomapObjects;
	ubyte NoFireAutoselect;
	int AlphaEffects;
	int DynLightColor;
	int OldKeyboardRamping;
	enum key_stick_type KeyStickRearView;
	enum key_stick_type KeyStickEnergyConvert;
	int ExtendedAmmoRack;
	int SkipLevelMovies;
	int SkipLevelBriefing;
} player_config;

extern struct player_config PlayerCfg;

#ifndef EZERO
#define EZERO 0
#endif

extern int Default_leveling_on;

// Used to save kconfig values to disk.
int write_player_file();

int new_player_config();

int read_player_file();

// set a new highest level for player for this mission
void set_highest_level(int levelnum);

// gets the player's highest level from the file for this mission
int get_highest_level(void);

#endif /* _PLAYSAVE_H */
