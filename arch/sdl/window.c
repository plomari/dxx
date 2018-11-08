/*
 *  A 'window' is simply a canvas that can receive events.
 *  It can be anything from a simple message box to the
 *	game screen when playing.
 *
 *  See event.c for event handling code.
 *
 *	-kreator 2009-05-06
 */

#include "gr.h"
#include "window.h"
#include "u_mem.h"
#include "dxxerror.h"

struct window
{
	grs_canvas w_canv;					// the window's canvas to draw to
	int (*w_callback)(window *wind, d_event *event, void *data);	// the event handler
	bool opaque;						// if true, don't render the windows behind this one
	bool grab_input;
	void *data;							// whatever the user wants (eg menu data for 'newmenu' menus)
	struct window *prev;				// the previous window in the doubly linked list
	struct window *next;				// the next window in the doubly linked list
};

static window *FrontWindow = NULL;
static window *FirstWindow = NULL;
static bool current_grab;

static void update_top_window(void)
{
	window *wind = window_get_front();

	bool new_grab = wind && wind->grab_input;
	if (new_grab != current_grab)
		gr_set_input_grab(new_grab);
	current_grab = new_grab;
}

window *window_create(grs_canvas *src, int x, int y, int w, int h, int (*event_callback)(window *wind, d_event *event, void *data), void *data)
{
	window *wind;
	window *prev = window_get_front();
	d_event event;
	
	wind = d_malloc(sizeof(window));
	if (wind == NULL)
		return NULL;

	*wind = (window){0};

	Assert(src != NULL);
	Assert(event_callback != NULL);
	gr_init_sub_canvas(&wind->w_canv, src, x, y, w, h);
	wind->w_callback = event_callback;
	wind->data = data;

	if (FirstWindow == NULL)
		FirstWindow = wind;
	wind->prev = FrontWindow;
	if (FrontWindow)
		FrontWindow->next = wind;
	wind->next = NULL;
	FrontWindow = wind;
	if (prev)
		WINDOW_SEND_EVENT(prev, EVENT_WINDOW_DEACTIVATED);

	WINDOW_SEND_EVENT(wind, EVENT_WINDOW_CREATED);
	WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);
	update_top_window();

	return wind;
}

int window_close(window *wind)
{
	window *prev;
	d_event event;
	int (*w_callback)(window *wind, d_event *event, void *data) = wind->w_callback;

	if (wind == window_get_front()) {
		WINDOW_SEND_EVENT(wind, EVENT_WINDOW_DEACTIVATED);	// Deactivate first
		update_top_window();
	}

	event.type = EVENT_WINDOW_CLOSE;
	con_printf(CON_DEBUG,	"Sending event EVENT_WINDOW_CLOSE to window of dimensions %dx%d\n",
			   (wind)->w_canv.cv_bitmap.bm_w, (wind)->w_canv.cv_bitmap.bm_h);
	if (window_send_event(wind, &event))
		printf("Warning: window doesn't want to be closed\n");

	if (wind == FrontWindow)
		FrontWindow = wind->prev;
	if (wind == FirstWindow)
		FirstWindow = wind->next;
	if (wind->next)
		wind->next->prev = wind->prev;
	if (wind->prev)
		wind->prev->next = wind->next;

	d_free(wind);

	if ((prev = window_get_front()))
		WINDOW_SEND_EVENT(prev, EVENT_WINDOW_ACTIVATED);

	update_top_window();

	return 1;
}

int window_exists(window *wind)
{
	window *w;

	for (w = FirstWindow; w && w != wind; w = w->next) {}
	
	return w && w == wind;
}

// Get the top window that's visible
window *window_get_front(void)
{
	return FrontWindow;
}

window *window_get_first(void)
{
	return FirstWindow;
}

window *window_get_next(window *wind)
{
	return wind->next;
}

window *window_get_prev(window *wind)
{
	return wind->prev;
}

// Make wind the front window
void window_select(window *wind)
{
	window *prev = window_get_front();
	d_event event;

	Assert (wind != NULL);

	if (wind == FrontWindow)
		return;
	if ((wind == FirstWindow) && FirstWindow->next)
		FirstWindow = FirstWindow->next;

	if (wind->next)
		wind->next->prev = wind->prev;
	if (wind->prev)
		wind->prev->next = wind->next;
	wind->prev = FrontWindow;
	FrontWindow->next = wind;
	wind->next = NULL;
	FrontWindow = wind;
	
	if (prev)
		WINDOW_SEND_EVENT(prev, EVENT_WINDOW_DEACTIVATED);
	WINDOW_SEND_EVENT(wind, EVENT_WINDOW_ACTIVATED);

	update_top_window();
}

void window_set_opaque(window *wind, bool val)
{
	wind->opaque = val;
}

bool window_get_opaque(window *wind)
{
	return wind->opaque;
}

grs_canvas *window_get_canvas(window *wind)
{
	return &wind->w_canv;
}

int window_send_event(window *wind, d_event *event)
{
	return wind->w_callback(wind, event, wind->data);
}

bool window_get_grab_input(window *wind)
{
	return wind->grab_input;
}

void window_set_grab_input(window *wind, bool v)
{
	wind->grab_input = v;
	update_top_window();
}
