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

typedef enum event_type
{
	EVENT_IDLE = 0,
	EVENT_MOUSE_BUTTON_DOWN,
	EVENT_MOUSE_BUTTON_UP,
	EVENT_KEY_COMMAND,
	EVENT_WINDOW_ACTIVATED,
	EVENT_WINDOW_DEACTIVATED,
	EVENT_WINDOW_DRAW,
	EVENT_WINDOW_CLOSE,
	EVENT_WINDOW_CLOSED,
	EVENT_USER	// spare for use by modules that use windows (e.g. newmenu)
} event_type;

// A vanilla event. Cast to the correct type of event according to 'type'.
typedef struct d_event
{
	event_type type;
} d_event;

int event_init();

// Sends input events to event handlers
void event_poll();

// Sends input, idle and draw events to event handlers
void event_process();

#endif
