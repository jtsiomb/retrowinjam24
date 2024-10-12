#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "gfx.h"

int gfx_init(void)
{
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
