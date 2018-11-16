/*
 * This is an alternate backend for the music system.
 * It uses SDL_mixer to provide a more reliable playback,
 * and allow processing of multiple audio formats.
 *
 *  -- MD2211 (2006-04-24)
 */

#include <SDL.h>
#include <SDL_mixer.h>
#include <string.h>
#include <stdlib.h>

#include "args.h"
#include "hmp.h"
#include "digi_mixer_music.h"
#include "cfile.h"
#include "u_mem.h"

#ifdef _WIN32
extern int digi_win32_play_midi_song( char * filename, int loop );
#endif
Mix_Music *current_music = NULL;
static unsigned char *current_music_hndlbuf = NULL;


/*
 *  Plays a music file from an absolute path or a relative path
 */

int mix_play_file(char *filename, int loop, void (*hook_finished_track)())
{
	SDL_RWops *rw = NULL;
	PHYSFS_file *filehandle = NULL;
	char full_path[PATH_MAX];
	char *fptr;
	unsigned int bufsize = 0;

	mix_free_music();	// stop and free what we're already playing, if anything

	fptr = strrchr(filename, '.');

	if (fptr == NULL)
		return 0;

	// It's a .hmp!
	if (!stricmp(fptr, ".hmp"))
	{
		hmp2mid(filename, &current_music_hndlbuf, &bufsize);
		rw = SDL_RWFromConstMem(current_music_hndlbuf,bufsize*sizeof(char));
		current_music = Mix_LoadMUS_RW(rw, 1);
	}

	// try loading music via given filename
	if (!current_music)
		current_music = Mix_LoadMUS(filename);

	// still nothing? Let's open via PhysFS in case it's located inside an archive
	if (!current_music)
	{
		filehandle = PHYSFS_openRead(filename);
		if (filehandle != NULL)
		{
			unsigned len = PHYSFS_fileLength(filehandle);
			unsigned char *p = (unsigned char *)d_realloc(current_music_hndlbuf, sizeof(char)*len);
			if (p)
			{
			current_music_hndlbuf = p;
			bufsize = PHYSFS_read(filehandle, current_music_hndlbuf, sizeof(char), len);
			rw = SDL_RWFromConstMem(current_music_hndlbuf,bufsize*sizeof(char));
			current_music = Mix_LoadMUS_RW(rw, 1);
			}
			PHYSFS_close(filehandle);
		}
	}

	if (current_music)
	{
		Mix_PlayMusic(current_music, (loop ? -1 : 1));
		Mix_HookMusicFinished(hook_finished_track ? hook_finished_track : mix_free_music);
		return 1;
	}
	else
	{
		con_printf(CON_CRITICAL,"Music %s could not be loaded: %s\n", filename, Mix_GetError());
		mix_stop_music();
	}

	return 0;
}

// What to do when stopping song playback
void mix_free_music()
{
	Mix_HaltMusic();
	if (current_music)
	{
		Mix_FreeMusic(current_music);
		current_music = NULL;
	}
	if (current_music_hndlbuf)
	{
		d_free(current_music_hndlbuf);
		current_music_hndlbuf = NULL;
	}
}

void mix_set_music_volume(int vol)
{
	vol *= MIX_MAX_VOLUME/8;
	Mix_VolumeMusic(vol);
}

void mix_stop_music()
{
	Mix_HaltMusic();
	if (current_music_hndlbuf)
	{
		d_free(current_music_hndlbuf);
		current_music_hndlbuf = NULL;
	}
}

void mix_pause_music()
{
	Mix_PauseMusic();
}

void mix_resume_music()
{
	Mix_ResumeMusic();
}

void mix_pause_resume_music()
{
	if (Mix_PausedMusic())
		Mix_ResumeMusic();
	else if (Mix_PlayingMusic())
		Mix_PauseMusic();
}
