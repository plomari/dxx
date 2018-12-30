/* $Id: event.c,v 1.1.1.1 2006/03/17 19:53:40 zicodxx Exp $ */
/*
 *
 * SDL Event related stuff
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "event.h"
#include "key.h"
#include "gr.h"
#include "window.h"
#include "timer.h"
#include "config.h"

#include <SDL.h>

extern void key_handler(SDL_KeyboardEvent *event);
extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
extern void joy_button_handler(SDL_JoyButtonEvent *jbe);
extern void joy_hat_handler(SDL_JoyHatEvent *jhe);
extern void joy_axis_handler(SDL_JoyAxisEvent *jae);
extern void mouse_cursor_autohide();
extern void mouse_toggle_cursor(int activate);

static int initialised=0;

void event_poll()
{
	SDL_Event event;
	int clean_uniframe=1;
	window *wind = window_get_front();
	int idle = 1;
	
	// If the front window changes, exit this loop, otherwise unintended behavior can occur
	// like pressing 'Return' really fast at 'Difficulty Level' causing multiple games to be started
	while ((wind == window_get_front()) && SDL_PollEvent(&event))
	{
		idle = 0;
		switch(event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (clean_uniframe)
					memset(unicode_frame_buffer,'\0',sizeof(unsigned char)*KEY_BUFFER_SIZE);
				clean_uniframe=0;
				key_handler((SDL_KeyboardEvent *)&event);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				mouse_button_handler((SDL_MouseButtonEvent *)&event);
				break;
			case SDL_MOUSEMOTION:
				mouse_motion_handler((SDL_MouseMotionEvent *)&event);
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				joy_button_handler((SDL_JoyButtonEvent *)&event);
				break;
			case SDL_JOYAXISMOTION:
				joy_axis_handler((SDL_JoyAxisEvent *)&event);
				break;
			case SDL_JOYHATMOTION:
				joy_hat_handler((SDL_JoyHatEvent *)&event);
				break;
			case SDL_JOYBALLMOTION:
				break;
			case SDL_QUIT: {
				d_event event = { EVENT_QUIT };
				call_default_handler(&event);
			} break;
                        case SDL_WINDOWEVENT: {
                            switch (event.window.event) {
                            case SDL_WINDOWEVENT_SIZE_CHANGED:
                            case SDL_WINDOWEVENT_RESIZED:
                                gr_sdl_ogl_resize_window(event.window.data1, event.window.data2);
                                break;
							case SDL_WINDOWEVENT_FOCUS_GAINED:
								gr_focus_lost = 0;
								break;
							case SDL_WINDOWEVENT_FOCUS_LOST:
								gr_focus_lost = 1;
								break;
                            }
                            break;
                        }
		}
	}

	// Send the idle event if there were no other events
	if (idle)
	{
		d_event event;
		
		event.type = EVENT_IDLE;
		event_send(&event);
		return;
	}
	
	mouse_cursor_autohide();
}

void event_flush()
{
	SDL_Event event;
	
	while (SDL_PollEvent(&event));
}

int event_init()
{
	// We should now be active and responding to events.
	initialised = 1;

	return 0;
}

int (*default_handler)(d_event *event) = NULL;

void set_default_handler(int (*handler)(d_event *event))
{
	default_handler = handler;
}

int call_default_handler(d_event *event)
{
	if (default_handler)
		return (*default_handler)(event);
	
	return 0;
}

void event_send(d_event *event)
{
	window *wind;

	if ((wind = window_get_front()))
	{
		if (!window_send_event(wind, event))
			call_default_handler(event);
	}
	else
		call_default_handler(event);
}

// Process the first event in queue, sending to the appropriate handler
// This is the new object-oriented system
// Uses the old system for now, but this may change
void event_process(void)
{
	d_event event;
	window *wind = window_get_front();

	event_poll();	// send input events first

	// Doing this prevents problems when a draw event can create a newmenu,
	// such as some network menus when they report a problem
	if (window_get_front() != wind)
		return;
	
	event.type = EVENT_WINDOW_DRAW;	// then draw all visible windows
	for (wind = window_get_first(); wind != NULL; wind = window_get_next(wind))
		if (window_is_visible(wind))
			window_send_event(wind, &event);

	gr_flip();
}

void event_toggle_focus(int activate_focus)
{
	/*
	if (activate_focus && GameCfg.Grabinput)
		SDL_WM_GrabInput(SDL_GRAB_ON);
	else
		SDL_WM_GrabInput(SDL_GRAB_OFF);
	*/
	mouse_toggle_cursor(!activate_focus);
}
