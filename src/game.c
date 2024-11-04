#include "config.h"

#include <stdio.h>
#include <math.h>
#include "game.h"
#include "gfx.h"
#include "tiles.h"
#include "options.h"


static struct tileset tset;


int game_init(void)
{
	int i, j, vmidx;
	struct gfxcolor pal[256];
	unsigned char *pptr;

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

	if(load_tileset(&tset, "data/test.set") == -1) {
		goto err;
	}

	for(i=0; i<tset.img->ncolors; i++) {
		pal[i].r = tset.img->cmap[i].r;
		pal[i].g = tset.img->cmap[i].g;
		pal[i].b = tset.img->cmap[i].b;
	}
	gfx_setcolors(0, tset.img->ncolors, pal);

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
	blit_tile(gfx_back, 300, 300, &tset, 0);

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

flip:
	gfx_swapbuffers(1);
}

