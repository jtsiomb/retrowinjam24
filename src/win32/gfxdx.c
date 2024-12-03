#define INITGUID
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <windows.h>
#include <ddraw.h>
#include "gfx.h"
#include "options.h"
#include "types.h"
#include "game.h"
#include "ddutil.h"
#include "util.h"

struct gfxmode *gfx_modes, *gfx_curmode;
int gfx_num_modes;
struct gfximage *gfx_front, *gfx_back;

extern HWND win;

static int fullscreen;
static IDirectDrawClipper *clipper;
static IDirectDrawPalette *ddpal;
static PALETTEENTRY pal[256];
static int client_xoffs, client_yoffs;

static HRESULT WINAPI enum_modes(DDSURFACEDESC *sdesc, void *cls);
static int calc_mask_shift(unsigned int mask);


static void gfx_swfill(struct gfximage *img, unsigned int color, struct gfxrect *rect);
static void gfx_swblit(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect);
static void gfx_swblitkey(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect);


int gfx_init(void)
{
	int i, modes_size;
	IDirectDraw *dd1;
	DDCAPS caps;

	if(!(gfx_front = calloc(2, sizeof *gfx_front))) {
		MessageBox(win, "failed to allocate memory", "fatal", MB_OK);
		return -1;
	}
	gfx_back = gfx_front + 1;

	if(DirectDrawCreate(0, &dd1, 0) != 0) {
		MessageBox(win, "failed to create DirectDraw instance", "fatal", MB_OK);
		return -1;
	}
	if(IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw2, (void**)&ddraw) != 0) {
		IDirectDraw_Release(dd1);
		MessageBox(win, "failed to query DirectDraw2 interface", "fatal", MB_OK);
		return -1;
	}
	IDirectDraw_Release(dd1);

	gfx_num_modes = 0;
	modes_size = 4;
	if(!(gfx_modes = malloc(modes_size * sizeof *gfx_modes))) {
		IDirectDraw2_Release(ddraw);
		MessageBox(win, "failed to allocate mode list", "fatal", MB_OK);
		return -1;
	}

	if(IDirectDraw2_EnumDisplayModes(ddraw, 0, 0, &modes_size, enum_modes) != 0) {
		MessageBox(win, "failed to enumerate available video modes", "fatal", MB_OK);
		free(gfx_modes);
		return -1;
	}

	printf("Found %d video modes:\n", gfx_num_modes);
	for(i=0; i<gfx_num_modes; i++) {
		printf(" - %dx%d %dbpp\n", gfx_modes[i].width, gfx_modes[i].height, gfx_modes[i].bpp);
	}

	memset(&caps, 0, sizeof caps);
	caps.dwSize = sizeof caps;
	if(IDirectDraw2_GetCaps(ddraw, &caps, 0) == 0) {
		unsigned long vmem, vmemfree;
		DDSCAPS ddscaps = {0};
		ddscaps.dwCaps = DDSCAPS_VIDEOMEMORY;
		if(IDirectDraw2_GetAvailableVidMem(ddraw, &ddscaps, &vmem, &vmemfree) != 0) {
			vmem = caps.dwVidMemTotal;
			vmemfree = caps.dwVidMemFree;
		}
		printf("video driver capabilities:\n");
		printf("  video RAM: %s ", memsizestr(vmem));
		printf("(free: %s)\n", memsizestr(vmemfree));
		printf("  blit: %s\n", (caps.dwCaps & DDCAPS_BLT) ? "yes" : "no");
		printf("  colorfill: %s\n", (caps.dwCaps & DDCAPS_BLTCOLORFILL) ? "yes" : "no");
		printf("  async blit: %s\n", (caps.dwCaps & DDCAPS_BLTQUEUE) ? "yes" : "no");
		printf("  scaling blit: %s\n", (caps.dwCaps & DDCAPS_BLTSTRETCH) ? "yes" : "no");
		printf("  blit from sys mem: %s\n", (caps.dwCaps & DDCAPS_CANBLTSYSMEM) ? "yes" : "no");
		printf("  colorkey: %s\n", (caps.dwCaps & DDCAPS_COLORKEY) ? "yes" : "no");
		if(caps.dwCaps & DDCAPS_COLORKEY) {
			printf("    srcblt: %s\n", (caps.dwCKeyCaps & DDCKEYCAPS_SRCBLT) ? "yes" : "no");
		}

		printf("  misc:");
		if(caps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM) printf(" nonlocalvmem");
		if(caps.dwCaps2 & DDCAPS2_NOPAGELOCKREQUIRED) printf(" nopagelock");
		putchar('\n');

		printf("  full 256c palette: %s\n", (caps.dwPalCaps & DDPCAPS_ALLOW256) ? "yes" : "no");
	}


	return 0;
}

static HRESULT WINAPI enum_modes(DDSURFACEDESC *sdesc, void *cls)
{
	struct gfxmode *mode;
	void *tmp;
	int newsz, maxsz = *(int*)cls;
	DDPIXELFORMAT *pf = &sdesc->ddpfPixelFormat;

	if(gfx_num_modes >= maxsz) {
		newsz = maxsz ? maxsz * 2 : 16;
		if(!(tmp = realloc(gfx_modes, newsz * sizeof *gfx_modes))) {
			return DDENUMRET_CANCEL;
		}
		gfx_modes = tmp;
		maxsz = *(int*)cls = newsz;
	}

	mode = gfx_modes + gfx_num_modes;
	memset(mode, 0, sizeof *mode);
	mode->width = sdesc->dwWidth;
	mode->height = sdesc->dwHeight;
	mode->pitch = sdesc->lPitch;
	mode->rate = sdesc->dwRefreshRate;

	if(pf->dwFlags & DDPF_PALETTEINDEXED8) {
		mode->bpp = 8;
	} else if(pf->dwFlags & DDPF_RGB) {
		mode->bpp = pf->dwRGBBitCount;
		mode->rmask = pf->dwRBitMask;
		mode->gmask = pf->dwGBitMask;
		mode->bmask = pf->dwBBitMask;
		mode->rshift = calc_mask_shift(mode->rmask);
		mode->gshift = calc_mask_shift(mode->gmask);
		mode->bshift = calc_mask_shift(mode->bmask);
	} else {
		return DDENUMRET_OK;	/* ignore unsupported bpp mode */
	}

	gfx_num_modes++;
	return DDENUMRET_OK;
}

static int calc_mask_shift(unsigned int mask)
{
	int shift = 0;

	if(!mask) return 0;

	while((mask & 1) == 0) {
		shift++;
		mask >>= 1;
	}
	return shift;
}

void gfx_destroy(void)
{
	if(fullscreen) {
		IDirectDraw2_RestoreDisplayMode(ddraw);
		IDirectDraw2_SetCooperativeLevel(ddraw, 0, DDSCL_NORMAL);
		ShowCursor(1);
	}
	if(clipper) {
		IDirectDrawClipper_Release(clipper);
		clipper = 0;
	}
	if(ddback) {
		IDirectDrawSurface_Release(ddback);
		ddback = 0;
	}
	if(ddfront) {
		IDirectDrawSurface_Release(ddfront);
		ddfront = 0;
	}
	if(ddpal) {
		IDirectDrawPalette_Release(ddpal);
		ddpal = 0;
	}
	if(ddraw) {
		IDirectDraw2_Release(ddraw);
		ddraw = 0;
	}
	free(gfx_front);
	gfx_front = gfx_back = 0;
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
	struct gfxmode *vm;

	if(modeidx < 0 || modeidx >= gfx_num_modes) {
		return -1;
	}
	vm = gfx_modes + modeidx;

	IDirectDraw2_SetCooperativeLevel(ddraw, win, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE |
			DDSCL_ALLOWMODEX | DDSCL_ALLOWREBOOT);

	if(IDirectDraw2_SetDisplayMode(ddraw, vm->width, vm->height, vm->bpp, vm->rate, 0) != 0) {
		return -1;
	}

	return 0;
}

int gfx_setup(int xsz, int ysz, int bpp, unsigned int flags)
{
	int i, fb_bpp;
	HRESULT res;
	DDSURFACEDESC sd = {0};
	DDSCAPS caps = {0};
	DDPIXELFORMAT pf;

	if(ddfront) {
		IDirectDrawSurface_Release(ddfront);
		ddfront = 0;
	}
	if(ddback) {
		IDirectDrawSurface_Release(ddback);
		ddback = 0;
	}
	if(ddpal) {
		IDirectDrawPalette_Release(ddpal);
		ddpal = 0;
	}
	if(clipper) {
		IDirectDrawClipper_Release(clipper);
		clipper = 0;
	}

	if(flags & GFX_FULLSCREEN) {
		sd.dwSize = sizeof sd;
		sd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		sd.dwBackBufferCount = 1;
		sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

		if(!(ddfront = create_ddsurf(&sd))) {
			MessageBox(win, "failed to create swap chain", "fatal", MB_OK);
			return -1;
		}

		caps.dwCaps = DDSCAPS_BACKBUFFER;
		if(IDirectDrawSurface_GetAttachedSurface(ddfront, &caps, &ddback) != 0) {
			MessageBox(win, "failed to get back buffer", "fatal", MB_OK);
			goto err;
		}

	} else {
		IDirectDraw2_SetCooperativeLevel(ddraw, 0, DDSCL_NORMAL);

		sd.dwSize = sizeof sd;
		sd.dwFlags = DDSD_CAPS;
		sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

		if(!(ddfront = create_ddsurf(&sd))) {
			MessageBox(win, "failed to create frontbuffer surface", "fatal", MB_OK);
			return -1;
		}

		sd.dwSize = sizeof sd;
		sd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
		sd.dwWidth = xsz;
		sd.dwHeight = ysz;
		sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;

		if(!(ddback = create_ddsurf(&sd))) {
			MessageBox(win, "failed to create backbuffer surface", "fatal", MB_OK);
			goto err;
		}
	}

	pf.dwSize = sizeof pf;
	IDirectDrawSurface_GetPixelFormat(ddfront, &pf);
	if(pf.dwFlags & DDPF_PALETTEINDEXED8) {
		fb_bpp = 8;
	} else if(pf.dwFlags & DDPF_RGB) {
		fb_bpp = pf.dwRGBBitCount;
	}

	for(i=0; i<2; i++) {
		gfx_front[i].width = xsz;
		gfx_front[i].height = ysz;
		gfx_front[i].flags = GFX_IMG_VIDMEM;
	}
	gfx_front->bpp = fb_bpp;
	gfx_front->data = ddfront;
	gfx_back->data = ddback;

	/* back buffer bpp is always the requested bpp. If it doesn't match the
	 * framebuffer bpp, then do pixel format conversion on swap buffers.
	 */
	gfx_back->bpp = bpp;
	if(fb_bpp != bpp) {
		if(gfx_imginit(gfx_back, xsz, ysz, bpp) == -1) {
			fprintf(stderr, "failed to create fake backbuffer surface\n");
			MessageBox(win, "failed to create fake backbuffer surface", "fatal", MB_OK);
			goto err;
		}
	}

	if(fb_bpp <= 8 || bpp <= 8) {
		/* if colormaps are involved at all, initialize the default palette */
		for(i=0; i<256; i++) {
			unsigned char c = i & 0xe0;
			pal[i].peRed = c | (c >> 3) | (c >> 6);
			c = (i << 3) & 0xe0;
			pal[i].peGreen = c | (c >> 3) | (c >> 6);
			c = (i << 5) & 0xc0;
			pal[i].peBlue = c | (c >> 2) | (c >> 4) | (c >> 6);
			pal[i].peFlags = PC_NOCOLLAPSE;
		}
	}

	if(fb_bpp <= 8 && !ddpal) {
		/* create direct draw palette and set it to the front surface */
		if((res = IDirectDraw2_CreatePalette(ddraw, DDPCAPS_8BIT |
				DDPCAPS_ALLOW256 | DDPCAPS_INITIALIZE, pal, &ddpal, 0)) != 0) {
			MessageBox(win, dderrstr(res), "Failed to create palette", MB_OK);
			goto err;
		}
		if(IDirectDrawSurface_SetPalette(ddfront, ddpal) != 0) {
			MessageBox(win, "failed to attach palette", "fatal", MB_OK);
			goto err;
		}
	}

	fullscreen = flags & GFX_FULLSCREEN;

	/* figure out initial client area */
	if(fullscreen) {
		RGNDATA *rgn;
		RECT *rect;

		client_xoffs = client_yoffs = 0;

		ShowCursor(0);

		if(!(rgn = malloc(sizeof(RGNDATAHEADER) + sizeof(RECT)))) {
			MessageBox(win, "failed to allocate clipping region", "error", MB_OK);
			goto err;
		}
		rect = (RECT*)rgn->Buffer;
		rect->left = rect->top = 0;
		rect->right = xsz;
		rect->bottom = ysz;

		rgn->rdh.dwSize = sizeof(RGNDATAHEADER) + sizeof(RECT);
		rgn->rdh.iType = RDH_RECTANGLES;
		rgn->rdh.nCount = 1;
		rgn->rdh.nRgnSize = sizeof(RECT);
		rgn->rdh.rcBound = *rect;

		if((res = IDirectDraw2_CreateClipper(ddraw, 0, &clipper, 0)) != 0) {
			MessageBox(win, dderrstr(res), "failed to create clipper", MB_OK);
			goto err;
		}
		IDirectDrawClipper_SetClipList(clipper, rgn, 0);
		IDirectDrawSurface_SetClipper(ddback, clipper);
	} else {
		/* for windowed we need to compute the client area offset */
		int ws = GetWindowLong(win, GWL_STYLE);
		RECT rect;
		rect.left = rect.top = 0;
		rect.right = xsz;
		rect.bottom = ysz;
		AdjustWindowRect(&rect, ws, 0);
		client_xoffs = -rect.left;
		client_yoffs = -rect.top;

		ShowCursor(1);

		if((res = IDirectDraw2_CreateClipper(ddraw, 0, &clipper, 0)) != 0) {
			MessageBox(win, dderrstr(res), "failed to create clipper", MB_OK);
			goto err;
		}
		IDirectDrawClipper_SetHWnd(clipper, 0, win);
		IDirectDrawSurface_SetClipper(ddback, clipper);
	}

	return 0;

err:
	if(clipper) {
		IDirectDrawClipper_Release(clipper);
		clipper = 0;
	}
	if(ddpal) {
		IDirectDrawPalette_Release(ddpal);
		ddpal = 0;
	}
	if(ddback) {
		IDirectDrawSurface_Release(ddback);
		ddback = 0;
	}
	if(ddfront) {
		IDirectDrawSurface_Release(ddfront);
		ddfront = 0;
	}
	return -1;
}

void gfx_setcolor(int idx, int r, int g, int b)
{
	pal[idx].peRed = r;
	pal[idx].peGreen = g;
	pal[idx].peBlue= b;
	if(ddpal) {
		IDirectDrawPalette_SetEntries(ddpal, 0, idx, 1, pal + idx);
	}
}

void gfx_setcolors(int start, int count, struct gfxcolor *colors)
{
	int i, idx;
	for(i=0; i<count; i++) {
		idx = start + i;
		pal[idx].peRed = colors->r;
		pal[idx].peGreen = colors->g;
		pal[idx].peBlue = colors->b;
		colors++;
	}
	if(ddpal) {
		IDirectDrawPalette_SetEntries(ddpal, 0, start, count, pal + start);
	}
}

int gfx_imginit(struct gfximage *img, int x, int y, int bpp)
{
	DDSURFACEDESC ddsd = {0};
	DDSurface *surf;

	ddsd.dwSize = sizeof ddsd;
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwWidth = x;
	ddsd.dwHeight = y;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;

	if(!(surf = create_ddsurf(&ddsd))) {
		return -1;
	}

	memset(img, 0, sizeof *img);
	img->width = x;
	img->height = y;
	img->bpp = bpp;
	img->data = surf;
	return 0;
}

void gfx_imgdestroy(struct gfximage *img)
{
	IDirectDrawSurface *surf = img->data;
	if(surf) {
		IDirectDrawSurface_Release(surf);
		img->data = 0;
	}
}

/* call imgstart to get a pointer to the pixel buffer, and imgend when done */
void *gfx_imgstart(struct gfximage *img)
{
	if(img->data) {
		img->pixels = ddlocksurf(img->data, &img->pitch);
	}
	return img->pixels;
}

void gfx_imgend(struct gfximage *img)
{
	if(img->data) {
		IDirectDrawSurface *surf = img->data;
		IDirectDrawSurface_Unlock(surf, 0);
		img->pixels = 0;
	}
}

void gfx_imgdebug(struct gfximage *img)
{
	if(img->data) {
		IDirectDrawSurface *surf = img->data;
		DDSURFACEDESC ddsd = {0};

		ddsd.dwSize = sizeof ddsd;
		if(IDirectDrawSurface_GetSurfaceDesc(surf, &ddsd) != 0) {
			printf("imgdebug: GetSurfaceDesc failed\n");
			return;
		}

		printf("ddsurf %dx%d (%s)\n", (int)ddsd.dwWidth, (int)ddsd.dwHeight,
				ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY ? "vmem" : "mainmem");
	} else {
		printf("nodd %dx%d\n", img->width, img->height);
	}
}

void gfx_fill(struct gfximage *img, unsigned int color, struct gfxrect *rect)
{
	RECT r, *rp = 0;
	DDBLTFX fx = {0};
	DDSurface *surf = img->data;

	if(!img->data) {
		gfx_swfill(img, color, rect);
		return;
	}

	if(rect) {
		rp = &r;
		r.left = rect->x;
		r.top = rect->y;
		r.right = rect->x + rect->width;
		r.bottom = rect->y + rect->height;
	}

	fx.dwSize = sizeof fx;
	fx.dwFillPixel = color;

	ddblit(surf, rp, 0, 0, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
}

/* set which color to be used as a colorkey for transparent blits */
void gfx_imgkey(struct gfximage *img, int ckey)
{
	DDCOLORKEY ddck;
	IDirectDrawSurface *surf = img->data;

	if(ckey >= 0) {
		ddck.dwColorSpaceLowValue = ddck.dwColorSpaceHighValue = ckey;
		IDirectDrawSurface_SetColorKey(surf, DDCKEY_SRCBLT, &ddck);
	}
	img->ckey = ckey;
}

void gfx_blit(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect)
{
	RECT dr, sr;
	DDSurface *ddsrc = src->data;

	if(!dest->data) {
		gfx_swblit(dest, x, y, src, srect);
		return;
	}

	if(srect) {
		sr.left = srect->x;
		sr.top = srect->y;
		sr.right = srect->x + srect->width;
		sr.bottom = srect->y + srect->height;
	} else {
		sr.left = sr.top = 0;
		sr.right = src->width;
		sr.bottom = src->height;
	}

	dr.left = x;
	dr.top = y;
	dr.right = x + sr.right - sr.left;
	dr.bottom = y + sr.bottom - sr.top;

#ifdef USE_DX5
	if(!(dest->flags & GFX_IMG_VIDMEM)) {
		IDirectDrawSurface3_PageLock(ddsrc, 0);
		ddblit(dest->data, &dr, ddsrc, &sr, DDBLT_WAIT, 0);
		IDirectDrawSurface3_PageUnlock(ddsrc, 0);
	} else
#endif
	ddblit(dest->data, &dr, ddsrc, &sr, DDBLT_WAIT, 0);
}

void gfx_blitkey(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect)
{
	RECT sr, dr;
	DDSurface *ddsrc = src->data;

	if(!dest->data) {
		gfx_swblitkey(dest, x, y, src, srect);
		return;
	}

	if(srect) {
		sr.left = srect->x;
		sr.top = srect->y;
		sr.right = srect->x + srect->width;
		sr.bottom = srect->y + srect->height;
	} else {
		sr.left = sr.top = 0;
		sr.right = src->width;
		sr.bottom = src->height;
	}

	dr.left = x;
	dr.top = y;
	dr.right = x + sr.right - sr.left;
	dr.bottom = y + sr.bottom - sr.top;

#ifdef USE_DX5
	if(!(dest->flags & GFX_IMG_VIDMEM)) {
		IDirectDrawSurface3_PageLock(ddsrc, 0);
		ddblit(dest->data, &dr, ddsrc, &sr, DDBLT_WAIT | DDBLT_KEYSRC, 0);
		IDirectDrawSurface3_PageUnlock(ddsrc, 0);
	} else
#endif
	ddblit(dest->data, &dr, ddsrc, &sr, DDBLT_WAIT | DDBLT_KEYSRC, 0);
}

void gfx_swapbuffers(int vsync)
{
	RECT rect;

	if(fullscreen) {
		ddflip(ddfront);
		return;
	}

	/* windowed swap */
	if(vsync) {
		gfx_waitvsync();
	}

	if(gfx_front->bpp != gfx_back->bpp) {
		int i, j;
		void *dpixels;
		unsigned char *dptr8, *sptr;
		uint32_t *dptr32;
		unsigned int dpitch;
		PALETTEENTRY *col;

		if(!gfx_imgstart(gfx_back)) return;

		sptr = gfx_back->pixels;
		dpixels = ddlocksurf(ddback, &dpitch);

		switch(gfx_front->bpp) {
		case 24:
			for(i=0; i<gfx_front->height; i++) {
				dptr8 = dpixels;
				for(j=0; j<gfx_front->width; j++) {
					col = pal + sptr[j];
					dptr8[0] = col->peBlue;
					dptr8[1] = col->peGreen;
					dptr8[2] = col->peRed;
					dptr8 += 3;
				}
				dpixels = (char*)dpixels + dpitch;
				sptr += gfx_back->pitch;
			}
			break;

		case 32:
			dptr32 = dpixels;
			for(i=0; i<gfx_front->height; i++) {
				for(j=0; j<gfx_front->width; j++) {
					col = pal + sptr[j];
					dptr32[j] = GFX_PACK32(col->peRed, col->peGreen, col->peBlue);
				}
				dptr32 = (uint32_t*)((char*)dptr32 + dpitch);
				sptr += gfx_back->pitch;
			}
			break;

		default:
			/* TODO more formats */
			break;
		}
		IDirectDrawSurface_Unlock(ddback, 0);
		gfx_imgend(gfx_back);
	}

	GetWindowRect(win, &rect);
#ifdef USE_DX5
	if(!(gfx_back->flags & GFX_IMG_VIDMEM)) {
		IDirectDrawSurface3_PageLock(ddback, 0);
		ddblitfast(ddfront, rect.left + client_xoffs, rect.top + client_yoffs,
				ddback, 0, DDBLTFAST_WAIT);
		IDirectDrawSurface3_PageUnlock(ddback, 0);
	} else
#endif
	ddblitfast(ddfront, rect.left + client_xoffs, rect.top + client_yoffs,
			ddback, 0, DDBLTFAST_WAIT);
}

void gfx_waitvsync(void)
{
	IDirectDraw2_WaitForVerticalBlank(ddraw, DDWAITVB_BLOCKBEGIN, 0);
}


/* TODO list for all the software fallback functions
 *  - currently assume all source images and colors are 8bpp, make it generic
 *  - RLE colorkey blits
 *  - pre-pack dest-bpp packed colors for all palette entries
 *  - faster clipping with pre-computing bounds instead of per-pixel checks
 */

static void gfx_swfill(struct gfximage *img, unsigned int color, struct gfxrect *rect)
{
	unsigned char *pptr;
	uint32_t *pptr32;
	int i, j, width, height;
	PALETTEENTRY *col;

	assert(img->pixels);

	pptr = img->pixels;
	if(rect) {
		pptr += img->pitch * rect->y + rect->x;
		width = rect->width;
		height = rect->height;
	} else {
		width = img->width;
		height = img->height;
	}

	switch(img->bpp) {
	case 8:
		for(i=0; i<height; i++) {
			memset(pptr, color, width);
			pptr += img->pitch;
		}
		break;

	case 32:
		pptr32 = (uint32_t*)pptr;
		col = pal + color;
		color = GFX_PACK32(col->peRed, col->peGreen, col->peBlue);
		for(i=0; i<height; i++) {
			for(j=0; j<width; j++) {
				pptr32[j] = color;
			}
			pptr32 = (uint32_t*)((char*)pptr32 + img->pitch);
		}
		break;

	default:
		abort();	/* not implemented yet */
	}
}

static void gfx_swblit(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect)
{
	int i, j, width, height, locked = 0;
	unsigned char *dptr, *sptr;
	uint32_t *dptr32;
	PALETTEENTRY *col;

	assert(dest->pixels);

	if(!src->pixels) {
		if(!gfx_imgstart(src)) return;
		locked = 1;
	}

	sptr = src->pixels;
	if(srect) {
		sptr += src->pitch * srect->y + srect->x;
		width = srect->width;
		height = srect->height;
	} else {
		width = src->width;
		height = src->height;
	}

	dptr = (unsigned char*)dest->pixels + y * dest->pitch + x;

	switch(dest->bpp) {
	case 8:
		for(i=0; i<height; i++) {
			memcpy(dptr, sptr, width);
			dptr += dest->pitch;
			sptr += src->pitch;
		}
		break;

	case 32:
		dptr32 = (uint32_t*)dptr;
		for(i=0; i<height; i++) {
			for(j=0; j<width; j++) {
				col = pal + sptr[j];
				dptr32[j] = GFX_PACK32(col->peRed, col->peGreen, col->peBlue);
			}
			dptr32 = (uint32_t*)((char*)dptr32 + dest->pitch);
			sptr += src->pitch;
		}
		break;
	}

	if(locked) {
		gfx_imgend(src);
	}
}

static void gfx_swblitkey(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect)
{
	int i, j, width, height, locked = 0;
	unsigned char *dptr, *sptr;
	uint32_t *dptr32;
	PALETTEENTRY *col;

	assert(dest->pixels);

	if(!src->pixels) {
		if(!gfx_imgstart(src)) return;
		locked = 1;
	}

	sptr = src->pixels;
	if(srect) {
		sptr += src->pitch * srect->y + srect->x;
		width = srect->width;
		height = srect->height;
	} else {
		width = src->width;
		height = src->height;
	}

	dptr = (unsigned char*)dest->pixels + y * dest->pitch + x;

	switch(dest->bpp) {
	case 8:
		for(i=0; i<height; i++) {
			if(y >= dest->height) break;
			if(y >= 0) {
				for(j=0; j<width; j++) {
					if(x + j < 0) continue;
					if(x + j >= dest->width) break;
					if(sptr[j] == src->ckey) continue;
					dptr[j] = sptr[j];
				}
			}
			dptr += dest->pitch;
			sptr += src->pitch;
			y++;
		}
		break;

	case 32:
		dptr32 = (uint32_t*)dptr;
		for(i=0; i<height; i++) {
			if(y >= dest->height) break;
			if(y >= 0) {
				for(j=0; j<width; j++) {
					if(x + j < 0) continue;
					if(x + j >= dest->width) break;
					if(sptr[j] == src->ckey) continue;
					col = pal + sptr[j];
					dptr32[j] = GFX_PACK32(col->peRed, col->peGreen, col->peBlue);
				}
			}
			dptr32 = (uint32_t*)((char*)dptr32 + dest->pitch);
			sptr += src->pitch;
			y++;
		}
		break;
	}

	if(locked) {
		gfx_imgend(src);
	}
}
