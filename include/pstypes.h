/* $Id: pstypes.h,v 1.1.1.1 2006/03/17 20:01:30 zicodxx Exp $ */
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
 * Common types for use in Miner
 *
 */

#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

typedef int8_t sbyte;
typedef uint8_t ubyte;
typedef uint16_t ushort;
typedef uint32_t uint;

#ifndef min
#define min(a,b) (((a)>(b))?(b):(a))
#endif
#ifndef max
#define max(a,b) (((a)<(b))?(b):(a))
#endif

// Exclude pointer types.
#define REQUIRE_ZERO(x) sizeof(int[(x) ? -1 : 1])
#define REQUIRE_ARRAY(x) REQUIRE_ZERO(__builtin_types_compatible_p(__typeof__(x), __typeof__(&(x)[0])))

#define ARRAY_ELEMS(x) (sizeof(x) / sizeof((x)[0]) + 0 * REQUIRE_ARRAY(x))

#ifdef __GNUC__
# define __pack__ __attribute__((packed))
#else
# error d2x will not work without packed structures
#endif

#ifndef PACKAGE_STRING
# define PACKAGE_STRING "d2x"
#endif

#endif //_TYPES_H

