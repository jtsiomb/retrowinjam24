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
	int i, j;
	unsigned char *pptr;
	float tsec = (float)time_msec / 1000.0f;

	gfx_fill(gfx_back, 4, 0);

	/*
	if(!gfx_imgstart(gfx_back)) {
		goto flip;
	}
	pptr = gfx_back->pixels;

	pptr += gfx_back->pitch * 20 + 20 + (int)(sin(tsec) * 100.0f + 100.0f);
	for(i=0; i<128; i++) {
		for(j=0; j<128; j++) {
			pptr[j] = i ^ j;
		}
		pptr += gfx_back->pitch;
	}

	pptr = (unsigned char*)gfx_back->pixels + gfx_back->pitch * 180 + 20;
	for(i=0; i<512; i++) {
		unsigned char *dest = pptr++;
		unsigned char col = i >> 1;

		for(j=0; j<64; j++) {
			*dest = col;
			dest += gfx_back->pitch;
		}
	}

	gfx_imgend(gfx_back);
	*/

	curscr->draw();

flip:
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
