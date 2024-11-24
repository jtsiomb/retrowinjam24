#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "gfx.h"


struct gfximage *gfx_front, *gfx_back;


int gfx_init(void)
{
	if(!(gfx_front = calloc(2, sizeof *gfx_front))) {
		fprintf(stderr, "failed to allocate memory\n");
		return -1;
	}
	gfx_back = gfx_front + 1;
	return 0;
}

void gfx_destroy(void)
{
}

int gfx_findmode(int xsz, int ysz, int bpp, int rate)
{
	return -1;
}

int gfx_setmode(int modeidx)
{
	return -1;
}

int gfx_setup(int xsz, int ysz, int bpp, unsigned int flags)
{
	int i;

	for(i=0; i<2; i++) {
		gfx_front[i].width = xsz;
		gfx_front[i].height = ysz;
		gfx_front[i].bpp = bpp;		/* TODO conversions */
	}

	if(!(gfx_back->pixels = malloc(xsz * ysz * bpp / 8))) {
		fprintf(stderr, "failed to allocate back buffer\n");
		return -1;
	}
	return 0;
}

void gfx_setcolor(int idx, int r, int g, int b)
{
}

void gfx_setcolors(int start, int count, struct gfxcolor *colors)
{
}

void *gfx_imgstart(struct gfximage *img)
{
	return 0;
}

void gfx_imgend(struct gfximage *img)
{
}

void gfx_imgdebug(struct gfximage *img)
{
}

void gfx_fill(struct gfximage *img, unsigned int color, struct gfxrect *rect)
{
}

void gfx_imgkey(struct gfximage *img, int ckey)
{
}

void gfx_blit(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect)
{
}

void gfx_blitkey(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect)
{
}

void gfx_swapbuffers(int vsync)
{
}

void gfx_waitvsync(void)
{
}
