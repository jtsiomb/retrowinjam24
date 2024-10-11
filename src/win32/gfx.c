#define INITGUID
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <windows.h>
#include <ddraw.h>
#include "gfx.h"
#include "options.h"
#include "types.h"

struct gfxmode *gfx_modes, *gfx_curmode;
int gfx_num_modes;
struct gfximage *gfx_front, *gfx_back;

extern HWND win;

static int fullscreen;
static IDirectDraw2 *dd;
static IDirectDrawSurface2 *fbsurf, *fbback;
static IDirectDrawClipper *clipper;
static IDirectDrawPalette *ddpal;
static PALETTEENTRY pal[256];
static int client_xoffs, client_yoffs;

static HRESULT WINAPI enum_modes(DDSURFACEDESC *sdesc, void *cls);
static int calc_mask_shift(unsigned int mask);
static IDirectDrawSurface2 *create_ddsurf2(DDSURFACEDESC *ddsd);
static const char *dderrstr(HRESULT err);

int gfx_init(void)
{
	int i, modes_size;
	IDirectDraw *dd1;

	if(!(gfx_front = calloc(2, sizeof *gfx_front))) {
		MessageBox(win, "failed to allocate memory", "fatal", MB_OK);
		return -1;
	}
	gfx_back = gfx_front + 1;

	if(DirectDrawCreate(0, &dd1, 0) != 0) {
		MessageBox(win, "failed to create DirectDraw instance", "fatal", MB_OK);
		return -1;
	}
	if(IDirectDraw_QueryInterface(dd1, &IID_IDirectDraw2, &dd) != 0) {
		MessageBox(win, "failed to query DirectDraw2 interface", "fatal", MB_OK);
		return -1;
	}
	IDirectDraw_Release(dd1);

	gfx_num_modes = 0;
	modes_size = 4;
	if(!(gfx_modes = malloc(modes_size * sizeof *gfx_modes))) {
		MessageBox(win, "failed to allocate mode list", "fatal", MB_OK);
		return -1;
	}

	if(IDirectDraw2_EnumDisplayModes(dd, 0, 0, &modes_size, enum_modes) != 0) {
		MessageBox(win, "failed to enumerate available video modes", "fatal", MB_OK);
		free(gfx_modes);
		return -1;
	}

	printf("Found %d video modes:\n", gfx_num_modes);
	for(i=0; i<gfx_num_modes; i++) {
		printf(" - %dx%d %dbpp\n", gfx_modes[i].width, gfx_modes[i].height, gfx_modes[i].bpp);
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
	if(clipper) {
		IDirectDrawClipper_Release(clipper);
		clipper = 0;
	}
	if(fbback) {
		IDirectDrawSurface2_Release(fbback);
		fbback = 0;
	}
	if(fbsurf) {
		IDirectDrawSurface2_Release(fbsurf);
		fbsurf = 0;
	}
	if(ddpal) {
		IDirectDrawPalette_Release(ddpal);
		ddpal = 0;
	}
	if(dd) {
		IDirectDraw2_Release(dd);
		dd = 0;
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

	IDirectDraw2_SetCooperativeLevel(dd, win, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE |
			DDSCL_ALLOWMODEX | DDSCL_ALLOWREBOOT);

	if(IDirectDraw2_SetDisplayMode(dd, vm->width, vm->height, vm->bpp, vm->rate, 0) != 0) {
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

	if(fbsurf) {
		IDirectDrawSurface2_Release(fbsurf);
		fbsurf = 0;
	}
	if(fbback) {
		IDirectDrawSurface2_Release(fbback);
		fbback = 0;
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

		if(!(fbsurf = create_ddsurf2(&sd))) {
			return -1;
		}

		caps.dwCaps = DDSCAPS_BACKBUFFER;
		if(IDirectDrawSurface2_GetAttachedSurface(fbsurf, &caps, &fbback) != 0) {
			MessageBox(win, "failed to get back buffer", "fatal", MB_OK);
			goto err;
		}

	} else {
		IDirectDraw2_SetCooperativeLevel(dd, 0, DDSCL_NORMAL);

		sd.dwSize = sizeof sd;
		sd.dwFlags = DDSD_CAPS;
		sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

		if(!(fbsurf = create_ddsurf2(&sd))) {
			return -1;
		}

		sd.dwSize = sizeof sd;
		sd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
		sd.dwWidth = xsz;
		sd.dwHeight = ysz;
		sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;// | DDSCAPS_VIDEOMEMORY;
		if(!(fbback = create_ddsurf2(&sd))) {
			goto err;
		}
	}

	pf.dwSize = sizeof pf;
	IDirectDrawSurface2_GetPixelFormat(fbsurf, &pf);
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
	gfx_front->data = fbsurf;
	gfx_back->data = fbback;

	/* back buffer bpp is always the requested bpp. If it doesn't match the
	 * framebuffer bpp, then do pixel format conversion on swap buffers.
	 */
	gfx_back->bpp = bpp;
	if(fb_bpp != bpp) {
		if(!(gfx_back->pixels = malloc(xsz * ysz * bpp / 8))) {
			fprintf(stderr, "failed to allocate fake backbuffer\n");
			MessageBox(win, "failed to allocate fake backbuffer", "fatal", MB_OK);
			goto err;
		}
		gfx_back->pitch = xsz * bpp / 8;
		gfx_back->flags = 0;	/* not in vidmem */
		gfx_back->data = 0;		/* no corresponding direct draw surface */
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
		if((res = IDirectDraw2_CreatePalette(dd, DDPCAPS_8BIT | DDPCAPS_ALLOW256, pal, &ddpal, 0)) != 0) {
			MessageBox(win, dderrstr(res), "Failed to create palette", MB_OK);
			goto err;
		}
		if(IDirectDrawSurface2_SetPalette(fbsurf, ddpal) != 0) {
			MessageBox(win, "failed to attach palette", "fatal", MB_OK);
			goto err;
		}
	}

	if((res = IDirectDraw2_CreateClipper(dd, 0, &clipper, 0)) != 0) {
		MessageBox(win, dderrstr(res), "failed to create clipper", MB_OK);
		goto err;
	}
	IDirectDrawClipper_SetHWnd(clipper, 0, win);
	IDirectDrawSurface2_SetClipper(fbsurf, clipper);

	fullscreen = flags & GFX_FULLSCREEN;

	/* figure out initial client area */
	if(fullscreen) {
		client_xoffs = client_yoffs = 0;
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
	if(fbback) {
		IDirectDrawSurface2_Release(fbback);
		fbback = 0;
	}
	if(fbsurf) {
		IDirectDrawSurface2_Release(fbsurf);
		fbsurf = 0;
	}
	return -1;
}

void gfx_setcolor(int idx, int r, int g, int b)
{
	pal[idx].peRed = r;
	pal[idx].peGreen = g;
	pal[idx].peBlue= b;
	if(ddpal) {
		IDirectDrawPalette_SetEntries(ddpal, 0, idx, 1, pal);
	}
}

void gfx_setcolors(int start, int count, struct gfxcolor *colors)
{
	int i;
	for(i=0; i<count; i++) {
		pal[i].peRed = colors->r;
		pal[i].peGreen = colors->g;
		pal[i].peBlue = colors->b;
	}
	if(ddpal) {
		IDirectDrawPalette_SetEntries(ddpal, 0, start, count, pal);
	}
}

static void *lock_ddsurf(IDirectDrawSurface2 *surf, long *pitchret)
{
	DDSURFACEDESC sd;
	sd.dwSize = sizeof sd;
	if(IDirectDrawSurface2_Lock(surf, 0, &sd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0) != 0) {
		return 0;
	}
	*pitchret = sd.lPitch;
	return sd.lpSurface;
}

/* call imgstart to get a pointer to the pixel buffer, and imgend when done */
void *gfx_imgstart(struct gfximage *img)
{
	if(img->data) {
		img->pixels = lock_ddsurf(img->data, &img->pitch);
	}
	return img->pixels;
}

void gfx_imgend(struct gfximage *img)
{
	if(img->data) {
		IDirectDrawSurface2 *surf = img->data;
		IDirectDrawSurface2_Unlock(surf, 0);
	}
}

void gfx_fill(struct gfximage *img, unsigned int color, struct gfxrect *rect)
{
	if(img->data) {
		RECT r, *rp = 0;
		DDBLTFX fx = {0};
		IDirectDrawSurface2 *surf = img->data;

		if(rect) {
			rp = &r;
			r.left = rect->x;
			r.top = rect->y;
			r.right = rect->x + rect->width;
			r.bottom = rect->y + rect->height;
		}

		fx.dwSize = sizeof fx;
		fx.dwFillPixel = color;
		IDirectDrawSurface2_Blt(fbback, rp, 0, 0, DDBLT_COLORFILL | DDBLT_WAIT, &fx);

	} else {
		unsigned char *pptr;
		int i, width, height;

		assert(img->pixels);

		/* TODO: support different pixel formats */
		pptr = img->pixels;
		if(rect) {
			pptr += img->pitch * rect->y + rect->x;
			width = rect->width;
			height = rect->height;
		} else {
			width = img->width;
			height = img->height;
		}

		for(i=0; i<height; i++) {
			memset(pptr, color, width);
			pptr += img->pitch;
		}
	}
}

/* set which color to be used as a colorkey for transparent blits */
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
	RECT rect;

	if(fullscreen) {
		//printf("before flip (%p)\n", fbsurf);
		IDirectDrawSurface2_Flip(fbsurf, 0, DDFLIP_WAIT);
		//printf("after flip\n");
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
		long dpitch;
		PALETTEENTRY *col;

		assert(gfx_back->data == 0);	/* there should be no dd surf for gfx_back */
		assert(gfx_back->pixels);		/* we should have allocated a pixel buffer */

		sptr = gfx_back->pixels;
		dpixels = lock_ddsurf(fbback, &dpitch);

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
		IDirectDrawSurface2_Unlock(fbback, 0);
	}

	GetWindowRect(win, &rect);
	rect.left += client_xoffs;
	rect.right += client_xoffs;
	rect.top += client_yoffs;
	rect.bottom += client_yoffs;
	IDirectDrawSurface2_Blt(fbsurf, &rect, fbback, 0, DDBLT_WAIT, 0);
}

void gfx_waitvsync(void)
{
	IDirectDraw2_WaitForVerticalBlank(dd, DDWAITVB_BLOCKBEGIN, 0);
}

static IDirectDrawSurface2 *create_ddsurf2(DDSURFACEDESC *ddsd)
{
	HRESULT res;
	IDirectDrawSurface *surf1;
	IDirectDrawSurface2 *surf;

	if((res = IDirectDraw2_CreateSurface(dd, ddsd, &surf1, 0)) != 0) {
		fprintf(stderr, "failed to create surface\n");
		MessageBox(win, dderrstr(res), "failed to create surface", MB_OK);
		return 0;
	}
	if(IDirectDrawSurface_QueryInterface(surf1, &IID_IDirectDrawSurface2, &surf) != 0) {
		IDirectDrawSurface_Release(surf1);
		fprintf(stderr, "failed to query IDirectDrawSurface2 interface\n");
		MessageBox(win, "failed to query IDirectDrawSurface2 interface", "fatal", MB_OK);
		return 0;
	}
	IDirectDrawSurface_Release(surf1);
	return surf;
}

static const char *dderrstr(HRESULT err)
{
	switch(err) {
	case DD_OK: return "DD_OK";
	case DDERR_ALREADYINITIALIZED:
	case DDERR_BLTFASTCANTCLIP: return "DDERR_BLTFASTCANTCLIP";
	case DDERR_CANNOTATTACHSURFACE: return "DDERR_CANNOTATTACHSURFACE";
	case DDERR_CANNOTDETACHSURFACE: return "DDERR_CANNOTDETACHSURFACE";
	case DDERR_CANTCREATEDC: return "DDERR_CANTCREATEDC";
	case DDERR_CANTDUPLICATE: return "DDERR_CANTDUPLICATE";
	case DDERR_CANTLOCKSURFACE: return "DDERR_CANTLOCKSURFACE";
	case DDERR_CANTPAGELOCK: return "DDERR_CANTPAGELOCK";
	case DDERR_CANTPAGEUNLOCK: return "DDERR_CANTPAGEUNLOCK";
	case DDERR_CLIPPERISUSINGHWND: return "DDERR_CLIPPERISUSINGHWND";
	case DDERR_COLORKEYNOTSET: return "DDERR_COLORKEYNOTSET";
	case DDERR_CURRENTLYNOTAVAIL: return "DDERR_CURRENTLYNOTAVAIL";
	case DDERR_DCALREADYCREATED: return "DDERR_DCALREADYCREATED";
	case DDERR_DEVICEDOESNTOWNSURFACE: return "DDERR_DEVICEDOESNTOWNSURFACE";
	case DDERR_DIRECTDRAWALREADYCREATED: return "DDERR_DIRECTDRAWALREADYCREATED";
	case DDERR_EXCEPTION: return "DDERR_EXCEPTION";
	case DDERR_EXCLUSIVEMODEALREADYSET: return "DDERR_EXCLUSIVEMODEALREADYSET";
	case DDERR_GENERIC: return "DDERR_GENERIC";
	case DDERR_HEIGHTALIGN: return "DDERR_HEIGHTALIGN";
	case DDERR_HWNDALREADYSET: return "DDERR_HWNDALREADYSET";
	case DDERR_HWNDSUBCLASSED: return "DDERR_HWNDSUBCLASSED";
	case DDERR_IMPLICITLYCREATED: return "DDERR_IMPLICITLYCREATED";
	case DDERR_INCOMPATIBLEPRIMARY: return "DDERR_INCOMPATIBLEPRIMARY";
	case DDERR_INVALIDCAPS: return "DDERR_INVALIDCAPS";
	case DDERR_INVALIDCLIPLIST: return "DDERR_INVALIDCLIPLIST";
	case DDERR_INVALIDDIRECTDRAWGUID: return "DDERR_INVALIDDIRECTDRAWGUID";
	case DDERR_INVALIDMODE: return "DDERR_INVALIDMODE";
	case DDERR_INVALIDOBJECT: return "DDERR_INVALIDOBJECT";
	case DDERR_INVALIDPARAMS: return "DDERR_INVALIDPARAMS";
	case DDERR_INVALIDPIXELFORMAT: return "DDERR_INVALIDPIXELFORMAT";
	case DDERR_INVALIDPOSITION: return "DDERR_INVALIDPOSITION";
	case DDERR_INVALIDRECT: return "DDERR_INVALIDRECT";
	case DDERR_INVALIDSURFACETYPE: return "DDERR_INVALIDSURFACETYPE";
	case DDERR_LOCKEDSURFACES: return "DDERR_LOCKEDSURFACES";
	case DDERR_MOREDATA: return "DDERR_MOREDATA";
	case DDERR_NO3D: return "DDERR_NO3D";
	case DDERR_NOALPHAHW: return "DDERR_NOALPHAHW";
	case DDERR_NOBLTHW: return "DDERR_NOBLTHW";
	case DDERR_NOCLIPLIST: return "DDERR_NOCLIPLIST";
	case DDERR_NOCLIPPERATTACHED: return "DDERR_NOCLIPPERATTACHED";
	case DDERR_NOCOLORCONVHW: return "DDERR_NOCOLORCONVHW";
	case DDERR_NOCOLORKEY: return "DDERR_NOCOLORKEY";
	case DDERR_NOCOLORKEYHW: return "DDERR_NOCOLORKEYHW";
	case DDERR_NOCOOPERATIVELEVELSET: return "DDERR_NOCOOPERATIVELEVELSET";
	case DDERR_NODC: return "DDERR_NODC";
	case DDERR_NODDROPSHW: return "DDERR_NODDROPSHW";
	case DDERR_NODIRECTDRAWHW: return "DDERR_NODIRECTDRAWHW";
	case DDERR_NODIRECTDRAWSUPPORT: return "DDERR_NODIRECTDRAWSUPPORT";
	case DDERR_NOEMULATION: return "DDERR_NOEMULATION";
	case DDERR_NOEXCLUSIVEMODE: return "DDERR_NOEXCLUSIVEMODE";
	case DDERR_NOFLIPHW: return "DDERR_NOFLIPHW";
	case DDERR_NOFOCUSWINDOW: return "DDERR_NOFOCUSWINDOW";
	case DDERR_NOGDI: return "DDERR_NOGDI";
	case DDERR_NOHWND: return "DDERR_NOHWND";
	case DDERR_NOMIPMAPHW: return "DDERR_NOMIPMAPHW";
	case DDERR_NOMIRRORHW: return "DDERR_NOMIRRORHW";
	case DDERR_NONONLOCALVIDMEM: return "DDERR_NONONLOCALVIDMEM";
	case DDERR_NOOPTIMIZEHW: return "DDERR_NOOPTIMIZEHW";
	case DDERR_NOOVERLAYDEST: return "DDERR_NOOVERLAYDEST";
	case DDERR_NOOVERLAYHW: return "DDERR_NOOVERLAYHW";
	case DDERR_NOPALETTEATTACHED: return "DDERR_NOPALETTEATTACHED";
	case DDERR_NOPALETTEHW: return "DDERR_NOPALETTEHW";
	case DDERR_NORASTEROPHW: return "DDERR_NORASTEROPHW";
	case DDERR_NOROTATIONHW: return "DDERR_NOROTATIONHW";
	case DDERR_NOSTRETCHHW: return "DDERR_NOSTRETCHHW";
	case DDERR_NOT4BITCOLOR: return "DDERR_NOT4BITCOLOR";
	case DDERR_NOT4BITCOLORINDEX: return "DDERR_NOT4BITCOLORINDEX";
	case DDERR_NOT8BITCOLOR: return "DDERR_NOT8BITCOLOR";
	case DDERR_NOTAOVERLAYSURFACE: return "DDERR_NOTAOVERLAYSURFACE";
	case DDERR_NOTEXTUREHW: return "DDERR_NOTEXTUREHW";
	case DDERR_NOTFLIPPABLE: return "DDERR_NOTFLIPPABLE";
	case DDERR_NOTFOUND: return "DDERR_NOTFOUND";
	case DDERR_NOTINITIALIZED: return "DDERR_NOTINITIALIZED";
	case DDERR_NOTLOADED: return "DDERR_NOTLOADED";
	case DDERR_NOTLOCKED: return "DDERR_NOTLOCKED";
	case DDERR_NOTPAGELOCKED: return "DDERR_NOTPAGELOCKED";
	case DDERR_NOTPALETTIZED: return "DDERR_NOTPALETTIZED";
	case DDERR_NOVSYNCHW: return "DDERR_NOVSYNCHW";
	case DDERR_NOZBUFFERHW: return "DDERR_NOZBUFFERHW";
	case DDERR_NOZOVERLAYHW: return "DDERR_NOZOVERLAYHW";
	case DDERR_OUTOFCAPS: return "DDERR_OUTOFCAPS";
	case DDERR_OUTOFMEMORY: return "DDERR_OUTOFMEMORY";
	case DDERR_OUTOFVIDEOMEMORY: return "DDERR_OUTOFVIDEOMEMORY";
	case DDERR_OVERLAYCANTCLIP: return "DDERR_OVERLAYCANTCLIP";
	case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: return "DDERR_OVERLAYCOLORKEYONLYONEACTIVE";
	case DDERR_OVERLAYNOTVISIBLE: return "DDERR_OVERLAYNOTVISIBLE";
	case DDERR_PALETTEBUSY: return "DDERR_PALETTEBUSY";
	case DDERR_PRIMARYSURFACEALREADYEXISTS: return "DDERR_PRIMARYSURFACEALREADYEXISTS";
	case DDERR_REGIONTOOSMALL: return "DDERR_REGIONTOOSMALL";
	case DDERR_SURFACEALREADYATTACHED: return "DDERR_SURFACEALREADYATTACHED";
	case DDERR_SURFACEALREADYDEPENDENT: return "DDERR_SURFACEALREADYDEPENDENT";
	case DDERR_SURFACEBUSY: return "DDERR_SURFACEBUSY";
	case DDERR_SURFACEISOBSCURED: return "DDERR_SURFACEISOBSCURED";
	case DDERR_SURFACELOST: return "DDERR_SURFACELOST";
	case DDERR_SURFACENOTATTACHED: return "DDERR_SURFACENOTATTACHED";
	case DDERR_TOOBIGHEIGHT: return "DDERR_TOOBIGHEIGHT";
	case DDERR_TOOBIGSIZE: return "DDERR_TOOBIGSIZE";
	case DDERR_TOOBIGWIDTH: return "DDERR_TOOBIGWIDTH";
	case DDERR_UNSUPPORTED: return "DDERR_UNSUPPORTED";
	case DDERR_UNSUPPORTEDFORMAT: return "DDERR_UNSUPPORTEDFORMAT";
	case DDERR_UNSUPPORTEDMASK: return "DDERR_UNSUPPORTEDMASK";
	case DDERR_UNSUPPORTEDMODE: return "DDERR_UNSUPPORTEDMODE";
	case DDERR_VERTICALBLANKINPROGRESS: return "DDERR_VERTICALBLANKINPROGRESS";
	case DDERR_VIDEONOTACTIVE: return "DDERR_VIDEONOTACTIVE";
	case DDERR_WASSTILLDRAWING: return "DDERR_WASSTILLDRAWING";
	case DDERR_WRONGMODE: return "DDERR_WRONGMODE";
	case DDERR_XALIGN: return "DDERR_XALIGN";
	default:
		break;
	}
	return "<unknown>";
}
