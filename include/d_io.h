// some misc. file/disk routines
// Arne de Bruijn, 1998
#ifndef _D_IO_H
#define _D_IO_H

#include <stdio.h>
// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out);

#endif
