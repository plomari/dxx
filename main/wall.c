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
 * Destroyable wall stuff
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "gr.h"
#include "wall.h"
#include "switch.h"
#include "inferno.h"
#include "segment.h"
#include "dxxerror.h"
#include "gamesave.h"
#include "gameseg.h"
#include "game.h"
#include "bm.h"
#include "vclip.h"
#include "player.h"
#include "gauges.h"
#include "text.h"
#include "fireball.h"
#include "textures.h"
#include "sounds.h"
#include "newdemo.h"
#include "multi.h"
#include "gameseq.h"
#include "laser.h"		//	For seeing if a flare is stuck in a wall.
#include "collide.h"
#include "effects.h"

//	Special door on boss level which is locked if not in multiplayer...sorry for this awful solution --MK.
#define	BOSS_LOCKED_DOOR_LEVEL	7
#define	BOSS_LOCKED_DOOR_SEG		595
#define	BOSS_LOCKED_DOOR_SIDE	5

wall Walls[MAX_WALLS];					// Master walls array
int Num_walls=0;							// Number of walls

wclip WallAnims[MAX_WALL_ANIMS];		// Wall animations
int Num_wall_anims;
//--unused-- int walls_bm_num[MAX_WALL_ANIMS];

//door Doors[MAX_DOORS];					//	Master doors array

active_door ActiveDoors[MAX_DOORS];
int Num_open_doors;						// Number of open doors

#define CLOAKING_WALL_TIME f1_0

#define MAX_CLOAKING_WALLS 10
cloaking_wall CloakingWalls[MAX_CLOAKING_WALLS];
int Num_cloaking_walls;

char	Wall_names[7][10] = {
	"NORMAL   ",
	"BLASTABLE",
	"DOOR     ",
	"ILLUSION ",
	"OPEN     ",
	"CLOSED   ",
	"EXTERNAL "
};

// Function prototypes
void kill_stuck_objects(int wallnum);

int wall_check_transparency(segment * seg, int side)
{
	int tmap_bottom = seg->sides[side].tmap_num;
	int tmap_top = seg->sides[side].tmap_num2 & 0x3FFF;

	if (tmap_top) {
		int f_top = GameBitmaps[Textures[tmap_top].index].bm_flags;

		if (f_top & BM_FLAG_SUPER_TRANSPARENT)
			return WID_RENDPAST_FLAG;
		if (!(f_top & BM_FLAG_TRANSPARENT))
			return 0;
	}

	int f_bottom = GameBitmaps[Textures[tmap_bottom].index].bm_flags;

	if (f_bottom & (BM_FLAG_TRANSPARENT | BM_FLAG_SUPER_TRANSPARENT)) {
		int flags = WID_RENDPAST_FLAG;
		if (f_bottom & BM_FLAG_LIGHTONLY)
			flags |= WID_LIGHTONLY_FLAG;
		return flags;
	}

	return 0;
}

//-----------------------------------------------------------------
// This function checks whether we can fly through the given side.
//	In other words, whether or not we have a 'doorway'
// It also informs you whether to render textures.
// Don't call directly. Use WALL_IS_DOORWAY().
int wall_is_doorway_rest(segment * seg, int side)
{
	int flags, type;

//--Covered by macro	// No child.
//--Covered by macro	if (seg->children[side] == -1)
//--Covered by macro		return WID_RENDER_FLAG;

//--Covered by macro	if (seg->children[side] == -2)
//--Covered by macro		return WID_EXTERNAL_FLAG;

//--Covered by macro // No wall present.
//--Covered by macro	if (seg->sides[side].wall_num == -1)
//--Covered by macro		return WID_NO_WALL;

	Assert(seg-Segments>=0 && seg-Segments<=Highest_segment_index);
	Assert(side>=0 && side<6);
	Assert(seg->sides[side].wall_num >= 0);

	type = Walls[seg->sides[side].wall_num].type;
	flags = Walls[seg->sides[side].wall_num].flags;

	int res = WID_RENDER_FLAG;

	if (type == WALL_OPEN)
		return WID_FLY_FLAG | WID_RENDPAST_FLAG;

	if (type == WALL_ILLUSION) {
		if (Walls[seg->sides[side].wall_num].flags & WALL_ILLUSION_OFF)
			return WID_FLY_FLAG | WID_RENDPAST_FLAG;
		res |= WID_FLY_FLAG;
	}

	if (type == WALL_BLASTABLE) {
		if (flags & WALL_BLASTED)
			res |= WID_FLY_FLAG;
	} else if (flags & WALL_DOOR_OPENED) {
		// (Are there any levels that use WALL_BLASTABLE with a WALL_DOOR_OPENED
		// flag?)
		res |= WID_FLY_FLAG;
	}

	if (type == WALL_CLOAKED) {
		return WID_RENDER_FLAG | WID_RENDPAST_FLAG | WID_CLOAKED_FLAG |
			   WID_RENDER_ALPHA_FLAG;
	}

	if (res & WID_RENDER_FLAG)
		res |= wall_check_transparency(seg, side);

	return res;
}

//set the tmap_num or tmap_num2 field for a wall/door
void wall_set_tmap_num(segment *seg,int side,segment *csegp,int cside,int anim_num,int frame_num)
{
	Assert(anim_num >= 0 && anim_num < Num_wall_anims);
	if (!(anim_num >= 0 && anim_num < Num_wall_anims))
		return;

	wclip *anim = &WallAnims[anim_num];
	int tmap = anim->frames[frame_num];

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	if (anim->flags & WCF_TMAP1)	{
		if (tmap != seg->sides[side].tmap_num || tmap != csegp->sides[cside].tmap_num)
		{
			seg->sides[side].tmap_num = csegp->sides[cside].tmap_num = tmap;
			if ( Newdemo_state == ND_STATE_RECORDING )
				newdemo_record_wall_set_tmap_num1(seg-Segments,side,csegp-Segments,cside,tmap);
		}
	} else	{
		Assert(tmap!=0 && seg->sides[side].tmap_num2!=0);
		if (tmap != seg->sides[side].tmap_num2 || tmap != csegp->sides[cside].tmap_num2)
		{
			seg->sides[side].tmap_num2 = csegp->sides[cside].tmap_num2 = tmap;
			if ( Newdemo_state == ND_STATE_RECORDING )
				newdemo_record_wall_set_tmap_num2(seg-Segments,side,csegp-Segments,cside,tmap);
		}
	}
}


// -------------------------------------------------------------------------------
//when the wall has used all its hitpoints, this will destroy it
void blast_blastable_wall(segment *seg, int side)
{
	int Connectside;
	segment *csegp;
	int a, n, cwall_num;

	Assert(seg->sides[side].wall_num != -1);

	Walls[seg->sides[side].wall_num].hps = -1;	//say it's blasted

	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = csegp->sides[Connectside].wall_num;
	kill_stuck_objects(seg->sides[side].wall_num);
	if (cwall_num > -1)
		kill_stuck_objects(cwall_num);

	a = Walls[seg->sides[side].wall_num].clip_num;
	Assert(a >= 0 && a < Num_wall_anims);

	//if this is an exploding wall, explode it
	if (WallAnims[a].flags & WCF_EXPLODES)
		explode_wall(seg-Segments,side);
	else {
		//if not exploding, set final frame, and make door passable
		n = WallAnims[a].num_frames;
		wall_set_tmap_num(seg,side,csegp,Connectside,a,n-1);
		Walls[seg->sides[side].wall_num].flags |= WALL_BLASTED;
		if (cwall_num > -1)
			Walls[cwall_num].flags |= WALL_BLASTED;
	}

}


//-----------------------------------------------------------------
// Destroys a blastable wall.
void wall_destroy(segment *seg, int side)
{
	Assert(seg->sides[side].wall_num != -1);
	Assert(seg-Segments != 0);

	if (Walls[seg->sides[side].wall_num].type == WALL_BLASTABLE)
		blast_blastable_wall( seg, side );
	else
		Error("Hey bub, you are trying to destroy an indestructable wall.");
}

//-----------------------------------------------------------------
// Deteriorate appearance of wall. (Changes bitmap (paste-ons))
void wall_damage(segment *seg, int side, fix damage)
{
	int a, i, n, cwall_num;

	if (seg->sides[side].wall_num == -1) {
		return;
	}

	if (Walls[seg->sides[side].wall_num].type != WALL_BLASTABLE)
		return;
	
	if (!(Walls[seg->sides[side].wall_num].flags & WALL_BLASTED) && Walls[seg->sides[side].wall_num].hps >= 0)
		{
		int Connectside;
		segment *csegp;

		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);
		cwall_num = csegp->sides[Connectside].wall_num;
		Walls[seg->sides[side].wall_num].hps -= damage;
		if (cwall_num > -1)
			Walls[cwall_num].hps -= damage;
			
		a = Walls[seg->sides[side].wall_num].clip_num;
		Assert(a >= 0 && a < Num_wall_anims);

		n = WallAnims[a].num_frames;
		
		if (Walls[seg->sides[side].wall_num].hps < WALL_HPS*1/n) {
			blast_blastable_wall( seg, side );			
			#ifdef NETWORK
			if (Game_mode & GM_MULTI)
				multi_send_door_open(seg-Segments, side,Walls[seg->sides[side].wall_num].flags);
			#endif
		}
		else
			for (i=0;i<n;i++)
				if (Walls[seg->sides[side].wall_num].hps < WALL_HPS*(n-i)/n) {
					wall_set_tmap_num(seg,side,csegp,Connectside,a,i);
				}
		}
}


//-----------------------------------------------------------------
// Opens a door
void wall_open_door(segment *seg, int side)
{
	wall *w;
	active_door *d;
	int Connectside, wall_num, cwall_num = -1;
	segment *csegp;

	if (WARN_ON(seg->sides[side].wall_num == -1)) 	//Opening door on illegal wall
		return;

	w = &Walls[seg->sides[side].wall_num];
	wall_num = w - Walls;
	//kill_stuck_objects(seg->sides[side].wall_num);

	if (WARN_ON(w->type != WALL_DOOR))
		return;

	if ((w->state == WALL_DOOR_OPENING) ||		//already opening
		 (w->state == WALL_DOOR_WAITING)	||		//open, waiting to close
		 (w->state == WALL_DOOR_OPEN))			//open, & staying open
		return;

	if (w->state == WALL_DOOR_CLOSING) {		//closing, so reuse door

		int i;
	
		d = NULL;

		for (i=0;i<Num_open_doors;i++) {		//find door

			d = &ActiveDoors[i];
	
			if (d->front_wallnum[0]==w-Walls || d->back_wallnum[0]==wall_num ||
				 (d->n_parts==2 && (d->front_wallnum[1]==wall_num || d->back_wallnum[1]==wall_num)))
				break;
		}

		if (i>=Num_open_doors) // likely in demo playback or multiplayer
		{
			d = &ActiveDoors[Num_open_doors];
			d->time = 0;
			Num_open_doors++;
			Assert( Num_open_doors < MAX_DOORS );
		}
		else
		{
			Assert( d!=NULL ); // Get John!

			d->time = WallAnims[w->clip_num].play_time - d->time;

			if (d->time < 0)
				d->time = 0;
		}
	
	}
	else {											//create new door
		Assert(w->state == WALL_DOOR_CLOSED);
		d = &ActiveDoors[Num_open_doors];
		d->time = 0;
		Num_open_doors++;
		Assert( Num_open_doors < MAX_DOORS );
	}


	w->state = WALL_DOOR_OPENING;

	// So that door can't be shot while opening
	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	if (Connectside >= 0)
	{
		cwall_num = csegp->sides[Connectside].wall_num;
		if (cwall_num > -1)
		{
			Walls[cwall_num].state = WALL_DOOR_OPENING;
			d->back_wallnum[0] = cwall_num;
		}
		d->front_wallnum[0] = seg->sides[side].wall_num;
	}
	else
		con_printf(CON_URGENT, "Illegal Connectside %i in wall_open_door. Trying to hop over. Please check your level!\n", side);

	Assert( seg-Segments != -1);

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_record_door_opening(seg-Segments, side);
	}

	if (w->linked_wall != -1) {
		wall *w2;
		segment *seg2;

		w2		= &Walls[w->linked_wall];
		seg2	= &Segments[w2->segnum];

		Assert(w2->linked_wall == seg->sides[side].wall_num);
		//Assert(!(w2->flags & WALL_DOOR_OPENING  ||  w2->flags & WALL_DOOR_OPENED));

		w2->state = WALL_DOOR_OPENING;

		csegp = &Segments[seg2->children[w2->sidenum]];
		Connectside = find_connect_side(seg2, csegp);
		Assert(Connectside != -1);
		if (cwall_num > -1)
			Walls[cwall_num].state = WALL_DOOR_OPENING;

		d->n_parts = 2;
		d->front_wallnum[1] = w->linked_wall;
		d->back_wallnum[1] = cwall_num;
	}
	else
		d->n_parts = 1;


	if ( Newdemo_state != ND_STATE_PLAYBACK )
	{
		// NOTE THE LINK TO ABOVE!!!!
		vms_vector cp;
		compute_center_point_on_side(&cp, seg, side );
		if (WallAnims[w->clip_num].open_sound > -1 )
			digi_link_sound_to_pos( WallAnims[w->clip_num].open_sound, seg-Segments, side, &cp, 0, F1_0 );

	}
}

//-----------------------------------------------------------------
// start the transition from closed -> open wall
void start_wall_cloak(segment *seg, int side)
{
	wall *w;
	cloaking_wall *d;
	int Connectside;
	segment *csegp;
	int i, cwall_num;

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	int child = seg->children[side];
	if (WARN_ON(child < 0))
		return;

	csegp = &Segments[child];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = csegp->sides[Connectside].wall_num;

	if (WARN_ON(seg->sides[side].wall_num < 0)) {
		// Workaround for some broken levels.
		if (cwall_num < 0)
			return; // no workaround available
		cwall_num = -1;
		segment *t = csegp;
		csegp = seg;
		seg = t;
		int t2 = side;
		side = Connectside;
		Connectside = t2;
	}

	w = &Walls[seg->sides[side].wall_num];

	if (w->type == WALL_OPEN || w->state == WALL_DOOR_CLOAKING)		//already open or cloaking
		return;

	if (w->state == WALL_DOOR_DECLOAKING) {	//decloaking, so reuse door

		int i;

		d = NULL;

		for (i=0;i<Num_cloaking_walls;i++) {		//find door

			d = &CloakingWalls[i];
	
			if (d->front_wallnum==w-Walls || d->back_wallnum==w-Walls )
				break;
		}

		Assert(i<Num_cloaking_walls);				//didn't find door!
		Assert( d!=NULL ); // Get John!

		d->time = CLOAKING_WALL_TIME - d->time;

	}
	else if (w->state == WALL_DOOR_CLOSED) {	//create new door
		d = &CloakingWalls[Num_cloaking_walls];
		d->time = 0;
		if (Num_cloaking_walls >= MAX_CLOAKING_WALLS) {		//no more!
			Int3();		//ran out of cloaking wall slots
			w->type = WALL_OPEN;
			if (cwall_num > -1)
				Walls[cwall_num].type = WALL_OPEN;
			return;
		}
		Num_cloaking_walls++;
	}
	else {
		Int3();		//unexpected wall state
		return;
	}

	w->state = WALL_DOOR_CLOAKING;
	if (cwall_num > -1)
		Walls[cwall_num].state = WALL_DOOR_CLOAKING;

	d->front_wallnum = seg->sides[side].wall_num;
	d->back_wallnum = cwall_num;

	assert( seg-Segments != -1);

	Assert(w->linked_wall == -1);

	if ( Newdemo_state != ND_STATE_PLAYBACK ) {
		vms_vector cp;
		compute_center_point_on_side(&cp, seg, side );
		digi_link_sound_to_pos( SOUND_WALL_CLOAK_ON, seg-Segments, side, &cp, 0, F1_0 );
	}

	for (i=0;i<4;i++) {
		d->front_ls[i] = seg->sides[side].uvls[i].l;
		if (cwall_num > -1)
			d->back_ls[i] = csegp->sides[Connectside].uvls[i].l;
	}
}

//-----------------------------------------------------------------
// start the transition from open -> closed wall
void start_wall_decloak(segment *seg, int side)
{
	wall *w;
	cloaking_wall *d;
	int Connectside;
	segment *csegp;
	int i, cwall_num;

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	if (WARN_ON(seg->sides[side].wall_num < 0))
		return; // workaround not possible

	w = &Walls[seg->sides[side].wall_num];

	if (w->type == WALL_CLOSED || w->state == WALL_DOOR_DECLOAKING)		//already closed or decloaking
		return;

	if (w->state == WALL_DOOR_CLOAKING) {	//cloaking, so reuse door

		int i;

		d = NULL;

		for (i=0;i<Num_cloaking_walls;i++) {		//find door

			d = &CloakingWalls[i];
	
			if (d->front_wallnum==w-Walls || d->back_wallnum==w-Walls )
				break;
		}

		Assert(i<Num_cloaking_walls);				//didn't find door!
		Assert( d!=NULL ); // Get John!

		d->time = CLOAKING_WALL_TIME - d->time;

	}
	else if (w->state == WALL_DOOR_CLOSED) {	//create new door
		d = &CloakingWalls[Num_cloaking_walls];
		d->time = 0;
		if (Num_cloaking_walls >= MAX_CLOAKING_WALLS) {		//no more!
			Int3();		//ran out of cloaking wall slots
			/* what is this _doing_ here?
			w->type = WALL_CLOSED;
			Walls[csegp->sides[Connectside].wall_num].type = WALL_CLOSED;
			*/
			return;
		}
		Num_cloaking_walls++;
	}
	else {
		Int3();		//unexpected wall state
		return;
	}

	w->state = WALL_DOOR_DECLOAKING;

	// So that door can't be shot while opening
	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = csegp->sides[Connectside].wall_num;
	if (cwall_num > -1)
		Walls[cwall_num].state = WALL_DOOR_DECLOAKING;

	d->front_wallnum = seg->sides[side].wall_num;
	d->back_wallnum = csegp->sides[Connectside].wall_num;

	Assert( seg-Segments != -1);

	Assert(w->linked_wall == -1);

	if ( Newdemo_state != ND_STATE_PLAYBACK ) {
		vms_vector cp;
		compute_center_point_on_side(&cp, seg, side );
		digi_link_sound_to_pos( SOUND_WALL_CLOAK_OFF, seg-Segments, side, &cp, 0, F1_0 );
	}

	for (i=0;i<4;i++) {
		d->front_ls[i] = seg->sides[side].uvls[i].l;
		if (cwall_num > -1)
			d->back_ls[i] = csegp->sides[Connectside].uvls[i].l;
	}
}

//-----------------------------------------------------------------
// This function closes the specified door and restores the closed
//  door texture.  This is called when the animation is done
void wall_close_door_num(int door_num)
{
	int p;
	active_door *d;
	int i, cwall_num;

	d = &ActiveDoors[door_num];

	for (p=0;p<d->n_parts;p++) {
		wall *w;
		int Connectside, side;
		segment *csegp, *seg;
	
		w = &Walls[d->front_wallnum[p]];

		seg = &Segments[w->segnum];
		side = w->sidenum;
	
		Assert(seg->sides[side].wall_num != -1);		//Closing door on illegal wall
		
		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);
		cwall_num = csegp->sides[Connectside].wall_num;
		Walls[seg->sides[side].wall_num].state = WALL_DOOR_CLOSED;
		if (cwall_num > -1)
			Walls[cwall_num].state = WALL_DOOR_CLOSED;
	
		wall_set_tmap_num(seg,side,csegp,Connectside,w->clip_num,0);

	}
	
	for (i=door_num;i<Num_open_doors;i++)
		ActiveDoors[i] = ActiveDoors[i+1];

	Num_open_doors--;

}

int check_poke(int objnum,int segnum,int side)
{
	object *obj = &Objects[objnum];

	//note: don't let objects with zero size block door

	if (obj->size && get_seg_masks(&obj->pos, segnum, obj->size, __FILE__, __LINE__).sidemask & (1 << side))
		return 1;		//pokes through side!
	else
		return 0;		//does not!

}

//returns true of door in unobjstructed (& thus can close)
int is_door_free(segment *seg,int side)
{
	int Connectside;
	segment *csegp;
	int objnum;
	
	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);

	//go through each object in each of two segments, and see if
	//it pokes into the connecting seg

	for (objnum=seg->objects;objnum!=-1;objnum=Objects[objnum].next)
		if (Objects[objnum].type!=OBJ_WEAPON && Objects[objnum].type!=OBJ_FIREBALL && check_poke(objnum,seg-Segments,side))
			return 0;	//not free

	for (objnum=csegp->objects;objnum!=-1;objnum=Objects[objnum].next)
		if (Objects[objnum].type!=OBJ_WEAPON && Objects[objnum].type!=OBJ_FIREBALL && check_poke(objnum,csegp-Segments,Connectside))
			return 0;	//not free

	return 1; 	//doorway is free!
}



//-----------------------------------------------------------------
// Closes a door
void wall_close_door(segment *seg, int side)
{
	wall *w;
	active_door *d;
	int Connectside, wall_num, cwall_num;
	segment *csegp;

	if (WARN_ON(seg->sides[side].wall_num < 0))
		return;

	w = &Walls[seg->sides[side].wall_num];
	wall_num = w - Walls;
	if ((w->state == WALL_DOOR_CLOSING) ||		//already closing
		 (w->state == WALL_DOOR_WAITING)	||		//open, waiting to close
		 (w->state == WALL_DOOR_CLOSED))			//closed
		return;

	if (!is_door_free(seg,side))
		return;

	if (w->state == WALL_DOOR_OPENING) {	//reuse door

		int i;
	
		d = NULL;

		for (i=0;i<Num_open_doors;i++) {		//find door

			d = &ActiveDoors[i];
	
			if (d->front_wallnum[0]==wall_num || d->back_wallnum[0]==wall_num ||
				 (d->n_parts==2 && (d->front_wallnum[1]==wall_num || d->back_wallnum[1]==wall_num)))
				break;
		}

		Assert(i<Num_open_doors);				//didn't find door!
		Assert( d!=NULL ); // Get John!

		d->time = WallAnims[w->clip_num].play_time - d->time;

		if (d->time < 0)
			d->time = 0;
	
	}
	else {											//create new door
		Assert(w->state == WALL_DOOR_OPEN);
		d = &ActiveDoors[Num_open_doors];
		d->time = 0;
		Num_open_doors++;
		Assert( Num_open_doors < MAX_DOORS );
	}

	w->state = WALL_DOOR_CLOSING;

	// So that door can't be shot while opening
	csegp = &Segments[seg->children[side]];
	Connectside = find_connect_side(seg, csegp);
	Assert(Connectside != -1);
	cwall_num = csegp->sides[Connectside].wall_num;
	if (cwall_num > -1)
		Walls[cwall_num].state = WALL_DOOR_CLOSING;

	d->front_wallnum[0] = seg->sides[side].wall_num;
	d->back_wallnum[0] = cwall_num;

	Assert( seg-Segments != -1);

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_record_door_opening(seg-Segments, side);
	}

	if (w->linked_wall != -1) {
		Int3();		//don't think we ever used linked walls
	}
	else
		d->n_parts = 1;


	if ( Newdemo_state != ND_STATE_PLAYBACK )
	{
		// NOTE THE LINK TO ABOVE!!!!
		vms_vector cp;
		compute_center_point_on_side(&cp, seg, side );
		if (WallAnims[w->clip_num].open_sound > -1 )
			digi_link_sound_to_pos( WallAnims[w->clip_num].open_sound, seg-Segments, side, &cp, 0, F1_0 );

	}
}

//-----------------------------------------------------------------
// Animates opening of a door.
// Called in the game loop.
void do_door_open(int door_num)
{
	int p;
	active_door *d;

// 	Assert(door_num != -1);		//Trying to do_door_open on illegal door
	if (door_num == -1)
		return;
	
	d = &ActiveDoors[door_num];

	for (p=0;p<d->n_parts;p++) {
		wall *w;
		int Connectside, side;
		segment *csegp, *seg;
		fix time_elapsed, time_total, one_frame;
		int i, n;
	
		w = &Walls[d->front_wallnum[p]];
		kill_stuck_objects(d->front_wallnum[p]);
		kill_stuck_objects(d->back_wallnum[p]);

		seg = &Segments[w->segnum];
		side = w->sidenum;
	
// 		Assert(seg->sides[side].wall_num != -1);		//Trying to do_door_open on illegal wall
		if (seg->sides[side].wall_num == -1)
		{
			con_printf(CON_URGENT, "Trying to do_door_open on illegal wall %i. Please check your level!\n",side);
			continue;
		}
	
		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);

		d->time += FrameTime;

		// caused by opening a WALL_CLOSED as door???
		Assert(w->clip_num >= 0 && w->clip_num < Num_wall_anims);
		if (w->clip_num < 0) {
			printf("blleelelrgh\n");
			continue;
		}
	
		time_elapsed = d->time;
		n = WallAnims[w->clip_num].num_frames;
		time_total = WallAnims[w->clip_num].play_time;

		one_frame = time_total/n;

		i = time_elapsed/one_frame;
	
		if (i < n)
			wall_set_tmap_num(seg,side,csegp,Connectside,w->clip_num,i);
	
		if (i> n/2) {
			Walls[seg->sides[side].wall_num].flags |= WALL_DOOR_OPENED;
			Walls[csegp->sides[Connectside].wall_num].flags |= WALL_DOOR_OPENED;
		}
	
		if (i >= n-1) {
			wall_set_tmap_num(seg,side,csegp,Connectside,w->clip_num,n-1);

			// If our door is not automatic just remove it from the list.
			if (!(Walls[seg->sides[side].wall_num].flags & WALL_DOOR_AUTO)) {
				for (i=door_num;i<Num_open_doors;i++)
					ActiveDoors[i] = ActiveDoors[i+1];
				Num_open_doors--;
				Walls[seg->sides[side].wall_num].state = WALL_DOOR_OPEN;
				Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_OPEN;
			}
			else {

				Walls[seg->sides[side].wall_num].state = WALL_DOOR_WAITING;
				Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_WAITING;

				ActiveDoors[Num_open_doors].time = 0;	//counts up
			}
		}

	}

}

//-----------------------------------------------------------------
// Animates and processes the closing of a door.
// Called from the game loop.
void do_door_close(int door_num)
{
	int p;
	active_door *d;
	wall *w;

	Assert(door_num != -1);		//Trying to do_door_open on illegal door
	
	d = &ActiveDoors[door_num];

	w = &Walls[d->front_wallnum[0]];

	//check for objects in doorway before closing
	if (w->flags & WALL_DOOR_AUTO)
		if (!is_door_free(&Segments[w->segnum],w->sidenum)) {
			digi_kill_sound_linked_to_segment(w->segnum,w->sidenum,-1);
			wall_open_door(&Segments[w->segnum],w->sidenum);		//re-open door
			return;
		}

	for (p=0;p<d->n_parts;p++) {
		wall *w;
		int Connectside, side;
		segment *csegp, *seg;
		fix time_elapsed, time_total, one_frame;
		int i, n;
	
		w = &Walls[d->front_wallnum[p]];

		seg = &Segments[w->segnum];
		side = w->sidenum;
	
		if (seg->sides[side].wall_num == -1) {
			return;
		}
	
		//if here, must be auto door
//		Assert(Walls[seg->sides[side].wall_num].flags & WALL_DOOR_AUTO);		
//don't assert here, because now we have triggers to close non-auto doors
	
		// Otherwise, close it.
		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);
		Assert(Connectside != -1);
	

		if ( Newdemo_state != ND_STATE_PLAYBACK )
			// NOTE THE LINK TO ABOVE!!
			if (p==0)	//only play one sound for linked doors
				if ( d->time==0 )	{		//first time
					vms_vector cp;
					compute_center_point_on_side(&cp, seg, side );
					if (WallAnims[w->clip_num].close_sound  > -1 )
						digi_link_sound_to_pos( WallAnims[Walls[seg->sides[side].wall_num].clip_num].close_sound, seg-Segments, side, &cp, 0, F1_0 );
				}
	
		d->time += FrameTime;

		Assert(w->clip_num >= 0 && w->clip_num < Num_wall_anims);
		if (w->clip_num < 0) {
			printf("blleelelrgh\n");
			continue;
		}

		time_elapsed = d->time;
		n = WallAnims[w->clip_num].num_frames;
		time_total = WallAnims[w->clip_num].play_time;
	
		one_frame = time_total/n;	
	
		i = n-time_elapsed/one_frame-1;
	
		if (i < n/2) {
			Walls[seg->sides[side].wall_num].flags &= ~WALL_DOOR_OPENED;
			Walls[csegp->sides[Connectside].wall_num].flags &= ~WALL_DOOR_OPENED;
		}
	
		// Animate door.
		if (i > 0) {
			wall_set_tmap_num(seg,side,csegp,Connectside,w->clip_num,i);

			Walls[seg->sides[side].wall_num].state = WALL_DOOR_CLOSING;
			Walls[csegp->sides[Connectside].wall_num].state = WALL_DOOR_CLOSING;

			ActiveDoors[Num_open_doors].time = 0;		//counts up

		} else
			wall_close_door_num(door_num);
	}
}


//-----------------------------------------------------------------
// Turns off an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void wall_illusion_off(segment *seg, int side)
{
	segment *csegp;
	int cside;

	csegp = &Segments[seg->children[side]];
	cside = find_connect_side(seg, csegp);
	Assert(cside != -1);

	int wall = seg->sides[side].wall_num;
	int cwall = csegp->sides[cside].wall_num;

	if (wall > -1) {
		Walls[wall].flags |= WALL_ILLUSION_OFF;
		kill_stuck_objects(wall);
	}

	if (cwall > -1) {
		Walls[cwall].flags |= WALL_ILLUSION_OFF;
		kill_stuck_objects(cwall);
	}
}

//-----------------------------------------------------------------
// Turns on an illusionary wall (This will be used primarily for
//  wall switches or triggers that can turn on/off illusionary walls.)
void wall_illusion_on(segment *seg, int side)
{
	segment *csegp;
	int cside;

	csegp = &Segments[seg->children[side]];
	cside = find_connect_side(seg, csegp);
	Assert(cside != -1);

	int wall = seg->sides[side].wall_num;
	int cwall = csegp->sides[cside].wall_num;

	if (wall > -1)
		Walls[wall].flags &= ~WALL_ILLUSION_OFF;
	if (cwall > -1)
		Walls[cwall].flags &= ~WALL_ILLUSION_OFF;
}

//	-----------------------------------------------------------------------------
//	Allowed to open the normally locked special boss door if in multiplayer mode.
int special_boss_opening_allowed(int segnum, int sidenum)
{
	if (Game_mode & GM_MULTI)
		return (Current_level_num == BOSS_LOCKED_DOOR_LEVEL) && (segnum == BOSS_LOCKED_DOOR_SEG) && (sidenum == BOSS_LOCKED_DOOR_SIDE);
	else
		return 0;
}

//-----------------------------------------------------------------
// Determines what happens when a wall is shot
//returns info about wall.  see wall.h for codes
//obj is the object that hit...either a weapon or the player himself
//playernum is the number the player who hit the wall or fired the weapon,
//or -1 if a robot fired the weapon
int wall_hit_process(segment *seg, int side, fix damage, int playernum, object *obj )
{
	wall	*w;
	fix	show_message;

	Assert (seg-Segments != -1);

	// If it is not a "wall" then just return.
	if ( seg->sides[side].wall_num < 0 )
		return WHP_NOT_SPECIAL;

	w = &Walls[seg->sides[side].wall_num];

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_wall_hit_process( seg-Segments, side, damage, playernum );

	if (w->type == WALL_BLASTABLE) {
		if (obj->ctype.laser_info.parent_type == OBJ_PLAYER)
			wall_damage(seg, side, damage);
		return WHP_BLASTABLE;
	}

	if (playernum != Player_num)	//return if was robot fire
		return WHP_NOT_SPECIAL;

	Assert( playernum > -1 );
	
	//	Determine whether player is moving forward.  If not, don't say negative
	//	messages because he probably didn't intentionally hit the door.
	if (obj->type == OBJ_PLAYER)
		show_message = (vm_vec_dot(&obj->orient.fvec, &obj->mtype.phys_info.velocity) > 0);
	else if (obj->type == OBJ_ROBOT)
		show_message = 0;
	else if ((obj->type == OBJ_WEAPON) && (obj->ctype.laser_info.parent_type == OBJ_ROBOT))
		show_message = 0;
	else
		show_message = 1;

	if (w->keys == KEY_BLUE)
		if (!(Players[playernum].flags & PLAYER_FLAGS_BLUE_KEY)) {
			if ( playernum==Player_num )
				if (show_message)
					HUD_init_message(HM_DEFAULT, "%s %s",TXT_BLUE,TXT_ACCESS_DENIED);
			return WHP_NO_KEY;
		}

	if (w->keys == KEY_RED)
		if (!(Players[playernum].flags & PLAYER_FLAGS_RED_KEY)) {
			if ( playernum==Player_num )
				if (show_message)
					HUD_init_message(HM_DEFAULT, "%s %s",TXT_RED,TXT_ACCESS_DENIED);
			return WHP_NO_KEY;
		}
	
	if (w->keys == KEY_GOLD)
		if (!(Players[playernum].flags & PLAYER_FLAGS_GOLD_KEY)) {
			if ( playernum==Player_num )
				if (show_message)
					HUD_init_message(HM_DEFAULT, "%s %s",TXT_YELLOW,TXT_ACCESS_DENIED);
			return WHP_NO_KEY;
		}

	if (w->type == WALL_DOOR)
	{
		if ((w->flags & WALL_DOOR_LOCKED ) && !(special_boss_opening_allowed(seg-Segments, side)) ) {
			if ( playernum==Player_num )
				if (show_message)
					HUD_init_message(HM_DEFAULT, TXT_CANT_OPEN_DOOR);
			return WHP_NO_KEY;
		}
		else {
			if (w->state != WALL_DOOR_OPENING)
			{
				wall_open_door(seg, side);
			#ifdef NETWORK
				if (Game_mode & GM_MULTI)
					multi_send_door_open(seg-Segments, side,w->flags);
			#endif
			}
			return WHP_DOOR;
			
		}
	}

	return WHP_NOT_SPECIAL;		//default is treat like normal wall
}

//-----------------------------------------------------------------
// Opens doors/destroys wall/shuts off triggers.
void wall_toggle(int segnum, int side)
{
	int wall_num; 

	if (segnum < 0 || segnum > Highest_segment_index || side < 0 || side >= MAX_SIDES_PER_SEGMENT)
	{
		printf("Warning: Can't toggle side %d (%i) of\nsegment %d (%i)!\n", side, MAX_SIDES_PER_SEGMENT, segnum, Highest_segment_index);
		return;
	}

	wall_num = Segments[segnum].sides[side].wall_num;

	if (wall_num == -1) {
		return;
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_wall_toggle(segnum, side );

	if (Walls[wall_num].type == WALL_BLASTABLE)
		wall_destroy(&Segments[segnum], side);

	if ((Walls[wall_num].type == WALL_DOOR) && (Walls[wall_num].state == WALL_DOOR_CLOSED))
		wall_open_door(&Segments[segnum], side);
}


//-----------------------------------------------------------------
// Tidy up Walls array for load/save purposes.
void reset_walls()
{
	int i;

	if (Num_walls < 0) {
		return;
	}

	for (i=Num_walls;i<MAX_WALLS;i++) {
		Walls[i].type = WALL_NORMAL;
		Walls[i].flags = 0;
		Walls[i].hps = 0;
		Walls[i].trigger = -1;
		Walls[i].clip_num = -1;
		}
}

void do_cloaking_wall_frame(int cloaking_wall_num)
{
	cloaking_wall *d;
	wall *wfront,*wback;
	sbyte old_cloak; // for demo recording

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	d = &CloakingWalls[cloaking_wall_num];
	wfront = &Walls[d->front_wallnum];
	wback = (d->back_wallnum > -1) ? Walls + d->back_wallnum : NULL;

	old_cloak = wfront->cloak_value;

	d->time += FrameTime;

	if (d->time > CLOAKING_WALL_TIME) {
		int i;

		wfront->type = WALL_OPEN;
		wfront->state = WALL_DOOR_CLOSED;		//why closed? why not?
		if (wback) {
			wback->type = WALL_OPEN;
			wback->state = WALL_DOOR_CLOSED;		//why closed? why not?
		}

		for (i=cloaking_wall_num;i<Num_cloaking_walls;i++)
			CloakingWalls[i] = CloakingWalls[i+1];
		Num_cloaking_walls--;

	}
	else if (d->time > CLOAKING_WALL_TIME/2) {
		int old_type=wfront->type;

		wfront->cloak_value = ((d->time - CLOAKING_WALL_TIME/2) * (GR_FADE_LEVELS-2)) / (CLOAKING_WALL_TIME/2);
		if (wback)
			wback->cloak_value = wfront->cloak_value;

		if (old_type != WALL_CLOAKED) {		//just switched
			int i;

			wfront->type = WALL_CLOAKED;
			if (wback)
				wback->type = WALL_CLOAKED;

			for (i=0;i<4;i++) {
				Segments[wfront->segnum].sides[wfront->sidenum].uvls[i].l = d->front_ls[i];
				if (wback)
					Segments[wback->segnum].sides[wback->sidenum].uvls[i].l = d->back_ls[i];
			}
		}
	}
	else {		//fading out
		fix light_scale;
		int i;

		light_scale = fixdiv(CLOAKING_WALL_TIME/2-d->time,CLOAKING_WALL_TIME/2);

		for (i=0;i<4;i++) {
			Segments[wfront->segnum].sides[wfront->sidenum].uvls[i].l = fixmul(d->front_ls[i],light_scale);
			if (wback)
				Segments[wback->segnum].sides[wback->sidenum].uvls[i].l = fixmul(d->back_ls[i],light_scale);
		}
	}

	// check if the actual cloak_value changed in this frame to prevent redundant recordings and wasted bytes
	if ( Newdemo_state == ND_STATE_RECORDING && (wfront->cloak_value != old_cloak || d->time == FrameTime) )
		newdemo_record_cloaking_wall(d->front_wallnum, d->back_wallnum, wfront->type, wfront->state, wfront->cloak_value, Segments[wfront->segnum].sides[wfront->sidenum].uvls[0].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[1].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[2].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[3].l);

}

void do_decloaking_wall_frame(int cloaking_wall_num)
{
	cloaking_wall *d;
	wall *wfront,*wback;
	sbyte old_cloak; // for demo recording

	if ( Newdemo_state==ND_STATE_PLAYBACK ) return;

	d = &CloakingWalls[cloaking_wall_num];
	wfront = &Walls[d->front_wallnum];
	wback = (d->back_wallnum > -1) ? Walls + d->back_wallnum : NULL;

	old_cloak = wfront->cloak_value;

	d->time += FrameTime;

	if (d->time > CLOAKING_WALL_TIME) {
		int i;

		wfront->state = WALL_DOOR_CLOSED;
		if (wback)
			wback->state = WALL_DOOR_CLOSED;

		for (i=0;i<4;i++) {
			Segments[wfront->segnum].sides[wfront->sidenum].uvls[i].l = d->front_ls[i];
			if (wback)
				Segments[wback->segnum].sides[wback->sidenum].uvls[i].l = d->back_ls[i];
		}

		for (i=cloaking_wall_num;i<Num_cloaking_walls;i++)
			CloakingWalls[i] = CloakingWalls[i+1];
		Num_cloaking_walls--;

	}
	else if (d->time > CLOAKING_WALL_TIME/2) {		//fading in
		fix light_scale;
		int i;

		wfront->type = WALL_CLOSED;
		if (wback)
			wback->type = wfront->type;

		light_scale = fixdiv(d->time-CLOAKING_WALL_TIME/2,CLOAKING_WALL_TIME/2);

		for (i=0;i<4;i++) {
			Segments[wfront->segnum].sides[wfront->sidenum].uvls[i].l = fixmul(d->front_ls[i],light_scale);
			if (wback)
			Segments[wback->segnum].sides[wback->sidenum].uvls[i].l = fixmul(d->back_ls[i],light_scale);
		}
	}
	else {		//cloaking in
		wfront->cloak_value = ((CLOAKING_WALL_TIME/2 - d->time) * (GR_FADE_LEVELS-2)) / (CLOAKING_WALL_TIME/2);
		wfront->type = WALL_CLOAKED;
		if (wback) {
			wback->cloak_value = wfront->cloak_value;
			wback->type = WALL_CLOAKED;
		}
	}

	// check if the actual cloak_value changed in this frame to prevent redundant recordings and wasted bytes
	if ( Newdemo_state == ND_STATE_RECORDING && (wfront->cloak_value != old_cloak || d->time == FrameTime) )
		newdemo_record_cloaking_wall(d->front_wallnum, d->back_wallnum, wfront->type, wfront->state, wfront->cloak_value, Segments[wfront->segnum].sides[wfront->sidenum].uvls[0].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[1].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[2].l, Segments[wfront->segnum].sides[wfront->sidenum].uvls[3].l);

}

void wall_frame_process()
{
	int i;

	for (i=0;i<Num_open_doors;i++) {
		active_door *d;
		wall *w;

		d = &ActiveDoors[i];
		w = &Walls[d->front_wallnum[0]];

		if (w->state == WALL_DOOR_OPENING)
			do_door_open(i);
		else if (w->state == WALL_DOOR_CLOSING)
			do_door_close(i);
		else if (w->state == WALL_DOOR_WAITING) {
			d->time += FrameTime;

			// set flags to fix occasional netgame problem where door is waiting to close but open flag isn't set
			w->flags |= WALL_DOOR_OPENED;
			if (d->back_wallnum[0] > -1)
				Walls[d->back_wallnum[0]].flags |= WALL_DOOR_OPENED;

			if (d->time > DOOR_WAIT_TIME && is_door_free(&Segments[w->segnum],w->sidenum)) {
				w->state = WALL_DOOR_CLOSING;
				d->time = 0;
			}
		}
		else if (w->state == WALL_DOOR_CLOSED || w->state == WALL_DOOR_OPEN) {
			//this shouldn't happen.  if the wall is in one of these states,
			//there shouldn't be an activedoor entry for it.  So we'll kill
			//the activedoor entry.  Tres simple.
			int t;
			Int3();		//a bad thing has happened, but I'll try to fix it up
			for (t=i;t<Num_open_doors;t++)
				ActiveDoors[t] = ActiveDoors[t+1];
			Num_open_doors--;
		}
	}

	for (i=0;i<Num_cloaking_walls;i++) {
		cloaking_wall *d;
		wall *w;

		d = &CloakingWalls[i];
		w = &Walls[d->front_wallnum];

		if (w->state == WALL_DOOR_CLOAKING)
			do_cloaking_wall_frame(i);
		else if (w->state == WALL_DOOR_DECLOAKING)
			do_decloaking_wall_frame(i);
#ifdef _DEBUG
		else
			Int3();	//unexpected wall state
#endif
	}
}

int	Num_stuck_objects=0;

stuckobj	Stuck_objects[MAX_STUCK_OBJECTS];

//	An object got stuck in a door (like a flare).
//	Add global entry.
void add_stuck_object(object *objp, int segnum, int sidenum)
{
	int	i;
	int	wallnum;

	wallnum = Segments[segnum].sides[sidenum].wall_num;

	if (wallnum != -1) {
		if (Walls[wallnum].flags & WALL_BLASTED)
			objp->flags |= OF_SHOULD_BE_DEAD;

		for (i=0; i<MAX_STUCK_OBJECTS; i++) {
			if (Stuck_objects[i].wallnum == -1) {
				Stuck_objects[i].wallnum = wallnum;
				Stuck_objects[i].objnum = objp-Objects;
				Stuck_objects[i].signature = objp->signature;
				Num_stuck_objects++;
				break;
			}
		}
	}



}

//	--------------------------------------------------------------------------------------------------
//	Look at the list of stuck objects, clean up in case an object has gone away, but not been removed here.
//	Removes up to one/frame.
void remove_obsolete_stuck_objects(void)
{
	int	objnum;

	//	Safety and efficiency code.  If no stuck objects, should never get inside the IF, but this is faster.
	if (!Num_stuck_objects)
		return;

	objnum = d_tick_count % MAX_STUCK_OBJECTS;

	if (Stuck_objects[objnum].wallnum != -1)
		if ((Walls[Stuck_objects[objnum].wallnum].state != WALL_DOOR_CLOSED) || (Objects[Stuck_objects[objnum].objnum].signature != Stuck_objects[objnum].signature)) {
			Num_stuck_objects--;
			Objects[Stuck_objects[objnum].objnum].lifeleft = F1_0/8;
			Stuck_objects[objnum].wallnum = -1;
		}

}

extern void flush_fcd_cache(void);

//	----------------------------------------------------------------------------------------------------
//	Door with wall index wallnum is opening, kill all objects stuck in it.
void kill_stuck_objects(int wallnum)
{
	int	i;

	if (wallnum < 0 || Num_stuck_objects == 0) {
		return;
	}

	Num_stuck_objects=0;

	for (i=0; i<MAX_STUCK_OBJECTS; i++)
		if (Stuck_objects[i].wallnum == wallnum) {
			if (Objects[Stuck_objects[i].objnum].type == OBJ_WEAPON) {
				Objects[Stuck_objects[i].objnum].lifeleft = F1_0/8;
			}
			Stuck_objects[i].wallnum = -1;
		} else if (Stuck_objects[i].wallnum != -1) {
			Num_stuck_objects++;
		}
	//	Ok, this is awful, but we need to do things whenever a door opens/closes/disappears, etc.
	flush_fcd_cache();

}


// -- unused -- // -----------------------------------------------------------------------------------
// -- unused -- //	Return object id of first flare found embedded in segp:sidenum.
// -- unused -- //	If no flare, return -1.
// -- unused -- int contains_flare(segment *segp, int sidenum)
// -- unused -- {
// -- unused -- 	int	i;
// -- unused --
// -- unused -- 	for (i=0; i<Num_stuck_objects; i++) {
// -- unused -- 		object	*objp = &Objects[Stuck_objects[i].objnum];
// -- unused --
// -- unused -- 		if ((objp->type == OBJ_WEAPON) && (objp->id == FLARE_ID)) {
// -- unused -- 			if (Walls[Stuck_objects[i].wallnum].segnum == segp-Segments)
// -- unused -- 				if (Walls[Stuck_objects[i].wallnum].sidenum == sidenum)
// -- unused -- 					return objp-Objects;
// -- unused -- 		}
// -- unused -- 	}
// -- unused --
// -- unused -- 	return -1;
// -- unused -- }

// -----------------------------------------------------------------------------------
// Initialize stuck objects array.  Called at start of level
void init_stuck_objects(void)
{
	int	i;

	for (i=0; i<MAX_STUCK_OBJECTS; i++)
		Stuck_objects[i].wallnum = -1;

	Num_stuck_objects = 0;
}

// -----------------------------------------------------------------------------------
// Clear out all stuck objects.  Called for a new ship
void clear_stuck_objects(void)
{
	int	i;

	for (i=0; i<MAX_STUCK_OBJECTS; i++) {
		if (Stuck_objects[i].wallnum != -1) {
			int	objnum;

			objnum = Stuck_objects[i].objnum;

			if ((Objects[objnum].type == OBJ_WEAPON) && (Objects[objnum].id == FLARE_ID))
				Objects[objnum].lifeleft = F1_0/8;

			Stuck_objects[i].wallnum = -1;

			Num_stuck_objects--;
		}
	}

	Assert(Num_stuck_objects == 0);

}

// -----------------------------------------------------------------------------------
#define	MAX_BLAST_GLASS_DEPTH	5

static void bng_process_segment(object *objp, fix damage, segment *segp, int depth, struct segment_bit_array *visited)
{
	int	i, sidenum;

	if (depth > MAX_BLAST_GLASS_DEPTH)
		return;

	depth++;

	for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
		int			tm;
		fix			dist;
		vms_vector	pnt;

		//	Process only walls which have glass.
		if ((tm=segp->sides[sidenum].tmap_num2) != 0) {
			int	ec, db;

			tm &= 0x3fff;			//tm without flags

			if ((((ec=TmapInfo[tm].eclip_num)!=-1) && ((db=Effects[ec].dest_bm_num)!=-1 && !(Effects[ec].flags&EF_ONE_SHOT))) ||	(ec==-1 && (TmapInfo[tm].destroyed!=-1))) {
				compute_center_point_on_side(&pnt, segp, sidenum);
				dist = vm_vec_dist_quick(&pnt, &objp->pos);
				if (dist < damage/2) {
					dist = find_connected_distance(&pnt, segp-Segments, &objp->pos, objp->segnum, MAX_BLAST_GLASS_DEPTH, WID_RENDPAST_FLAG);
					if ((dist > 0) && (dist < damage/2))
					{
						Assert(objp->type == OBJ_WEAPON);
						// Blow up the wall, but only if it's not a trigger.
						int wall_num = segp->sides[sidenum].wall_num;
						if (!(wall_num >= 0 && Walls[wall_num].trigger >= 0))
							check_effect_blowup(segp, sidenum, &pnt, objp, 1);
					}
				}
			}
		}
	}

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
		int	segnum = segp->children[i];

		if (segnum != -1) {
			if (!SEGMENT_BIT_ARRAY_GET(visited, segnum)) {
				if (WALL_IS_DOORWAY(segp, i) & WID_FLY_FLAG) {
					SEGMENT_BIT_ARRAY_SET(visited, segnum);
					bng_process_segment(objp, damage, &Segments[segnum], depth, visited);
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------------
//	objp is going to detonate
//	blast nearby monitors, lights, maybe other things
void blast_nearby_glass(object *objp, fix damage)
{
	segment	*cursegp;

	cursegp = &Segments[objp->segnum];
	struct segment_bit_array visited = {{0}};

	SEGMENT_BIT_ARRAY_SET(&visited, objp->segnum);
	bng_process_segment(objp, damage, cursegp, 0, &visited);


}

/*
 * reads a wclip structure from a CFILE
 */
int wclip_read_n(wclip *wc, int n, CFILE *fp)
{
	int i, j;

	for (i = 0; i < n; i++) {
		wc[i].play_time = cfile_read_fix(fp);
		wc[i].num_frames = cfile_read_short(fp);
		for (j = 0; j < MAX_CLIP_FRAMES; j++)
			wc[i].frames[j] = cfile_read_short(fp);
		wc[i].open_sound = cfile_read_short(fp);
		wc[i].close_sound = cfile_read_short(fp);
		wc[i].flags = cfile_read_short(fp);
		cfread(wc[i].filename, 13, 1, fp);
		wc[i].pad = cfile_read_byte(fp);
	}
	return i;
}

/*
 * reads a v16_wall structure from a CFILE
 */
extern void v16_wall_read(v16_wall *w, CFILE *fp)
{
	w->type = cfile_read_byte(fp);
	w->flags = cfile_read_byte(fp);
	w->hps = cfile_read_fix(fp);
	w->trigger = cfile_read_byte(fp);
	w->clip_num = cfile_read_byte(fp);
	w->keys = cfile_read_byte(fp);
}

/*
 * reads a v19_wall structure from a CFILE
 */
extern void v19_wall_read(v19_wall *w, CFILE *fp)
{
	w->segnum = cfile_read_int(fp);
	w->sidenum = cfile_read_int(fp);
	w->type = cfile_read_byte(fp);
	w->flags = cfile_read_byte(fp);
	w->hps = cfile_read_fix(fp);
	w->trigger = cfile_read_byte(fp);
	w->clip_num = cfile_read_byte(fp);
	w->keys = cfile_read_byte(fp);
	w->linked_wall = cfile_read_int(fp);
}

/*
 * reads a wall structure from a CFILE
 */
extern void wall_read(wall *w, CFILE *fp, int version)
{
	w->segnum = cfile_read_int(fp);
	w->sidenum = cfile_read_int(fp);
	w->hps = cfile_read_fix(fp);
	w->linked_wall = cfile_read_int(fp);
	w->type = cfile_read_byte(fp);
	if (version < 37) {
		w->flags = cfile_read_byte(fp);
	} else {
		w->flags = cfile_read_short(fp);
	}
	w->state = cfile_read_byte(fp);
	w->trigger = (uint8_t)cfile_read_byte(fp);
	if (w->trigger == (uint8_t)-1)
		w->trigger = -1;
	w->clip_num = cfile_read_byte(fp);
	w->keys = cfile_read_byte(fp);
	cfile_read_byte(fp); // skip controlling_trigger
	w->cloak_value = cfile_read_byte(fp);
}

/*
 * reads a v19_door structure from a CFILE
 */
extern void v19_door_read(v19_door *d, CFILE *fp)
{
	d->n_parts = cfile_read_int(fp);
	d->seg[0] = cfile_read_short(fp);
	d->seg[1] = cfile_read_short(fp);
	d->side[0] = cfile_read_short(fp);
	d->side[1] = cfile_read_short(fp);
	d->type[0] = cfile_read_short(fp);
	d->type[1] = cfile_read_short(fp);
	d->open = cfile_read_fix(fp);
}

/*
 * reads an active_door structure from a CFILE
 */
extern void active_door_read(active_door *ad, CFILE *fp)
{
	ad->n_parts = cfile_read_int(fp);
	ad->front_wallnum[0] = cfile_read_short(fp);
	ad->front_wallnum[1] = cfile_read_short(fp);
	ad->back_wallnum[0] = cfile_read_short(fp);
	ad->back_wallnum[1] = cfile_read_short(fp);
	ad->time = cfile_read_fix(fp);
}

void wall_write(wall *w, short version, CFILE *fp)
{
	if (version >= 17)
	{
		PHYSFS_writeSLE32(fp, w->segnum);
		PHYSFS_writeSLE32(fp, w->sidenum);
	}

	if (version >= 20)
	{
		PHYSFSX_writeFix(fp, w->hps);
		PHYSFS_writeSLE32(fp, w->linked_wall);
	}
	
	PHYSFSX_writeU8(fp, w->type);
	PHYSFSX_writeU8(fp, w->flags);
	
	if (version < 20)
		PHYSFSX_writeFix(fp, w->hps);
	else
		PHYSFSX_writeU8(fp, w->state);
	
	PHYSFSX_writeU8(fp, w->trigger);
	PHYSFSX_writeU8(fp, w->clip_num);
	PHYSFSX_writeU8(fp, w->keys);
	
	if (version >= 20)
	{
		PHYSFSX_writeU8(fp, -1);
		PHYSFSX_writeU8(fp, w->cloak_value);
	}
	else if (version >= 17)
		PHYSFS_writeSLE32(fp, w->linked_wall);
}
