#include "maths.h"

static unsigned int d_rand_seed;

int d_rand()
{
	return ((d_rand_seed = d_rand_seed * 0x41c64e6d + 0x3039) >> 16) & 0x7fff;
}

void d_srand(unsigned int seed)
{
	d_rand_seed = seed;
}
