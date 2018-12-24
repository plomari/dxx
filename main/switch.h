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
 * Triggers and Switches.
 *
 */

#ifndef _SWITCH_H
#define _SWITCH_H

#include "inferno.h"
#include "segment.h"

#define MAX_TRIGGERS		254
#define MAX_OBJ_TRIGGERS	254
#define MAX_ALL_TRIGGERS	(MAX_TRIGGERS + MAX_OBJ_TRIGGERS)
#define MAX_WALLS_PER_LINK  10

// Trigger types

#define TT_OPEN_DOOR        0   // Open a door
#define TT_CLOSE_DOOR       1   // Close a door
#define TT_MATCEN           2   // Activate a matcen
#define TT_EXIT             3   // End the level
#define TT_SECRET_EXIT      4   // Go to secret level
#define TT_ILLUSION_OFF     5   // Turn an illusion off
#define TT_ILLUSION_ON      6   // Turn an illusion on
#define TT_UNLOCK_DOOR      7   // Unlock a door
#define TT_LOCK_DOOR        8   // Lock a door
#define TT_OPEN_WALL        9   // Makes a wall open
#define TT_CLOSE_WALL       10  // Makes a wall closed
#define TT_ILLUSORY_WALL    11  // Makes a wall illusory
#define TT_LIGHT_OFF        12  // Turn a light off
#define TT_LIGHT_ON         13  // Turn s light on
// D2X-XL
#define TT_TELEPORT			14
#define TT_SPEEDBOOST		15
#define TT_CAMERA			16
#define TT_SHIELD_DAMAGE	17
#define TT_ENERGY_DRAIN		18
#define TT_CHANGE_TEXTURE	19
#define TT_SMOKE_LIFE		20
#define TT_SMOKE_SPEED		21
#define TT_SMOKE_DENS		22
#define TT_SMOKE_SIZE		23
#define TT_SMOKE_DRIFT		24
#define TT_COUNTDOWN		25
#define TT_SPAWN_BOT		26
#define TT_SMOKE_BRIGHTNESS 27
#define TT_SET_SPAWN		28
#define TT_MESSAGE			29
#define TT_SOUND			30
#define TT_MASTER			31
#define TT_ENABLE_TRIGGER	32
#define TT_DISABLE_TRIGGER	33
#define TT_DISARM_ROBOT		34
#define TT_REPROGRAM_ROBOT  35
#define TT_SHAKE_MINE		36
#define NUM_TRIGGER_TYPES   37

// Trigger flags

//could also use flags for one-shots

#define TF_NO_MESSAGE       1   // Don't show a message when triggered
#define TF_ONE_SHOT         2   // Only trigger once
#define TF_DISABLED         4   // Set after one-shot fires
// D2X-XL
#define TF_PERMANENT		8
#define TF_ALTERNATE		16
#define TF_SET_ORIENT		32
#define TF_SILENT			64
#define TF_AUTOPLAY			128
#define TF_PLAYING_SOUND	256
#define TF_FLY_THROUGH		512
// This fork. (D2X-XL also has such a flag, but as separate int32 bool field.)
#define TF_SHOT				(1 << 17)

//old trigger structs

typedef struct v29_trigger {
	sbyte   type;
	short   flags;
	fix     value;
	fix     time;
	sbyte   link_num;
	short   num_links;
	short   seg[MAX_WALLS_PER_LINK];
	short   side[MAX_WALLS_PER_LINK];
} __pack__ v29_trigger;

typedef struct v30_trigger {
	short   flags;
	sbyte   num_links;
	sbyte   pad;                        //keep alignment
	fix     value;
	fix     time;
	short   seg[MAX_WALLS_PER_LINK];
	short   side[MAX_WALLS_PER_LINK];
} __pack__ v30_trigger;

//flags for V30 & below triggers
#define TRIGGER_CONTROL_DOORS      1    // Control Trigger
#define TRIGGER_SHIELD_DAMAGE      2    // Shield Damage Trigger
#define TRIGGER_ENERGY_DRAIN       4    // Energy Drain Trigger
#define TRIGGER_EXIT               8    // End of level Trigger
#define TRIGGER_ON                16    // Whether Trigger is active
#define TRIGGER_ONE_SHOT          32    // If Trigger can only be triggered once
#define TRIGGER_MATCEN            64    // Trigger for materialization centers
#define TRIGGER_ILLUSION_OFF     128    // Switch Illusion OFF trigger
#define TRIGGER_SECRET_EXIT      256    // Exit to secret level
#define TRIGGER_ILLUSION_ON      512    // Switch Illusion ON trigger
#define TRIGGER_UNLOCK_DOORS    1024    // Unlocks a door
#define TRIGGER_OPEN_WALL       2048    // Makes a wall open
#define TRIGGER_CLOSE_WALL      4096    // Makes a wall closed
#define TRIGGER_ILLUSORY_WALL   8192    // Makes a wall illusory

//the trigger really should have both a type & a flags, since most of the
//flags bits are exclusive of the others.
typedef struct trigger {
	ubyte   type;       //what this trigger does
	uint32_t flags;     //TF_* bits
	sbyte   num_links;  //how many doors, etc. linked to this
	short   seg[MAX_WALLS_PER_LINK];
	short   side[MAX_WALLS_PER_LINK];
	// d2x-xl only fields
	fix     value;			// unknown function (present in D2 but never used)
	fix     time;			// shake duration? (present in D2 but never used)
	fix		delay;			// something absurd about delaying triggers?
	fix		last_operated; 	// last time at which this was "operated"
	int8_t	last_player; 	// last player by which this was "operated"
	int16_t	object_id;		// object triggers: Objects[] index
							// multiple triggers can point to the same object
							// -1 if dead/not object trigger
} trigger;

#define TRIGGER_DEFAULTS (trigger){		\
	.last_operated = -1,				\
	.last_player = -1,					\
	.object_id = -1,					\
}

extern trigger Triggers[MAX_ALL_TRIGGERS];

extern int Num_triggers;
extern int Num_object_triggers;

#define ObjectTriggers (Triggers + Num_triggers)
#define IS_OBJECT_TRIGGER(id) ((id) >= Num_triggers && \
							   (id) < Num_triggers + Num_object_triggers)
#define OBJECT_TRIGGER_INDEX(id) ((id) + Num_triggers)

extern void trigger_init();
extern void check_trigger(segment *seg, short side, short objnum,int shot);
extern int check_trigger_sub(int trigger_num, int player_num,int shot);
extern void triggers_frame_process();

void trigger_delete_object(int objnum);
bool trigger_warn_unsupported(int idx, bool hud);

/*
 * reads a v29_trigger structure from a CFILE
 */
extern void v29_trigger_read(v29_trigger *t, CFILE *fp);

/*
 * reads a v30_trigger structure from a CFILE
 */
extern void v30_trigger_read(v30_trigger *t, CFILE *fp);

/*
 * reads a trigger structure from a CFILE
 */
extern void trigger_read(trigger *t, CFILE *fp, bool obj_trigger);

#endif
