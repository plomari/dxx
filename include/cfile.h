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

#ifndef _CFILE_H
#define _CFILE_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "pstypes.h"
#include "maths.h"
#include "vecmat.h"
#include "physfsx.h"
#include "strutil.h"
#include "console.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "args.h"

typedef struct CFILE CFILE;

#define PHYSFS_file CFILE

CFILE *cfopen(const char *filename, const char *mode);
size_t cfread( void * buf, size_t elsize, size_t nelem, CFILE * fp );
#define PHYSFS_read(fp, buf, elsize, nelem) cfread(buf, elsize, nelem, fp)
int cfclose( CFILE * cfile );
int cfclose_inv( CFILE * cfile );
#define PHYSFS_close cfclose_inv
int64_t cftell( CFILE * fp );
#define PHYSFS_tell cftell
int64_t cfilelength( CFILE *fp );
#define PHYSFS_fileLength cfilelength
int cfile_ferror(CFILE *fp);
int cfile_feof(CFILE *fp);
#define PHYSFS_eof cfile_feof
int cfexist(const char *fname);
#define PHYSFS_exists cfexist
int cfile_hog_add(char *hogname, int add_to_end);
int cfile_hog_remove(char *hogname);
int64_t cfile_size(char *hogname);
int cfgetc(CFILE *fp);
int cfseek(CFILE *fp, int64_t offset, int where);
void cfskip(CFILE *fp, int64_t amount);
#define PHYSFS_seek(fp, offs) cfseek(fp, offs, SEEK_SET)
char * cfgets(char *buf, size_t n, CFILE *fp);
int cfile_read_int(CFILE *file);
short cfile_read_short(CFILE *file);
sbyte cfile_read_byte(CFILE *file);
fix cfile_read_fix(CFILE *file);
fixang cfile_read_fixang(CFILE *file);
void cfile_read_vector(vms_vector *v, CFILE *file);
void cfile_read_angvec(vms_angvec *v, CFILE *file);
void cfile_read_matrix(vms_matrix *m,CFILE *file);

#define CF_READ_MODE "rb"
#define CF_WRITE_MODE "wb"

#define BSWAP32(i) ((((i) & 0xFF) << 24) | ((((i) >> 8) & 0xFF) << 16) | \
				    ((((i) >> 16) & 0xFF) << 8) | (((i) >> 24) & 0xFF))

#define PHYSFS_sint64 int64_t

static inline int PHYSFS_readSBE32(CFILE *file, int *v)
{
	uint32_t i;
	if (!cfread(&i, sizeof(i), 1, file)) {
		*v = 0;
		return EOF;
	}
	*v = BSWAP32(i);
	return 0;
}

static inline int PHYSFS_readSLE32(CFILE *file, int *v)
{
	uint32_t i;
	if (!cfread(&i, sizeof(i), 1, file)) {
		*v = 0;
		return EOF;
	}
	*v = i;
	return 0;
}

#define BSWAP16(i) ((((i) & 0xFF) << 8) | (((i) >> 8) & 0xFF))

static inline int PHYSFS_readSBE16(CFILE *file, short *v)
{
	uint16_t i;
	if (!cfread(&i, sizeof(i), 1, file)) {
		*v = 0;
		return EOF;
	}
	*v = BSWAP16(i);
	return 0;
}

void cfile_init_paths(int argc, char *argv[]);

size_t cfile_write(CFILE *file, const void *ptr, size_t elsize, size_t nelem);
#define PHYSFS_write cfile_write

int cfile_mkdir(const char *path);
#define PHYSFS_mkdir cfile_mkdir

int cfile_unlink(const char *path);
#define PHYSFS_delete cfile_unlink

bool cfile_write_fixed_str(CFILE *file, size_t field_len, const char *str);
bool cfile_read_fixed_str(CFILE *file, size_t field_len, char *buf);

void cfile_gets_0(CFILE *file, char *s, size_t sz);
void cfile_gets_nl(CFILE *file, char *s, size_t sz);

static inline int PHYSFSX_writeU8(PHYSFS_file *file, uint8_t val)
{
	return cfile_write(file, &val, 1, 1);
}

static inline int PHYSFSX_writeString(PHYSFS_file *file, char *s)
{
	return cfile_write(file, s, 1, strlen(s) + 1);
}

static inline int PHYSFSX_puts(PHYSFS_file *file, char *s)
{
	return cfile_write(file, s, 1, strlen(s));
}

static inline int PHYSFSX_putc(PHYSFS_file *file, int c)
{
	unsigned char ch = (unsigned char)c;

	if (cfile_write(file, &ch, 1, 1) < 1)
		return -1;
	else
		return (int)c;
}

PRINTF_FORMAT(2, 3)
static inline int PHYSFSX_printf(PHYSFS_file *file, char *format, ...)
{
	char buffer[1024];
	va_list args;

	va_start(args, format);
	vsprintf(buffer, format, args);

	return PHYSFSX_puts(file, buffer);
}

static inline size_t cfile_write_le32(CFILE *file, uint32_t v)
{
	return cfile_write(file, &v, sizeof(v), 1);
}

static inline size_t cfile_write_le16(CFILE *file, uint16_t v)
{
	return cfile_write(file, &v, sizeof(v), 1);
}

#define PHYSFS_writeSLE32	cfile_write_le32
#define PHYSFS_writeULE32	cfile_write_le32
#define PHYSFS_writeSLE16	cfile_write_le16
#define PHYSFS_writeULE16	cfile_write_le16

static inline void PHYSFS_writeSBE32(CFILE *file, uint32_t v)
{
	v = BSWAP32(v);
	cfile_write(file, &v, sizeof(v), 1);
}

static inline void PHYSFS_writeSBE16(CFILE *file, uint16_t v)
{
	v = BSWAP16(v);
	cfile_write(file, &v, sizeof(v), 1);
}

#define PHYSFSX_writeFix	PHYSFS_writeSLE32
#define PHYSFSX_writeFixAng	PHYSFS_writeSLE16

static inline int PHYSFSX_writeVector(PHYSFS_file *file, vms_vector *v)
{
	if (PHYSFSX_writeFix(file, v->x) < 1 ||
	 PHYSFSX_writeFix(file, v->y) < 1 ||
	 PHYSFSX_writeFix(file, v->z) < 1)
		return 0;

	return 1;
}

static inline int PHYSFSX_writeAngleVec(PHYSFS_file *file, vms_angvec *v)
{
	if (PHYSFSX_writeFixAng(file, v->p) < 1 ||
	 PHYSFSX_writeFixAng(file, v->b) < 1 ||
	 PHYSFSX_writeFixAng(file, v->h) < 1)
		return 0;

	return 1;
}

static inline int PHYSFSX_writeMatrix(PHYSFS_file *file, vms_matrix *m)
{
	if (PHYSFSX_writeVector(file, &m->rvec) < 1 ||
	 PHYSFSX_writeVector(file, &m->uvec) < 1 ||
	 PHYSFSX_writeVector(file, &m->fvec) < 1)
		return 0;

	return 1;
}

int cfile_rename(char *oldpath, char *newpath);
#define PHYSFSX_rename cfile_rename

char **cfile_find_files(char *path, const char *const *exts);
void cfile_find_files_free(char **files);
#define PHYSFSX_findFiles cfile_find_files
#define PHYSFS_enumerateFiles(path) cfile_find_files(path, NULL)
#define PHYSFS_freeList cfile_find_files_free

bool cfile_is_directory(char *path);
#define PHYSFS_isDirectory cfile_is_directory

#define PHYSFSX_openReadBuffered(filename) 	cfopen(filename, "rb")
#define PHYSFSX_openWriteBuffered(filename)	cfopen(filename, "wb")
#define PHYSFS_openWrite(filename)			cfopen(filename, "wb")
#define PHYSFS_openRead(filename)			cfopen(filename, "rb")

CFILE *cfile_open_data_dir_file(const char *filename);
#define PHYSFSX_openDataFile cfile_open_data_dir_file

#define CFILE_var_w(fp, ptr) cfile_write((fp), (ptr), sizeof(*(ptr)), 1)
#define CFILE_var_r(fp, ptr)  cfread((ptr), sizeof(*(ptr)), 1, (fp))

#endif
