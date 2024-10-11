#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "game.h"
#include "options.h"

static Display *dpy;
static Window win;

int main(void)
{
	XEvent ev;

	load_options("game.cfg");

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
			proc_event(&ev);
			if(quit) goto end;
		}

		game_draw();
	}

end:
	gfx_destroy();
	XDestroyWindow(dpy, win);
	return 0;
}
