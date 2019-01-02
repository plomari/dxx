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
 * Functions to load & save player's settings (*.plr file)
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "dxxerror.h"
#include "strutil.h"
#include "game.h"
#include "gameseq.h"
#include "player.h"
#include "playsave.h"
#include "joy.h"
#include "digi.h"
#include "newmenu.h"
#include "palette.h"
#include "menu.h"
#include "config.h"
#include "text.h"
#include "state.h"
#include "gauges.h"
#include "screens.h"
#include "powerup.h"
#include "makesig.h"
#include "byteswap.h"
#include "u_mem.h"
#include "strio.h"
#include "physfsx.h"
#include "vers_id.h"
#include "newdemo.h"
#include "gauges.h"

//version 5  ->  6: added new highest level information
//version 6  ->  7: stripped out the old saved_game array.
//version 7  ->  8: added reticle flag, & window size
//version 8  ->  9: removed player_struct_version
//version 9  -> 10: added default display mode
//version 10 -> 11: added all toggles in toggle menu
//version 11 -> 12: added weapon ordering
//version 12 -> 13: added more keys
//version 13 -> 14: took out marker key
//version 14 -> 15: added guided in big window
//version 15 -> 16: added small windows in cockpit
//version 16 -> 17: ??
//version 17 -> 18: save guidebot name
//version 18 -> 19: added automap-highres flag
//version 19 -> 20: added kconfig data for windows joysticks
//version 20 -> 21: save seperate config types for DOS & Windows
//version 21 -> 22: save lifetime netstats 
//version 22 -> 23: ??
//version 23 -> 24: add name of joystick for windows version.

#define SAVE_FILE_ID MAKE_SIG('D','P','L','R')
#define PLAYER_FILE_VERSION 24 //increment this every time the player file changes
#define COMPATIBLE_PLAYER_FILE_VERSION 17

struct player_config PlayerCfg;
int get_lifetime_checksum (int a,int b);
extern void InitWeaponOrdering();

int new_player_config()
{
	InitWeaponOrdering (); //setup default weapon priorities
	PlayerCfg.ControlType=0; // Assume keyboard
	memcpy(PlayerCfg.KeySettings, DefaultKeySettings, sizeof(DefaultKeySettings));
	memcpy(PlayerCfg.KeySettingsD2X, DefaultKeySettingsD2X, sizeof(DefaultKeySettingsD2X));
	kc_set_controls();

	PlayerCfg.DefaultDifficulty = 1;
	PlayerCfg.AutoLeveling = 1;
	PlayerCfg.NHighestLevels = 1;
	PlayerCfg.HighestLevels[0].Shortname[0] = 0; //no name for mission 0
	PlayerCfg.HighestLevels[0].LevelNum = 1; //was highest level in old struct
	PlayerCfg.KeyboardSens[0] = PlayerCfg.KeyboardSens[1] = PlayerCfg.KeyboardSens[2] = PlayerCfg.KeyboardSens[3] = PlayerCfg.KeyboardSens[4] = 16;
	PlayerCfg.JoystickSens[0] = PlayerCfg.JoystickSens[1] = PlayerCfg.JoystickSens[2] = PlayerCfg.JoystickSens[3] = PlayerCfg.JoystickSens[4] = PlayerCfg.JoystickSens[5] = 8;
	PlayerCfg.JoystickDead[0] = PlayerCfg.JoystickDead[1] = PlayerCfg.JoystickDead[2] = PlayerCfg.JoystickDead[3] = PlayerCfg.JoystickDead[4] = PlayerCfg.JoystickDead[5] = 0;
	PlayerCfg.MouseFlightSim = 0;
	PlayerCfg.MouseSens[0] = PlayerCfg.MouseSens[1] = PlayerCfg.MouseSens[2] = PlayerCfg.MouseSens[3] = PlayerCfg.MouseSens[4] = PlayerCfg.MouseSens[5] = 8;
	PlayerCfg.MouseFSDead = 0;
	PlayerCfg.MouseFSIndicator = 1;
	PlayerCfg.CockpitMode[0] = PlayerCfg.CockpitMode[1] = CM_FULL_COCKPIT;
	PlayerCfg.Cockpit3DView[0]=CV_NONE;
	PlayerCfg.Cockpit3DView[1]=CV_NONE;
	PlayerCfg.ReticleType = RET_TYPE_CLASSIC;
	PlayerCfg.ReticleRGBA[0] = RET_COLOR_DEFAULT_R; PlayerCfg.ReticleRGBA[1] = RET_COLOR_DEFAULT_G; PlayerCfg.ReticleRGBA[2] = RET_COLOR_DEFAULT_B; PlayerCfg.ReticleRGBA[3] = RET_COLOR_DEFAULT_A;
	PlayerCfg.ReticleSize = 0;
	PlayerCfg.MissileViewEnabled = 1;
	PlayerCfg.HeadlightActiveDefault = 1;
	PlayerCfg.GuidedInBigWindow = 0;
	strcpy(PlayerCfg.GuidebotName,"GUIDE-BOT");
	strcpy(PlayerCfg.GuidebotNameReal,"GUIDE-BOT");
	PlayerCfg.HudMode = 0;
	PlayerCfg.EscortHotKeys = 1;
	PlayerCfg.PersistentDebris = 0;
	PlayerCfg.PRShot = 0;
	PlayerCfg.NoRedundancy = 0;
	PlayerCfg.MultiMessages = 0;
	PlayerCfg.NoRankings = 0;
	PlayerCfg.AutomapFreeFlight = 0;
	PlayerCfg.AutomapObjects = 0;
	PlayerCfg.NoFireAutoselect = 0;
	PlayerCfg.AlphaEffects = 0;
	PlayerCfg.DynLightColor = 0;
	PlayerCfg.OldKeyboardRamping = 1;
	PlayerCfg.KeyStickRearView = KEY_STICK_STICKY;
	PlayerCfg.KeyStickEnergyConvert = KEY_STICK_NORMAL;

	// Default taunt macros
	#ifdef NETWORK
	strcpy(PlayerCfg.NetworkMessageMacro[0], "Why can't we all just get along?");
	strcpy(PlayerCfg.NetworkMessageMacro[1], "Hey, I got a present for ya");
	strcpy(PlayerCfg.NetworkMessageMacro[2], "I got a hankerin' for a spankerin'");
	strcpy(PlayerCfg.NetworkMessageMacro[3], "This one's headed for Uranus");
	PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
	#endif
	
	return 1;
}

int read_player_d2x(char *filename)
{
	PHYSFS_file *f;
	int rc = 0;
	char line[50],*word;
	int Stop=0;

	f = PHYSFSX_openReadBuffered(filename);

	if(!f || PHYSFS_eof(f))
		return errno;

	while(!Stop && !PHYSFS_eof(f))
	{
		cfgets(line,50,f);
		word=splitword(line,':');
		strupr(word);
		if (strstr(word,"KEYBOARD"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"SENSITIVITY0"))
					PlayerCfg.KeyboardSens[0] = atoi(line);
				if(!strcmp(word,"SENSITIVITY1"))
					PlayerCfg.KeyboardSens[1] = atoi(line);
				if(!strcmp(word,"SENSITIVITY2"))
					PlayerCfg.KeyboardSens[2] = atoi(line);
				if(!strcmp(word,"SENSITIVITY3"))
					PlayerCfg.KeyboardSens[3] = atoi(line);
				if(!strcmp(word,"SENSITIVITY4"))
					PlayerCfg.KeyboardSens[4] = atoi(line);
				if(!strcmp(word,"OLDRAMPING"))
					PlayerCfg.OldKeyboardRamping = atoi(line);
				if(!strcmp(word,"STICKREARVIEW"))
					PlayerCfg.KeyStickRearView = atoi(line);
				if(!strcmp(word,"STICKENERGYCONVERT"))
					PlayerCfg.KeyStickEnergyConvert = atoi(line);
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"JOYSTICK"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"SENSITIVITY0"))
					PlayerCfg.JoystickSens[0] = atoi(line);
				if(!strcmp(word,"SENSITIVITY1"))
					PlayerCfg.JoystickSens[1] = atoi(line);
				if(!strcmp(word,"SENSITIVITY2"))
					PlayerCfg.JoystickSens[2] = atoi(line);
				if(!strcmp(word,"SENSITIVITY3"))
					PlayerCfg.JoystickSens[3] = atoi(line);
				if(!strcmp(word,"SENSITIVITY4"))
					PlayerCfg.JoystickSens[4] = atoi(line);
				if(!strcmp(word,"SENSITIVITY5"))
					PlayerCfg.JoystickSens[5] = atoi(line);
				if(!strcmp(word,"DEADZONE0"))
					PlayerCfg.JoystickDead[0] = atoi(line);
				if(!strcmp(word,"DEADZONE1"))
					PlayerCfg.JoystickDead[1] = atoi(line);
				if(!strcmp(word,"DEADZONE2"))
					PlayerCfg.JoystickDead[2] = atoi(line);
				if(!strcmp(word,"DEADZONE3"))
					PlayerCfg.JoystickDead[3] = atoi(line);
				if(!strcmp(word,"DEADZONE4"))
					PlayerCfg.JoystickDead[4] = atoi(line);
				if(!strcmp(word,"DEADZONE5"))
					PlayerCfg.JoystickDead[5] = atoi(line);
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"MOUSE"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"FLIGHTSIM"))
					PlayerCfg.MouseFlightSim = atoi(line);
				if(!strcmp(word,"SENSITIVITY0"))
					PlayerCfg.MouseSens[0] = atoi(line);
				if(!strcmp(word,"SENSITIVITY1"))
					PlayerCfg.MouseSens[1] = atoi(line);
				if(!strcmp(word,"SENSITIVITY2"))
					PlayerCfg.MouseSens[2] = atoi(line);
				if(!strcmp(word,"SENSITIVITY3"))
					PlayerCfg.MouseSens[3] = atoi(line);
				if(!strcmp(word,"SENSITIVITY4"))
					PlayerCfg.MouseSens[4] = atoi(line);
				if(!strcmp(word,"SENSITIVITY5"))
					PlayerCfg.MouseSens[5] = atoi(line);
				if(!strcmp(word,"FSDEAD"))
					PlayerCfg.MouseFSDead = atoi(line);
				if(!strcmp(word,"FSINDI"))
					PlayerCfg.MouseFSIndicator = atoi(line);
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"WEAPON KEYS V2"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				int kc1=0,kc2=0,kc3=0;
				int i=atoi(word);
				
				if(i==0) i=10;
					i=(i-1)*3;
		
				sscanf(line,"0x%x,0x%x,0x%x",&kc1,&kc2,&kc3);
				PlayerCfg.KeySettingsD2X[i]   = kc1;
				PlayerCfg.KeySettingsD2X[i+1] = kc2;
				PlayerCfg.KeySettingsD2X[i+2] = kc3;
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"COCKPIT"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"HUD"))
					PlayerCfg.HudMode = atoi(line);
				else if(!strcmp(word,"RETTYPE"))
					PlayerCfg.ReticleType = atoi(line);
				else if(!strcmp(word,"RETRGBA"))
					sscanf(line,"%i,%i,%i,%i",&PlayerCfg.ReticleRGBA[0],&PlayerCfg.ReticleRGBA[1],&PlayerCfg.ReticleRGBA[2],&PlayerCfg.ReticleRGBA[3]);
				else if(!strcmp(word,"RETSIZE"))
					PlayerCfg.ReticleSize = atoi(line);
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"TOGGLES"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"ESCORTHOTKEYS"))
					PlayerCfg.EscortHotKeys = atoi(line);
				if(!strcmp(word,"PERSISTENTDEBRIS"))
					PlayerCfg.PersistentDebris = atoi(line);
				if(!strcmp(word,"PRSHOT"))
					PlayerCfg.PRShot = atoi(line);
				if(!strcmp(word,"NOREDUNDANCY"))
					PlayerCfg.NoRedundancy = atoi(line);
				if(!strcmp(word,"MULTIMESSAGES"))
					PlayerCfg.MultiMessages = atoi(line);
				if(!strcmp(word,"NORANKINGS"))
					PlayerCfg.NoRankings = atoi(line);
				if(!strcmp(word,"AUTOMAPFREEFLIGHT"))
					PlayerCfg.AutomapFreeFlight = atoi(line);
				if(!strcmp(word,"AUTOMAPOBJECTS"))
					PlayerCfg.AutomapObjects = atoi(line);
				if(!strcmp(word,"NOFIREAUTOSELECT"))
					PlayerCfg.NoFireAutoselect = atoi(line);
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"GRAPHICS"))
		{
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
	
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				if(!strcmp(word,"ALPHAEFFECTS"))
					PlayerCfg.AlphaEffects = atoi(line);
				if(!strcmp(word,"DYNLIGHTCOLOR"))
					PlayerCfg.DynLightColor = atoi(line);
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
		}
		else if (strstr(word,"PLX VERSION")) // know the version this pilot was used last with - allow modifications
		{
			int v1=0,v2=0,v3=0;
			d_free(word);
			cfgets(line,50,f);
			word=splitword(line,'=');
			strupr(word);
			while(!strstr(word,"END") && !PHYSFS_eof(f))
			{
				sscanf(line,"%i.%i.%i",&v1,&v2,&v3);
				d_free(word);
				cfgets(line,50,f);
				word=splitword(line,'=');
				strupr(word);
			}
			if (v1 == 0 && v2 == 56 && v3 == 0) // was 0.56.0
				if (D2XMAJORi != v1 || D2XMINORi != v2 || D2XMICROi != v3) // newer (presumably)
				{
					// reset joystick/mouse cycling fields
					PlayerCfg.KeySettings[2][28] = 255;
					PlayerCfg.KeySettings[2][29] = 255;
				}
		}
		else if (strstr(word,"END") || PHYSFS_eof(f))
		{
			Stop=1;
		}
		else
		{
			if(word[0]=='['&&!strstr(word,"D2X OPTIONS"))
			{
				while(!strstr(line,"END") && !PHYSFS_eof(f))
				{
					cfgets(line,50,f);
					strupr(line);
				}
			}
		}

		if(word)
			d_free(word);
	}

	PHYSFS_close(f);

	return rc;
}

int write_player_d2x(char *filename)
{
	PHYSFS_file *fout;
	int rc=0;
	char tempfile[PATH_MAX];

	strcpy(tempfile,filename);
	tempfile[strlen(tempfile)-4]=0;
	strcat(tempfile,".pl$");
	fout=PHYSFSX_openWriteBuffered(tempfile);
	
	if (!fout && GameArg.SysUsePlayersDir)
	{
		PHYSFS_mkdir("Players/");	//try making directory
		fout=PHYSFSX_openWriteBuffered(tempfile);
	}
	
	if(fout)
	{
		PHYSFSX_printf(fout,"[D2X OPTIONS]\n");
		PHYSFSX_printf(fout,"[keyboard]\n");
		PHYSFSX_printf(fout,"sensitivity0=%d\n",PlayerCfg.KeyboardSens[0]);
		PHYSFSX_printf(fout,"sensitivity1=%d\n",PlayerCfg.KeyboardSens[1]);
		PHYSFSX_printf(fout,"sensitivity2=%d\n",PlayerCfg.KeyboardSens[2]);
		PHYSFSX_printf(fout,"sensitivity3=%d\n",PlayerCfg.KeyboardSens[3]);
		PHYSFSX_printf(fout,"sensitivity4=%d\n",PlayerCfg.KeyboardSens[4]);
		PHYSFSX_printf(fout,"oldramping=%d\n",PlayerCfg.OldKeyboardRamping);
		PHYSFSX_printf(fout,"stickrearview=%d\n", PlayerCfg.KeyStickRearView);
		PHYSFSX_printf(fout,"stickenergyconvert=%d\n", PlayerCfg.KeyStickEnergyConvert);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[joystick]\n");
		PHYSFSX_printf(fout,"sensitivity0=%d\n",PlayerCfg.JoystickSens[0]);
		PHYSFSX_printf(fout,"sensitivity1=%d\n",PlayerCfg.JoystickSens[1]);
		PHYSFSX_printf(fout,"sensitivity2=%d\n",PlayerCfg.JoystickSens[2]);
		PHYSFSX_printf(fout,"sensitivity3=%d\n",PlayerCfg.JoystickSens[3]);
		PHYSFSX_printf(fout,"sensitivity4=%d\n",PlayerCfg.JoystickSens[4]);
		PHYSFSX_printf(fout,"sensitivity5=%d\n",PlayerCfg.JoystickSens[5]);
		PHYSFSX_printf(fout,"deadzone0=%d\n",PlayerCfg.JoystickDead[0]);
		PHYSFSX_printf(fout,"deadzone1=%d\n",PlayerCfg.JoystickDead[1]);
		PHYSFSX_printf(fout,"deadzone2=%d\n",PlayerCfg.JoystickDead[2]);
		PHYSFSX_printf(fout,"deadzone3=%d\n",PlayerCfg.JoystickDead[3]);
		PHYSFSX_printf(fout,"deadzone4=%d\n",PlayerCfg.JoystickDead[4]);
		PHYSFSX_printf(fout,"deadzone5=%d\n",PlayerCfg.JoystickDead[5]);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[mouse]\n");
		PHYSFSX_printf(fout,"flightsim=%d\n",PlayerCfg.MouseFlightSim);
		PHYSFSX_printf(fout,"sensitivity0=%d\n",PlayerCfg.MouseSens[0]);
		PHYSFSX_printf(fout,"sensitivity1=%d\n",PlayerCfg.MouseSens[1]);
		PHYSFSX_printf(fout,"sensitivity2=%d\n",PlayerCfg.MouseSens[2]);
		PHYSFSX_printf(fout,"sensitivity3=%d\n",PlayerCfg.MouseSens[3]);
		PHYSFSX_printf(fout,"sensitivity4=%d\n",PlayerCfg.MouseSens[4]);
		PHYSFSX_printf(fout,"sensitivity5=%d\n",PlayerCfg.MouseSens[5]);
		PHYSFSX_printf(fout,"fsdead=%d\n",PlayerCfg.MouseFSDead);
		PHYSFSX_printf(fout,"fsindi=%d\n",PlayerCfg.MouseFSIndicator);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[weapon keys v2]\n");
		PHYSFSX_printf(fout,"1=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD2X[0],PlayerCfg.KeySettingsD2X[1],PlayerCfg.KeySettingsD2X[2]);
		PHYSFSX_printf(fout,"2=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD2X[3],PlayerCfg.KeySettingsD2X[4],PlayerCfg.KeySettingsD2X[5]);
		PHYSFSX_printf(fout,"3=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD2X[6],PlayerCfg.KeySettingsD2X[7],PlayerCfg.KeySettingsD2X[8]);
		PHYSFSX_printf(fout,"4=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD2X[9],PlayerCfg.KeySettingsD2X[10],PlayerCfg.KeySettingsD2X[11]);
		PHYSFSX_printf(fout,"5=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD2X[12],PlayerCfg.KeySettingsD2X[13],PlayerCfg.KeySettingsD2X[14]);
		PHYSFSX_printf(fout,"6=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD2X[15],PlayerCfg.KeySettingsD2X[16],PlayerCfg.KeySettingsD2X[17]);
		PHYSFSX_printf(fout,"7=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD2X[18],PlayerCfg.KeySettingsD2X[19],PlayerCfg.KeySettingsD2X[20]);
		PHYSFSX_printf(fout,"8=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD2X[21],PlayerCfg.KeySettingsD2X[22],PlayerCfg.KeySettingsD2X[23]);
		PHYSFSX_printf(fout,"9=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD2X[24],PlayerCfg.KeySettingsD2X[25],PlayerCfg.KeySettingsD2X[26]);
		PHYSFSX_printf(fout,"0=0x%x,0x%x,0x%x\n",PlayerCfg.KeySettingsD2X[27],PlayerCfg.KeySettingsD2X[28],PlayerCfg.KeySettingsD2X[29]);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[cockpit]\n");
		PHYSFSX_printf(fout,"hud=%i\n",PlayerCfg.HudMode);
		PHYSFSX_printf(fout,"rettype=%i\n",PlayerCfg.ReticleType);
		PHYSFSX_printf(fout,"retrgba=%i,%i,%i,%i\n",PlayerCfg.ReticleRGBA[0],PlayerCfg.ReticleRGBA[1],PlayerCfg.ReticleRGBA[2],PlayerCfg.ReticleRGBA[3]);
		PHYSFSX_printf(fout,"retsize=%i\n",PlayerCfg.ReticleSize);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[toggles]\n");
		PHYSFSX_printf(fout,"escorthotkeys=%i\n",PlayerCfg.EscortHotKeys);
		PHYSFSX_printf(fout,"persistentdebris=%i\n",PlayerCfg.PersistentDebris);
		PHYSFSX_printf(fout,"prshot=%i\n",PlayerCfg.PRShot);
		PHYSFSX_printf(fout,"noredundancy=%i\n",PlayerCfg.NoRedundancy);
		PHYSFSX_printf(fout,"multimessages=%i\n",PlayerCfg.MultiMessages);
		PHYSFSX_printf(fout,"norankings=%i\n",PlayerCfg.NoRankings);
		PHYSFSX_printf(fout,"automapfreeflight=%i\n",PlayerCfg.AutomapFreeFlight);
		PHYSFSX_printf(fout,"automapobjects=%i\n",PlayerCfg.AutomapObjects);
		PHYSFSX_printf(fout,"nofireautoselect=%i\n",PlayerCfg.NoFireAutoselect);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[graphics]\n");
		PHYSFSX_printf(fout,"alphaeffects=%i\n",PlayerCfg.AlphaEffects);
		PHYSFSX_printf(fout,"dynlightcolor=%i\n",PlayerCfg.DynLightColor);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[plx version]\n");
		PHYSFSX_printf(fout,"plx version=%s\n", VERSION);
		PHYSFSX_printf(fout,"[end]\n");
		PHYSFSX_printf(fout,"[end]\n");

		PHYSFS_close(fout);
		if(rc==0)
		{
			PHYSFS_delete(filename);
			rc = PHYSFSX_rename(tempfile,filename);
		}
		return rc;
	}
	else
		return errno;

}

ubyte control_type_dos,control_type_win;

//read in the player's saved games.  returns errno (0 == no error)
int read_player_file()
{
	char filename[PATH_MAX];
	PHYSFS_file *file;
	int id, i;
	short player_file_version;
	int rewrite_it=0;
	int swap = 0;

	Assert(Player_num>=0 && Player_num<MAX_PLAYERS);

	memset(filename, '\0', PATH_MAX);
	snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%.8s.plr" : "%.8s.plr", Players[Player_num].callsign);
	if (!PHYSFS_exists(filename))
		return ENOENT;

	file = PHYSFSX_openReadBuffered(filename);

	if (!file)
		goto read_player_file_failed;

	new_player_config(); // Set defaults!

	PHYSFS_readSLE32(file, &id);

	if (id!=SAVE_FILE_ID) {
		nm_messagebox(TXT_ERROR, "Invalid player file", TXT_OK);
		PHYSFS_close(file);
		return -1;
	}

	player_file_version = cfile_read_short(file);

	if (player_file_version > 255) // bigendian file?
		swap = 1;

	if (swap)
		player_file_version = SWAPSHORT(player_file_version);

	if (player_file_version<COMPATIBLE_PLAYER_FILE_VERSION) {
		nm_messagebox(TXT_ERROR, TXT_ERROR_PLR_VERSION, TXT_OK);
		PHYSFS_close(file);
		return -1;
	}

	PHYSFS_seek(file,PHYSFS_tell(file)+2*sizeof(short)); //skip Game_window_w,Game_window_h
	PlayerCfg.DefaultDifficulty = cfile_read_byte(file);
	PlayerCfg.AutoLeveling       = cfile_read_byte(file);
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); // skip ReticleOn
	PlayerCfg.CockpitMode[0] = PlayerCfg.CockpitMode[1] = cfile_read_byte(file);
	PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); //skip Default_display_mode
	PlayerCfg.MissileViewEnabled      = cfile_read_byte(file);
	PlayerCfg.HeadlightActiveDefault  = cfile_read_byte(file);
	PlayerCfg.GuidedInBigWindow      = cfile_read_byte(file);
	if (player_file_version >= 19)
		PHYSFS_seek(file,PHYSFS_tell(file)+sizeof(sbyte)); //skip Automap_always_hires

	//read new highest level info

	PlayerCfg.NHighestLevels = cfile_read_short(file);
	if (swap)
		PlayerCfg.NHighestLevels = SWAPSHORT(PlayerCfg.NHighestLevels);
	Assert(PlayerCfg.NHighestLevels <= MAX_MISSIONS);

	if (PHYSFS_read(file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels)
		goto read_player_file_failed;

	//read taunt macros
	{
#ifdef NETWORK
		int i,len;

		len = MAX_MESSAGE_LEN;

		for (i = 0; i < 4; i++)
			if (PHYSFS_read(file, PlayerCfg.NetworkMessageMacro[i], len, 1) != 1)
				goto read_player_file_failed;
#else
		char dummy[4][MAX_MESSAGE_LEN];

		cfread(dummy, MAX_MESSAGE_LEN, 4, file);
#endif
	}

	//read kconfig data
	{
		ubyte dummy_joy_sens;

		if (PHYSFS_read(file, &PlayerCfg.KeySettings[0], sizeof(PlayerCfg.KeySettings[0]),1)!=1)
			goto read_player_file_failed;
		if (PHYSFS_read(file, &PlayerCfg.KeySettings[1], sizeof(PlayerCfg.KeySettings[1]),1)!=1)
			goto read_player_file_failed;
		PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS*3) ); // Skip obsolete Flightstick/Thrustmaster/Gravis map fields
		if (PHYSFS_read(file, &PlayerCfg.KeySettings[2], sizeof(PlayerCfg.KeySettings[2]),1)!=1)
			goto read_player_file_failed;
		PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS) ); // Skip obsolete Cyberman map field
		if (player_file_version>=20)
			PHYSFS_seek( file, PHYSFS_tell(file)+(sizeof(ubyte)*MAX_CONTROLS) ); // Skip obsolete Winjoy map field
		if (PHYSFS_read(file, (ubyte *)&control_type_dos, sizeof(ubyte), 1) != 1)
			goto read_player_file_failed;
		else if (player_file_version >= 21 && PHYSFS_read(file, (ubyte *)&control_type_win, sizeof(ubyte), 1) != 1)
			goto read_player_file_failed;
		else if (PHYSFS_read(file, &dummy_joy_sens, sizeof(ubyte), 1) !=1 )
			goto read_player_file_failed;

		PlayerCfg.ControlType = control_type_dos;
	
		for (i=0;i<11;i++)
		{
			PlayerCfg.PrimaryOrder[i] = cfile_read_byte(file);
			PlayerCfg.SecondaryOrder[i] = cfile_read_byte(file);
		}

		if (player_file_version>=16)
		{
			PHYSFS_readSLE32(file, &PlayerCfg.Cockpit3DView[0]);
			PHYSFS_readSLE32(file, &PlayerCfg.Cockpit3DView[1]);
			if (swap)
			{
				PlayerCfg.Cockpit3DView[0] = SWAPINT(PlayerCfg.Cockpit3DView[0]);
				PlayerCfg.Cockpit3DView[1] = SWAPINT(PlayerCfg.Cockpit3DView[1]);
			}
		}
	}

	if (player_file_version>=22)
	{
#ifdef NETWORK
		PHYSFS_readSLE32(file, &PlayerCfg.NetlifeKills);
		PHYSFS_readSLE32(file, &PlayerCfg.NetlifeKilled);
		if (swap) {
			PlayerCfg.NetlifeKills = SWAPINT(PlayerCfg.NetlifeKills);
			PlayerCfg.NetlifeKilled = SWAPINT(PlayerCfg.NetlifeKilled);
		}
#else
		{
			int dummy;
			PHYSFS_readSLE32(file, &dummy);
			PHYSFS_readSLE32(file, &dummy);
		}
#endif
	}
#ifdef NETWORK
	else
	{
		PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
	}
#endif

	if (player_file_version>=23)
	{
		PHYSFS_readSLE32(file, &i);
		if (swap)
			i = SWAPINT(i);
#ifdef NETWORK
		if (i!=get_lifetime_checksum (PlayerCfg.NetlifeKills,PlayerCfg.NetlifeKilled))
		{
			PlayerCfg.NetlifeKills=0; PlayerCfg.NetlifeKilled=0;
			nm_messagebox(NULL, "Trying to cheat eh?", "Shame on me");
			rewrite_it=1;
		}
#endif
	}

	//read guidebot name
	if (player_file_version >= 18)
		cfile_gets_0(file, PlayerCfg.GuidebotName, sizeof(PlayerCfg.GuidebotName));
	else
		strcpy(PlayerCfg.GuidebotName,"GUIDE-BOT");

	strcpy(PlayerCfg.GuidebotNameReal,PlayerCfg.GuidebotName);

	{
		char buf[128];

		if (player_file_version >= 24) 
			cfile_gets_0(file, buf, sizeof(buf));			// Just read it in fpr DPS.
	}

	if (!PHYSFS_close(file))
		goto read_player_file_failed;

	if (rewrite_it)
		write_player_file();

	filename[strlen(filename) - 4] = 0;
	strcat(filename, ".plx");
	read_player_d2x(filename);
	kc_set_controls();

	gr_set_attributes();

	return EZERO;

 read_player_file_failed:
	nm_messagebox(TXT_ERROR, "Error reading PLR file", TXT_OK);
	if (file)
		PHYSFS_close(file);

	return -1;
}


//finds entry for this level in table.  if not found, returns ptr to 
//empty entry.  If no empty entries, takes over last one 
int find_hli_entry()
{
	int i;

	for (i=0;i<PlayerCfg.NHighestLevels;i++)
		if (!stricmp(PlayerCfg.HighestLevels[i].Shortname, Current_mission_filename))
			break;

	if (i==PlayerCfg.NHighestLevels) { //not found. create entry

		if (i==MAX_MISSIONS)
			i--; //take last entry
		else
			PlayerCfg.NHighestLevels++;

		strcpy(PlayerCfg.HighestLevels[i].Shortname, Current_mission_filename);
		PlayerCfg.HighestLevels[i].LevelNum = 0;
	}

	return i;
}

//set a new highest level for player for this mission
void set_highest_level(int levelnum)
{
	int ret,i;

	if ((ret=read_player_file()) != EZERO)
		if (ret != ENOENT)		//if file doesn't exist, that's ok
			return;

	i = find_hli_entry();

	if (levelnum > PlayerCfg.HighestLevels[i].LevelNum)
		PlayerCfg.HighestLevels[i].LevelNum = levelnum;

	write_player_file();
}

//gets the player's highest level from the file for this mission
int get_highest_level(void)
{
	int i;
	int highest_saturn_level = 0;
	read_player_file();
#ifndef SATURN
	if (strlen(Current_mission_filename)==0 )	{
		for (i=0;i<PlayerCfg.NHighestLevels;i++)
			if (!stricmp(PlayerCfg.HighestLevels[i].Shortname, "DESTSAT")) 	//	Destination Saturn.
		 		highest_saturn_level			= PlayerCfg.HighestLevels[i].LevelNum; 
	}
#endif
	i = PlayerCfg.HighestLevels[find_hli_entry()].LevelNum;
	if ( highest_saturn_level > i )
		i = highest_saturn_level;
	return i;
}

//write out player's saved games.  returns errno (0 == no error)
int write_player_file()
{
	char filename[PATH_MAX];
	PHYSFS_file *file;
	int i;

	if ( Newdemo_state == ND_STATE_PLAYBACK )
		return -1;

	WriteConfigFile();

	memset(filename, '\0', PATH_MAX);
	snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%.8s.plx" : "%.8s.plx", Players[Player_num].callsign);
	write_player_d2x(filename);
	snprintf(filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%.8s.plr" : "%.8s.plr", Players[Player_num].callsign);
	file = PHYSFSX_openWriteBuffered(filename);

	if (!file)
		return -1;

	//Write out player's info
	PHYSFS_writeULE32(file, SAVE_FILE_ID);
	PHYSFS_writeULE16(file, PLAYER_FILE_VERSION);

	
	PHYSFS_writeULE16(file, 0); // skip Game_window_w
	PHYSFS_writeULE16(file, 0);	// skip Game_window_h
	PHYSFSX_writeU8(file, PlayerCfg.DefaultDifficulty);
	PHYSFSX_writeU8(file, PlayerCfg.AutoLeveling);
	PHYSFSX_writeU8(file, PlayerCfg.ReticleType==RET_TYPE_NONE?0:1);
	PHYSFSX_writeU8(file, PlayerCfg.CockpitMode[0]);
	PHYSFSX_writeU8(file, 0); // skip Default_display_mode
	PHYSFSX_writeU8(file, PlayerCfg.MissileViewEnabled);
	PHYSFSX_writeU8(file, PlayerCfg.HeadlightActiveDefault);
	PHYSFSX_writeU8(file, PlayerCfg.GuidedInBigWindow);
	PHYSFSX_writeU8(file, 0); // skip Automap_always_hires

	//write higest level info
	PHYSFS_writeULE16(file, PlayerCfg.NHighestLevels);
	if ((PHYSFS_write(file, PlayerCfg.HighestLevels, sizeof(hli), PlayerCfg.NHighestLevels) != PlayerCfg.NHighestLevels))
		goto write_player_file_failed;

#ifdef NETWORK
	if ((PHYSFS_write(file, PlayerCfg.NetworkMessageMacro, MAX_MESSAGE_LEN, 4) != 4))
		goto write_player_file_failed;
#else
	{
		char dummy[4][MAX_MESSAGE_LEN];	// Pull the messages from a hat! ;-)

		if ((PHYSFS_write(file, dummy, MAX_MESSAGE_LEN, 4) != 4))
			goto write_player_file_failed;
	}
#endif

	//write kconfig info
	{

		ubyte old_avg_joy_sensitivity = 8;
		control_type_dos = PlayerCfg.ControlType;

		if (PHYSFS_write(file, PlayerCfg.KeySettings[0], sizeof(PlayerCfg.KeySettings[0]), 1) != 1)
			goto write_player_file_failed;
		if (PHYSFS_write(file, PlayerCfg.KeySettings[1], sizeof(PlayerCfg.KeySettings[1]), 1) != 1)
			goto write_player_file_failed;
		for (i = 0; i < MAX_CONTROLS*3; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Flightstick/Thrustmaster/Gravis map fields
				goto write_player_file_failed;
		if (PHYSFS_write(file, PlayerCfg.KeySettings[2], sizeof(PlayerCfg.KeySettings[2]), 1) != 1)
			goto write_player_file_failed;
		for (i = 0; i < MAX_CONTROLS*2; i++)
			if (PHYSFS_write(file, "0", sizeof(ubyte), 1) != 1) // Skip obsolete Cyberman/Winjoy map fields
				goto write_player_file_failed;
		if (PHYSFS_write(file, &control_type_dos, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;
		if (PHYSFS_write(file, &control_type_win, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;
		if (PHYSFS_write(file, &old_avg_joy_sensitivity, sizeof(ubyte), 1) != 1)
			goto write_player_file_failed;

		for (i = 0; i < 11; i++)
		{
			PHYSFS_write(file, &PlayerCfg.PrimaryOrder[i], sizeof(ubyte), 1);
			PHYSFS_write(file, &PlayerCfg.SecondaryOrder[i], sizeof(ubyte), 1);
		}

		PHYSFS_writeULE32(file, PlayerCfg.Cockpit3DView[0]);
		PHYSFS_writeULE32(file, PlayerCfg.Cockpit3DView[1]);

#ifdef NETWORK
		PHYSFS_writeULE32(file, PlayerCfg.NetlifeKills);
		PHYSFS_writeULE32(file, PlayerCfg.NetlifeKilled);
		i=get_lifetime_checksum (PlayerCfg.NetlifeKills,PlayerCfg.NetlifeKilled);
#else
		PHYSFS_writeULE32(file, 0);
		PHYSFS_writeULE32(file, 0);
		i = get_lifetime_checksum (0, 0);
#endif
		PHYSFS_writeULE32(file, i);
	}

	//write guidebot name
	PHYSFSX_writeString(file, PlayerCfg.GuidebotNameReal);

	{
		char buf[128];
		strcpy(buf, "DOS joystick");
		PHYSFSX_writeString(file, buf);		// Write out current joystick for player.
	}

	if (!PHYSFS_close(file))
		goto write_player_file_failed;

	return EZERO;

 write_player_file_failed:
	nm_messagebox(TXT_ERROR, TXT_ERROR_WRITING_PLR, TXT_OK);
	if (file)
	{
		PHYSFS_close(file);
		PHYSFS_delete(filename);        //delete bogus file
	}

	return -1;
}

int get_lifetime_checksum (int a,int b)
{
  int num;

  // confusing enough to beat amateur disassemblers? Lets hope so

  num=(a<<8 ^ b);
  num^=(a | b);
  num*=num>>2;
  return (num);
}
