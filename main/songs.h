/* $Id: songs.h,v 1.1.1.1 2006/03/17 19:55:35 zicodxx Exp $ */
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
 * Header for songs.c
 *
 */

#ifndef _SONGS_H
#define _SONGS_H

typedef struct song_info {
	char    filename[16];
	char    melodic_bank_file[16];
	char    drum_bank_file[16];
	int	id; // representative number for each song
} song_info;

extern int Num_songs;   //how many songs
extern song_info Songs[];

#define SONG_TITLE              0
#define SONG_BRIEFING           1
#define SONG_ENDLEVEL           2
#define SONG_ENDGAME            3
#define SONG_CREDITS            4
#define SONG_FIRST_LEVEL_SONG   5
#define MAX_NUM_SONGS          (5+MAX_LEVELS_PER_MISSION+MAX_SECRET_LEVELS_PER_MISSION)

int songs_play_song( int songnum, int repeat );
int songs_play_level_song( int levelnum, int offset );

//stop any songs - midi, redbook or jukebox - that are currently playing
void songs_stop_all(void);

// check which song is playing
int songs_is_playing();

void songs_pause(void);
void songs_resume(void);
void songs_pause_resume(void);

// set volume for selected music playback system
void songs_set_volume(int volume);

void songs_uninit();

#endif

