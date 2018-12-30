/* $Id: inferno.h,v 1.1.1.1 2006/03/17 19:57:28 zicodxx Exp $ */
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
 * Header file for Inferno.  Should be included in all source files.
 *
 */

#ifndef _INFERNO_H
#define _INFERNO_H

#include <setjmp.h>

#include "pstypes.h"


// MACRO for single line #ifdef WINDOWS #else DOS
#ifdef WINDOWS
#define WINDOS(x,y) x
#define WIN(x) x
#else
#define WINDOS(x,y) y
#define WIN(x)
#endif

#if defined(__APPLE__) || defined(macintosh)
#define KEY_MAC(x) x
#else
// do not use MAC, it will break MSVC compilation somewhere in rpcdce.h
#define KEY_MAC(x)
#endif


/**
 **	Constants
 **/

// How close two points must be in all dimensions to be considered the
// same point.
#define	FIX_EPSILON	10

// the maximum length of a filename
#define FILENAME_LEN 13

//for Function_mode variable
#define FMODE_EXIT		0		// leaving the program
#define FMODE_MENU		1		// Using the menu
#define FMODE_GAME		2		// running the game
#define FMODE_EDITOR	3		// running the editor

/**
 **	Global variables
 **/

extern jmp_buf LeaveEvents;
extern int Quitting;
extern int Function_mode;		// in game or editor?
extern int Screen_mode;			// editor screen or game screen?

// The version number of the game
extern ubyte Version_major, Version_minor;

#endif


