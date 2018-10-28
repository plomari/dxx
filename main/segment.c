
/*
 *
 * Segment Loading Stuff
 *
 */


#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include "segment.h"
#include "cfile.h"
#include "gamesave.h"

static int map_new_d2x_xl_seg_function(int val)
{
	switch (val) {
	case SEGMENT_FUNC_NONE:				return SEGMENT_IS_NOTHING;
	case SEGMENT_FUNC_FUELCENTER:		return SEGMENT_IS_FUELCEN;
	case SEGMENT_FUNC_REPAIRCENTER:		return SEGMENT_IS_REPAIRCEN;
	case SEGMENT_FUNC_REACTOR:			return SEGMENT_IS_CONTROLCEN;
	case SEGMENT_FUNC_ROBOTMAKER:		return SEGMENT_IS_ROBOTMAKER;
	case SEGMENT_FUNC_GOAL_BLUE:		return SEGMENT_IS_GOAL_BLUE;
	case SEGMENT_FUNC_GOAL_RED:			return SEGMENT_IS_GOAL_RED;
	case SEGMENT_FUNC_TEAM_BLUE:		return SEGMENT_IS_TEAM_BLUE;
	case SEGMENT_FUNC_TEAM_RED:			return SEGMENT_IS_TEAM_RED;
	case SEGMENT_FUNC_SPEEDBOOST:		return SEGMENT_IS_SPEEDBOOST;
	case SEGMENT_FUNC_SKYBOX:			return SEGMENT_IS_SKYBOX;
	case SEGMENT_FUNC_EQUIPMAKER:		return SEGMENT_IS_EQUIPMAKER;
	default:
		printf("D2X-XL: warning: unknown m_function==%d\n", val);
		return 0;
	}
}

/*
 * reads a segment2 structure from a CFILE
 */
void segment2_read(segment2 *s2, CFILE *fp)
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
	s2->s2_flags = cfile_read_byte(fp);
	if (Gamesave_current_version > 20) {
		// D2X stuff
		int props = cfile_read_byte(fp);
		cfile_read_short(fp); // m_xDamage [0]
		cfile_read_short(fp); // m_xDamage [1]
		// See d2x-xl source code CSegment::Upgrade(). Internally, D2X-XL just
		// stores m_props and m_function, which are derived from s2->special.
		// For older file formats, it maps them from s2->special. We don't have
		// separate m_props/m_function, so we do the _reverse_. We essentially
		// ignore m_props anyway, so the main thing we need to do is to map
		// back D2X-XL's "new" m_function to the old values.
		s2->special = map_new_d2x_xl_seg_function(s2->special);
		if (props & SEGMENT_PROP_NODAMAGE) {
			if (s2->special) {
				printf("D2X-XL: discarding NODAMAGE (function already used)\n");
			} else {
				s2->special = SEGMENT_IS_NODAMAGE;
			}
		}
		if (props & SEGMENT_PROP_BLOCKED) {
			if (s2->special) {
				printf("D2X-XL: discarding BLOCKED (function already used)\n");
			} else {
				s2->special = SEGMENT_IS_BLOCKED;
			}
		}
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

void segment2_write(segment2 *s2, PHYSFS_file *fp)
{
	PHYSFSX_writeU8(fp, s2->special);
	PHYSFSX_writeU8(fp, s2->matcen_num);
	PHYSFSX_writeU8(fp, s2->value);
	PHYSFSX_writeU8(fp, s2->s2_flags);
	PHYSFSX_writeFix(fp, s2->static_light);
}

void delta_light_write(delta_light *dl, PHYSFS_file *fp)
{
	PHYSFS_writeSLE16(fp, dl->segnum);
	PHYSFSX_writeU8(fp, dl->sidenum);
	PHYSFSX_writeU8(fp, dl->dummy);
	PHYSFSX_writeU8(fp, dl->vert_light[0]);
	PHYSFSX_writeU8(fp, dl->vert_light[1]);
	PHYSFSX_writeU8(fp, dl->vert_light[2]);
	PHYSFSX_writeU8(fp, dl->vert_light[3]);
}

void dl_index_write(dl_index *di, PHYSFS_file *fp)
{
	PHYSFS_writeSLE16(fp, di->segnum);
	PHYSFSX_writeU8(fp, di->sidenum);
	PHYSFSX_writeU8(fp, di->count);
	PHYSFS_writeSLE16(fp, di->index);
}
