#include "config.h"

#include <stdio.h>
#include "game.h"
#include "gfx.h"
#include "options.h"


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

	gfx_setcolor(4, 255, 0, 0);

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

	gfx_fill(gfx_back, 4, 0);

	gfx_imgstart(gfx_back);
	pptr = gfx_back->pixels;

	pptr += gfx_back->pitch * 20 + 20;
	for(i=0; i<128; i++) {
		for(j=0; j<128; j++) {
			pptr[j] = i ^ j;
		}
		pptr += gfx_back->pitch;
	}
	gfx_imgend(gfx_back);

	gfx_swapbuffers(1);
}

