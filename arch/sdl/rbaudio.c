/*
 *
 * SDL CD Audio functions
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "rbaudio.h"

#if 1
void RBAInit(void) {}
void RBAExit() {}
int RBAPlayTrack(int track) {return -1;}
int RBAPlayTracks(int first, int last, void (*hook_finished)(void)) {return 0;}
int	RBAPeekPlayStatus(void) {return 0;}
void RBAStop(void) {}
void RBAEjectDisk(void) {}
void RBASetVolume(int volume) {}
int	RBAEnabled(void) {return 0;}
void RBADisable(void) {}
void RBAEnable(void) {}
int	RBAGetNumberOfTracks(void) {return -1;}
void	RBAPause() {}
int	RBAResume() {return -1;}
int	RBAPauseResume() {return 0;}
int RBAGetTrackNum() {return 0;}
unsigned long RBAGetDiscID() {return 0;}
void RBAList(void) {}
void RBACheckFinishedHook() {}

#else

#include <SDL/SDL.h>

#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#endif

#include "pstypes.h"
#include "error.h"
#include "args.h"
#include "console.h"
#include "timer.h"

#define REDBOOK_VOLUME_SCALE 255

static SDL_CD *s_cd = NULL;
static int initialised = 0;

void RBAExit()
{
	if (s_cd)
	{
		SDL_CDStop(s_cd);
		SDL_CDClose(s_cd);
	}
}

void RBAInit()
{
	int num_cds;
	int i,j;
	
	if (initialised) return;

	if (SDL_Init(SDL_INIT_CDROM) < 0)
	{
		Warning("SDL library initialisation failed: %s.",SDL_GetError());
		return;
	}

	num_cds = SDL_CDNumDrives();
	if (num_cds < 1)
	{
		con_printf(CON_NORMAL, "No cdrom drives found!\n");
#if defined(__APPLE__) || defined(macintosh)
		SDL_QuitSubSystem(SDL_INIT_CDROM);	// necessary for rescanning CDROMs
#endif
		return;
	}
	
	for (i = 0; i < num_cds; i++)
	{
		if (s_cd)
			SDL_CDClose(s_cd);
		s_cd = SDL_CDOpen(i);
		
		if (s_cd && CD_INDRIVE(SDL_CDStatus(s_cd)))
		{
			for (j = 0; j < s_cd->numtracks; j++)
			{
				if (s_cd->track[j].type == SDL_AUDIO_TRACK)
					break;
			}
			
			if (j != s_cd->numtracks)
				break;	// we've found an audio CD
		}
		else if (s_cd == NULL)
			Warning("Could not open cdrom %i for redbook audio:%s\n", i, SDL_GetError());
	}
	
	if (i == num_cds)
	{
		con_printf(CON_NORMAL, "No audio CDs found\n");
		if (s_cd)	// if there's no audio CD, say that there's no redbook and hence play MIDI instead
		{
			SDL_CDClose(s_cd);
			s_cd = NULL;
		}
#if defined(__APPLE__) || defined(macintosh)
		SDL_QuitSubSystem(SDL_INIT_CDROM);	// necessary for rescanning CDROMs
#endif
		return;
	}
	
	initialised = 1;
}

int RBAEnabled()
{
	return initialised;
}

int RBAPlayTrack(int a)
{
	if (!s_cd) return -1;

	if (CD_INDRIVE(SDL_CDStatus(s_cd)) ) {
		if (SDL_CDPlayTracks(s_cd, a-1, 0, 0, 0) == 0)
			return a;
	}
	return -1;
}

void (*redbook_finished_hook)() = NULL;

void RBAStop()
{
	if (!s_cd) return;
	SDL_CDStop(s_cd);
	redbook_finished_hook = NULL;	// no calling the finished hook - stopped means stopped here
}

void RBAEjectDisk()
{
	if (!s_cd) return;
	SDL_CDEject(s_cd);	// play nothing until it tries to load a song
#if defined(__APPLE__) || defined(macintosh)
	SDL_QuitSubSystem(SDL_INIT_CDROM);	// necessary for rescanning CDROMs
#endif
	initialised = 0;
}

void RBASetVolume(int volume)
{
#ifdef __linux__
	int cdfile, level;
	struct cdrom_volctrl volctrl;

	if (!s_cd) return;

	cdfile = s_cd->id;
	level = volume*REDBOOK_VOLUME_SCALE/8;

	if ((level<0) || (level>REDBOOK_VOLUME_SCALE)) {
		con_printf(CON_CRITICAL, "illegal volume value (allowed values 0-%i)\n",REDBOOK_VOLUME_SCALE);
		return;
	}

	volctrl.channel0
		= volctrl.channel1
		= volctrl.channel2
		= volctrl.channel3
		= level;
	if ( ioctl(cdfile, CDROMVOLCTRL, &volctrl) == -1 ) {
		con_printf(CON_CRITICAL, "CDROMVOLCTRL ioctl failed\n");
		return;
	}
#endif
}

void RBAPause()
{
	if (!s_cd) return;
	SDL_CDPause(s_cd);
}

int RBAResume()
{
	if (!s_cd) return -1;
	SDL_CDResume(s_cd);
	return 1;
}

int RBAPauseResume()
{
	if (!s_cd) return 0;

	if (SDL_CDStatus(s_cd) == CD_PLAYING)
		SDL_CDPause(s_cd);
	else if (SDL_CDStatus(s_cd) == CD_PAUSED)
		SDL_CDResume(s_cd);
	else
		return 0;

	return 1;
}

int RBAGetNumberOfTracks()
{
	if (!s_cd) return -1;
	SDL_CDStatus(s_cd);
	return s_cd->numtracks;
}

// check if we need to call the 'finished' hook
// needs to go in all event loops
// a real hook would be ideal, but is currently unsupported by SDL Audio CD
void RBACheckFinishedHook()
{
	static fix64 last_check_time = 0;
	
	if (!s_cd) return;

	if ((timer_query() - last_check_time) >= F2_0)
	{
		if ((SDL_CDStatus(s_cd) == CD_STOPPED) && redbook_finished_hook)
			redbook_finished_hook();
		last_check_time = timer_query();
	}
}

// plays tracks first through last, inclusive
int RBAPlayTracks(int first, int last, void (*hook_finished)(void))
{
	if (!s_cd)
		return 0;

	if (CD_INDRIVE(SDL_CDStatus(s_cd)))
	{
		redbook_finished_hook = hook_finished;
		return SDL_CDPlayTracks(s_cd, first - 1, 0, last - first + 1, 0) == 0;
	}
	return 0;
}

// return the track number currently playing.  Useful if RBAPlayTracks()
// is called.  Returns 0 if no track playing, else track number
int RBAGetTrackNum()
{
	if (!s_cd)
		return 0;

	if (SDL_CDStatus(s_cd) != CD_PLAYING)
		return 0;

	return s_cd->cur_track + 1;
}

int RBAPeekPlayStatus()
{
	if (!s_cd)
		return 0;

	if (SDL_CDStatus(s_cd) == CD_PLAYING)
		return 1;
	else if (SDL_CDStatus(s_cd) == CD_PAUSED)	// hack so it doesn't keep restarting paused music
		return -1;
	
	return 0;
}

static int cddb_sum(int n)
{
	int ret;

	/* For backward compatibility this algorithm must not change */

	ret = 0;

	while (n > 0) {
		ret = ret + (n % 10);
		n = n / 10;
	}

	return (ret);
}


unsigned long RBAGetDiscID()
{
	int i, t = 0, n = 0;

	if (!s_cd)
		return 0;

	/* For backward compatibility this algorithm must not change */

	i = 0;

	while (i < s_cd->numtracks) {
		n += cddb_sum(s_cd->track[i].offset / CD_FPS);
		i++;
	}

	t = (s_cd->track[s_cd->numtracks].offset / CD_FPS) -
	    (s_cd->track[0].offset / CD_FPS);

	return ((n % 0xff) << 24 | t << 8 | s_cd->numtracks);
}

void RBAList(void)
{
	int i;

	if (!s_cd)
		return;

	for (i = 0; i < s_cd->numtracks; i++)
		con_printf(CON_DEBUG, "CD track %d, type %s, length %d, offset %d", s_cd->track[i].id, (s_cd->track[i].type == SDL_AUDIO_TRACK) ? "audio" : "data", s_cd->track[i].length, s_cd->track[i].offset);
}

#endif