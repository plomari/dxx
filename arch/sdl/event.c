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

#include <SDL.h>

extern void key_handler(SDL_KeyboardEvent *event);
extern void mouse_button_handler(SDL_MouseButtonEvent *mbe);
extern void mouse_motion_handler(SDL_MouseMotionEvent *mme);
extern void joy_button_handler(SDL_JoyButtonEvent *jbe);
extern void joy_hat_handler(SDL_JoyHatEvent *jhe);
extern void joy_axis_handler(SDL_JoyAxisEvent *jae);

static int initialised=0;

void event_poll()
{
	SDL_Event event;
	int clean_uniframe=1;

	while (SDL_PollEvent(&event)) {
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
				void quit_request();
				quit_request();
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
}

int event_init()
{
	// We should now be active and responding to events.
	initialised = 1;

	return 0;
}

// Process the first event in queue, sending to the appropriate handler
// This is the new object-oriented system
// Uses the old system for now, but this will change
void event_process(void)
{
	d_event event;
	window *wind;

	// Very trivial system for now.
	event.type = EVENT_IDLE;	// user input handled in idle event for now (except for newmenu)
	wind = window_get_front();
	if (!wind)
		return;

	window_send_event(wind, &event);

	event.type = EVENT_DRAW;	// then draw all visible windows
	for (wind = window_get_first(); wind != NULL; wind = window_get_next(wind))
		if (window_is_visible(wind))
			window_send_event(wind, &event);

	gr_flip();
}
