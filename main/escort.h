/*
 *
 * Header for escort.c
 *
 */

#ifndef _ESCORT_H
#define _ESCORT_H

#include "object.h"

#define GUIDEBOT_NAME_LEN 9
extern int Buddy_dude_cheat;
extern void change_guidebot_name(void);
extern void do_escort_menu(void);
extern void detect_escort_goal_accomplished(int index);
extern void set_escort_special_goal(int key);
extern void recreate_thief(object *objp);

#endif // _ESCORT_H
