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
 * Definitions for fueling centers.
 *
 */

#ifndef _FUELCEN_H
#define _FUELCEN_H

#include "segment.h"
#include "object.h"

//------------------------------------------------------------
// A refueling center is one segment... to identify it in the
// segment structure, the "special" field is set to
// SEGMENT_IS_FUELCEN.  The "value" field is then used for how
// much fuel the center has left, with a maximum value of 100.

//-------------------------------------------------------------
// To hook into Inferno:
// * When all segents are deleted or before a new mine is created
//   or loaded, call fuelcen_reset().
// * Add call to fuelcen_create(segment * segp) to make a segment
//   which isn't a fuel center be a fuel center.
// * When a mine is loaded call fuelcen_activate(segp) with each
//   new segment as it loads. Always do this.
// * When a segment is deleted, always call fuelcen_delete(segp).
// * Call fuelcen_replentish_all() to fill 'em all up, like when
//   a new game is started.
// * When an object that needs to be refueled is in a segment, call
//   fuelcen_give_fuel(segp) to get fuel. (Call once for any refueling
//   object once per frame with the object's current segment.) This
//   will return a value between 0 and 100 that tells how much fuel
//   he got.


void fuelcen_init(segment *segp);

// Create a matcen robot
extern object *create_morph_robot(segment *segp, vms_vector *object_pos, int object_id);

// Returns the amount of fuel/shields this segment can give up.
// Can be from 0 to 100.
fix fuelcen_give_fuel(segment *segp, fix MaxAmountCanTake );
fix repaircen_give_shields(segment *segp, fix MaxAmountCanTake );

// Call once per frame.
void fuelcen_update_all();

// Called when hit by laser.
void fuelcen_damage(segment *segp, fix AmountOfDamage );

// Called to repair an object
//--repair-- int refuel_do_repair_effect( object * obj, int first_time, int repair_seg );

#define MAX_NUM_FUELCENS    500

//--repair-- //do the repair center for this frame
//--repair-- void do_repair_sequence(object *obj);
//--repair--
//--repair-- //see if we should start the repair center
//--repair-- void check_start_repair_center(object *obj);
//--repair--
//--repair-- //if repairing, cut it short
//--repair-- abort_repair_center();

// An array of pointers to segments with fuel centers.
typedef struct FuelCenter {
	int     Type;
	int     segnum;
	sbyte   Flag;
	sbyte   Enabled;
	sbyte   Lives;          // Number of times this can be enabled.
	sbyte   dum1;
	fix     Capacity;
	fix     MaxCapacity;
	fix     Timer;          // used in matcen for when next robot comes out
	fix     Disable_time;   // Time until center disabled.
	//object  *last_created_obj;
	//int     last_created_sig;
	vms_vector Center;
} __pack__ FuelCenter;

#define MAX_ROBOT_CENTERS  100
#define MAX_EQUIP_CENTERS  100

extern int Num_robot_centers;
extern int Num_equip_centers;

typedef struct  {
	int     robot_flags;    // Up to 32 different robots
	fix     hit_points;     // How hard it is to destroy this particular matcen
	fix     interval;       // Interval between materialogrifizations
	short   segnum;         // Segment this is attached to.
	short   fuelcen_num;    // Index in fuelcen array.
} __pack__ old_matcen_info;

typedef struct matcen_info {
	int     robot_flags[2]; // Up to 64 different robots
	fix     hit_points;     // How hard it is to destroy this particular matcen
	fix     interval;       // Interval between materialogrifizations
	short   dummy_segnum;   // Segment this is attached to. (unused)
	short   fuelcen_num;    // Index in fuelcen array.
} __pack__ matcen_info;

extern matcen_info RobotCenters[MAX_ROBOT_CENTERS];
extern matcen_info EquipCenters[MAX_EQUIP_CENTERS];

//--repair-- extern object *RepairObj;  // which object getting repaired, or NULL

extern void trigger_fuelcen(int segnum);

extern void disable_matcens(void);

int pick_robot_from_matcen_seg(int segnum);

extern FuelCenter Station[MAX_NUM_FUELCENS];
extern int Num_fuelcenters;

extern void init_all_matcens(void);

extern fix EnergyToCreateOneRobot;

void fuelcen_check_for_hoard_goal(segment *segp);

/*
 * reads an old_matcen_info structure from a CFILE
 */
void old_matcen_info_read(old_matcen_info *mi, CFILE *fp);

/*
 * reads a matcen_info structure from a CFILE
 */
void matcen_info_read(matcen_info *ps, CFILE *fp);

#endif
