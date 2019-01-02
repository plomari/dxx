/*
 *
 * SDL keyboard input support
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "event.h"
#include "dxxerror.h"
#include "key.h"
#include "timer.h"
#include "window.h"
#include "console.h"
#include "args.h"

static unsigned char Installed = 0;

//-------- Variable accessed by outside functions ---------
unsigned char 	keyd_last_pressed;
unsigned char 	keyd_last_released;
unsigned char	keyd_pressed[256];
fix64			keyd_time_when_last_pressed;
unsigned char		unicode_frame_buffer[KEY_BUFFER_SIZE];

typedef struct key_props {
	char *key_text;
	unsigned char ascii_value;
	SDL_Keycode sym;
} key_props;

key_props key_properties[256] = {
{ "",       255,    -1                 }, // 0
{ "ESC",    255,    SDLK_ESCAPE        },
{ "1",      '1',    SDLK_1             },
{ "2",      '2',    SDLK_2             },
{ "3",      '3',    SDLK_3             },
{ "4",      '4',    SDLK_4             },
{ "5",      '5',    SDLK_5             },
{ "6",      '6',    SDLK_6             },
{ "7",      '7',    SDLK_7             },
{ "8",      '8',    SDLK_8             },
{ "9",      '9',    SDLK_9             }, // 10
{ "0",      '0',    SDLK_0             },
{ "-",      '-',    SDLK_MINUS         },
{ "=",      '=',    SDLK_EQUALS        },
{ "BSPC",   255,    SDLK_BACKSPACE     },
{ "TAB",    255,    SDLK_TAB           },
{ "Q",      'q',    SDLK_q             },
{ "W",      'w',    SDLK_w             },
{ "E",      'e',    SDLK_e             },
{ "R",      'r',    SDLK_r             },
{ "T",      't',    SDLK_t             }, // 20
{ "Y",      'y',    SDLK_y             },
{ "U",      'u',    SDLK_u             },
{ "I",      'i',    SDLK_i             },
{ "O",      'o',    SDLK_o             },
{ "P",      'p',    SDLK_p             },
{ "[",      '[',    SDLK_LEFTBRACKET   },
{ "]",      ']',    SDLK_RIGHTBRACKET  },
{ "ENTER",  255,    SDLK_RETURN        },
{ "LCTRL",  255,    SDLK_LCTRL         },
{ "A",      'a',    SDLK_a             }, // 30
{ "S",      's',    SDLK_s             },
{ "D",      'd',    SDLK_d             },
{ "F",      'f',    SDLK_f             },
{ "G",      'g',    SDLK_g             },
{ "H",      'h',    SDLK_h             },
{ "J",      'j',    SDLK_j             },
{ "K",      'k',    SDLK_k             },
{ "L",      'l',    SDLK_l             },
{ ";",      ';',    SDLK_SEMICOLON     },
{ "'",      '\'',   SDLK_QUOTE         }, // 40
{ "`",      '`',    SDLK_BACKQUOTE     },
{ "LSHFT",  255,    SDLK_LSHIFT        },
{ "\\",     '\\',   SDLK_BACKSLASH     },
{ "Z",      'z',    SDLK_z             },
{ "X",      'x',    SDLK_x             },
{ "C",      'c',    SDLK_c             },
{ "V",      'v',    SDLK_v             },
{ "B",      'b',    SDLK_b             },
{ "N",      'n',    SDLK_n             },
{ "M",      'm',    SDLK_m             }, // 50
{ ",",      ',',    SDLK_COMMA         },
{ ".",      '.',    SDLK_PERIOD        },
{ "/",      '/',    SDLK_SLASH         },
{ "RSHFT",  255,    SDLK_RSHIFT        },
{ "PAD*",   '*',    SDLK_KP_MULTIPLY   },
{ "LALT",   255,    SDLK_LALT          },
{ "SPC",    ' ',    SDLK_SPACE         },
{ "CPSLK",  255,    SDLK_CAPSLOCK      },
{ "F1",     255,    SDLK_F1            },
{ "F2",     255,    SDLK_F2            }, // 60
{ "F3",     255,    SDLK_F3            },
{ "F4",     255,    SDLK_F4            },
{ "F5",     255,    SDLK_F5            },
{ "F6",     255,    SDLK_F6            },
{ "F7",     255,    SDLK_F7            },
{ "F8",     255,    SDLK_F8            },
{ "F9",     255,    SDLK_F9            },
{ "F10",    255,    SDLK_F10           },
{ "NMLCK",  255,    SDLK_NUMLOCKCLEAR  },
{ "SCLK",   255,    SDLK_SCROLLLOCK    }, // 70
{ "PAD7",   255,    SDLK_KP_7          },
{ "PAD8",   255,    SDLK_KP_8          },
{ "PAD9",   255,    SDLK_KP_9          },
{ "PAD-",   255,    SDLK_KP_MINUS      },
{ "PAD4",   255,    SDLK_KP_4          },
{ "PAD5",   255,    SDLK_KP_5          },
{ "PAD6",   255,    SDLK_KP_6          },
{ "PAD+",   255,    SDLK_KP_PLUS       },
{ "PAD1",   255,    SDLK_KP_1          },
{ "PAD2",   255,    SDLK_KP_2          }, // 80
{ "PAD3",   255,    SDLK_KP_3          },
{ "PAD0",   255,    SDLK_KP_0          },
{ "PAD.",   255,    SDLK_KP_PERIOD     },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "F11",    255,    SDLK_F11           },
{ "F12",    255,    SDLK_F12           },
{ "",       255,    -1                 },	
{ "",       255,    -1                 }, // 90
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "PAUSE",  255,    SDLK_PAUSE         },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 100
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 110
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 120
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 130
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 140
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 150
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "PAD",    255,    SDLK_KP_ENTER      },
{ "RCTRL",  255,    SDLK_RCTRL         },
{ "LCMD",   255,    -1 /*SDLK_LMETA*/         }, // TODO: SDL1.x
{ "RCMD",   255,    -1 /*SDLK_RMETA*/         },
{ "",       255,    -1                 }, // 160
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 170
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 180
{ "PAD/",   255,    SDLK_KP_DIVIDE     },
{ "",       255,    -1                 },
{ "PRSCR",  255,    SDLK_PRINTSCREEN   },
{ "RALT",   255,    SDLK_RALT          },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 190
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "HOME",   255,    SDLK_HOME          },
{ "UP",     255,    SDLK_UP            }, // 200
{ "PGUP",   255,    SDLK_PAGEUP        },
{ "",       255,    -1                 },
{ "LEFT",   255,    SDLK_LEFT          },
{ "",       255,    -1                 },
{ "RIGHT",  255,    SDLK_RIGHT         },
{ "",       255,    -1                 },
{ "END",    255,    SDLK_END           },
{ "DOWN",   255,    SDLK_DOWN          },
{ "PGDN",   255,    SDLK_PAGEDOWN      },
{ "INS",    255,    SDLK_INSERT        }, // 210
{ "DEL",    255,    SDLK_DELETE        },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 220
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 230
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 240
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 250
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 },
{ "",       255,    -1                 }, // 255
};

typedef struct d_event_keycommand
{
	event_type	type;	// EVENT_KEY_COMMAND/RELEASE
	int			keycode;
	bool		repeated;
} d_event_keycommand;

char *key_text[256];

unsigned char key_ascii()
{
	static unsigned char unibuffer[KEY_BUFFER_SIZE];
	int i=0, offset=0, count=0;
	
	offset=strlen((const char*)unibuffer);

	// move temporal chars from unicode_frame_buffer to empty space behind last unibuffer char (if any)
	for (i=offset; i < KEY_BUFFER_SIZE; i++)
		if (unicode_frame_buffer[count] != '\0')
		{
			unibuffer[i]=unicode_frame_buffer[count];
			unicode_frame_buffer[count]='\0';
			count++;
		}

	// unibuffer is not empty. store first char, remove it, shift all chars one step left and then print our char
	if (unibuffer[0] != '\0')
	{
		unsigned char retval = unibuffer[0];
		unsigned char unibuffer_shift[KEY_BUFFER_SIZE];
		memset(unibuffer_shift,'\0',sizeof(unsigned char)*KEY_BUFFER_SIZE);
		memcpy(unibuffer_shift,unibuffer+1,sizeof(unsigned char)*(KEY_BUFFER_SIZE-1));
		memcpy(unibuffer,unibuffer_shift,sizeof(unsigned char)*KEY_BUFFER_SIZE);
		return retval;
	}
	else
		return 255;
}

void key_handler(SDL_KeyboardEvent *kevent)
{
	// Read SDLK symbol and state
	int event_keysym = kevent->keysym.sym;
	int key_state = (kevent->state == SDL_PRESSED)?1:0;

	// fill the unicode frame-related unicode buffer 
	// TODO: SDL1.x used proper unicode (but it didn't work there either)
	if (key_state && event_keysym > 31 && event_keysym < 255)
	{
		int i = 0;
 		for (i = 0; i < KEY_BUFFER_SIZE; i++)
			if (unicode_frame_buffer[i] == '\0')
			{
				unicode_frame_buffer[i] = event_keysym;
				break;
			}
	}

	int keycode;
	for (keycode = 255; keycode > 0; keycode--)
		if (key_properties[keycode].sym == event_keysym)
			break;

	if (keycode == 0)
		return;

	d_event_keycommand event;

	// now update the key props
	if (key_state) {
		keyd_last_pressed = keycode;
		keyd_pressed[keycode] = 1;
	} else {
		keyd_pressed[keycode] = 0;
	}

	if ( keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT])
		keycode |= KEY_SHIFTED;
	if ( keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT])
		keycode |= KEY_ALTED;
	if ( keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL])
		keycode |= KEY_CTRLED;
	if ( keyd_pressed[KEY_DELETE] )
		keycode |= KEY_DEBUGGED;
	if ( keyd_pressed[KEY_LMETA] || keyd_pressed[KEY_RMETA])
		keycode |= KEY_METAED;

	// We allowed the key to be added to the queue for now,
	// because there are still input loops without associated windows
	event.type = key_state?EVENT_KEY_COMMAND:EVENT_KEY_RELEASE;
	event.keycode = keycode;
	event.repeated = kevent->repeat;
	con_printf(CON_DEBUG, "Sending event %s: %s %s %s %s %s %s\n",
			(key_state)                  ? "EVENT_KEY_COMMAND": "EVENT_KEY_RELEASE",
			(keycode & KEY_METAED)	? "META" : "",
			(keycode & KEY_DEBUGGED)	? "DEBUG" : "",
			(keycode & KEY_CTRLED)	? "CTRL" : "",
			(keycode & KEY_ALTED)	? "ALT" : "",
			(keycode & KEY_SHIFTED)	? "SHIFT" : "",
			key_properties[keycode & 0xff].key_text
			);
	event_send((d_event *)&event);
}

void key_close()
{
      Installed = 0;
}

void key_init()
{
	int i;
	
	if (Installed) return;

	Installed=1;

	keyd_time_when_last_pressed = timer_query();
	
	for(i=0; i<256; i++)
		key_text[i] = key_properties[i].key_text;
}

int event_key_get(d_event *event)
{
	Assert(event->type == EVENT_KEY_COMMAND || event->type == EVENT_KEY_RELEASE);
	return ((d_event_keycommand *)event)->keycode;
}

// same as above but without mod states
int event_key_get_raw(d_event *event)
{
	int keycode = ((d_event_keycommand *)event)->keycode;
	Assert(event->type == EVENT_KEY_COMMAND || event->type == EVENT_KEY_RELEASE);
	if ( keycode & KEY_SHIFTED ) keycode &= ~KEY_SHIFTED;
	if ( keycode & KEY_ALTED ) keycode &= ~KEY_ALTED;
	if ( keycode & KEY_CTRLED ) keycode &= ~KEY_CTRLED;
	if ( keycode & KEY_DEBUGGED ) keycode &= ~KEY_DEBUGGED;
	if ( keycode & KEY_METAED ) keycode &= ~KEY_METAED;
	return keycode;
}

bool event_key_get_repeated(d_event *event)
{
	Assert(event->type == EVENT_KEY_COMMAND || event->type == EVENT_KEY_RELEASE);
	return ((d_event_keycommand *)event)->repeated;
}