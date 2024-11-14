#include "config.h"

#include <stdio.h>
#include <math.h>
#include "game.h"
#include "gfx.h"
#include "tiles.h"
#include "options.h"
#include "screen.h"

struct level lvl;

int game_init(void)
{
	int vmidx;

	if(gfx_init() == -1) {
		return -1;
	}

	if(opt.fullscreen) {
		if((vmidx = gfx_findmode(opt.xres, opt.yres, 8, 0)) == -1) {
			fprintf(stderr, "failed to find suitable video mode, falling back to windowed\n");
			opt.fullscreen = 0;
			goto windowed;
		}
		printf("found video mode %d (%dx%d 8bpp), switching ...\n", vmidx, opt.xres, opt.yres);
		if(gfx_setmode(vmidx) == -1) {
			fprintf(stderr, "failed to set video mode, falling back to windowed\n");
			opt.fullscreen = 0;
			goto windowed;
		}
		printf("setting up %dx%d fullscreen\n", opt.xres, opt.yres);
		if(gfx_setup(opt.xres, opt.yres, 8, GFX_FULLSCREEN) == -1) {
			fprintf(stderr, "graphics setup failed\n");
			goto err;
		}
	} else {
windowed:
		printf("setting up %dx%d windowed\n", opt.xres, opt.yres);
		if(gfx_setup(opt.xres, opt.yres, 8, GFX_WINDOWED) == -1) {
			fprintf(stderr, "graphics setup failed\n");
			goto err;
		}
	}

	if(init_leveled() == -1) return -1;
	start_screen(screens[0]);

	return 0;

err:
	gfx_destroy();
	return -1;
}

void game_cleanup(void)
{
	gfx_destroy();
}


void game_draw(void)
{
	gfx_fill(gfx_back, 4, 0);

	curscr->draw();

	gfx_swapbuffers(1);
}


void game_keyboard(int key, int press)
{
	if(press && key == 27) {
		game_quit();
		return;
	}

	curscr->keyb(key, press);
}

void game_mousebtn(int bn, int st, int x, int y)
{
	curscr->mbutton(bn, st, x, y);
}

void game_mousemove(int x, int y)
{
	curscr->mmotion(x, y);
}
