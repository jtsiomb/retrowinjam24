#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#include "gfx.h"
#include "game.h"
#include "options.h"

struct imgdata {
	SDL_Surface *surf;
	unsigned int palno;
};

#define IMGDATA(img)	((struct imgdata*)((img)->data))

struct gfxmode *gfx_modes, *gfx_curmode;
int gfx_num_modes;

struct gfximage *gfx_front, *gfx_back;
static struct gfximage swapchain_buf[2];
static SDL_Color pal[256];
static int cur_palno;

static struct gfxmode *nextvm;
static SDL_Surface *fbsurf;
static struct imgdata fbdata;
static int fullscreen;

int fbscale = 1;

#define SURF_FLAGS	(SDL_HWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF)

int gfx_init(void)
{
	int i, j, count, newsz;
	struct gfxmode *modeptr;
	char buf[256];
	SDL_Rect **rects;
	SDL_PixelFormat pfmt;
	void *tmp;
	static const int bpplist[] = {8, 16, 24, 32, 0};
	static const int defmodes[][2] = {{640, 480}, {800, 600}, {1024, 768},
		{1280, 1024}, {1280, 800}, {1280, 960}, {1440, 1080}, {1600, 1200},
		{1920, 1080}, {1920, 1200}, {0, 0}};

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) == -1) {
		fprintf(stderr, "failed to initialize SDL\n");
		return -1;
	}
	SDL_VideoDriverName(buf, sizeof buf);
	printf("SDL video driver: %s\n", buf);

	gfx_num_modes = 0;
	gfx_modes = 0;

	for(i=0; i<bpplist[i]; i++) {
		pfmt.BitsPerPixel = bpplist[i];
		if(!(rects = SDL_ListModes(&pfmt, SURF_FLAGS | SDL_FULLSCREEN))) {
			continue;
		}
		if(rects == (SDL_Rect**)-1) {
			count = 0;
			for(j=0; defmodes[j][0]; j++) count++;

			newsz = gfx_num_modes + count;

			if(!(tmp = realloc(gfx_modes, newsz * sizeof *gfx_modes))) {
				fprintf(stderr, "failed to grow the video mode list\n");
				SDL_Quit();
				return -1;
			}
			gfx_modes = tmp;
			modeptr = gfx_modes + gfx_num_modes;
			gfx_num_modes = newsz;

			for(j=0; defmodes[j][0]; j++) {
				modeptr->width = defmodes[j][0];
				modeptr->height = defmodes[j][1];
				modeptr->bpp = bpplist[i];
				modeptr++;
			}

		} else {
			count = 0;
			for(j=0; rects[j]; j++) count++;

			newsz = gfx_num_modes + count;

			if(!(tmp = realloc(gfx_modes, newsz * sizeof *gfx_modes))) {
				fprintf(stderr, "failed to grow the video mode list\n");
				SDL_Quit();
				return -1;
			}
			gfx_modes = tmp;
			modeptr = gfx_modes + gfx_num_modes;
			gfx_num_modes = newsz;

			for(j=0; rects[j]; j++) {
				modeptr->width = rects[j]->w;
				modeptr->height = rects[j]->h;
				modeptr->bpp = bpplist[i];
				modeptr++;
			}
		}
	}

	gfx_front = swapchain_buf;
	gfx_back = gfx_front + 1;

	cur_palno = 0;

	printf("Found %d video modes:\n", gfx_num_modes);
	for(i=0; i<gfx_num_modes; i++) {
		printf(" - %dx%d %dbpp\n", gfx_modes[i].width, gfx_modes[i].height, gfx_modes[i].bpp);
	}
	return 0;
}

void gfx_destroy(void)
{
	SDL_Quit();
}

int gfx_findmode(int xsz, int ysz, int bpp, int rate)
{
	int i;

	for(i=0; i<gfx_num_modes; i++) {
		if(gfx_modes[i].width != xsz) continue;
		if(gfx_modes[i].height != ysz) continue;
		if(gfx_modes[i].bpp != bpp) continue;	/* TODO handle 15==16 and 24==32 */
		if(rate > 0) {
			if(gfx_modes[i].rate != rate) continue;
		}
		return i;
	}

	return -1;
}

int gfx_setmode(int modeidx)
{
	if(modeidx < 0 || modeidx >= gfx_num_modes) {
		return -1;
	}
	nextvm = gfx_modes + modeidx;
	return 0;
}

int gfx_setup(int xsz, int ysz, int bpp, unsigned int flags)
{
	int i;

	if(flags & GFX_FULLSCREEN) {
		if(!(fbsurf = SDL_SetVideoMode(nextvm->width, nextvm->height, nextvm->bpp,
						SDL_FULLSCREEN | SURF_FLAGS))) {
			fprintf(stderr, "failed to set video mode\n");
			return -1;
		}
		fbscale = 1;

	} else {
		char *env;
		if((env = getenv("FBSCALE"))) {
			if((fbscale = atoi(env)) < 1 || fbscale > 8) {
				fprintf(stderr, "ignoring absurd FBSCALE (%d)\n", fbscale);
				fbscale = 1;
			} else {
				printf("FBSCALE: %d\n", fbscale);
			}
		}

		if(!(fbsurf = SDL_SetVideoMode(xsz * fbscale, ysz * fbscale, bpp, SURF_FLAGS))) {
			fprintf(stderr, "failed to initialize graphics\n");
			return -1;
		}
	}

	fbdata.surf = fbsurf;
	fbdata.palno = 0;

	for(i=0; i<2; i++) {
		gfx_front[i].width = xsz;
		gfx_front[i].height = ysz;
		gfx_front[i].flags = GFX_IMG_VIDMEM;
		gfx_front[i].data = &fbdata;
	}
	gfx_front->bpp = bpp;

	if(bpp <= 8) {
		/* if colormaps are involved at all, initialize the default palette */
		for(i=0; i<256; i++) {
			unsigned char c = i & 0xe0;
			pal[i].r = c | (c >> 3) | (c >> 6);
			c = (i << 3) & 0xe0;
			pal[i].g = c | (c >> 3) | (c >> 6);
			c = (i << 5) & 0xc0;
			pal[i].b = c | (c >> 2) | (c >> 4) | (c >> 6);
		}
		SDL_SetColors(fbsurf, pal, 0, 256);
	}

	fullscreen = flags & GFX_FULLSCREEN;

	return 0;
}

void gfx_setcolor(int idx, int r, int g, int b)
{
	pal[idx].r = r;
	pal[idx].g = g;
	pal[idx].b= b;

	SDL_SetColors(fbsurf, pal + idx, idx, 1);
	fbdata.palno++;
}

void gfx_setcolors(int start, int count, struct gfxcolor *colors)
{
	int i, idx;
	for(i=0; i<count; i++) {
		idx = start + i;
		pal[idx].r = colors->r;
		pal[idx].g = colors->g;
		pal[idx].b = colors->b;
		colors++;
	}

	SDL_SetColors(fbsurf, pal + start, start, count);
	fbdata.palno++;
}

int gfx_imginit(struct gfximage *img, int x, int y, int bpp)
{
	struct imgdata *data;

	if(!(data = calloc(1, sizeof *data))) {
		fprintf(stderr, "failed to create image data\n");
		return -1;
	}
	if(!(data->surf = SDL_CreateRGBSurface(SDL_SWSURFACE, x, y, bpp, 0, 0, 0, 0))) {
		fprintf(stderr, "failed to create image surface\n");
		free(data);
		return -1;
	}
	SDL_SetColors(data->surf, pal, 0, 256);

	memset(img, 0, sizeof *img);
	img->width = x;
	img->height = y;
	img->bpp = bpp;
	img->data = data;
	img->ckey = -1;
	return 0;
}

void gfx_imgdestroy(struct gfximage *img)
{
	struct imgdata *data = img->data;
	if(!data) return;
	if(data->surf) {
		SDL_FreeSurface(data->surf);
		img->data = 0;
	}
	if(data != &fbdata) {
		free(data);
	}
}

void *gfx_imgstart(struct gfximage *img)
{
	SDL_Surface *surf;

	if(img->data && (surf = IMGDATA(img)->surf)) {
		if(SDL_MUSTLOCK(surf)) {
			SDL_LockSurface(surf);
		}
		img->pixels = surf->pixels;
		img->pitch = surf->pitch;
	}
	return img->pixels;
}

void gfx_imgend(struct gfximage *img)
{
	SDL_Surface *surf;

	if(img->data && (surf = IMGDATA(img)->surf)) {
		if(SDL_MUSTLOCK(surf)) {
			SDL_UnlockSurface(surf);
		}
	}
}

void gfx_imgdebug(struct gfximage *img)
{
}

void gfx_fill(struct gfximage *img, unsigned int color, struct gfxrect *rect)
{
	SDL_Surface *surf;
	SDL_Rect r, *rp = 0;

	if(!img->data || !(surf = IMGDATA(img)->surf)) return;

	if(rect) {
		rp = &r;
		r.x = rect->x;
		r.y = rect->y;
		r.w = rect->width;
		r.h = rect->height;
	}

	SDL_FillRect(surf, rp, color);
}

void gfx_imgkey(struct gfximage *img, int ckey)
{
	SDL_Surface *surf;

	if(!img->data || !(surf = IMGDATA(img)->surf)) return;

	if(ckey >= 0) {
		SDL_SetColorKey(surf, SDL_SRCCOLORKEY, ckey);
	} else {
		SDL_SetColorKey(surf, 0, 0);
	}
	img->ckey = ckey;
}

void gfx_blit(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect)
{
	struct imgdata *sdata, *ddata;
	SDL_Rect dr, sr;

	if(!dest->data || !src->data) {
		return;
	}
	if(!(ddata = IMGDATA(dest)) || !(sdata = IMGDATA(src))) {
		return;
	}

	if(sdata->palno < ddata->palno) {
		SDL_SetColors(sdata->surf, pal, 0, 256);
		sdata->palno = cur_palno;
	}

	if(srect) {
		sr.x = srect->x;
		sr.y = srect->y;
		sr.w = srect->width;
		sr.h = srect->height;
	} else {
		sr.x = sr.y = 0;
		sr.w = src->width;
		sr.h = src->height;
	}

	dr.x = x;
	dr.y = y;
	dr.w = sr.w;
	dr.h = sr.h;

	if(SDL_BlitSurface(sdata->surf, &sr, ddata->surf, &dr) != 0) {
		fprintf(stderr, "SDL_BlitSurface failed\n");
	}
}

void gfx_blitkey(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect)
{
	gfx_blit(dest, x, y, src, srect);
}

static void scale_scanline(unsigned char *dest, unsigned char *src, int width)
{
	int i;

	for(i=0; i<width; i++) {
		unsigned char col = *src++;

		switch(fbscale) {
		case 8: *dest++ = col;
		case 7: *dest++ = col;
		case 6: *dest++ = col;
		case 5: *dest++ = col;
		case 4: *dest++ = col;
		case 3: *dest++ = col;
		case 2: *dest++ = col;
		case 1: *dest++ = col;
		default:
			break;
		}
	}
}

void gfx_swapbuffers(int vsync)
{
	if(fbscale > 1) {
		int i, j;
		unsigned char *dest, *src;

		if(SDL_MUSTLOCK(fbsurf)) {
			SDL_LockSurface(fbsurf);
		}

		dest = (unsigned char*)fbsurf->pixels + (fbsurf->h - 1) * fbsurf->pitch;
		src = (unsigned char*)fbsurf->pixels + (gfx_back->height - 1) * fbsurf->pitch;

		for(i=0; i<gfx_back->height; i++) {
			for(j=0; j<fbscale; j++) {
				scale_scanline(dest, src, gfx_back->width);
				dest -= fbsurf->pitch;
			}
			src -= fbsurf->pitch;
		}

		if(SDL_MUSTLOCK(fbsurf)) {
			SDL_UnlockSurface(fbsurf);
		}
	}
	SDL_Flip(fbsurf);
}

void gfx_waitvsync(void)
{
}
