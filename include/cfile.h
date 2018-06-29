/* $Id: cfile.h,v 1.1.1.1 2006/03/17 20:01:40 zicodxx Exp $ */
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
 * Wrappers for physfs abstraction layer
 *
 */

#ifndef _CFILE_H
#define _CFILE_H

#include <stdio.h>
#include <string.h>
#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#include "pstypes.h"
#include "maths.h"
#include "vecmat.h"
#include "physfsx.h"
#include "strutil.h"
#include "ignorecase.h"
#include "console.h"

#define CFILE            PHYSFS_file
#define cfopen(f,m)      PHYSFSX_openReadBuffered(f)
#define cfread(p,s,n,fp) PHYSFS_read(fp,p,s,n)
#define cfclose          PHYSFS_close
#define cftell           PHYSFS_tell
#define cfilelength      PHYSFS_fileLength

static inline int cfexist(const char *fname)
{
    char filename[2048];
    snprintf(filename, sizeof(filename), "%s", fname);
    PHYSFSEXT_locateCorrectCase(filename);
    return PHYSFS_exists(filename);
}

//Specify the name of the hogfile.  Returns 1 if hogfile found & had files
static inline int cfile_init(char *hogname, int add_to_end)
{
	char pathname[PATH_MAX];

	if (!PHYSFSX_getRealPath(hogname, pathname))
		return 0;

	if (!PHYSFS_addToSearchPath(pathname, add_to_end))
	{	// try the 'Data' directory for old Mac Descent directories compatibility
		char std_path[PATH_MAX] = "Data/";

		strncat(std_path, hogname, PATH_MAX - 1 - strlen(std_path));
		std_path[PATH_MAX - 1] = 0;

		if (!PHYSFSX_getRealPath(std_path, pathname))
			return 0;
		
		return PHYSFS_addToSearchPath(pathname, add_to_end);
	}
	
	return 1;
}

static inline int cfile_close(char *hogname)
{
	char pathname[PATH_MAX];

	if (!PHYSFSX_getRealPath(hogname, pathname))
		return 0;

	if (!PHYSFS_removeFromSearchPath(pathname))
	{
		char std_path[PATH_MAX] = "Data/";
		
		strncat(std_path, hogname, PATH_MAX - 1 - strlen(std_path));
		std_path[PATH_MAX - 1] = 0;
		
		if (!PHYSFSX_getRealPath(std_path, pathname))
			return 0;
		
		return PHYSFS_removeFromSearchPath(pathname);
	}
	
	return 1;
}


static inline int cfile_size(char *hogname)
{
	PHYSFS_file *fp;
	int size;

	fp = PHYSFS_openRead(hogname);
	if (fp == NULL)
	{
		char std_path[PATH_MAX] = "Data/";
		
		strncat(std_path, hogname, PATH_MAX - 1 - strlen(std_path));
		std_path[PATH_MAX - 1] = 0;
		
		if (!(fp = PHYSFS_openRead(std_path)))
			return -1;
	}

	size = PHYSFS_fileLength(fp);
	cfclose(fp);

	return size;
}

static inline int cfgetc(PHYSFS_file *const fp)
{
	unsigned char c;

	if (PHYSFS_read(fp, &c, 1, 1) != 1)
		return EOF;

	return c;
}

static inline int cfseek(PHYSFS_file *fp, long int offset, int where)
{
	int c, goal_position;

	switch(where)
	{
	case SEEK_SET:
		goal_position = offset;
		break;
	case SEEK_CUR:
		goal_position = PHYSFS_tell(fp) + offset;
		break;
	case SEEK_END:
		goal_position = PHYSFS_fileLength(fp) + offset;
		break;
	default:
		return 1;
	}
	c = PHYSFS_seek(fp, goal_position);
	return !c;
}

static inline char * cfgets(char *buf, size_t n, PHYSFS_file *const fp)
{
	size_t i;
	int c;

	for (i = 0; i < n - 1; i++)
	{
		do
		{
			c = cfgetc(fp);
			if (c == EOF)
			{
				*buf = 0;

				return NULL;
			}
			if (c == 0 || c == 10)  // Unix line ending
				break;
			if (c == 13)            // Mac or DOS line ending
			{
				int c1;

				c1 = cfgetc(fp);
				if (c1 != EOF)  // The file could end with a Mac line ending
					cfseek(fp, -1, SEEK_CUR);
				if (c1 == 10) // DOS line ending
					continue;
				else            // Mac line ending
					break;
			}
		} while (c == 13);
		if (c == 13)    // because cr-lf is a bad thing on the mac
			c = '\n';   // and anyway -- 0xod is CR on mac, not 0x0a
		if (c == '\n')
			break;
		*buf++ = c;
	}
	*buf = 0;

	return buf;
}


/*
 * read some data types...
 */

static inline int cfile_read_int(PHYSFS_file *file)
{
	int i;

	if (!PHYSFS_readSLE32(file, &i))
	{
		Error("Error reading int in cfile_read_int()");
		exit(1);
	}

	return i;
}

static inline short cfile_read_short(PHYSFS_file *file)
{
	int16_t s;

	if (!PHYSFS_readSLE16(file, &s))
	{
		Error("Error reading short in cfile_read_short()");
		exit(1);
	}

	return s;
}

static inline sbyte cfile_read_byte(PHYSFS_file *file)
{
	sbyte b;

	if (PHYSFS_read(file, &b, sizeof(b), 1) != 1)
	{
		Error("Error reading byte in cfile_read_byte()");
		exit(1);
	}

	return b;
}

static inline fix cfile_read_fix(PHYSFS_file *file)
{
	int f;  // a fix is defined as a long for Mac OS 9, and MPW can't convert from (long *) to (int *)

	if (!PHYSFS_readSLE32(file, &f))
	{
		Error("Error reading fix in cfile_read_fix()");
		exit(1);
	}

	return f;
}

static inline fixang cfile_read_fixang(PHYSFS_file *file)
{
	fixang f;

	if (!PHYSFS_readSLE16(file, &f))
	{
		Error("Error reading fixang in cfile_read_fixang()");
		exit(1);
	}

	return f;
}

static inline void cfile_read_vector(vms_vector *v, PHYSFS_file *file)
{
	v->x = cfile_read_fix(file);
	v->y = cfile_read_fix(file);
	v->z = cfile_read_fix(file);
}

static inline void cfile_read_angvec(vms_angvec *v, PHYSFS_file *file)
{
	v->p = cfile_read_fixang(file);
	v->b = cfile_read_fixang(file);
	v->h = cfile_read_fixang(file);
}

static inline void cfile_read_matrix(vms_matrix *m,PHYSFS_file *file)
{
	cfile_read_vector(&m->rvec,file);
	cfile_read_vector(&m->uvec,file);
	cfile_read_vector(&m->fvec,file);
}

#endif