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
 * Header for gamemine.c
 *
 */


#ifndef _GAMEMINE_H
#define _GAMEMINE_H

#define MINE_VERSION        20  // Current version expected
#define COMPATIBLE_VERSION  16  // Oldest version that can safely be loaded.

// returns 1 if error, else 0
int game_load_mine(char * filename);

// loads from an already-open file
// returns 0=everything ok, 1=old version, -1=error
int load_mine_data(CFILE *LoadFile);
int load_mine_data_compiled(CFILE *LoadFile);

extern short tmap_xlate_table[];
extern fix Level_shake_frequency, Level_shake_duration;
extern int Secret_return_segment;
extern vms_matrix Secret_return_orient;
extern int Sky_box_segment;

#define TMAP_NUM_MASK 0x3FFF

/* stuff for loading descent.pig of descent 1 */
extern short convert_d1_tmap_num(short d1_tmap_num);
extern int d1_tmap_num_unique(short d1_tmap_num); //is d1_tmap_num's texture only in d1?

#endif /* _GAMEMINE_H */
