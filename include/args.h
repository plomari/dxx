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
 * Prototypes for accessing arguments.
 *
 */


#ifndef _ARGS_H
#define _ARGS_H

#include "pstypes.h"	// for __packed__ definition (varies with compiler)

extern int Num_args;
extern char *Args[];
extern void AppendIniArgs(void);
extern void InitArgs(int argc, char **argv);
extern void args_exit();

// Struct that keeps all variables used by FindArg
// Prefixes are:
//   Sys - System Options
//   Ctl - Control Options
//   Snd - Sound Options
//   Gfx - Graphics Options
//   Ogl - OpenGL Options
//   Mpl - Multiplayer Options
//   Edi - Editor Options
//   Dbg - Debugging/Undocumented Options
typedef struct Arg
{
	int SysShowCmdHelp;
	int SysFPSIndicator;
	int SysUseNiceFPS;
	int SysMaxFPS;
	char *SysHogDir;
	int SysNoHogDir;
	int SysUsePlayersDir;
	int SysLowMem;
	char *SysPilot;
	int SysWindow;
	int SysAutoDemo;
	int SysNoMovies;
	int SysNoRedundancy;
	int CtlNoMouse;
	int CtlNoJoystick;
	int CtlMouselook;
	int CtlNoStickyKeys;
	int SndNoSound;
	int SndNoMusic;
	int SndDigiSampleRate;
	int SndDisableSdlMixer;
	int GfxHiresGFXAvailable;
	int GfxHiresFNTAvailable;
	int GfxMovieHires;
	int OglFixedFont;
	int MplNoRankings;
	int MplTxtMessages;
	const char *MplIpxNetwork;
	const char *MplUdpHostAddr;
	int MplUdpHostPort;
	int MplUdpMyPort;
	char *EdiAutoLoad;
	int EdiSaveHoardData;
	int EdiMacData; // also used for some read routines in non-editor build
	int DbgVerbose;
	int DbgNoRun;
	int DbgRenderStats;
	char *DbgAltTex;
	char *DbgTexMap;
	int DbgShowMemInfo;
	int DbgUseDoubleBuffer;
	int DbgBigPig;
	int DbgAltTexMerge;
} Arg;

extern struct Arg GameArg;

#endif
