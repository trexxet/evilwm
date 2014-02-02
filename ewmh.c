/* evilwm - Minimalist Window Manager for X
 * Copyright (C) 1999-2011 Ciaran Anscomb <evilwm@6809.org.uk>
 * see README for license and other details. */

#include <stdlib.h>
#include <unistd.h>
#include "evilwm.h"
#include "log.h"

/* Standard X protocol atoms */
Atom xa_wm_state;
Atom xa_wm_protos;
Atom xa_wm_delete;
Atom xa_wm_cmapwins;

/* Motif atoms */
Atom mwm_hints;

/* evilwm atoms */
Atom xa_evilwm_unmaximised_horz;
Atom xa_evilwm_unmaximised_vert;

/* Root Window Properties (and Related Messages) */
static Atom xa_net_supported;
static Atom xa_net_client_list;
static Atom xa_net_client_list_stacking;
#ifdef VWM
static Atom xa_net_number_of_desktops;
#endif
static Atom xa_net_desktop_geometry;
static Atom xa_net_desktop_viewport;
#ifdef VWM
Atom xa_net_current_desktop;
#endif
Atom xa_net_active_window;
static Atom xa_net_workarea;
static Atom xa_net_supporting_wm_check;

/* Other Root Window Messages */
Atom xa_net_close_window;
Atom xa_net_moveresize_window;
Atom xa_net_restack_window;
Atom xa_net_request_frame_extents;

/* Application Window Properties */
static Atom xa_net_wm_name;
#ifdef VWM
Atom xa_net_wm_desktop;
#endif
Atom xa_net_wm_window_type;
Atom xa_net_wm_window_type_desktop;
Atom xa_net_wm_window_type_dock;
Atom xa_net_wm_state;
Atom xa_net_wm_state_maximized_vert;
Atom xa_net_wm_state_maximized_horz;
Atom xa_net_wm_state_fullscreen;
Atom xa_net_wm_state_hidden;
static Atom xa_net_wm_allowed_actions;
static Atom xa_net_wm_action_move;
static Atom xa_net_wm_action_resize;
static Atom xa_net_wm_action_maximize_horz;
static Atom xa_net_wm_action_maximize_vert;
static Atom xa_net_wm_action_fullscreen;
static Atom xa_net_wm_action_change_desktop;
static Atom xa_net_wm_action_close;
static Atom xa_net_wm_pid;
Atom xa_net_frame_extents;

/* Maintain a reasonably sized allocated block of memory for lists
 * of windows (for feeding to XChangeProperty in one hit). */
static Window *window_array = NULL;
static Window *alloc_window_array(void);

static const struct {
	Atom *dest;
	const char *name;
} atom_list[] = {
	/* Standard X protocol atoms */
	{ &xa_wm_state, "WM_STATE" },
	{ &xa_wm_protos, "WM_PROTOCOLS" },
	{ &xa_wm_delete, "WM_DELETE_WINDOW" },
	{ &xa_wm_cmapwins, "WM_COLORMAP_WINDOWS" },
	/* Motif atoms */
	{ &mwm_hints, _XA_MWM_HINTS },
	/* evilwm atoms */
	{ &xa_evilwm_unmaximised_horz, "_EVILWM_UNMAXIMISED_HORZ" },
	{ &xa_evilwm_unmaximised_vert, "_EVILWM_UNMAXIMISED_VERT" },

	/*
	 * extended windowmanager hints
	 */

	/* Root Window Properties (and Related Messages) */
	{ &xa_net_supported, "_NET_SUPPORTED" },
	{ &xa_net_client_list, "_NET_CLIENT_LIST" },
	{ &xa_net_client_list_stacking, "_NET_CLIENT_LIST_STACKING" },
#ifdef VWM
	{ &xa_net_number_of_desktops, "_NET_NUMBER_OF_DESKTOPS" },
#endif
	{ &xa_net_desktop_geometry, "_NET_DESKTOP_GEOMETRY" },
	{ &xa_net_desktop_viewport, "_NET_DESKTOP_VIEWPORT" },
#ifdef VWM
	{ &xa_net_current_desktop, "_NET_CURRENT_DESKTOP" },
#endif
	{ &xa_net_active_window, "_NET_ACTIVE_WINDOW" },
	{ &xa_net_workarea, "_NET_WORKAREA" },
	{ &xa_net_supporting_wm_check, "_NET_SUPPORTING_WM_CHECK" },

	/* Other Root Window Messages */
	{ &xa_net_close_window, "_NET_CLOSE_WINDOW" },
	{ &xa_net_moveresize_window, "_NET_MOVERESIZE_WINDOW" },
	{ &xa_net_restack_window, "_NET_RESTACK_WINDOW" },
	{ &xa_net_request_frame_extents, "_NET_REQUEST_FRAME_EXTENTS" },

	/* Application Window Properties */
	{ &xa_net_wm_name, "_NET_WM_NAME" },
#ifdef VWM
	{ &xa_net_wm_desktop, "_NET_WM_DESKTOP" },
#endif
	{ &xa_net_wm_window_type, "_NET_WM_WINDOW_TYPE" },
	{ &xa_net_wm_window_type_desktop, "_NET_WM_WINDOW_TYPE_DESKTOP" },
	{ &xa_net_wm_window_type_dock, "_NET_WM_WINDOW_TYPE_DOCK" },
	{ &xa_net_wm_state, "_NET_WM_STATE" },
	{ &xa_net_wm_state_maximized_vert, "_NET_WM_STATE_MAXIMIZED_VERT" },
	{ &xa_net_wm_state_maximized_horz, "_NET_WM_STATE_MAXIMIZED_HORZ" },
	{ &xa_net_wm_state_fullscreen, "_NET_WM_STATE_FULLSCREEN" },
	{ &xa_net_wm_state_hidden, "_NET_WM_STATE_HIDDEN" },
	{ &xa_net_wm_allowed_actions, "_NET_WM_ALLOWED_ACTIONS" },
	{ &xa_net_wm_action_move, "_NET_WM_ACTION_MOVE" },
	{ &xa_net_wm_action_resize, "_NET_WM_ACTION_RESIZE" },
	{ &xa_net_wm_action_maximize_horz, "_NET_WM_ACTION_MAXIMIZE_HORZ" },
	{ &xa_net_wm_action_maximize_vert, "_NET_WM_ACTION_MAXIMIZE_VERT" },
	{ &xa_net_wm_action_fullscreen, "_NET_WM_ACTION_FULLSCREEN" },
	{ &xa_net_wm_action_change_desktop, "_NET_WM_ACTION_CHANGE_DESKTOP" },
	{ &xa_net_wm_action_close, "_NET_WM_ACTION_CLOSE" },
	{ &xa_net_wm_pid, "_NET_WM_PID" },
	{ &xa_net_frame_extents, "_NET_FRAME_EXTENTS" },
};

void ewmh_init(void) {
	for (unsigned i = 0; i < (sizeof(atom_list)/sizeof(atom_list[0])); i++) {
		*(atom_list[i].dest) = XInternAtom(dpy, atom_list[i].name, False);
	}
}

void ewmh_init_screen(ScreenInfo *s) {
	unsigned long pid = getpid();
	Atom supported[] = {
		xa_net_client_list,
		xa_net_client_list_stacking,
#ifdef VWM
		xa_net_number_of_desktops,
#endif
		xa_net_desktop_geometry,
		xa_net_desktop_viewport,
#ifdef VWM
		xa_net_current_desktop,
#endif
		xa_net_active_window,
		xa_net_workarea,
		xa_net_supporting_wm_check,

		xa_net_close_window,
		xa_net_moveresize_window,
		xa_net_restack_window,
		xa_net_request_frame_extents,

#ifdef VWM
		xa_net_wm_desktop,
#endif
		xa_net_wm_window_type,
		xa_net_wm_window_type_desktop,
		xa_net_wm_window_type_dock,
		xa_net_wm_state,
		xa_net_wm_state_maximized_vert,
		xa_net_wm_state_maximized_horz,
		xa_net_wm_state_fullscreen,
		xa_net_wm_state_hidden,
		xa_net_wm_allowed_actions,
		/* Not sure if it makes any sense including every action here
		 * as they'll already be listed per-client in the
		 * _NET_WM_ALLOWED_ACTIONS property, but EWMH spec is unclear.
		 * */
		xa_net_wm_action_move,
		xa_net_wm_action_resize,
		xa_net_wm_action_maximize_horz,
		xa_net_wm_action_maximize_vert,
		xa_net_wm_action_fullscreen,
		xa_net_wm_action_change_desktop,
		xa_net_wm_action_close,
		xa_net_frame_extents,
	};
#ifdef VWM
	unsigned long num_desktops = 8;
	unsigned long vdesk = s->vdesk;
#endif
	unsigned long workarea[4] = {
		0, 0,
		DisplayWidth(dpy, s->screen), DisplayHeight(dpy, s->screen)
	};
	s->supporting = XCreateSimpleWindow(dpy, s->root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, s->root, xa_net_supported,
			XA_ATOM, 32, PropModeReplace,
			(unsigned char *)&supported,
			sizeof(supported) / sizeof(Atom));
#ifdef VWM
	XChangeProperty(dpy, s->root, xa_net_number_of_desktops,
			XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&num_desktops, 1);
#endif
	XChangeProperty(dpy, s->root, xa_net_desktop_geometry,
			XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&workarea[2], 2);
	XChangeProperty(dpy, s->root, xa_net_desktop_viewport,
			XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&workarea[0], 2);
#ifdef VWM
	XChangeProperty(dpy, s->root, xa_net_current_desktop,
			XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&vdesk, 1);
#endif
	XChangeProperty(dpy, s->root, xa_net_workarea,
			XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&workarea, 4);
	XChangeProperty(dpy, s->root, xa_net_supporting_wm_check,
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *)&s->supporting, 1);
	XChangeProperty(dpy, s->supporting, xa_net_supporting_wm_check,
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *)&s->supporting, 1);
	XChangeProperty(dpy, s->supporting, xa_net_wm_name,
			XA_STRING, 8, PropModeReplace,
			(const unsigned char *)"evilwm", 6);
	XChangeProperty(dpy, s->supporting, xa_net_wm_pid,
			XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&pid, 1);
}

void ewmh_deinit_screen(ScreenInfo *s) {
	XDeleteProperty(dpy, s->root, xa_net_supported);
	XDeleteProperty(dpy, s->root, xa_net_client_list);
	XDeleteProperty(dpy, s->root, xa_net_client_list_stacking);
#ifdef VWM
	XDeleteProperty(dpy, s->root, xa_net_number_of_desktops);
#endif
	XDeleteProperty(dpy, s->root, xa_net_desktop_geometry);
	XDeleteProperty(dpy, s->root, xa_net_desktop_viewport);
#ifdef VWM
	XDeleteProperty(dpy, s->root, xa_net_current_desktop);
#endif
	XDeleteProperty(dpy, s->root, xa_net_active_window);
	XDeleteProperty(dpy, s->root, xa_net_workarea);
	XDeleteProperty(dpy, s->root, xa_net_supporting_wm_check);
	XDestroyWindow(dpy, s->supporting);
}

void ewmh_init_client(Client *c) {
	Atom allowed_actions[] = {
		xa_net_wm_action_move,
		xa_net_wm_action_maximize_horz,
		xa_net_wm_action_maximize_vert,
		xa_net_wm_action_fullscreen,
		xa_net_wm_action_change_desktop,
		xa_net_wm_action_close,
		/* nelements reduced to omit this if not possible: */
		xa_net_wm_action_resize,
	};
	int nelements = sizeof(allowed_actions) / sizeof(Atom);
	/* Omit resize element if resizing not possible: */
	if (c->max_width && c->max_width == c->min_width
			&& c->max_height && c->max_height == c->min_height)
		nelements--;
	XChangeProperty(dpy, c->window, xa_net_wm_allowed_actions,
			XA_ATOM, 32, PropModeReplace,
			(unsigned char *)&allowed_actions,
			nelements);
}

void ewmh_deinit_client(Client *c) {
	XDeleteProperty(dpy, c->window, xa_net_wm_allowed_actions);
}

void ewmh_withdraw_client(Client *c) {
#ifdef VWM
	XDeleteProperty(dpy, c->window, xa_net_wm_desktop);
#endif
	XDeleteProperty(dpy, c->window, xa_net_wm_state);
}

void ewmh_select_client(Client *c) {
	clients_tab_order = list_to_head(clients_tab_order, c);
}

void ewmh_set_net_client_list(ScreenInfo *s) {
	Window *windows = alloc_window_array();
	struct list *iter;
	int i = 0;
	for (iter = clients_mapping_order; iter; iter = iter->next) {
		Client *c = iter->data;
		if (c->screen == s) {
			windows[i++] = c->window;
		}
	}
	XChangeProperty(dpy, s->root, xa_net_client_list,
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *)windows, i);
}

void ewmh_set_net_client_list_stacking(ScreenInfo *s) {
	Window *windows = alloc_window_array();
	struct list *iter;
	int i = 0;
	for (iter = clients_stacking_order; iter; iter = iter->next) {
		Client *c = iter->data;
		if (c->screen == s) {
			windows[i++] = c->window;
		}
	}
	XChangeProperty(dpy, s->root, xa_net_client_list_stacking,
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *)windows, i);
}

#ifdef VWM
void ewmh_set_net_current_desktop(ScreenInfo *s) {
	unsigned long vdesk = s->vdesk;
	XChangeProperty(dpy, s->root, xa_net_current_desktop,
			XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&vdesk, 1);
}
#endif

void ewmh_set_net_active_window(Client *c) {
	int i;
	for (i = 0; i < num_screens; i++) {
		Window w;
		if (c && i == c->screen->screen) {
			w = c->window;
		} else {
			w = None;
		}
		XChangeProperty(dpy, screens[i].root, xa_net_active_window,
				XA_WINDOW, 32, PropModeReplace,
				(unsigned char *)&w, 1);
	}
}

#ifdef VWM
void ewmh_set_net_wm_desktop(Client *c) {
	unsigned long vdesk = c->vdesk;
	XChangeProperty(dpy, c->window, xa_net_wm_desktop,
			XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&vdesk, 1);
}
#endif

unsigned int ewmh_get_net_wm_window_type(Window w) {
	Atom *aprop;
	unsigned long nitems, i;
	unsigned int type = 0;
	if ( (aprop = get_property(w, xa_net_wm_window_type, XA_ATOM, &nitems)) ) {
		for (i = 0; i < nitems; i++) {
			if (aprop[i] == xa_net_wm_window_type_desktop)
				type |= EWMH_WINDOW_TYPE_DESKTOP;
			if (aprop[i] == xa_net_wm_window_type_dock)
				type |= EWMH_WINDOW_TYPE_DOCK;
		}
		XFree(aprop);
	}
	return type;
}

void ewmh_set_net_wm_state(Client *c) {
	Atom state[3];
	int i = 0;
	if (c->oldh)
		state[i++] = xa_net_wm_state_maximized_vert;
	if (c->oldw)
		state[i++] = xa_net_wm_state_maximized_horz;
	if (c->oldh && c->oldw)
		state[i++] = xa_net_wm_state_fullscreen;
	XChangeProperty(dpy, c->window, xa_net_wm_state,
			XA_ATOM, 32, PropModeReplace,
			(unsigned char *)&state, i);
}

void ewmh_set_net_frame_extents(Window w) {
	unsigned long extents[4];
	extents[0] = extents[1] = extents[2] = extents[3] = opt_bw;
	XChangeProperty(dpy, w, xa_net_frame_extents,
			XA_CARDINAL, 32, PropModeReplace,
			(unsigned char *)&extents, 4);
}

static Window *alloc_window_array(void) {
	struct list *iter;
	unsigned int count = 0;
	for (iter = clients_mapping_order; iter; iter = iter->next) {
		count++;
	}
	if (count == 0) count++;
	/* Round up to next block of 128 */
	count = (count + 127) & ~127;
	window_array = realloc(window_array, count * sizeof(Window));
	return window_array;
}
