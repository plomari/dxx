/*
 *  A 'window' is simply a canvas that can receive events.
 *  It can be anything from a simple message box to the
 *	game screen when playing.
 *
 *  See event.c for event handling code.
 *
 *	-kreator 2009-05-06
 */

#ifndef DESCENT_WINDOW_H
#define DESCENT_WINDOW_H

#include "event.h"
#include "gr.h"
#include "console.h"

typedef struct window window;

extern window *window_create(grs_canvas *src, int x, int y, int w, int h, int (*event_callback)(window *wind, d_event *event, void *data), void *data);
extern int window_close(window *wind);
extern window *window_get_front(void);
extern window *window_get_first(void);
extern window *window_get_next(window *wind);
extern window *window_get_prev(window *wind);
extern void window_select(window *wind);
void window_set_opaque(window *wind, bool val);
bool window_get_opaque(window *wind);
extern grs_canvas *window_get_canvas(window *wind);
extern int window_send_event(window *wind, d_event *event);
void window_run_event_loop(window *wind);
bool window_get_grab_input(window *wind);
void window_set_grab_input(window *wind, bool v);
void window_register_weak_ptr(window **ptr);
void window_unregister_weak_ptr(window **ptr);

#define WINDOW_SEND_EVENT(w, e)	\
do {	\
	con_printf(CON_DEBUG, "Sending event %s to window of dimensions %dx%d\n", #e, window_get_canvas(w)->cv_bitmap.bm_w, window_get_canvas(w)->cv_bitmap.bm_h);	\
	event.type = e;	\
	window_send_event(w, &event);	\
} while (0)

#endif
