/*
 * $Source: /cvsroot/dxx-rebirth/d2x-rebirth/arch/include/event.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:54:32 $
 *
 * Event header file
 *
 * $Log: event.h,v $
 * Revision 1.1.1.1  2006/03/17 19:54:32  zicodxx
 * initial import
 *
 * Revision 1.1  2001/01/28 16:10:57  bradleyb
 * unified input headers.
 *
 *
 */

#ifndef _EVENT_H
#define _EVENT_H
#include "maths.h"

#include <stdint.h>

typedef enum event_type
{
	EVENT_IDLE = 0,
	EVENT_QUIT,

	EVENT_JOYSTICK_BUTTON_DOWN,
	EVENT_JOYSTICK_BUTTON_UP,
	EVENT_JOYSTICK_MOVED,

	EVENT_MOUSE_BUTTON_DOWN,
	EVENT_MOUSE_BUTTON_UP,
	EVENT_MOUSE_DOUBLE_CLICKED,
	EVENT_MOUSE_MOVED,

	EVENT_KEY_COMMAND,
	EVENT_KEY_RELEASE,

	EVENT_WINDOW_CREATED,
	EVENT_WINDOW_ACTIVATED,
	EVENT_WINDOW_DEACTIVATED,
	EVENT_WINDOW_DRAW,
	EVENT_WINDOW_CLOSE,

	EVENT_NEWMENU_DRAW,					// draw after the newmenu stuff is drawn (e.g. savegame previews)
	EVENT_NEWMENU_CHANGED,				// an item had its value/text changed
	EVENT_NEWMENU_SELECTED,				// user chose something - pressed enter/clicked on it
	
	EVENT_UI_DIALOG_DRAW,				// draw after the dialog stuff is drawn (e.g. spinning robots)
	EVENT_UI_GADGET_PRESSED,				// user 'pressed' a gadget
	EVENT_UI_LISTBOX_MOVED,
	EVENT_UI_LISTBOX_SELECTED,
	EVENT_UI_USERBOX_DRAGGED
} event_type;

// A vanilla event. Cast to the correct type of event according to 'type'.
typedef struct d_event
{
	event_type type;
} d_event;

// Set and call the default event handler
void set_default_handler(int (*handler)(d_event *event));
int call_default_handler(d_event *event);

// Send an event to the front window as first priority, then to the windows behind if it's not modal (editor), then the default handler
void event_send(d_event *event);

#endif
