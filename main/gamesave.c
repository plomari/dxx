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
 * Save game information
 *
 */

#include <stdio.h>
#include <string.h>
#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "key.h"
#include "gr.h"
#include "palette.h"
#include "newmenu.h"
#include "inferno.h"
#include "dxxerror.h"
#include "object.h"
#include "game.h"
#include "screens.h"
#include "wall.h"
#include "gamemine.h"
#include "robot.h"
#include "cfile.h"
#include "bm.h"
#include "menu.h"
#include "switch.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "weapon.h"
#include "newdemo.h"
#include "gameseq.h"
#include "automap.h"
#include "polyobj.h"
#include "text.h"
#include "gamefont.h"
#include "gamesave.h"
#include "gamepal.h"
#include "laser.h"
#include "byteswap.h"
#include "multi.h"
#include "makesig.h"

char Gamesave_current_filename[PATH_MAX];

int Gamesave_current_version;

#define GAME_VERSION            32
#define GAME_COMPATIBLE_VERSION 22

//version 28->29  add delta light support
//version 27->28  controlcen id now is reactor number, not model number
//version 28->29  ??
//version 29->30  changed trigger structure
//version 30->31  changed trigger structure some more
//version 31->32  change segment structure, make it 512 bytes w/o editor, add Segment2s array.

#define MENU_CURSOR_X_MIN       MENU_X
#define MENU_CURSOR_X_MAX       MENU_X+6

struct {
	ushort  fileinfo_signature;
	ushort  fileinfo_version;
	int     fileinfo_sizeof;
} game_top_fileinfo;    // Should be same as first two fields below...

struct {
	ushort  fileinfo_signature;
	ushort  fileinfo_version;
	int     fileinfo_sizeof;
	char    mine_filename[15];
	int     level;
	int     player_offset;              // Player info
	int     player_sizeof;
	int     object_offset;              // Object info
	int     object_howmany;
	int     object_sizeof;
	int     walls_offset;
	int     walls_howmany;
	int     walls_sizeof;
	int     doors_offset;
	int     doors_howmany;
	int     doors_sizeof;
	int     triggers_offset;
	int     triggers_howmany;
	int     triggers_sizeof;
	int     links_offset;
	int     links_howmany;
	int     links_sizeof;
	int     control_offset;
	int     control_howmany;
	int     control_sizeof;
	int     matcen_offset;
	int     matcen_howmany;
	int     matcen_sizeof;
	int     dl_indices_offset;
	int     dl_indices_howmany;
	int     dl_indices_sizeof;
	int     delta_light_offset;
	int     delta_light_howmany;
	int     delta_light_sizeof;
} game_fileinfo;

//  LINT: adding function prototypes
void read_object(object *obj, CFILE *f, int version);
void write_object(object *obj, short version, PHYSFS_file *f);

extern char MaxPowerupsAllowed[MAX_POWERUP_TYPES];
extern char PowerupsInMine[MAX_POWERUP_TYPES];

int Gamesave_num_org_robots = 0;
//--unused-- grs_bitmap * Gamesave_saved_bitmap = NULL;

//--unused-- vms_angvec zero_angles={0,0,0};

#define vm_angvec_zero(v) do {(v)->p=(v)->b=(v)->h=0;} while (0)

int Gamesave_num_players=0;

int N_save_pof_names;
char Save_pof_names[MAX_POLYGON_MODELS][FILENAME_LEN];

void check_and_fix_matrix(vms_matrix *m);

void verify_object( object * obj )	{

	obj->lifeleft = IMMORTAL_TIME;		//all loaded object are immortal, for now

	if ( obj->type == OBJ_ROBOT )	{
		Gamesave_num_org_robots++;

		// Make sure valid id...
		if ( obj->id >= N_robot_types )
			obj->id = obj->id % N_robot_types;

		// Make sure model number & size are correct...
		if ( obj->render_type == RT_POLYOBJ ) {
			Assert(Robot_info[obj->id].model_num != -1);
				//if you fail this assert, it means that a robot in this level
				//hasn't been loaded, possibly because he's marked as
				//non-shareware.  To see what robot number, print obj->id.

			Assert(Robot_info[obj->id].always_0xabcd == 0xabcd);
				//if you fail this assert, it means that the robot_ai for
				//a robot in this level hasn't been loaded, possibly because
				//it's marked as non-shareware.  To see what robot number,
				//print obj->id.

			obj->rtype.pobj_info.model_num = Robot_info[obj->id].model_num;
			obj->size = Polygon_models[obj->rtype.pobj_info.model_num].rad;

			//@@Took out this ugly hack 1/12/96, because Mike has added code
			//@@that should fix it in a better way.
			//@@//this is a super-ugly hack.  Since the baby stripe robots have
			//@@//their firing point on their bounding sphere, the firing points
			//@@//can poke through a wall if the robots are very close to it. So
			//@@//we make their radii bigger so the guns can't get too close to 
			//@@//the walls
			//@@if (Robot_info[obj->id].flags & RIF_BIG_RADIUS)
			//@@	obj->size = (obj->size*3)/2;

			//@@if (obj->control_type==CT_AI && Robot_info[obj->id].attack_type)
			//@@	obj->size = obj->size*3/4;
		}

		if (obj->id == 65)						//special "reactor" robots
			obj->movement_type = MT_NONE;

		if (obj->movement_type == MT_PHYSICS) {
			obj->mtype.phys_info.mass = Robot_info[obj->id].mass;
			obj->mtype.phys_info.drag = Robot_info[obj->id].drag;
		}
	}
	else {		//Robots taken care of above

		if ( obj->render_type == RT_POLYOBJ ) {
			int i;
			char *name = Save_pof_names[obj->rtype.pobj_info.model_num];

			for (i=0;i<N_polygon_models;i++)
				if (!stricmp(Pof_names[i],name)) {		//found it!	
					obj->rtype.pobj_info.model_num = i;
					break;
				}
		}
	}

	if ( obj->type == OBJ_POWERUP ) {
		if ( obj->id >= N_powerup_types )	{
			obj->id = 0;
			Assert( obj->render_type != RT_POLYOBJ );
		}
		obj->control_type = CT_POWERUP;
		obj->size = Powerup_info[obj->id].size;
		obj->ctype.powerup_info.creation_time = 0;

#ifdef NETWORK
		if (Game_mode & GM_NETWORK)
		{
			if (multi_powerup_is_4pack(obj->id))
			{
				PowerupsInMine[obj->id-1]+=4;
				MaxPowerupsAllowed[obj->id-1]+=4;
			}
			else
			{
				PowerupsInMine[obj->id]++;
				MaxPowerupsAllowed[obj->id]++;
			}
		}
#endif

	}

	if ( obj->type == OBJ_WEAPON )	{
		if ( obj->id >= N_weapon_types )	{
			obj->id = 0;
			Assert( obj->render_type != RT_POLYOBJ );
		}

		if (obj->id == PMINE_ID) {		//make sure pmines have correct values

			obj->mtype.phys_info.mass = Weapon_info[obj->id].mass;
			obj->mtype.phys_info.drag = Weapon_info[obj->id].drag;
			obj->mtype.phys_info.flags |= PF_FREE_SPINNING;

			// Make sure model number & size are correct...		
			Assert( obj->render_type == RT_POLYOBJ );

			obj->rtype.pobj_info.model_num = Weapon_info[obj->id].model_num;
			obj->size = Polygon_models[obj->rtype.pobj_info.model_num].rad;
		}
	}

	if ( obj->type == OBJ_CNTRLCEN ) {

		obj->render_type = RT_POLYOBJ;
		obj->control_type = CT_CNTRLCEN;

		if (Gamesave_current_version <= 1) { // descent 1 reactor
			obj->id = 0;                         // used to be only one kind of reactor
			obj->rtype.pobj_info.model_num = Reactors[0].model_num;// descent 1 reactor
		}

		// Make sure model number is correct...
		//obj->rtype.pobj_info.model_num = Reactors[obj->id].model_num;
	}

	if ( obj->type == OBJ_PLAYER )	{
		//int i;

		//Assert(obj == Player);

		if ( obj == ConsoleObject )		
			init_player_object();
		else
			if (obj->render_type == RT_POLYOBJ)	//recover from Matt's pof file matchup bug
				obj->rtype.pobj_info.model_num = Player_ship->model_num;

		//Make sure orient matrix is orthogonal
		check_and_fix_matrix(&obj->orient);

		obj->id = Gamesave_num_players++;
	}

	if (obj->type == OBJ_HOSTAGE) {

		//@@if (obj->id > N_hostage_types)
		//@@	obj->id = 0;

		obj->render_type = RT_HOSTAGE;
		obj->control_type = CT_POWERUP;
	}

}

//static gs_skip(int len,CFILE *file)
//{
//
//	cfseek(file,len,SEEK_CUR);
//}


extern int multi_powerup_is_4pack(int);
//reads one object of the given version from the given file
void read_object(object *obj,CFILE *f,int version)
{

	obj->type           = cfile_read_byte(f);
	obj->id             = cfile_read_byte(f);

	obj->control_type   = cfile_read_byte(f);
	obj->movement_type  = cfile_read_byte(f);
	obj->render_type    = cfile_read_byte(f);
	obj->flags          = cfile_read_byte(f);

	if (version > 37) {
		bool multiplayer = !!cfile_read_byte(f);
		if (multiplayer)
			printf("D2X-XL multiplayer object\n");
	}

	obj->segnum         = cfile_read_short(f);
	obj->attached_obj   = -1;

	cfile_read_vector(&obj->pos,f);
	cfile_read_matrix(&obj->orient,f);

	obj->size           = cfile_read_fix(f);
	obj->shields        = cfile_read_fix(f);

	cfile_read_vector(&obj->last_pos,f);

	obj->contains_type  = cfile_read_byte(f);
	obj->contains_id    = cfile_read_byte(f);
	obj->contains_count = cfile_read_byte(f);

	switch (obj->movement_type) {

		case MT_PHYSICS:

			cfile_read_vector(&obj->mtype.phys_info.velocity,f);
			cfile_read_vector(&obj->mtype.phys_info.thrust,f);

			obj->mtype.phys_info.mass		= cfile_read_fix(f);
			obj->mtype.phys_info.drag		= cfile_read_fix(f);
			obj->mtype.phys_info.brakes	= cfile_read_fix(f);

			cfile_read_vector(&obj->mtype.phys_info.rotvel,f);
			cfile_read_vector(&obj->mtype.phys_info.rotthrust,f);

			obj->mtype.phys_info.turnroll	= cfile_read_fixang(f);
			obj->mtype.phys_info.flags		= cfile_read_short(f);

			break;

		case MT_SPINNING:

			cfile_read_vector(&obj->mtype.spin_rate,f);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (obj->control_type) {

		case CT_AI: {
			int i;

			obj->ctype.ai_info.behavior				= cfile_read_byte(f);

			for (i=0;i<MAX_AI_FLAGS;i++)
				obj->ctype.ai_info.flags[i]			= cfile_read_byte(f);

			obj->ctype.ai_info.hide_segment			= cfile_read_short(f);
			obj->ctype.ai_info.hide_index			= cfile_read_short(f);
			obj->ctype.ai_info.path_length			= cfile_read_short(f);
			obj->ctype.ai_info.cur_path_index		= cfile_read_short(f);

			if (version <= 25) {
				cfile_read_short(f);	//				obj->ctype.ai_info.follow_path_start_seg	= 
				cfile_read_short(f);	//				obj->ctype.ai_info.follow_path_end_seg		= 
			}

			break;
		}

		case CT_EXPLOSION:

			obj->ctype.expl_info.spawn_time		= cfile_read_fix(f);
			obj->ctype.expl_info.delete_time		= cfile_read_fix(f);
			obj->ctype.expl_info.delete_objnum	= cfile_read_short(f);
			obj->ctype.expl_info.next_attach = obj->ctype.expl_info.prev_attach = obj->ctype.expl_info.attach_parent = -1;

			break;

		case CT_WEAPON:

			//do I really need to read these?  Are they even saved to disk?

			obj->ctype.laser_info.parent_type		= cfile_read_short(f);
			obj->ctype.laser_info.parent_num		= cfile_read_short(f);
			obj->ctype.laser_info.parent_signature	= cfile_read_int(f);

			break;

		case CT_LIGHT:

			obj->ctype.light_info.intensity = cfile_read_fix(f);
			break;

		case CT_POWERUP:

			if (version >= 25)
				obj->ctype.powerup_info.count = cfile_read_int(f);
			else
				obj->ctype.powerup_info.count = 1;

			if (obj->id == POW_VULCAN_WEAPON)
					obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

			if (obj->id == POW_GAUSS_WEAPON)
					obj->ctype.powerup_info.count = VULCAN_WEAPON_AMMO_AMOUNT;

			if (obj->id == POW_OMEGA_WEAPON)
					obj->ctype.powerup_info.count = MAX_OMEGA_CHARGE;

			break;


		case CT_NONE:
		case CT_FLYING:
		case CT_DEBRIS:
			break;

		case CT_SLEW:		//the player is generally saved as slew
			break;

		case CT_CNTRLCEN:
			break;

		// D2X-XL types
		case CT_WAYPOINT:
			cfile_read_int(f); // id
			cfile_read_int(f); // next point?
			cfile_read_int(f); // "speed"
			break;

		case CT_MORPH:
		case CT_FLYTHROUGH:
		case CT_REPAIRCEN:
		default:
			Int3();
	
	}

	switch (obj->render_type) {

		case RT_NONE:
			break;

		case RT_MORPH:
		case RT_POLYOBJ: {
			int i,tmo;

			obj->rtype.pobj_info.model_num		= cfile_read_int(f);

			for (i=0;i<MAX_SUBMODELS;i++)
				cfile_read_angvec(&obj->rtype.pobj_info.anim_angles[i],f);

			obj->rtype.pobj_info.subobj_flags	= cfile_read_int(f);

			tmo = cfile_read_int(f);

			obj->rtype.pobj_info.tmap_override	= tmo;

			obj->rtype.pobj_info.alt_textures	= 0;

			break;
		}

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:

			obj->rtype.vclip_info.vclip_num	= cfile_read_int(f);
			obj->rtype.vclip_info.frametime	= cfile_read_fix(f);
			obj->rtype.vclip_info.framenum	= cfile_read_byte(f);

			if (obj->rtype.vclip_info.vclip_num >= Num_vclips) {
				printf("Warning: object %zd: vclip %d >= Num_clips %d\n",
					   obj - Objects, obj->rtype.vclip_info.vclip_num, Num_vclips);
				obj->rtype.vclip_info.vclip_num = 0;
			}

			break;

		case RT_LASER:
			break;

		// D2X-XL types
		case RT_THRUSTER:
			break;
		case RT_PARTICLES:
			cfile_read_int(f); // life
			cfile_read_int(f); // size
			cfile_read_int(f); // parts
			cfile_read_int(f); // speed
			cfile_read_int(f); // drift
			cfile_read_int(f); // brightness
			cfile_read_byte(f); // r
			cfile_read_byte(f); // g
			cfile_read_byte(f); // b
			cfile_read_byte(f); // a
			cfile_read_byte(f); // side?
			if (Gamesave_current_version >= 18)
				cfile_read_byte(f); // type
			if (Gamesave_current_version >= 19)
				cfile_read_byte(f); // enabled
			break;
		case RT_LIGHTNING:
			cfile_read_int(f); // life
			cfile_read_int(f); // delay
			cfile_read_int(f); // length
			cfile_read_int(f); // ampl.
			cfile_read_int(f); // offset
			if (Gamesave_current_version > 22)
				cfile_read_int(f); // waypoint
			cfile_read_short(f); // bolts
			cfile_read_short(f); // id
			cfile_read_short(f); // target
			cfile_read_short(f); // nodes
			cfile_read_short(f); // children
			cfile_read_short(f); // frames
			if (Gamesave_current_version > 21)
				cfile_read_byte(f); // width
			cfile_read_byte(f); // angle
			cfile_read_byte(f); // style
			cfile_read_byte(f); // smooth
			cfile_read_byte(f); // clamp
			cfile_read_byte(f); // glow
			cfile_read_byte(f); // sound
			cfile_read_byte(f); // random
			cfile_read_byte(f); // inplane
			cfile_read_byte(f); // r
			cfile_read_byte(f); // g
			cfile_read_byte(f); // b
			cfile_read_byte(f); // a
			if (Gamesave_current_version >= 19)
				cfile_read_byte(f); // enabled
			break;
		case RT_SOUND: {
			char filename[40];
			cfread(filename, sizeof(filename), 1, f);
			cfile_read_int(f); // volume
			if (Gamesave_current_version >= 19)
				cfile_read_byte(f); // enabled
			break;
		}

		default:
			Int3();

	}

}

//writes one object to the given file
void write_object(object *obj, short version, PHYSFS_file *f)
{
	PHYSFSX_writeU8(f, obj->type);
	PHYSFSX_writeU8(f, obj->id);

	PHYSFSX_writeU8(f, obj->control_type);
	PHYSFSX_writeU8(f, obj->movement_type);
	PHYSFSX_writeU8(f, obj->render_type);
	PHYSFSX_writeU8(f, obj->flags);

	PHYSFS_writeSLE16(f, obj->segnum);

	PHYSFSX_writeVector(f, &obj->pos);
	PHYSFSX_writeMatrix(f, &obj->orient);

	PHYSFSX_writeFix(f, obj->size);
	PHYSFSX_writeFix(f, obj->shields);

	PHYSFSX_writeVector(f, &obj->last_pos);

	PHYSFSX_writeU8(f, obj->contains_type);
	PHYSFSX_writeU8(f, obj->contains_id);
	PHYSFSX_writeU8(f, obj->contains_count);

	switch (obj->movement_type) {

		case MT_PHYSICS:

	 		PHYSFSX_writeVector(f, &obj->mtype.phys_info.velocity);
			PHYSFSX_writeVector(f, &obj->mtype.phys_info.thrust);

			PHYSFSX_writeFix(f, obj->mtype.phys_info.mass);
			PHYSFSX_writeFix(f, obj->mtype.phys_info.drag);
			PHYSFSX_writeFix(f, obj->mtype.phys_info.brakes);

			PHYSFSX_writeVector(f, &obj->mtype.phys_info.rotvel);
			PHYSFSX_writeVector(f, &obj->mtype.phys_info.rotthrust);

			PHYSFSX_writeFixAng(f, obj->mtype.phys_info.turnroll);
			PHYSFS_writeSLE16(f, obj->mtype.phys_info.flags);

			break;

		case MT_SPINNING:

			PHYSFSX_writeVector(f, &obj->mtype.spin_rate);
			break;

		case MT_NONE:
			break;

		default:
			Int3();
	}

	switch (obj->control_type) {

		case CT_AI: {
			int i;

			PHYSFSX_writeU8(f, obj->ctype.ai_info.behavior);

			for (i = 0; i < MAX_AI_FLAGS; i++)
				PHYSFSX_writeU8(f, obj->ctype.ai_info.flags[i]);

			PHYSFS_writeSLE16(f, obj->ctype.ai_info.hide_segment);
			PHYSFS_writeSLE16(f, obj->ctype.ai_info.hide_index);
			PHYSFS_writeSLE16(f, obj->ctype.ai_info.path_length);
			PHYSFS_writeSLE16(f, obj->ctype.ai_info.cur_path_index);

			if (version <= 25)
			{
				PHYSFS_writeSLE16(f, -1);	//obj->ctype.ai_info.follow_path_start_seg
				PHYSFS_writeSLE16(f, -1);	//obj->ctype.ai_info.follow_path_end_seg
			}

			break;
		}

		case CT_EXPLOSION:

			PHYSFSX_writeFix(f, obj->ctype.expl_info.spawn_time);
			PHYSFSX_writeFix(f, obj->ctype.expl_info.delete_time);
			PHYSFS_writeSLE16(f, obj->ctype.expl_info.delete_objnum);

			break;

		case CT_WEAPON:

			//do I really need to write these objects?

			PHYSFS_writeSLE16(f, obj->ctype.laser_info.parent_type);
			PHYSFS_writeSLE16(f, obj->ctype.laser_info.parent_num);
			PHYSFS_writeSLE32(f, obj->ctype.laser_info.parent_signature);

			break;

		case CT_LIGHT:

			PHYSFSX_writeFix(f, obj->ctype.light_info.intensity);
			break;

		case CT_POWERUP:

			if (version >= 25)
				PHYSFS_writeSLE32(f, obj->ctype.powerup_info.count);
			break;

		case CT_NONE:
		case CT_FLYING:
		case CT_DEBRIS:
			break;

		case CT_SLEW:		//the player is generally saved as slew
			break;

		case CT_CNTRLCEN:
			break;			//control center object.

		case CT_MORPH:
		case CT_REPAIRCEN:
		case CT_FLYTHROUGH:
		default:
			Int3();

	}

	switch (obj->render_type) {

		case RT_NONE:
			break;

		case RT_MORPH:
		case RT_POLYOBJ: {
			int i;

			PHYSFS_writeSLE32(f, obj->rtype.pobj_info.model_num);

			for (i = 0; i < MAX_SUBMODELS; i++)
				PHYSFSX_writeAngleVec(f, &obj->rtype.pobj_info.anim_angles[i]);

			PHYSFS_writeSLE32(f, obj->rtype.pobj_info.subobj_flags);

			PHYSFS_writeSLE32(f, obj->rtype.pobj_info.tmap_override);

			break;
		}

		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:

			PHYSFS_writeSLE32(f, obj->rtype.vclip_info.vclip_num);
			PHYSFSX_writeFix(f, obj->rtype.vclip_info.frametime);
			PHYSFSX_writeU8(f, obj->rtype.vclip_info.framenum);

			break;

		case RT_LASER:
			break;

		default:
			Int3();

	}

}

extern int remove_trigger_num(int trigger_num);

// --------------------------------------------------------------------
// Load game 
// Loads all the relevant data for a level.
// If level != -1, it loads the filename with extension changed to .min
// Otherwise it loads the appropriate level mine.
// returns 0=everything ok, 1=old version, -1=error
int load_game_data(CFILE *LoadFile)
{
	int i,j;

	short game_top_fileinfo_version;
	int object_offset;
	int gs_num_objects;
	int num_delta_lights;
	int trig_size;

	//===================== READ FILE INFO ========================

#if 0
	cfread(&game_top_fileinfo, sizeof(game_top_fileinfo), 1, LoadFile);
#endif

	// Check signature
	if (cfile_read_short(LoadFile) != 0x6705)
		return -1;

	// Read and check version number
	game_top_fileinfo_version = cfile_read_short(LoadFile);
	if (game_top_fileinfo_version < GAME_COMPATIBLE_VERSION )
		return -1;

	// We skip some parts of the former game_top_fileinfo
	cfseek(LoadFile, 31, SEEK_CUR);

	object_offset = cfile_read_int(LoadFile);
	gs_num_objects = cfile_read_int(LoadFile);
	cfile_read_int(LoadFile); // size
	Assert(gs_num_objects >= 0 && gs_num_objects < MAX_OBJECTS);

	int wall_offset = cfile_read_int(LoadFile);
	Num_walls = cfile_read_int(LoadFile);
	cfile_read_int(LoadFile); // size
	Assert(Num_walls >= 0 && Num_walls < MAX_WALLS);

	cfile_read_int(LoadFile); // door offset
	cfile_read_int(LoadFile); // door num
	cfile_read_int(LoadFile); // door size

	int trigger_offset = cfile_read_int(LoadFile);
	Num_triggers = cfile_read_int(LoadFile);
	cfile_read_int(LoadFile); // size
	Assert(Num_triggers >= 0 && Num_triggers < MAX_TRIGGERS);

	Num_object_triggers = 0;

	cfile_read_int(LoadFile); // links offset
	cfile_read_int(LoadFile); // links num
	cfile_read_int(LoadFile); // links size

	int control_offset = cfile_read_int(LoadFile);
	int num_control = cfile_read_int(LoadFile);
	trig_size = cfile_read_int(LoadFile);
	Assert(trig_size == sizeof(ControlCenterTriggers));
	Assert(num_control == 1);

	int robot_center_offset = cfile_read_int(LoadFile);
	Num_robot_centers = cfile_read_int(LoadFile);
	cfile_read_int(LoadFile); // size
	Assert(Num_robot_centers >= 0 && Num_robot_centers < MAX_ROBOT_CENTERS);

	int static_light_offset, delta_light_offset;
	if (game_top_fileinfo_version >= 29) {
		static_light_offset = cfile_read_int(LoadFile);
		Num_static_lights = cfile_read_int(LoadFile);
		cfile_read_int(LoadFile); // size
		Assert(Num_static_lights >= 0 && Num_static_lights < MAX_DL_INDICES);
		delta_light_offset = cfile_read_int(LoadFile);
		num_delta_lights = cfile_read_int(LoadFile);
		cfile_read_int(LoadFile); // size
		Assert(num_delta_lights >= 0 && num_delta_lights < MAX_DELTA_LIGHTS);
	} else {
		static_light_offset = -1;
		Num_static_lights = 0;
		delta_light_offset = -1;
		num_delta_lights = 0;
	}

	if (Gamesave_current_version >= GAMESAVE_D2X_XL_VERSION) {
		if (Gamesave_current_version > 15)
			cfseek(LoadFile, 4 * 3, SEEK_CUR); // equipGen offset/count/size
		if (Gamesave_current_version > 26)
			cfseek(LoadFile, 4 * 4, SEEK_CUR); // 4x fog color (?)
	}

	if (game_top_fileinfo_version >= 31) //load mine filename
		// read newline-terminated string, not sure what version this changed.
		cfgets(Current_level_name,sizeof(Current_level_name),LoadFile);
	else if (game_top_fileinfo_version >= 14) { //load mine filename
		// read null-terminated string
		cfgets(Current_level_name, sizeof(Current_level_name), LoadFile);
	}
	else
		Current_level_name[0]=0;

	if (game_top_fileinfo_version >= 19) {	//load pof names
		N_save_pof_names = cfile_read_short(LoadFile);
		if (N_save_pof_names != 0x614d && N_save_pof_names != 0x5547) { // "Ma"de w/DMB beta/"GU"ILE
			Assert(N_save_pof_names < MAX_POLYGON_MODELS);
			cfread(Save_pof_names,N_save_pof_names,FILENAME_LEN,LoadFile);
		}
	}

	//===================== READ PLAYER INFO ==========================


	//===================== READ OBJECT INFO ==========================

	Gamesave_num_org_robots = 0;
	Gamesave_num_players = 0;

	if (object_offset > -1) {
		if (cfseek( LoadFile, object_offset, SEEK_SET ))
			Error( "Error seeking to object_offset in gamesave.c" );

		for (i = 0; i < gs_num_objects; i++) {

			read_object(&Objects[i], LoadFile, game_top_fileinfo_version);

			Objects[i].signature = obj_get_signature();
			verify_object( &Objects[i] );

			check_and_fix_matrix(&Objects[i].orient);
		}

	}

	//===================== READ WALL INFO ============================

	if (wall_offset > -1)
		Assert(cftell(LoadFile) == wall_offset);

	for (i = 0; i < Num_walls; i++) {
		if (game_top_fileinfo_version >= 20)
			wall_read(&Walls[i], LoadFile, game_top_fileinfo_version); // v20 walls and up.
		else if (game_top_fileinfo_version >= 17) {
			v19_wall w;
			v19_wall_read(&w, LoadFile);
			Walls[i].segnum	        = w.segnum;
			Walls[i].sidenum	= w.sidenum;
			Walls[i].linked_wall	= w.linked_wall;
			Walls[i].type		= w.type;
			Walls[i].flags		= w.flags;
			Walls[i].hps		= w.hps;
			Walls[i].trigger	= w.trigger;
			Walls[i].clip_num	= w.clip_num;
			Walls[i].keys		= w.keys;
			Walls[i].state		= WALL_DOOR_CLOSED;
		} else {
			v16_wall w;
			v16_wall_read(&w, LoadFile);
			Walls[i].segnum = Walls[i].sidenum = Walls[i].linked_wall = -1;
			Walls[i].type		= w.type;
			Walls[i].flags		= w.flags;
			Walls[i].hps		= w.hps;
			Walls[i].trigger	= w.trigger;
			Walls[i].clip_num	= w.clip_num;
			Walls[i].keys		= w.keys;
		}
	}

#if 0
	//===================== READ DOOR INFO ============================

	if (game_fileinfo.doors_offset > -1)
	{
		if (!cfseek( LoadFile, game_fileinfo.doors_offset,SEEK_SET ))	{

			for (i=0;i<game_fileinfo.doors_howmany;i++) {

				if (game_top_fileinfo_version >= 20)
					active_door_read(&ActiveDoors[i], LoadFile); // version 20 and up
				else {
					v19_door d;
					int p;

					v19_door_read(&d, LoadFile);

					ActiveDoors[i].n_parts = d.n_parts;

					for (p=0;p<d.n_parts;p++) {
						int cseg,cside;

						cseg = Segments[d.seg[p]].children[d.side[p]];
						cside = find_connect_side(&Segments[d.seg[p]],&Segments[cseg]);

						ActiveDoors[i].front_wallnum[p] = Segments[d.seg[p]].sides[d.side[p]].wall_num;
						ActiveDoors[i].back_wallnum[p] = Segments[cseg].sides[cside].wall_num;
					}
				}

			}
		}
	}
#endif // 0

	//==================== READ TRIGGER INFO ==========================


// for MACINTOSH -- assume all triggers >= verion 31 triggers.

	if (trigger_offset > -1)
		Assert(cftell(LoadFile) == trigger_offset);

	for (i = 0; i < Num_triggers; i++)
	{
		Triggers[i] = TRIGGER_DEFAULTS;

		if (game_top_fileinfo_version < 31)
		{
			v30_trigger trig;
			int t,type;
			type=0;

			if (game_top_fileinfo_version < 30) {
				v29_trigger trig29;
				int t;
				v29_trigger_read(&trig29, LoadFile);
				trig.flags	= trig29.flags;
				trig.num_links	= trig29.num_links;
				trig.num_links	= trig29.num_links;
				trig.value	= trig29.value;
				trig.time	= trig29.time;

				for (t=0;t<trig.num_links;t++) {
					trig.seg[t]  = trig29.seg[t];
					trig.side[t] = trig29.side[t];
				}
			}
			else
				v30_trigger_read(&trig, LoadFile);

			//Assert(trig.flags & TRIGGER_ON);
			trig.flags &= ~TRIGGER_ON;

			if (trig.flags & TRIGGER_CONTROL_DOORS)
				type = TT_OPEN_DOOR;
			else if (trig.flags & TRIGGER_SHIELD_DAMAGE)
				Int3();
			else if (trig.flags & TRIGGER_ENERGY_DRAIN)
				Int3();
			else if (trig.flags & TRIGGER_EXIT)
				type = TT_EXIT;
			else if (trig.flags & TRIGGER_ONE_SHOT)
				Int3();
			else if (trig.flags & TRIGGER_MATCEN)
				type = TT_MATCEN;
			else if (trig.flags & TRIGGER_ILLUSION_OFF)
				type = TT_ILLUSION_OFF;
			else if (trig.flags & TRIGGER_SECRET_EXIT)
				type = TT_SECRET_EXIT;
			else if (trig.flags & TRIGGER_ILLUSION_ON)
				type = TT_ILLUSION_ON;
			else if (trig.flags & TRIGGER_UNLOCK_DOORS)
				type = TT_UNLOCK_DOOR;
			else if (trig.flags & TRIGGER_OPEN_WALL)
				type = TT_OPEN_WALL;
			else if (trig.flags & TRIGGER_CLOSE_WALL)
				type = TT_CLOSE_WALL;
			else if (trig.flags & TRIGGER_ILLUSORY_WALL)
				type = TT_ILLUSORY_WALL;
			else
				Int3();
			Triggers[i].type        = type;
			Triggers[i].flags       = 0;
			Triggers[i].num_links   = trig.num_links;
			Triggers[i].num_links   = trig.num_links;
			Triggers[i].value       = trig.value;
			Triggers[i].time        = trig.time;
			for (t=0;t<trig.num_links;t++) {
				Triggers[i].seg[t] = trig.seg[t];
				Triggers[i].side[t] = trig.side[t];
			}
		}
		else
			trigger_read(&Triggers[i], LoadFile, false);
	}



	if (game_top_fileinfo_version >= 33 && trigger_offset >= 0) {
		Num_object_triggers = cfile_read_int(LoadFile);

		for (i = 0; i < Num_object_triggers; i++) {
			ObjectTriggers[i] = TRIGGER_DEFAULTS;
			trigger_read(&ObjectTriggers[i], LoadFile, true);
		}

		for (i = 0; i < Num_object_triggers; i++) {
			if (game_top_fileinfo_version < 40)
				cfile_read_int(LoadFile); // 2x short unused by D2X-XL
			ObjectTriggers[i].object_id = cfile_read_short(LoadFile);
		}

		if (Num_object_triggers) {
			int weirdshitdiscard = 0;
			if (game_top_fileinfo_version >= 36 && game_top_fileinfo_version < 40) {
				weirdshitdiscard = cfile_read_short(LoadFile) * 4;
			} else if (game_top_fileinfo_version < 36) {
				weirdshitdiscard = 1400;
			}
			if (cfseek(LoadFile, weirdshitdiscard, SEEK_CUR))
				Error("Error skipping weird shit in gamesave.c");
		}
	}

	//================ READ CONTROL CENTER TRIGGER INFO ===============

#if 0
	if (game_fileinfo.control_offset > -1)
		if (!cfseek(LoadFile, game_fileinfo.control_offset, SEEK_SET))
		{
			Assert(game_fileinfo.control_sizeof == sizeof(control_center_triggers));
#endif // 0
	if (control_offset > -1)
		Assert(cftell(LoadFile) == control_offset);
	control_center_triggers_read_n(&ControlCenterTriggers, 1, LoadFile);

	//================ READ MATERIALOGRIFIZATIONATORS INFO ===============

	if (robot_center_offset > -1)
		Assert(cftell(LoadFile) == robot_center_offset);

	for (i = 0; i < Num_robot_centers; i++) {
		if (game_top_fileinfo_version < 27) {
			old_matcen_info m;
			old_matcen_info_read(&m, LoadFile);
			RobotCenters[i].robot_flags[0] = m.robot_flags;
			RobotCenters[i].robot_flags[1] = 0;
			RobotCenters[i].hit_points = m.hit_points;
			RobotCenters[i].interval = m.interval;
			RobotCenters[i].segnum = m.segnum;
			RobotCenters[i].fuelcen_num = m.fuelcen_num;
		}
		else
			matcen_info_read(&RobotCenters[i], LoadFile);
			//	Set links in RobotCenters to Station array
			for (j = 0; j <= Highest_segment_index; j++)
			if (Segments[j].special == SEGMENT_IS_ROBOTMAKER)
				if (Segments[j].matcen_num == i)
					RobotCenters[i].fuelcen_num = Segments[j].value;
	}

	//================ READ DL_INDICES INFO ===============

	if (static_light_offset > -1)
		Assert(cftell(LoadFile) == static_light_offset);

	for (i = 0; i < Num_static_lights; i++) {
		if (game_top_fileinfo_version < 29) {
			Int3();	//shouldn't be here!!!
		} else
			dl_index_read(&Dl_indices[i], LoadFile);
	}

	//	Indicate that no light has been subtracted from any vertices.
	clear_light_subtracted();

	//================ READ DELTA LIGHT INFO ===============

	if (delta_light_offset > -1)
		Assert(cftell(LoadFile) == delta_light_offset);

	for (i = 0; i < num_delta_lights; i++) {
		if (game_top_fileinfo_version < 29) {
			;
		} else
			delta_light_read(&Delta_lights[i], LoadFile);
	}

	//========================= UPDATE VARIABLES ======================

	reset_objects(gs_num_objects);

	for (i=0; i<MAX_OBJECTS; i++) {
		Objects[i].next = Objects[i].prev = -1;
		if (Objects[i].type != OBJ_NONE) {
			int objsegnum = Objects[i].segnum;

			if (objsegnum > Highest_segment_index)		//bogus object
			{
				Warning("Object %u is in non-existent segment %i, highest=%i", i, objsegnum, Highest_segment_index);
				Objects[i].type = OBJ_NONE;
			}
			else {
				Objects[i].segnum = -1;			//avoid Assert()
				obj_link(i,objsegnum);
			}
		}
	}

	clear_transient_objects(1);		//1 means clear proximity bombs

	// Make sure non-transparent doors are set correctly.
	for (i=0; i< Num_segments; i++)
		for (j=0;j<MAX_SIDES_PER_SEGMENT;j++) {
			side	*sidep = &Segments[i].sides[j];
			if ((sidep->wall_num != -1) && (Walls[sidep->wall_num].clip_num != -1)) {
				if (WallAnims[Walls[sidep->wall_num].clip_num].flags & WCF_TMAP1) {
					sidep->tmap_num = WallAnims[Walls[sidep->wall_num].clip_num].frames[0];
					sidep->tmap_num2 = 0;
				}
			}
		}


	reset_walls();

#if 0
	Num_open_doors = game_fileinfo.doors_howmany;
#endif // 0
	Num_open_doors = 0;

	//go through all walls, killing references to invalid triggers
	for (i=0;i<Num_walls;i++)
		if (Walls[i].trigger >= Num_triggers) {
			Walls[i].trigger = -1;	//kill trigger
		}

	//go through all triggers, killing unused ones
	for (i=0;i<Num_triggers;) {
		int w;

		//	Find which wall this trigger is connected to.
		for (w=0; w<Num_walls; w++)
			if (Walls[w].trigger == i)
				break;
		i++;
	}

	for (i = 0; i < Num_object_triggers; i++) {
		int objid = ObjectTriggers[i].object_id;
		if (objid >= 0) {
			Assert(objid <= Highest_object_index);
			Objects[objid].flags |= OF_HAS_TRIGGERS;
		}
	}

	//	MK, 10/17/95: Make walls point back at the triggers that control them.
	//	Go through all triggers, set WALL_HAS_TRIGGERS.

	for (int t=0; t<Num_triggers + Num_object_triggers; t++) {
		trigger_warn_unsupported(t, false);

		int	l;
		for (l=0; l<Triggers[t].num_links; l++) {
			int	seg_num, side_num, wall_num;

			seg_num = Triggers[t].seg[l];
			side_num = Triggers[t].side[l];

			// D2X-XL trying to be funny. The -1 apparently means that this
			// manipulates a referenced _object_. These are effects only, so
			// ignore it.
			if (side_num == -1 && (Triggers[t].type == TT_ENABLE_TRIGGER ||
				                   Triggers[t].type == TT_DISABLE_TRIGGER))
			{
				int object_index = seg_num; // yep.
				Assert(object_index >= 0 && object_index < gs_num_objects);
				continue;
			}

			Assert(seg_num >= 0 && seg_num <= Highest_segment_index);
			Assert(side_num >= 0 && side_num < MAX_SIDES_PER_SEGMENT);

			wall_num = Segments[seg_num].sides[side_num].wall_num;

			Assert(wall_num >= -1 && wall_num < Num_walls);

			// -- if (Walls[wall_num].controlling_trigger != -1)
			// -- 	Int3();

			//check to see that if a trigger requires a wall that it has one,
			//and if it requires a matcen that it has one

			switch (Triggers[t].type) {
			case TT_MATCEN:
				if (Segments[seg_num].special != SEGMENT_IS_ROBOTMAKER) {
					if (!is_d2x_xl_level())
						Int3(); //matcen trigger doesn't point to matcen
				}
				break;
			case TT_SPAWN_BOT:
				if (seg_num >= 0 && Segments[seg_num].special != SEGMENT_IS_ROBOTMAKER)
					Int3();		//matcen trigger doesn't point to matcen
				break;
			case TT_MASTER:
				Assert(wall_num > -1);
				Assert(Walls[wall_num].trigger > -1);
				break;
			case TT_LIGHT_OFF:
			case TT_LIGHT_ON:
			case TT_TELEPORT:
			case TT_SET_SPAWN:
			case TT_SPEEDBOOST:
				break;
			default:
				if (wall_num == -1) {
 					printf("Error: no wall for trigger type %d\n", Triggers[t].type);
					Int3();	//	This is illegal.  This trigger requires a wall
				} else {
					Walls[wall_num].flags |= WALL_HAS_TRIGGERS;
				}
			}
		}
	}

	for (i = 0; i < Num_walls; i++) {
		int seg_num = Walls[i].segnum;
		int side_num = Walls[i].sidenum;

		Assert(seg_num >= 0 && seg_num <= Highest_segment_index);
		Assert(side_num >= 0 && side_num < MAX_SIDES_PER_SEGMENT);

		Assert(Walls[i].linked_wall >= -1 && Walls[i].linked_wall < Num_walls);
		Assert(Walls[i].trigger >= -1 && Walls[i].trigger < Num_triggers);

		Assert(Segments[seg_num].sides[side_num].wall_num == i);
	}

	//fix old wall structs
	if (game_top_fileinfo_version < 17) {
		int segnum,sidenum,wallnum;

		for (segnum=0; segnum<=Highest_segment_index; segnum++)
			for (sidenum=0;sidenum<6;sidenum++)
				if ((wallnum=Segments[segnum].sides[sidenum].wall_num) != -1) {
					Walls[wallnum].segnum = segnum;
					Walls[wallnum].sidenum = sidenum;
				}
	}

	//create_local_segment_data();

	fix_object_segs();

	Sky_box_segment = -1;
	if (Gamesave_current_version >= GAMESAVE_D2X_XL_VERSION) {
		for (int segnum = 0; segnum <= Highest_segment_index; segnum++) {
			if (Segments[segnum].special == SEGMENT_IS_SKYBOX) {
				Sky_box_segment = segnum;
				break;
			}
		}
	}

	if (game_top_fileinfo_version < GAME_VERSION
	    && !(game_top_fileinfo_version == 25 && GAME_VERSION == 26))
		return 1;		//means old version
	else
		return 0;
}


int check_segment_connections(void);

extern void	set_ambient_sound_flags(void);

// ----------------------------------------------------------------------------

#define LEVEL_FILE_VERSION      8
//1 -> 2  add palette name
//2 -> 3  add control center explosion time
//3 -> 4  add reactor strength
//4 -> 5  killed hostage text stuff
//5 -> 6  added Secret_return_segment and Secret_return_orient
//6 -> 7  added flickering lights
//7 -> 8  made version 8 to be not compatible with D2 1.0 & 1.1

extern int Slide_segs_computed;
extern int d1_pig_present;

int no_old_level_file_error=0;

//loads a level (.LVL) file from disk
//returns 0 if success, else error code
int load_level(const char * filename_passed)
{
	CFILE * LoadFile;
	char filename[PATH_MAX];
	int sig, minedata_offset, gamedata_offset;
	int mine_err, game_err;
#ifdef NETWORK
	int i;
#endif

	Slide_segs_computed = 0;

#ifdef NETWORK
   if (Game_mode & GM_NETWORK)
	 {
	  for (i=0;i<MAX_POWERUP_TYPES;i++)
		{
			MaxPowerupsAllowed[i]=0;
			PowerupsInMine[i]=0;
		}
	 }
#endif

	strcpy(filename,filename_passed);

	if (!cfexist(filename))
		sprintf(filename,"%s%s",MISSION_DIR,filename_passed);

	LoadFile = cfopen( filename, "rb" );

	if (!LoadFile)	{
			Error("Can't open file <%s>\n",filename);
	}

	strcpy( Gamesave_current_filename, filename );

	sig                      = cfile_read_int(LoadFile);
	Gamesave_current_version = cfile_read_int(LoadFile);
	minedata_offset          = cfile_read_int(LoadFile);
	gamedata_offset          = cfile_read_int(LoadFile);

	Assert(sig == MAKE_SIG('P','L','V','L'));

	if (Gamesave_current_version >= GAMESAVE_D2X_XL_VERSION) {
		printf("Warning: this is a D2X-XL level (v%d). It won't work.\n",
			   Gamesave_current_version);
	}

	if (Gamesave_current_version >= 8) {    //read dummy data
		cfile_read_int(LoadFile);
		cfile_read_short(LoadFile);
		cfile_read_byte(LoadFile);
	}

	if (Gamesave_current_version < 5)
		cfile_read_int(LoadFile);       //was hostagetext_offset

	if (Gamesave_current_version > 1)
		cfgets(Current_level_palette,sizeof(Current_level_palette),LoadFile);
	if (Gamesave_current_version <= 1 || Current_level_palette[0]==0) // descent 1 level
		strcpy(Current_level_palette, DEFAULT_LEVEL_PALETTE);

	if (Gamesave_current_version >= 3)
		Base_control_center_explosion_time = cfile_read_int(LoadFile);
	else
		Base_control_center_explosion_time = DEFAULT_CONTROL_CENTER_EXPLOSION_TIME;

	if (Gamesave_current_version >= 4)
		Reactor_strength = cfile_read_int(LoadFile);
	else
		Reactor_strength = -1;  //use old defaults

	if (Gamesave_current_version >= 7) {
		int i;

		Num_flickering_lights = cfile_read_int(LoadFile);
		Assert((Num_flickering_lights >= 0) && (Num_flickering_lights < MAX_FLICKERING_LIGHTS));
		for (i = 0; i < Num_flickering_lights; i++)
			flickering_light_read(&Flickering_lights[i], LoadFile);
	}
	else
		Num_flickering_lights = 0;

	if (Gamesave_current_version < 6) {
		Secret_return_segment = 0;
		Secret_return_orient.rvec.x = F1_0;
		Secret_return_orient.rvec.y = 0;
		Secret_return_orient.rvec.z = 0;
		Secret_return_orient.fvec.x = 0;
		Secret_return_orient.fvec.y = F1_0;
		Secret_return_orient.fvec.z = 0;
		Secret_return_orient.uvec.x = 0;
		Secret_return_orient.uvec.y = 0;
		Secret_return_orient.uvec.z = F1_0;
	} else {
		Secret_return_segment = cfile_read_int(LoadFile);
		Secret_return_orient.rvec.x = cfile_read_int(LoadFile);
		Secret_return_orient.rvec.y = cfile_read_int(LoadFile);
		Secret_return_orient.rvec.z = cfile_read_int(LoadFile);
		Secret_return_orient.fvec.x = cfile_read_int(LoadFile);
		Secret_return_orient.fvec.y = cfile_read_int(LoadFile);
		Secret_return_orient.fvec.z = cfile_read_int(LoadFile);
		Secret_return_orient.uvec.x = cfile_read_int(LoadFile);
		Secret_return_orient.uvec.y = cfile_read_int(LoadFile);
		Secret_return_orient.uvec.z = cfile_read_int(LoadFile);
	}

	cfseek(LoadFile,minedata_offset,SEEK_SET);

	mine_err = load_mine_data_compiled(LoadFile);

	if (mine_err == -1) {   //error!!
		cfclose(LoadFile);
		return 2;
	}

	cfseek(LoadFile,gamedata_offset,SEEK_SET);
	game_err = load_game_data(LoadFile);

	if (game_err == -1) {   //error!!
		cfclose(LoadFile);
		return 3;
	}

	//======================== CLOSE FILE =============================

	cfclose( LoadFile );

	set_ambient_sound_flags();

	Assert(!check_segment_connections());

	// Convoluted way to warn against unsupported D2X-XL .oof replacement models.
	// As an example, we use nonsense models for Anthology, at least in the
	// 3rd part of the level.
	for (i = 0; i < N_polygon_models; i++) {
		char name[32];
		snprintf(name, sizeof(name), "model%d.oof", i);
		CFILE *t = cfopen(name, "rb");
		if (t) {
			printf("D2X-XL: unsupported OOF replacement model %d\n", i);
			cfclose(t);
		}
	}

	return 0;
}
