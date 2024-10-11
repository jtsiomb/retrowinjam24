#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "game.h"
#include "gfx.h"
#include "options.h"

struct gfxmode *gfx_modes, *gfx_curmode;
int gfx_num_modes;
struct gfximage *gfx_front, *gfx_back;


static int create_win(int xsz, int ysz, const char *title);
static void handle_event(XEvent *ev);

static void get_window_pos(int *x, int *y);
static void get_window_size(int *w, int *h);
static void get_screen_size(int *scrw, int *scrh);
static void set_fullscreen_mwm(int fs);
static int have_netwm_fullscr(void);
static void set_fullscreen_ewmh(int fs);
static void set_fullscreen(int fs);


static Display *dpy;
static Window win, root_win;
static Colormap cmap;
static Atom xa_wm_proto, xa_wm_del_win;
static Atom xa_net_wm_state, xa_net_wm_state_fullscr;
static Atom xa_motif_wm_hints;
static int win_width, win_height;
static int mapped;
static int fullscreen;
static int prev_win_x, prev_win_y, prev_win_width, prev_win_height;
static int quit;


int main(void)
{
	XEvent ev;

	load_options("game.cfg");

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server\n");
		return 1;
	}
	root_win = DefaultRootWindow(dpy);
	xa_wm_proto = XInternAtom(dpy, "WM_PROTOCOLS", False);
	xa_wm_del_win = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	xa_motif_wm_hints = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
	xa_net_wm_state_fullscr = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	if(have_netwm_fullscr()) {
		xa_net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
	}

	if(create_win(opt.xres, opt.yres, "win32 retro jam 2024 (UNIX version)") == -1) {
		return 1;
	}

	if(game_init() == -1) {
		return 1;
	}

	if(gfx_init() == -1) {
		return 1;
	}

	if(opt.fullscreen) {
		/* TODO */
	} else {
		printf("setting up %dx%d windowed\n", opt.xres, opt.yres);
		if(gfx_setup(opt.xres, opt.yres, 8, GFX_WINDOWED) == -1) {
			fprintf(stderr, "graphics setup failed\n");
			return 1;
		}
	}

	gfx_setcolor(4, 255, 0, 0);

	for(;;) {
		while(XPending(dpy)) {
			XNextEvent(dpy, &ev);
			handle_event(&ev);
			if(quit) goto end;
		}

		game_draw();
	}

end:
	gfx_destroy();
	XDestroyWindow(dpy, win);
	return 0;
}

static int create_win(int xsz, int ysz, const char *title)
{
	XVisualInfo vinf;
	XSetWindowAttributes xattr;
	int scr;
	XTextProperty tprop;

	scr = DefaultScreen(dpy);

	if(!XMatchVisualInfo(dpy, scr, 8, PseudoColor, &vinf)) {
		fprintf(stderr, "failed to find suitable visual\n");
		/* TODO: TrueColor fallback */
		return -1;
	}

	if(!(cmap = XCreateColormap(dpy, root_win, vinf.visual, AllocAll))) {
		fprintf(stderr, "failed to create colormap\n");
		return -1;
	}

	xattr.background_pixel = BlackPixel(dpy, scr);
	xattr.colormap = cmap;

	if(!(win = XCreateWindow(dpy, root_win, 0, 0, xsz, ysz, 0, 8, InputOutput,
					vinf.visual, CWBackPixel | CWColormap, &xattr))) {
		return -1;
	}
	XSelectInput(dpy, win, KeyPressMask | KeyReleaseMask | ButtonPressMask |
			ButtonReleaseMask | ButtonMotionMask | PointerMotionMask |
			StructureNotifyMask | ExposureMask);

	XSetWMProtocols(dpy, win, &xa_wm_del_win, 1);
	if(XStringListToTextProperty((char**)&title, 1, &tprop)) {
		XSetWMName(dpy, win, &tprop);
		XFree(tprop.value);
	}
	XMapWindow(dpy, win);

	return 0;
}

static void handle_event(XEvent *ev)
{
	switch(ev->type) {
	case MapNotify:
		mapped = 1;
		break;
	case UnmapNotify:
		mapped = 0;
		break;
	case ConfigureNotify:
		if(ev->xconfigure.width != win_width || ev->xconfigure.height != win_height) {
			/* TODO whatever */
		}
		break;

	case ClientMessage:
		if(ev->xclient.message_type == xa_wm_proto) {
			if(ev->xclient.data.l[0] == xa_wm_del_win) {
				quit = 1;
			}
		}
		break;

	case KeyPress:
	case ButtonPress:
	case ButtonRelease:
	case MotionNotify:
		break;

	default:
		break;
	}
}



static void get_window_pos(int *x, int *y)
{
	Window child;
	XTranslateCoordinates(dpy, win, root_win, 0, 0, x, y, &child);
}

static void get_window_size(int *w, int *h)
{
	XWindowAttributes wattr;
	XGetWindowAttributes(dpy, win, &wattr);
	*w = wattr.width;
	*h = wattr.height;
}

static void get_screen_size(int *scrw, int *scrh)
{
	XWindowAttributes wattr;
	XGetWindowAttributes(dpy, root_win, &wattr);
	*scrw = wattr.width;
	*scrh = wattr.height;
}


struct mwm_hints {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long input_mode;
	unsigned long status;
};

#define MWM_HINTS_DECORATIONS	2
#define MWM_DECOR_ALL			1

static void set_fullscreen_mwm(int fs)
{
	struct mwm_hints hints;
	int scr_width, scr_height;

	if(fs) {
		get_window_pos(&prev_win_x, &prev_win_y);
		get_window_size(&prev_win_width, &prev_win_height);
		get_screen_size(&scr_width, &scr_height);

		hints.decorations = 0;
		hints.flags = MWM_HINTS_DECORATIONS;
		XChangeProperty(dpy, win, xa_motif_wm_hints, xa_motif_wm_hints, 32,
				PropModeReplace, (unsigned char*)&hints, 5);

		XMoveResizeWindow(dpy, win, 0, 0, scr_width, scr_height);
	} else {
		XDeleteProperty(dpy, win, xa_motif_wm_hints);
		XMoveResizeWindow(dpy, win, prev_win_x, prev_win_y, prev_win_width, prev_win_height);
	}
}

static int have_netwm_fullscr(void)
{
	int fmt;
	long offs = 0;
	unsigned long i, count, rem;
	Atom *prop, type;
	Atom xa_net_supported = XInternAtom(dpy, "_NET_SUPPORTED", False);

	do {
		XGetWindowProperty(dpy, root_win, xa_net_supported, offs, 8, False, AnyPropertyType,
				&type, &fmt, &count, &rem, (unsigned char**)&prop);

		for(i=0; i<count; i++) {
			if(prop[i] == xa_net_wm_state_fullscr) {
				XFree(prop);
				return 1;
			}
		}
		XFree(prop);
		offs += count;
	} while(rem > 0);

	return 0;
}


static void set_fullscreen_ewmh(int fs)
{
	XClientMessageEvent msg = {0};

	msg.type = ClientMessage;
	msg.window = win;
	msg.message_type = xa_net_wm_state;	/* _NET_WM_STATE */
	msg.format = 32;
	msg.data.l[0] = fs ? 1 : 0;
	msg.data.l[1] = xa_net_wm_state_fullscr;	/* _NET_WM_STATE_FULLSCREEN */
	msg.data.l[2] = 0;
	msg.data.l[3] = 1;	/* source regular application */
	XSendEvent(dpy, root_win, False, SubstructureNotifyMask | SubstructureRedirectMask, (XEvent*)&msg);
}

static void set_fullscreen(int fs)
{
	if(fullscreen == fs) return;

	if(xa_net_wm_state && xa_net_wm_state_fullscr) {
		set_fullscreen_ewmh(fs);
		fullscreen = fs;
	} else if(xa_motif_wm_hints) {
		set_fullscreen_mwm(fs);
		fullscreen = fs;
	}
}

