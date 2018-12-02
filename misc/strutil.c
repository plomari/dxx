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
 * string manipulation utility code
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "u_mem.h"
#include "dxxerror.h"
#include "strutil.h"
#include "inferno.h"

#ifndef _WIN32

void strlwr( char *s1 )
{
	while( *s1 )	{
		*s1 = tolower(*s1);
		s1++;
	}
}

void strupr( char *s1 )
{
	while( *s1 )	{
		*s1 = toupper(*s1);
		s1++;
	}
}

void strrev( char *s1 )
{
	char *h, *t;
	h = s1;
	t = s1 + strlen(s1) - 1;
	while (h < t) {
		char c;
		c = *h;
		*h++ = *t;
		*t-- = c;
	}
}
#endif

//give a filename a new extension, won't append if strlen(dest) > 8 chars.
void change_filename_extension( char *dest, const char *src, char *ext )
{
	char *p;
	
	strcpy (dest, src);
	
	if (ext[0] == '.')
		ext++;
	
	p = strrchr(dest, '.');
	if (!p) {
		if (strlen(dest) > FILENAME_LEN - 5)
			return;	// a non-opened file is better than a bad memory access
		
		p = dest + strlen(dest);
		*p = '.';
	}
	
	strcpy(p+1,ext);
}

#if !(defined(_WIN32))
void _splitpath(char *name, char *drive, char *path, char *base, char *ext)
{
	char *s, *p;

	p = name;
	s = strchr(p, ':');
	if ( s != NULL ) {
		if (drive) {
			*s = '\0';
			strcpy(drive, p);
			*s = ':';
		}
		p = s+1;
		if (!p)
			return;
	} else if (drive)
		*drive = '\0';
	
	s = strrchr(p, '\\');
	if ( s != NULL) {
		if (path) {
			char c;
			
			c = *(s+1);
			*(s+1) = '\0';
			strcpy(path, p);
			*(s+1) = c;
		}
		p = s+1;
		if (!p)
			return;
	} else if (path)
		*path = '\0';

	s = strchr(p, '.');
	if ( s != NULL) {
		if (base) {
			*s = '\0';
			strcpy(base, p);
			*s = '.';
		}
		p = s+1;
		if (!p)
			return;
	} else if (base)
		*base = '\0';
		
	if (ext)
		strcpy(ext, p);		
}
#endif

char *tprintf_buf(char *buf, size_t buf_size, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, buf_size, format, ap);
    va_end(ap);
    return buf;
}
