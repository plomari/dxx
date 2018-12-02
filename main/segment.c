
/*
 *
 * Segment Loading Stuff
 *
 */


#include "segment.h"
#include "cfile.h"
#include "gamesave.h"

// Map back v20 or earlier D2X-XL extended segment types.
static const int old_d2x_xl_conv[][2] = {
	[7  - 7] = {0, 						S2F_AMBIENT_WATER},	// old SEGMENT_IS_WATER
	[8  - 7] = {0, 						S2F_AMBIENT_LAVA}, 	// old SEGMENT_IS_LAVA
	[9  - 7] = {SEGMENT_IS_TEAM_BLUE, 	0}, 				// old SEGMENT_IS_TEAM_BLUE
	[10 - 7] = {SEGMENT_IS_TEAM_RED, 	0}, 				// old SEGMENT_IS_TEAM_RED
	[11 - 7] = {SEGMENT_IS_SPEEDBOOST, 	0}, 				// old SEGMENT_IS_SPEEDBOOST
	[12 - 7] = {0, 						S2F_BLOCKED}, 		// old SEGMENT_IS_BLOCKED
	[13 - 7] = {0, 						S2F_NODAMAGE}, 		// old SEGMENT_IS_NODAMAGE
	[14 - 7] = {SEGMENT_IS_SKYBOX, 		S2F_BLOCKED}, 		// old SEGMENT_IS_SKYBOX
	[15 - 7] = {SEGMENT_IS_EQUIPMAKER, 	0}, 				// old SEGMENT_IS_EQUIPMAKER
	[16 - 7] = {0, 						S2F_OUTDOORS}, 		// old SEGMENT_IS_LIGHT_SELF
};

// Similar to d2x-xl source code CSegment::Upgrade().
static void fix_old_d2x_xl_segment_special(segment *s)
{
	if (s->special < 7)
		return;

	(void)WARN_IF_FALSE(Gamesave_current_version >= GAMESAVE_D2X_XL_VERSION);
	Assert(Gamesave_current_version <= 20);

	ubyte val = s->special - 7;
	s->special = 0;

	if (WARN_ON(val >= ARRAY_ELEMS(old_d2x_xl_conv)))
		return;

	s->special = old_d2x_xl_conv[val][0];
	s->s2_flags |= old_d2x_xl_conv[val][1];
}

/*
 * reads a segment2 structure from a CFILE
 */
void segment2_read(segment *s2, CFILE *fp)
{
	s2->special = cfile_read_byte(fp);
	if (Gamesave_current_version < 24) {
		s2->matcen_num = cfile_read_byte(fp);
		s2->value = cfile_read_byte(fp);
	} else {
		// D2X stuff
		int16_t v = cfile_read_short(fp);
		if (v < -128 || v > 127) {
			printf("D2X-XL: discarding matcen_num=%d\n", v);
			v = 0;
		}
		s2->matcen_num = v;
		v = cfile_read_short(fp);
		if (v < -128 || v > 127) {
			printf("D2X-XL: discarding value=%d\n", v);
			v = 0;
		}
		s2->value = v;
	}
	// (D2X-XL levels, e.g. Anthology, have extra bits with unknown purpose)
	s2->s2_flags = (ubyte)cfile_read_byte(fp) & 3u;
	if (Gamesave_current_version > 20) {
		// D2X stuff
		// We reuse s2_flags for what D2X-XL calls m_prop
		s2->s2_flags |= (ubyte)cfile_read_byte(fp);
		cfile_read_short(fp); // m_xDamage [0]
		cfile_read_short(fp); // m_xDamage [1]
	} else {
		fix_old_d2x_xl_segment_special(s2);
	}
	s2->static_light = cfile_read_fix(fp);
}

/*
 * reads a delta_light structure from a CFILE
 */
void delta_light_read(delta_light *dl, CFILE *fp)
{
	dl->segnum = cfile_read_short(fp);
	dl->sidenum = cfile_read_byte(fp);
	dl->dummy = cfile_read_byte(fp);
	dl->vert_light[0] = cfile_read_byte(fp);
	dl->vert_light[1] = cfile_read_byte(fp);
	dl->vert_light[2] = cfile_read_byte(fp);
	dl->vert_light[3] = cfile_read_byte(fp);
}


/*
 * reads a dl_index structure from a CFILE
 */
void dl_index_read(dl_index *di, CFILE *fp)
{
	di->segnum = cfile_read_short(fp);
	di->sidenum = cfile_read_byte(fp);
	di->count = cfile_read_byte(fp);
	di->index = cfile_read_short(fp);
}
