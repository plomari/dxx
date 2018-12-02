#ifndef _STRUTILS_H
#define _STRUTILS_H

#include <stdio.h>
#include <string.h>

#include "pstypes.h"

#define APPENDF(s, ...)  do { 									\
	static_assert(sizeof(s) == ARRAY_ELEMS(s), "need char[]");	\
	int pos_ = strlen(s);										\
	snprintf((s) + pos_, sizeof(s) - pos_, __VA_ARGS__);		\
} while(0)

#define tprintf(SIZE, format, ...) \
    tprintf_buf((char[SIZE]){0}, (SIZE), (format), __VA_ARGS__)
char *tprintf_buf(char *buf, size_t buf_size, const char *format, ...)
    PRINTF_FORMAT(3, 4);

#ifndef _WIN32

#define stricmp(a,b) strcasecmp(a,b)
#define strnicmp(a,b,c) strncasecmp(a,b,c)
void strupr( char *s1 );
void strlwr( char *s1 );

void strrev( char *s1 );
#endif

//give a filename a new extension, doesn't work with paths with no extension already there
extern void change_filename_extension( char *dest, const char *src, char *new_ext );

#if !(defined(_WIN32))
void _splitpath(char *name, char *drive, char *path, char *base, char *ext);
#endif

#endif /* _STRUTILS_H */
