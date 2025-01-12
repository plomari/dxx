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
 * Include file for functions which need to access segment data structure.
 *
 */

#ifndef _SEGMENT_H
#define _SEGMENT_H

#include "pstypes.h"
#include "fix.h"
#include "vecmat.h"
//#include "3d.h"
//#include "inferno.h"
#include "cfile.h"
#include "bitarray.h"

// Version 1 - Initial version
// Version 2 - Mike changed some shorts to bytes in segments, so incompatible!

#define SIDE_IS_QUAD    1   // render side as quadrilateral
#define SIDE_IS_TRI_02  2   // render side as two triangles, triangulated along edge from 0 to 2
#define SIDE_IS_TRI_13  3   // render side as two triangles, triangulated along edge from 1 to 3

// Set maximum values for segment and face data structures.
#define MAX_VERTICES_PER_SEGMENT    8
#define MAX_SIDES_PER_SEGMENT       6
#define MAX_VERTICES_PER_POLY       4
#define WLEFT                       0
#define WTOP                        1
#define WRIGHT                      2
#define WBOTTOM                     3
#define WBACK                       4
#define WFRONT                      5

#define MAX_SEGMENTS           20000
#define MAX_SEGMENT_VERTICES   (20000*3)

//normal everyday vertices

#define DEFAULT_LIGHTING        0   // (F1_0/2)

#define MAX_VERTICES			(MAX_SEGMENT_VERTICES)

// Returns true if segnum references a child, else returns false.
// Note that -1 means no connection, -2 means a connection to the outside world.
#define IS_CHILD(segnum) ((segnum) > -1)

//Structure for storing u,v,light values.
//NOTE: this structure should be the same as the one in 3d.h
typedef struct uvl {
	fix u, v, l;
} uvl;

typedef struct side {
	sbyte   type;           // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
	short   wall_num;
	short   tmap_num;
	short   tmap_num2;
	uvl     uvls[4];
	vms_vector normals[2];  // 2 normals, if quadrilateral, both the same.
} side;

typedef struct segment {
	side    sides[MAX_SIDES_PER_SEGMENT];       // 6 sides
	short   children[MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
	int     verts[MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
	int     objects;    // pointer to objects in this segment
	ubyte   special;	// one of SEGMENT_IS_* values (D2X-XL: m_function)
	sbyte   matcen_num;
	sbyte   fuelcen_num;
	ubyte   s2_flags;	// S2F_* flags (D2X-XL: m_prop)
	fix     static_light;
} segment;

// Note: overlaps with SEGMENT_PROP_*, must not conflict
#define S2F_AMBIENT_WATER   0x01
#define S2F_AMBIENT_LAVA    0x02
#define S2F_BLOCKED			4
#define S2F_NODAMAGE		8
#define S2F_OUTDOORS		16
#define S2F_LIGHT_FOG		32
#define S2F_DENSE_FOG		64

//values for special field
#define SEGMENT_IS_NOTHING      0
#define SEGMENT_IS_FUELCEN      1
#define SEGMENT_IS_REPAIRCEN    2
#define SEGMENT_IS_CONTROLCEN   3
#define SEGMENT_IS_ROBOTMAKER   4
#define SEGMENT_IS_GOAL_BLUE    5
#define SEGMENT_IS_GOAL_RED     6
// D2X-XL (the d2x-xl source code calls them SEGMENT_FUNC_*)
// D2X-XL rl2 level versions 20 and lower: the source code uses SEGMENT_IS_*
//    values, which are _not_ the same as here.
// D2X-XL rl2 level versions 21 and higher: essentially renamed to
//    SEGMENT_FUNC_*, and these values must match with the SEGMENT_FUNC_* ones.
#define SEGMENT_IS_TEAM_BLUE	7
#define SEGMENT_IS_TEAM_RED		8
#define SEGMENT_IS_SPEEDBOOST	9
#define SEGMENT_IS_SKYBOX		10
#define SEGMENT_IS_EQUIPMAKER	11

// Globals from mglobal.c
extern vms_vector   Vertices[];
extern segment      Segments[];
extern int          Num_segments;
extern int          Num_vertices;

extern const sbyte Side_to_verts[MAX_SIDES_PER_SEGMENT][4];       // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const int  Side_to_verts_int[MAX_SIDES_PER_SEGMENT][4];    // Side_to_verts[my_side] is list of vertices forming side my_side.
extern const char Side_opposite[];                                // Side_opposite[my_side] returns side opposite cube from my_side.

#define SEG_PTR_2_NUM(segptr) (Assert((unsigned) (segptr-Segments)<MAX_SEGMENTS),(segptr)-Segments)

// New stuff, 10/14/95: For shooting out lights and monitors.
// Light cast upon vert_light vertices in segnum:sidenum by some light
typedef struct {
	short   segnum;
	sbyte   sidenum;
	sbyte   dummy;
	ubyte   vert_light[4];
} delta_light;

// Light at segnum:sidenum casts light on count sides beginning at index (in array Delta_lights)
typedef struct {
	short   segnum;
	sbyte   sidenum;
	sbyte   count;
	short   index;
} dl_index;

// that's a lot of lights... using (crazy) d2x-xl limits
#define MAX_DL_INDICES      (MAX_SEGMENTS / 2)
#define MAX_DELTA_LIGHTS    (MAX_SEGMENTS * 10)

#define DL_SCALE            2048    // Divide light to allow 3 bits integer, 5 bits fraction.

extern dl_index     Dl_indices[MAX_DL_INDICES];
extern delta_light  Delta_lights[MAX_DELTA_LIGHTS];
extern int          Num_static_lights;

extern int subtract_light(int segnum, int sidenum);
extern int add_light(int segnum, int sidenum);
extern void restore_all_lights_in_mine(void);
extern void clear_light_subtracted(void);

extern ubyte Light_subtracted[MAX_SEGMENTS];

// ----------------------------------------------------------------------------
// --------------------- Segment interrogation functions ----------------------
// Do NOT read the segment data structure directly.  Use these
// functions instead.  The segment data structure is GUARANTEED to
// change MANY TIMES.  If you read the segment data structure
// directly, your code will break, I PROMISE IT!

// Return a pointer to the list of vertex indices for the current
// segment in vp and the number of vertices in *nv.
extern void med_get_vertex_list(segment *s,int *nv,int **vp);

// Return a pointer to the list of vertex indices for face facenum in
// vp and the number of vertices in *nv.
extern void med_get_face_vertex_list(segment *s,int side, int facenum,int *nv,int **vp);

// Set *nf = number of faces in segment s.
extern void med_get_num_faces(segment *s,int *nf);

void med_validate_segment_side(segment *sp,int side);

// Delete segment function added for curves.c
extern int med_delete_segment(segment *sp);

// Delete segment from group
extern void delete_segment_from_group(int segment_num, int group_num);

// Add segment to group
extern void add_segment_to_group(int segment_num, int group_num);

// Verify that all vertices are legal.
extern void med_check_all_vertices();

/*
 * reads a segment2 structure from a CFILE
 */
void segment2_read(segment *s2, CFILE *fp);

/*
 * reads a delta_light structure from a CFILE
 */
void delta_light_read(delta_light *dl, CFILE *fp);

/*
 * reads a dl_index structure from a CFILE
 */
void dl_index_read(dl_index *di, CFILE *fp);

struct segment_bit_array {
	uint8_t bits[BIT_ARRAY_SIZE(MAX_SEGMENTS)];
};

#define SEGMENT_BIT_ARRAY_GET(arr, n) bit_array_get((arr)->bits, n)
#define SEGMENT_BIT_ARRAY_SET(arr, n) bit_array_set((arr)->bits, n, true)

#endif
