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
 * Header for wall.c
 *
 */

#ifndef _WALL_H
#define _WALL_H

#include "inferno.h"
#include "segment.h"
#include "object.h"
#include "cfile.h"

//#include "vclip.h"

#define MAX_WALLS               2047 // Maximum number of walls
#define MAX_WALL_ANIMS          60  // Maximum different types of doors
#define MAX_DOORS               90  // Maximum number of open doors

// Various wall types.
#define WALL_NORMAL             0   // Normal wall
#define WALL_BLASTABLE          1   // Removable (by shooting) wall
#define WALL_DOOR               2   // Door
#define WALL_ILLUSION           3   // Wall that appears to be there, but you can fly thru
#define WALL_OPEN               4   // Just an open side. (Trigger)
#define WALL_CLOSED             5   // Wall.  Used for transparent walls.
#define WALL_OVERLAY            6   // Goes over an actual solid side.  For triggers
#define WALL_CLOAKED            7   // Can see it, and see through it

// Various wall flags.
#define WALL_BLASTED            1   // Blasted out wall.
#define WALL_DOOR_OPENED        2   // Open door.
#define WALL_DOOR_LOCKED        8   // Door is locked.
#define WALL_DOOR_AUTO          16  // Door automatically closes after time.
#define WALL_ILLUSION_OFF       32  // Illusionary wall is shut off.
#define WALL_WALL_SWITCH        64  // This wall is openable by a wall switch.
#define WALL_BUDDY_PROOF        128 // Buddy assumes he cannot get through this wall.
// D2X-XL
#define WALL_IGNORE_MARKER		256
// This fork. Internally set.
#define WALL_HAS_TRIGGERS		(1 << 15)

// Wall states
#define WALL_DOOR_CLOSED        0       // Door is closed
#define WALL_DOOR_OPENING       1       // Door is opening.
#define WALL_DOOR_WAITING       2       // Waiting to close
#define WALL_DOOR_CLOSING       3       // Door is closing
#define WALL_DOOR_OPEN          4       // Door is open, and staying open
#define WALL_DOOR_CLOAKING      5       // Wall is going from closed -> open
#define WALL_DOOR_DECLOAKING    6       // Wall is going from open -> closed

//note: a door is considered opened (i.e., it has WALL_OPENED set) when it
//is more than half way open.  Thus, it can have any of OPENING, CLOSING,
//or WAITING bits set when OPENED is set.

#define KEY_NONE                1
#define KEY_BLUE                2
#define KEY_RED                 4
#define KEY_GOLD                8

#define WALL_HPS                100*F1_0    // Normal wall's hp
#define WALL_DOOR_INTERVAL      5*F1_0      // How many seconds a door is open

#define DOOR_OPEN_TIME          i2f(2)      // How long takes to open
#define DOOR_WAIT_TIME          i2f(5)      // How long before auto door closes

#define MAX_CLIP_FRAMES         50

// WALL_IS_DOORWAY flags.
#define WID_FLY_FLAG            1	// player can go through (for physics)
#define WID_RENDER_FLAG         2	// a texture/polygon of any kind is rendered
#define WID_RENDPAST_FLAG       4	// things behind the wall need to be rendered
#define WID_EXTERNAL_FLAG       8	// ?
#define WID_CLOAKED_FLAG        16	// cloaked wall (renderer special effects)
#define WID_RENDER_ALPHA_FLAG	32	// texture contains alpha values other than
									// 0 and 255 (implies RENDER/RENDPAST)
#define WID_LIGHTONLY_FLAG		64	// only light can travel through this wall
									// (WID_RENDPAST_FLAG is also set)

#define MAX_STUCK_OBJECTS   32

typedef struct stuckobj {
	short   objnum, wallnum;
	int     signature;
} stuckobj;

//Start old wall structures

typedef struct v16_wall {
	sbyte   type;               // What kind of special wall.
	sbyte   flags;              // Flags for the wall.
	fix     hps;                // "Hit points" of the wall.
	sbyte   trigger;            // Which trigger is associated with the wall.
	sbyte   clip_num;           // Which animation associated with the wall.
	sbyte   keys;
} __pack__ v16_wall;

typedef struct v19_wall {
	int     segnum,sidenum;     // Seg & side for this wall
	sbyte   type;               // What kind of special wall.
	sbyte   flags;              // Flags for the wall.
	fix     hps;                // "Hit points" of the wall.
	sbyte   trigger;            // Which trigger is associated with the wall.
	sbyte   clip_num;           // Which animation associated with the wall.
	sbyte   keys;
	int linked_wall;            // number of linked wall
} __pack__ v19_wall;

typedef struct v19_door {
	int     n_parts;            // for linked walls
	short   seg[2];             // Segment pointer of door.
	short   side[2];            // Side number of door.
	short   type[2];            // What kind of door animation.
	fix     open;               // How long it has been open.
} __pack__ v19_door;

//End old wall structures

typedef struct wall {
	int     segnum,sidenum;     // Seg & side for this wall
	fix     hps;                // "Hit points" of the wall.
	int     linked_wall;        // number of linked wall
	ubyte   type;               // What kind of special wall.
	ubyte   state;              // Opening, closing, etc.
	uint16_t flags;             // Flags for the wall.
	int16_t trigger;            // Which trigger is associated with the wall.
	sbyte   clip_num;           // Which animation associated with the wall.
	ubyte   keys;               // which keys are required
	sbyte   cloak_value;        // if this wall is cloaked, the fade value
} wall;

typedef struct active_door {
	int     n_parts;            // for linked walls
	short   front_wallnum[2];   // front wall numbers for this door
	short   back_wallnum[2];    // back wall numbers for this door
	fix     time;               // how long been opening, closing, waiting
} __pack__ active_door;

typedef struct cloaking_wall {
	short       front_wallnum;  // front wall numbers for this door
	short       back_wallnum;   // back wall numbers for this door
	fix     front_ls[4];        // front wall saved light values
	fix     back_ls[4];         // back wall saved light values
	fix     time;               // how long been cloaking or decloaking
} __pack__ cloaking_wall;

//wall clip flags
#define WCF_EXPLODES    1       //door explodes when opening
#define WCF_BLASTABLE   2       //this is a blastable wall
#define WCF_TMAP1       4       //this uses primary tmap, not tmap2
#define WCF_HIDDEN      8       //this uses primary tmap, not tmap2
// D2X-XL
#define WCF_ALTFMT		16

typedef struct {
	fix     play_time;
	short   num_frames;
	short   frames[MAX_CLIP_FRAMES];
	short   open_sound;
	short   close_sound;
	short   flags;
	char    filename[13];
	char    pad;
} __pack__ wclip;

extern char Wall_names[7][10];

// Automatically checks if a there is a doorway (i.e. can fly through)
#define WALL_IS_DOORWAY(seg,side) (((seg)->children[(side)] == -1) ? WID_RENDER_FLAG : ((seg)->children[(side)] == -2) ? WID_EXTERNAL_FLAG : ((seg)->sides[(side)].wall_num == -1) ? (WID_FLY_FLAG|WID_RENDPAST_FLAG) : wall_is_doorway_rest((seg), (side)))

extern wall Walls[MAX_WALLS];           // Master walls array
extern int Num_walls;                   // Number of walls

extern active_door ActiveDoors[MAX_DOORS];  //  Master doors array
extern int Num_open_doors;              // Number of open doors

extern cloaking_wall CloakingWalls[];
extern int Num_cloaking_walls;

extern wclip WallAnims[MAX_WALL_ANIMS];
extern int Num_wall_anims;

extern int walls_bm_num[MAX_WALL_ANIMS];

// Initializes all walls (i.e. no special walls.)
extern void wall_init();

// Call WALL_IS_DOORWAY().
extern int wall_is_doorway_rest(segment *seg, int side );

// Deteriorate appearance of wall. (Changes bitmap (paste-ons))
extern void wall_damage(segment *seg, int side, fix damage);

// Destroys a blastable wall. (So it is an opening afterwards)
extern void wall_destroy(segment *seg, int side);

void wall_illusion_on(segment *seg, int side);

void wall_illusion_off(segment *seg, int side);

// Opens a door, including animation and other processing.
void do_door_open(int door_num);

// Closes a door, including animation and other processing.
void do_door_close(int door_num);

// Opens a door
extern void wall_open_door(segment *seg, int side);

// Closes a door
extern void wall_close_door(segment *seg, int side);

//return codes for wall_hit_process()
#define WHP_NOT_SPECIAL     0       //wasn't a quote-wall-unquote
#define WHP_NO_KEY          1       //hit door, but didn't have key
#define WHP_BLASTABLE       2       //hit blastable wall
#define WHP_DOOR            3       //a door (which will now be opening)

// Determines what happens when a wall is shot
//obj is the object that hit...either a weapon or the player himself
extern int wall_hit_process(segment *seg, int side, fix damage, int playernum, object *obj );

// Opens/destroys specified door.
extern void wall_toggle(int segnum, int side);

// Tidy up Walls array for load/save purposes.
extern void reset_walls();

// Called once per frame..
void wall_frame_process();

extern stuckobj Stuck_objects[MAX_STUCK_OBJECTS];

//  An object got stuck in a door (like a flare).
//  Add global entry.
extern void add_stuck_object(object *objp, int segnum, int sidenum);
extern void remove_obsolete_stuck_objects(void);

//set the tmap_num or tmap_num2 field for a wall/door
extern void wall_set_tmap_num(segment *seg,int side,segment *csegp,int cside,int anim_num,int frame_num);

// Remove any flares from a wall
void kill_stuck_objects(int wallnum);

//start wall open <-> closed transitions
void start_wall_cloak(segment *seg, int side);
void start_wall_decloak(segment *seg, int side);

int wall_check_transparency(segment * seg, int side);

/*
 * reads n wclip structs from a CFILE
 */
extern int wclip_read_n(wclip *wc, int n, CFILE *fp);

/*
 * reads a v16_wall structure from a CFILE
 */
extern void v16_wall_read(v16_wall *w, CFILE *fp);

/*
 * reads a v19_wall structure from a CFILE
 */
extern void v19_wall_read(v19_wall *w, CFILE *fp);

/*
 * reads a wall structure from a CFILE
 */
extern void wall_read(wall *w, CFILE *fp, int version);

/*
 * reads a v19_door structure from a CFILE
 */
extern void v19_door_read(v19_door *d, CFILE *fp);

/*
 * reads an active_door structure from a CFILE
 */
extern void active_door_read(active_door *ad, CFILE *fp);

extern void wall_write(wall *w, short version, PHYSFS_file *fp);

#endif
