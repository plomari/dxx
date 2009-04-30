/*
 *
 * SDL mouse driver
 *
 */

#include <string.h>
#include <SDL.h>

#include "fix.h"
#include "timer.h"
#include "event.h"
#include "mouse.h"
#include "playsave.h"

struct mousebutton {
	ubyte pressed;
	fix time_went_down;
	fix time_held_down;
	uint num_downs;
	uint num_ups;
};

static struct mouseinfo {
	struct mousebutton buttons[MOUSE_MAX_BUTTONS];
	int delta_x, delta_y, delta_z;
	int x,y,z;
} Mouse;

void d_mouse_init(void)
{
	memset(&Mouse,0,sizeof(Mouse));
}

void mouse_button_handler(SDL_MouseButtonEvent *mbe)
{
	// to bad, SDL buttons use a different mapping as descent expects,
	// this is at least true and tested for the first three buttons 
	int button_remap[17] = {
		MBTN_LEFT,
		MBTN_MIDDLE,
		MBTN_RIGHT,
		MBTN_Z_UP,
		MBTN_Z_DOWN,
		MBTN_PITCH_BACKWARD,
		MBTN_PITCH_FORWARD,
		MBTN_BANK_LEFT,
		MBTN_BANK_RIGHT,
		MBTN_HEAD_LEFT,
		MBTN_HEAD_RIGHT,
		MBTN_11,
		MBTN_12,
		MBTN_13,
		MBTN_14,
		MBTN_15,
		MBTN_16
	};

	int button = button_remap[mbe->button - 1]; // -1 since SDL seems to start counting at 1

	if (mbe->state == SDL_PRESSED) {
		Mouse.buttons[button].pressed = 1;
		Mouse.buttons[button].time_went_down = timer_get_fixed_seconds();
		Mouse.buttons[button].num_downs++;

		if (button == MBTN_Z_UP) {
			Mouse.delta_z += Z_SENSITIVITY;
			Mouse.z += Z_SENSITIVITY;
		} else if (button == MBTN_Z_DOWN) {
			Mouse.delta_z -= Z_SENSITIVITY;
			Mouse.z -= Z_SENSITIVITY;
		}
	} else {
		Mouse.buttons[button].pressed = 0;
		Mouse.buttons[button].time_held_down += timer_get_fixed_seconds() - Mouse.buttons[button].time_went_down;
		Mouse.buttons[button].num_ups++;
	}
}

void mouse_motion_handler(SDL_MouseMotionEvent *mme)
{
	Mouse.x += mme->xrel;
	Mouse.y += mme->yrel;
}

void mouse_flush()	// clears all mice events...
{
	int i;
	fix current_time;

	event_poll();

	current_time = timer_get_fixed_seconds();
	for (i=0; i<MOUSE_MAX_BUTTONS; i++) {
		Mouse.buttons[i].pressed=0;
		Mouse.buttons[i].time_went_down=current_time;
		Mouse.buttons[i].time_held_down=0;
		Mouse.buttons[i].num_ups=0;
		Mouse.buttons[i].num_downs=0;
	}
	Mouse.delta_x = 0;
	Mouse.delta_y = 0;
	Mouse.delta_z = 0;
	Mouse.x = 0;
	Mouse.y = 0;
	Mouse.z = 0;
	SDL_GetMouseState(&Mouse.x, &Mouse.y); // necessary because polling only gives us the delta.
}

//========================================================================
void mouse_get_pos( int *x, int *y, int *z )
{
	event_poll();
	*x=Mouse.x;
	*y=Mouse.y;
	*z=Mouse.z;
}

void mouse_get_delta( int *dx, int *dy, int *dz )
{
	static int old_delta_x = 0, old_delta_y = 0;

	SDL_GetRelativeMouseState( &Mouse.delta_x, &Mouse.delta_y );
	*dx = Mouse.delta_x;
	*dy = Mouse.delta_y;
	*dz = Mouse.delta_z;

	// filter delta?
	if (PlayerCfg.MouseFilter)
	{
		Mouse.delta_x = (*dx + old_delta_x) * 0.5;
		Mouse.delta_y = (*dy + old_delta_y) * 0.5;
	}

	old_delta_x = *dx;
	old_delta_y = *dy;

	Mouse.delta_x = 0;
	Mouse.delta_y = 0;
	Mouse.delta_z = 0;
}

int mouse_get_btns()
{
	int i;
	uint flag=1;
	int status = 0;

	event_poll();

	for (i=0; i<MOUSE_MAX_BUTTONS; i++ ) {
		if (Mouse.buttons[i].pressed)
			status |= flag;
		flag <<= 1;
	}

	return status;
}

// Returns how long this button has been down since last call.
fix mouse_button_down_time(int button)
{
	fix time_down, time;

	event_poll();

	if (!Mouse.buttons[button].pressed) {
		time_down = Mouse.buttons[button].time_held_down;
		Mouse.buttons[button].time_held_down = 0;
	} else {
		time = timer_get_fixed_seconds();
		time_down = time - Mouse.buttons[button].time_held_down;
		Mouse.buttons[button].time_held_down = time;
	}
	return time_down;
}

// Returns how many times this button has went down since last call
int mouse_button_down_count(int button)
{
	int count;

	event_poll();

	count = Mouse.buttons[button].num_downs;
	Mouse.buttons[button].num_downs = 0;

	return count;
}

// Returns 1 if this button is currently down
int mouse_button_state(int button)
{
	event_poll();
	return Mouse.buttons[button].pressed;
}
