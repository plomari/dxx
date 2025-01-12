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
 * Prototypes for reading controls
 *
 */


#ifndef _KCONFIG_H
#define _KCONFIG_H

#include "config.h"
#include "gamestat.h"
#include "event.h"
#include "key.h"
#include "joy.h"
#include "mouse.h"

struct ramp_state {
	float down_time;
	ubyte state;
};

typedef struct control_info_ {
	struct ramp_state	key_pitch_forward,
						key_pitch_backward,
						key_heading_left,
						key_heading_right,
						key_slide_left,
						key_slide_right,
						key_slide_up,
						key_slide_down,
						key_bank_left,
						key_bank_right;
	fix pitch_time, vertical_thrust_time, heading_time, sideways_thrust_time, bank_time, forward_thrust_time;
	ubyte btn_slide_left_state, btn_slide_right_state, btn_slide_up_state, btn_slide_down_state, btn_bank_left_state, btn_bank_right_state;
	ubyte slide_on_state, bank_on_state;
	ubyte accelerate_state, reverse_state, cruise_plus_state, cruise_minus_state, cruise_off_count;
	ubyte rear_view_state;
	ubyte fire_primary_state, fire_secondary_state, fire_flare_count, drop_bomb_count;
	ubyte automap_state;
	ubyte cycle_primary_count, cycle_secondary_count, select_weapon_count;
	ubyte toggle_bomb_count;
	ubyte afterburner_state, headlight_count, energy_to_shield_state;
	fix joy_axis[JOY_MAX_AXES], raw_joy_axis[JOY_MAX_AXES], mouse_axis[3], raw_mouse_axis[3];
	fix64 mouse_delta_time;
} control_info;

#define CONTROL_USING_JOYSTICK	1
#define CONTROL_USING_MOUSE		2
#define MOUSEFS_DELTA_RANGE 512
#define MAX_D2X_CONTROLS    30
#define MAX_CONTROLS        60		// there are actually 48, so this leaves room for more

extern control_info Controls;
extern void kconfig_read_controls(d_event *event, int automap_flag);
void kconfig_process_controls_frame(void);
extern void kconfig(int n, char *title);

extern const ubyte DefaultKeySettingsD2X[MAX_D2X_CONTROLS];
extern const ubyte DefaultKeySettings[3][MAX_CONTROLS];

extern void kc_set_controls();

//set the cruise speed to zero
extern void reset_cruise(void);

#endif /* _KCONFIG_H */
