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

#ifndef _WIN32

#define stricmp(a,b) strcasecmp(a,b)
#define strnicmp(a,b,c) strncasecmp(a,b,c)
void strupr( char *s1 );
void strlwr( char *s1 );

void strrev( char *s1 );
#endif

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out);

//give a filename a new extension, doesn't work with paths with no extension already there
extern void change_filename_extension( char *dest, const char *src, char *new_ext );

#if !(defined(_WIN32))
void _splitpath(char *name, char *drive, char *path, char *base, char *ext);
#endif

// create a growing 2D array with a single growing buffer for the text
// this system is likely to cause less memory fragmentation than having one malloc'd buffer per string
int string_array_new(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf);

// add a string to a growing 2D array
int string_array_add(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf, const char *str);

// sort function passed to qsort - also useful for 2d string arrays with individual string pointers 
int string_array_sort_func(char **e0, char **e1);

// reallocate pointers to save memory, sort list alphabetically and remove duplicates according to 'comp'
void string_array_tidy(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf, int offset, int (*comp)( const char *, const char * ));

#endif /* _STRUTILS_H */
