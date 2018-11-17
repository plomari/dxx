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
#include "mouse.h"
#include "window.h"
#include "timer.h"
#include "config.h"

#include <SDL.h>

extern void key_handler(SDL_KeyboardEvent *event);
extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
extern void joy_button_handler(SDL_JoyButtonEvent *jbe);
extern void joy_hat_handler(SDL_JoyHatEvent *jhe);
extern int joy_axis_handler(SDL_JoyAxisEvent *jae);
extern void mouse_cursor_autohide();

static int initialised=0;
static bool quitting;

static void event_poll(void)
{
	SDL_Event event;
	int clean_uniframe=1;
	window *wind = window_get_front();
	int idle = 1;

	window *cur = window_get_front();
	window_register_weak_ptr(&cur);

	while (1) {
		// If the front window changes, exit this loop, otherwise unintended behavior can occur
		// like pressing 'Return' really fast at 'Difficulty Level' causing multiple games to be started
		if (cur != window_get_front())
			goto done;

		if (!SDL_PollEvent(&event))
			break;

		switch(event.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (clean_uniframe)
					memset(unicode_frame_buffer,'\0',sizeof(unsigned char)*KEY_BUFFER_SIZE);
				clean_uniframe=0;
				key_handler((SDL_KeyboardEvent *)&event);
				idle = 0;
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				mouse_button_handler(&event.button);
				idle = 0;
				break;
			case SDL_MOUSEMOTION:
				mouse_motion_handler(&event.motion);
				idle = 0;
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				joy_button_handler(&event.jbutton);
				idle = 0;
				break;
			case SDL_JOYAXISMOTION:
				if (joy_axis_handler(&event.jaxis))
					idle = 0;
				break;
			case SDL_JOYHATMOTION:
 				joy_hat_handler(&event.jhat);
// 				idle = 0;
				break;
			case SDL_JOYBALLMOTION:
				break;
			case SDL_QUIT: {
				d_event qevent = { EVENT_QUIT };
				event_send(&qevent);
				idle = 0;
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
		d_event ievent;
		
		ievent.type = EVENT_IDLE;
		event_send(&ievent);
	}
	else
		event_reset_idle_seconds();
	
	mouse_cursor_autohide();

done:
	window_unregister_weak_ptr(&cur);
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
	if (event->type == EVENT_QUIT) {
		quitting = true;
		return;
	}

	window *wind = window_get_front();

	if (!wind || !window_send_event(wind, event))
		call_default_handler(event);
}

// Process the first event in queue, sending to the appropriate handler
// This is the new object-oriented system
// Uses the old system for now, but this may change
static void event_process(void)
{
	window *wind = window_get_front();

	if (wind && quitting) {
		window_close(wind);
		return;
	}

	timer_update();

	event_poll();	// send input events first

	// Doing this prevents problems when a draw event can create a newmenu,
	// such as some network menus when they report a problem
	if (window_get_front() != wind)
		return;

	gr_prepare_frame();

	window *last_opaque = NULL;
	for (window *w = window_get_first(); w; w = window_get_next(w)) {
		if (!last_opaque || window_get_opaque(w))
			last_opaque = w;
	}
	wind = last_opaque;

	while (wind != NULL)
	{
		window *prev = window_get_prev(wind);

		window_register_weak_ptr(&prev);
		window_register_weak_ptr(&wind);

		window_send_event(wind, &(d_event){EVENT_WINDOW_DRAW});

		window_unregister_weak_ptr(&wind);
		window_unregister_weak_ptr(&prev);

		if (!wind) {
			if (!prev) // well there isn't a previous window ...
				break; // ... just bail out - we've done everything for this frame we can.
			wind = window_get_next(prev); // the current window seemed to be closed. so take the next one from the previous which should be able to point to the one after the current closed
		}
		else
			wind = window_get_next(wind);
	}

	gr_flip();
}

// Run event loop until it's closed.
void window_run_event_loop(window *wind)
{
	window_register_weak_ptr(&wind);
	while (wind)
		event_process();
	window_unregister_weak_ptr(&wind);
}

static fix64 last_event = 0;

void event_reset_idle_seconds()
{
	last_event = timer_query();
}

fix event_get_idle_seconds()
{
	return (timer_query() - last_event)/F1_0;
}

