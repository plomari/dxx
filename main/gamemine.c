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
 * Functions for loading mines in the game
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>

#include "pstypes.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "dxxerror.h"
#include "gameseg.h"
#include "switch.h"
#include "game.h"
#include "newmenu.h"
#include "cfile.h"
#include "hash.h"
#include "key.h"
#include "piggy.h"
#include "byteswap.h"
#include "gamesave.h"

#define REMOVE_EXT(s)  (*(strchr( (s), '.' ))='\0')

fix Level_shake_frequency = 0, Level_shake_duration = 0;
int Secret_return_segment = 0;
vms_matrix Secret_return_orient;
int Sky_box_segment = -1;

typedef struct v16_segment {
	side    sides[MAX_SIDES_PER_SEGMENT];       // 6 sides
	short   children[MAX_SIDES_PER_SEGMENT];    // indices of 6 children segments, front, left, top, right, bottom, back
	short   verts[MAX_VERTICES_PER_SEGMENT];    // vertex ids of 4 front and 4 back vertices
	short   objects;            // pointer to objects in this segment
	ubyte   special;            // what type of center this is
	sbyte   matcen_num;         // which center segment is associated with.
	short   value;
	fix     static_light;       // average static light in segment
	short   pad;                // make structure longword aligned
} v16_segment;

#if 0
struct mfi_v19 {
	ushort  fileinfo_signature;
	ushort  fileinfo_version;
	int     fileinfo_sizeof;
	int     header_offset;      // Stuff common to game & editor
	int     header_size;
	int     editor_offset;      // Editor specific stuff
	int     editor_size;
	int     segment_offset;
	int     segment_howmany;
	int     segment_sizeof;
	int     newseg_verts_offset;
	int     newseg_verts_howmany;
	int     newseg_verts_sizeof;
	int     group_offset;
	int     group_howmany;
	int     group_sizeof;
	int     vertex_offset;
	int     vertex_howmany;
	int     vertex_sizeof;
	int     texture_offset;
	int     texture_howmany;
	int     texture_sizeof;
	int     walls_offset;
	int     walls_howmany;
	int     walls_sizeof;
	int     triggers_offset;
	int     triggers_howmany;
	int     triggers_sizeof;
	int     links_offset;
	int     links_howmany;
	int     links_sizeof;
	int     object_offset;      // Object info
	int     object_howmany;
	int     object_sizeof;
	int     unused_offset;      // was: doors_offset
	int     unused_howmamy;     // was: doors_howmany
	int     unused_sizeof;      // was: doors_sizeof
	short   level_shake_frequency;  // Shakes every level_shake_frequency seconds
	short   level_shake_duration;   // for level_shake_duration seconds (on average, random).  In 16ths second.
	int     secret_return_segment;
	vms_matrix  secret_return_orient;

	int     dl_indices_offset;
	int     dl_indices_howmany;
	int     dl_indices_sizeof;

	int     delta_light_offset;
	int     delta_light_howmany;
	int     delta_light_sizeof;

};
#endif // 0

int CreateDefaultNewSegment();

int New_file_format_load = 1; // "new file format" is everything newer than d1 shareware

int d1_pig_present = 0; // can descent.pig from descent 1 be loaded?

/* returns nonzero if d1_tmap_num references a texture which isn't available in d2. */
int d1_tmap_num_unique(short d1_tmap_num) {
	switch (d1_tmap_num) {
	case   0: case   2: case   4: case   5: case   6: case   7: case   9:
	case  10: case  11: case  12: case  17: case  18:
	case  20: case  21: case  25: case  28:
	case  38: case  39: case  41: case  44: case  49:
	case  50: case  55: case  57: case  88:
	case 132: case 141: case 147:
	case 154: case 155: case 158: case 159:
	case 160: case 161: case 167: case 168: case 169:
	case 170: case 171: case 174: case 175: case 185:
	case 193: case 194: case 195: case 198: case 199:
	case 200: case 202: case 210: case 211:
	case 220: case 226: case 227: case 228: case 229: case 230:
	case 240: case 241: case 242: case 243: case 246:
	case 250: case 251: case 252: case 253: case 257: case 258: case 259:
	case 260: case 263: case 266: case 283: case 298:
	case 315: case 317: case 319: case 320: case 321:
	case 330: case 331: case 332: case 333: case 349:
	case 351: case 352: case 353: case 354:
	case 355: case 357: case 358: case 359:
	case 362: case 370: return 1;
	default: return 0;
	}
}

/* Converts descent 1 texture numbers to descent 2 texture numbers.
 * Textures from d1 which are unique to d1 have extra spaces around "return".
 * If we can load the original d1 pig, we make sure this function is bijective.
 * This function was updated using the file config/convtabl.ini from devil 2.2.
 */
short convert_d1_tmap_num(short d1_tmap_num) {
	switch (d1_tmap_num) {
	case 0: case 2: case 4: case 5:
		// all refer to grey rock001 (exception to bijectivity rule)
		return  d1_pig_present ? 137 : 43; // (devil:95)
	case   1: return 0;
	case   3: return 1; // rock021
	case   6:  return  270; // blue rock002
	case   7:  return  271; // yellow rock265
	case   8: return 2; // rock004
	case   9:  return  d1_pig_present ? 138 : 62; // purple (devil:179)
	case  10:  return  272; // red rock006
	case  11:  return  d1_pig_present ? 139 : 117;
	case  12:  return  d1_pig_present ? 140 : 12; //devil:43
	case  13: return 3; // rock014
	case  14: return 4; // rock019
	case  15: return 5; // rock020
	case  16: return 6;
	case  17:  return  d1_pig_present ? 141 : 52;
	case  18:  return  129;
	case  19: return 7;
	case  20:  return  d1_pig_present ? 142 : 22;
	case  21:  return  d1_pig_present ? 143 : 9;
	case  22: return 8;
	case  23: return 9;
	case  24: return 10;
	case  25:  return  d1_pig_present ? 144 : 12; //devil:35
	case  26: return 11;
	case  27: return 12;
	case  28:  return  d1_pig_present ? 145 : 11; //devil:43
	//range handled by default case, returns 13..21 (- 16)
	case  38:  return  163; //devil:27
	case  39:  return  147; //31
	case  40: return 22;
	case  41:  return  266;
	case  42: return 23;
	case  43: return 24;
	case  44:  return  136; //devil:135
	case  45: return 25;
	case  46: return 26;
	case  47: return 27;
	case  48: return 28;
	case  49:  return  d1_pig_present ? 146 : 43; //devil:60
	case  50:  return  131; //devil:138
	case  51: return 29;
	case  52: return 30;
	case  53: return 31;
	case  54: return 32;
	case  55:  return  165; //devil:193
	case  56: return 33;
	case  57:  return  132; //devil:119
	// range handled by default case, returns 34..63 (- 24)
	case  88:  return  197; //devil:15
	// range handled by default case, returns 64..106 (- 25)
	case 132:  return  167;
        // range handled by default case, returns 107..114 (- 26)
	case 141:  return  d1_pig_present ? 148 : 110; //devil:106
	case 142: return 115;
	case 143: return 116;
	case 144: return 117;
	case 145: return 118;
	case 146: return 119;
	case 147:  return  d1_pig_present ? 149 : 93;
	case 148: return 120;
	case 149: return 121;
	case 150: return 122;
	case 151: return 123;
	case 152: return 124;
	case 153: return 125; // rock263
	case 154:  return  d1_pig_present ? 150 : 27;
	case 155:  return  126; // rock269
	case 156: return 200; // metl002
	case 157: return 201; // metl003
	case 158:  return  186; //devil:227
	case 159:  return  190; //devil:246
	case 160:  return  d1_pig_present ? 151 : 206;
	case 161:  return  d1_pig_present ? 152 : 114; //devil:206
	case 162: return 202;
	case 163: return 203;
	case 164: return 204;
	case 165: return 205;
	case 166: return 206;
	case 167:  return  d1_pig_present ? 153 : 206;
	case 168:  return  d1_pig_present ? 154 : 206;
	case 169:  return  d1_pig_present ? 155 : 206;
	case 170:  return  d1_pig_present ? 156 : 227;//206;
	case 171:  return  d1_pig_present ? 157 : 206;//227;
	case 172: return 207;
	case 173: return 208;
	case 174:  return  d1_pig_present ? 158 : 202;
	case 175:  return  d1_pig_present ? 159 : 206;
	// range handled by default case, returns 209..217 (+ 33)
	case 185:  return  d1_pig_present ? 160 : 217;
	// range handled by default case, returns 218..224 (+ 32)
	case 193:  return  d1_pig_present ? 161 : 206;
	case 194:  return  d1_pig_present ? 162 : 203;//206;
	case 195:  return  d1_pig_present ? 166 : 234;
	case 196: return 225;
	case 197: return 226;
	case 198:  return  d1_pig_present ? 193 : 225;
	case 199:  return  d1_pig_present ? 168 : 206; //devil:204
	case 200:  return  d1_pig_present ? 169 : 206; //devil:204
	case 201: return 227;
	case 202:  return  d1_pig_present ? 170 : 206; //devil:227
	// range handled by default case, returns 228..234 (+ 25)
	case 210:  return  d1_pig_present ? 171 : 234; //devil:242
	case 211:  return  d1_pig_present ? 172 : 206; //devil:240
	// range handled by default case, returns 235..242 (+ 23)
	case 220:  return  d1_pig_present ? 173 : 242; //devil:240
	case 221: return 243;
	case 222: return 244;
	case 223:  return  d1_pig_present ? 174 : 313;
	case 224: return 245;
	case 225: return 246;
	case 226:  return  164;//247; matching names but not matching textures
	case 227:  return  179; //devil:181
	case 228:  return  196;//248; matching names but not matching textures
	case 229:  return  d1_pig_present ? 175 : 15; //devil:66
	case 230:  return  d1_pig_present ? 176 : 15; //devil:66
	// range handled by default case, returns 249..257 (+ 18)
	case 240:  return  d1_pig_present ? 177 : 6; //devil:132
	case 241:  return  130; //devil:131
	case 242:  return  d1_pig_present ? 178 : 78; //devil:15
	case 243:  return  d1_pig_present ? 180 : 33; //devil:38
	case 244: return 258;
	case 245: return 259;
	case 246:  return  d1_pig_present ? 181 : 321; // grate metl127
	case 247: return 260;
	case 248: return 261;
	case 249: return 262;
	case 250:  return  340; //  white doorframe metl126
	case 251:  return  412; //    red doorframe metl133
	case 252:  return  410; //   blue doorframe metl134
	case 253:  return  411; // yellow doorframe metl135
	case 254: return 263; // metl136
	case 255: return 264; // metl139
	case 256: return 265; // metl140
	case 257:  return  d1_pig_present ? 182 : 249;//246; brig001
	case 258:  return  d1_pig_present ? 183 : 251;//246; brig002
	case 259:  return  d1_pig_present ? 184 : 252;//246; brig003
	case 260:  return  d1_pig_present ? 185 : 256;//246; brig004
	case 261: return 273; // exit01
	case 262: return 274; // exit02
	case 263:  return  d1_pig_present ? 187 : 281; // ceil001
	case 264: return 275; // ceil002
	case 265: return 276; // ceil003
	case 266:  return  d1_pig_present ? 188 : 279; //devil:291
	// range handled by default case, returns 277..291 (+ 10)
	case 282: return 293;
	case 283:  return  d1_pig_present ? 189 : 295;
	case 284: return 295;
	case 285: return 296;
	case 286: return 298;
	// range handled by default case, returns 300..310 (+ 13)
	case 298:  return  d1_pig_present ? 191 : 364; // devil:374 misc010
	// range handled by default case, returns 311..326 (+ 12)
	case 315:  return  d1_pig_present ? 192 : 361; // bad producer misc044
	// range handled by default case,  returns  327..337 (+ 11)
	case 327: return 352; // arw01
	case 328: return 353; // misc17
	case 329: return 354; // fan01
	case 330:  return  380; // mntr04
	case 331:  return  379;//373; matching names but not matching textures
	case 332:  return  355;//344; matching names but not matching textures
 	case 333:  return  409; // lava misc11 //devil:404
	case 334: return 356; // ctrl04
	case 335: return 357; // ctrl01
	case 336: return 358; // ctrl02
	case 337: return 359; // ctrl03
	case 338: return 360; // misc14
	case 339: return 361; // producer misc16
	case 340: return 362; // misc049
	case 341: return 364; // misc060
	case 342: return 363; // blown01
	case 343: return 366; // misc061
	case 344: return 365;
	case 345: return 368;
	case 346: return 376;
	case 347: return 370;
	case 348: return 367;
	case 349:  return  372;
	case 350: return 369;
	case 351:  return  374;//429; matching names but not matching textures
	case 352:  return  375;//387; matching names but not matching textures
	case 353:  return  371;
	case 354:  return  377;//425; matching names but not matching textures
	case 355:  return  408;
	case 356: return 378; // lava02
	case 357:  return  383;//384; matching names but not matching textures
	case 358:  return  384;//385; matching names but not matching textures
	case 359:  return  385;//386; matching names but not matching textures
	case 360: return 386;
	case 361: return 387;
	case 362:  return  d1_pig_present ? 194 : 388; // mntr04b (devil: -1)
	case 363: return 388;
	case 364: return 391;
	case 365: return 392;
	case 366: return 393;
	case 367: return 394;
	case 368: return 395;
	case 369: return 396;
	case 370:  return  d1_pig_present ? 195 : 392; // mntr04d (devil: -1)
	// range 371..584 handled by default case (wall01 and door frames)
	default:
		// ranges:
		if (d1_tmap_num >= 29 && d1_tmap_num <= 37)
			return d1_tmap_num - 16;
		if (d1_tmap_num >= 58 && d1_tmap_num <= 87)
			return d1_tmap_num - 24;
		if (d1_tmap_num >= 89 && d1_tmap_num <= 131)
			return d1_tmap_num - 25;
		if (d1_tmap_num >= 133 && d1_tmap_num <= 140)
			return d1_tmap_num - 26;
		if (d1_tmap_num >= 176 && d1_tmap_num <= 184)
			return d1_tmap_num + 33;
		if (d1_tmap_num >= 186 && d1_tmap_num <= 192)
			return d1_tmap_num + 32;
		if (d1_tmap_num >= 203 && d1_tmap_num <= 209)
			return d1_tmap_num + 25;
		if (d1_tmap_num >= 212 && d1_tmap_num <= 219)
			return d1_tmap_num + 23;
		if (d1_tmap_num >= 231 && d1_tmap_num <= 239)
			return d1_tmap_num + 18;
		if (d1_tmap_num >= 267 && d1_tmap_num <= 281)
			return d1_tmap_num + 10;
		if (d1_tmap_num >= 287 && d1_tmap_num <= 297)
			return d1_tmap_num + 13;
		if (d1_tmap_num >= 299 && d1_tmap_num <= 314)
			return d1_tmap_num + 12;
		if (d1_tmap_num >= 316 && d1_tmap_num <= 326)
			 return  d1_tmap_num + 11; // matching names but not matching textures
		// wall01 and door frames:
		if (d1_tmap_num > 370 && d1_tmap_num < 584) {
			if (New_file_format_load) return d1_tmap_num + 64;
			// d1 shareware needs special treatment:
			if (d1_tmap_num < 410) return d1_tmap_num + 68;
			if (d1_tmap_num < 417) return d1_tmap_num + 73;
			if (d1_tmap_num < 446) return d1_tmap_num + 91;
			if (d1_tmap_num < 453) return d1_tmap_num + 104;
			if (d1_tmap_num < 462) return d1_tmap_num + 111;
			if (d1_tmap_num < 486) return d1_tmap_num + 117;
			if (d1_tmap_num < 494) return d1_tmap_num + 141;
			if (d1_tmap_num < 584) return d1_tmap_num + 147;
		}
		{ // handle rare case where orientation != 0
			short tmap_num = d1_tmap_num &  TMAP_NUM_MASK;
			short orient = d1_tmap_num & ~TMAP_NUM_MASK;
			if (orient != 0) {
				return orient | convert_d1_tmap_num(tmap_num);
			} else {
				Warning("can't convert unknown descent 1 texture #%d.\n", tmap_num);
				return d1_tmap_num;
			}
		}
	}
}

#define COMPILED_MINE_VERSION 0

void read_children(int segnum,ubyte bit_mask,CFILE *LoadFile)
{
	int bit;

	for (bit=0; bit<MAX_SIDES_PER_SEGMENT; bit++) {
		if (bit_mask & (1 << bit)) {
			Segments[segnum].children[bit] = cfile_read_short(LoadFile);
		} else
			Segments[segnum].children[bit] = -1;
	}
}

void read_verts(int segnum,CFILE *LoadFile)
{
	int i;
	// Read short Segments[segnum].verts[MAX_VERTICES_PER_SEGMENT]
	for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++)
		Segments[segnum].verts[i] = (uint16_t)cfile_read_short(LoadFile);
}

void read_special(int segnum,ubyte bit_mask,CFILE *LoadFile)
{
	if (bit_mask & (1 << MAX_SIDES_PER_SEGMENT)) {
		// Read ubyte	Segments[segnum].special
		Segments[segnum].special = cfile_read_byte(LoadFile);
		// Read byte	Segments[segnum].matcen_num
		Segments[segnum].matcen_num = cfile_read_byte(LoadFile);
		// Read short	Segments[segnum].value
		Segments[segnum].fuelcen_num = cfile_read_short(LoadFile);
	} else {
		Segments[segnum].special = 0;
		Segments[segnum].matcen_num = -1;
		Segments[segnum].fuelcen_num = 0;
	}
}

int load_mine_data_compiled(CFILE *LoadFile)
{
	int     i, segnum, sidenum;
	ubyte   compiled_version;
	short   temp_short;
	ushort  temp_ushort = 0;
	ubyte   bit_mask;

	d1_pig_present = cfexist(D1_PIGFILE);
#if 0 // the following will be deleted once reading old pigfiles works reliably
	if (d1_pig_present) {
		CFILE * d1_Piggy_fp = cfopen( D1_PIGFILE, "rb" );
		switch (cfilelength(d1_Piggy_fp)) {
		case D1_SHARE_BIG_PIGSIZE:
		case D1_SHARE_10_PIGSIZE:
		case D1_SHARE_PIGSIZE:
		case D1_10_BIG_PIGSIZE:
		case D1_10_PIGSIZE:
			//d1_pig_present = 0;
		}
		cfclose (d1_Piggy_fp);
	}
#endif

	if (!strcmp(strchr(Gamesave_current_filename, '.'), ".sdl"))
		New_file_format_load = 0; // descent 1 shareware
	else
		New_file_format_load = 1;

//	memset( Segments, 0, sizeof(segment)*MAX_SEGMENTS );

	//=============================== Reading part ==============================
	compiled_version = cfile_read_byte(LoadFile);

	if (New_file_format_load)
		Num_vertices = cfile_read_short(LoadFile);
	else
		Num_vertices = cfile_read_int(LoadFile);
	Assert( Num_vertices <= MAX_VERTICES );

	if (New_file_format_load)
		Num_segments = cfile_read_short(LoadFile);
	else
		Num_segments = cfile_read_int(LoadFile);
	Assert( Num_segments <= MAX_SEGMENTS );

	for (i = 0; i < Num_vertices; i++)
		cfile_read_vector( &(Vertices[i]), LoadFile);

	for (segnum=0; segnum<Num_segments; segnum++ )	{
		segment *seg = &Segments[segnum];

		*seg = (segment){.objects = -1};

		if (Gamesave_current_version >= GAMESAVE_D2X_XL_VERSION) {
			cfile_read_byte(LoadFile); // m_owner
			cfile_read_byte(LoadFile); // m_group
		}

		if (New_file_format_load)
			bit_mask = cfile_read_byte(LoadFile);
		else
			bit_mask = 0x7f; // read all six children and special stuff...

		if (Gamesave_current_version == 5) { // d2 SHAREWARE level
			read_special(segnum,bit_mask,LoadFile);
			read_verts(segnum,LoadFile);
			read_children(segnum,bit_mask,LoadFile);
		} else {
			read_children(segnum,bit_mask,LoadFile);
			read_verts(segnum,LoadFile);
			if (Gamesave_current_version <= 1) { // descent 1 level
				read_special(segnum,bit_mask,LoadFile);
			}
		}

		if (Gamesave_current_version <= 5) { // descent 1 thru d2 SHAREWARE level
			// Read fix	seg->static_light (shift down 5 bits, write as short)
			temp_ushort = cfile_read_short(LoadFile);
			seg->static_light	= ((fix)temp_ushort) << 4;
			//cfread( &seg->static_light, sizeof(fix), 1, LoadFile );
		}

		if (New_file_format_load)
			bit_mask = cfile_read_byte(LoadFile);
		else
			bit_mask = 0x3f; // read all six sides
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			side *sidep = &seg->sides[sidenum];

			int wallnum = -1;
			if (bit_mask & (1 << sidenum)) {
				if (Gamesave_current_version >= 13) {
					wallnum = (uint16_t)cfile_read_short(LoadFile);
					if (wallnum > SHRT_MAX)
						printf("Warning: wallnum out of bounds (%d)\n", wallnum);
					// Guessing here, the d2x-xl code doesn't make too much sense.
					if (wallnum == 2047)
						wallnum = -1;
				} else {
					wallnum = (uint8_t)cfile_read_byte(LoadFile);
					if (wallnum == 255)
						wallnum = -1;
				}
			}
			sidep->wall_num = wallnum;
		}

		bool vert_used[MAX_VERTICES_PER_SEGMENT] = {0};
		bool has_corners = Gamesave_current_version > 24;

		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++ )	{
			side *sidep = &seg->sides[sidenum];

			if (has_corners) {
				int num_corners = 0;
				for (int n = 0; n < 4; n++) {
					ubyte corner = cfile_read_byte(LoadFile);
					if (corner != 0xFF) {
						if (WARN_IF_FALSE(corner < MAX_VERTICES_PER_SEGMENT))
							vert_used[corner] = true;
						num_corners++;
					}
				}
				if (num_corners != 4) {
					printf("D2X-XL: unsupported reduced shape at segment %d side %d, "
					       "attempting ugly looking workaround\n", segnum, sidenum);
				}
			}

			if ( (seg->children[sidenum]==-1) || (sidep->wall_num!=-1) )	{
				// Read short sidep->tmap_num;
				if (New_file_format_load) {
					temp_ushort = cfile_read_short(LoadFile);
					sidep->tmap_num = temp_ushort & 0x7fff;
				} else
					sidep->tmap_num = cfile_read_short(LoadFile);

				if (Gamesave_current_version <= 1)
					sidep->tmap_num = convert_d1_tmap_num(sidep->tmap_num);

				if (New_file_format_load && !(temp_ushort & 0x8000))
					sidep->tmap_num2 = 0;
				else {
					// Read short sidep->tmap_num2;
					sidep->tmap_num2 = cfile_read_short(LoadFile);
					if (Gamesave_current_version <= 1 && sidep->tmap_num2 != 0)
						sidep->tmap_num2 = convert_d1_tmap_num(sidep->tmap_num2);
				}

				// Read uvl sidep->uvls[4] (u,v>>5, write as short, l>>1 write as short)
				for (i=0; i<4; i++ )	{
					temp_short = cfile_read_short(LoadFile);
					sidep->uvls[i].u = ((fix)temp_short) * (1 << 5);
					temp_short = cfile_read_short(LoadFile);
					sidep->uvls[i].v = ((fix)temp_short) * (1 << 5);
					temp_ushort = cfile_read_short(LoadFile);
					sidep->uvls[i].l = ((fix)temp_ushort) * (1 << 1);
					//cfread( &sidep->uvls[i].l, sizeof(fix), 1, LoadFile );
				}
			}
		}

		for (int n = 0; n < MAX_VERTICES_PER_SEGMENT; n++) {
			if (vert_used[n] || !has_corners)
				continue;
			// D2X-XL per side vertex lists (reduced shapes) lead to unused
			// segment vertexes. These are either set to dummy values (vertexes
			// not even close to the segment), or invalid values (out of range).
			// (Worth noting that these are functionally different, as plain
			// invalid values are used by D2X-XL to determine the "shape" of
			// segments.)
			// We don't want to change the rest of the game to deal with this
			// obscure corner case. Instead, try to achieve roughly the same
			// by adding the missing vertexes, resulting in degenerate sides.

			int neigh = -1;
			for (int i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
				for (int x = 0; x < 4; x++) {
					if (Side_to_verts[i][x] == n) {
						// The vertex n should have been used by side i. Find a
						// defined neighboring vertex (neigh).
						for (int d = 1; d < 4; d++) {
							int t = Side_to_verts[i][(x + d) % 4];
							if (vert_used[t]) {
								neigh = t;
								goto found;
							}
						}
					}
				}
			}

		found:

			if (neigh >= 0 && WARN_IF_FALSE(Num_vertices < MAX_VERTICES)) {
				Vertices[Num_vertices] = Vertices[seg->verts[neigh]];
				seg->verts[n] = Num_vertices;
				Num_vertices++;
			} else {
				seg->verts[n] = 0; // better than crashing
			}
		}

		for (int n = 0; n < MAX_VERTICES_PER_SEGMENT; n++)
			Assert(seg->verts[n] >= 0 && seg->verts[n] < Num_vertices);
	}

#if 0
	{
		FILE *fp;

		fp = fopen("segments.out", "wt");
		for (i = 0; i <= Highest_segment_index; i++) {
			side	sides[MAX_SIDES_PER_SEGMENT];	// 6 sides
			short	children[MAX_SIDES_PER_SEGMENT];	// indices of 6 children segments, front, left, top, right, bottom, back
			short	verts[MAX_VERTICES_PER_SEGMENT];	// vertex ids of 4 front and 4 back vertices
			int		objects;								// pointer to objects in this segment

			for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
				sbyte   type;                           // replaces num_faces and tri_edge, 1 = quad, 2 = 0:2 triangulation, 3 = 1:3 triangulation
				ubyte	pad;									//keep us longword alligned
				short	wall_num;
				short	tmap_num;
				short	tmap_num2;
				uvl		uvls[4];
				vms_vector	normals[2];						// 2 normals, if quadrilateral, both the same.
				fprintf(fp, "%d\n", Segments[i].sides[j].type);
				fprintf(fp, "%d\n", Segments[i].sides[j].pad);
				fprintf(fp, "%d\n", Segments[i].sides[j].wall_num);
				fprintf(fp, "%d\n", Segments[i].tmap_num);

			}
			fclose(fp);
		}
	}
#endif

	Highest_vertex_index = Num_vertices-1;
	Highest_segment_index = Num_segments-1;

	validate_segment_all();			// Fill in side type and normals.

	for (i=0; i<Num_segments; i++) {
		if (Gamesave_current_version > 5)
			segment2_read(&Segments[i], LoadFile);
	}

	reset_objects(1);		//one object, the player

	return 0;
}
