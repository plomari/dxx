/* fileio.c in /misc for d1x file reading */
#ifndef _STRIO_H
#define _STRIO_H

#include "cfile.h"

char* fgets_unlimited(PHYSFS_file *f);
char *splitword(char *s, char splitchar);

#endif
