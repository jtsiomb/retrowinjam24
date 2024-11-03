#ifndef GFX_H_
#define GFX_H_

#define GFX_PACK32(r, g, b)		(((r) << 16) | ((g) << 8) | (b))

enum {
	GFX_IMG_VIDMEM	= 1
};

/* gfx_setup flags */
enum { GFX_WINDOWED, GFX_FULLSCREEN };

struct gfxcolor {
	unsigned char r, g, b;
};

struct gfximage {
	int width, height, bpp;
	unsigned int pitch;			/* scanline size in bytes */
	unsigned int flags;
	unsigned int rmask, gmask, bmask;
	int rshift, gshift, bshift;
	void *pixels;
	struct gfxcolor cmap[256];
	int ncolors;				/* only for palettized images */
	int ckey;					/* transparent color (-1 when not set) */
	void *data;					/* extra implementation data */
};

struct gfxrect {
	int x, y;
	int width, height;
};

struct gfxmode {
	unsigned int modeno;		/* hardware mode number */
	int width, height, bpp;
	int rate;					/* refresh rate */
	unsigned int pitch;
	unsigned int rmask, gmask, bmask;
	int rshift, gshift, bshift;
};

/* populated by gfx_init */
extern struct gfxmode *gfx_modes, *gfx_curmode;
extern int gfx_num_modes;
extern struct gfximage *gfx_front, *gfx_back;

int gfx_init(void);
void gfx_destroy(void);

int gfx_findmode(int xsz, int ysz, int bpp, int rate);
int gfx_setmode(int modeidx);

int gfx_setup(int xsz, int ysz, int bpp, unsigned int flags);

void gfx_viewport(int x, int y, int width, int height);

void gfx_setcolor(int idx, int r, int g, int b);
void gfx_setcolors(int start, int count, struct gfxcolor *colors);

int gfx_imginit(struct gfximage *img, int x, int y, int bpp);
void gfx_imgdestroy(struct gfximage *img);

/* call imgstart to get a pointer to the pixel buffer, and imgend when done */
void *gfx_imgstart(struct gfximage *img);
void gfx_imgend(struct gfximage *img);

int gfx_loadimg(struct gfximage *img, const char *fname);

void gfx_fill(struct gfximage *img, unsigned int color, struct gfxrect *rect);

/* set which color to be used as a colorkey for transparent blits */
void gfx_imgkey(struct gfximage *img, int ckey);

void gfx_blit(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect);
void gfx_blitkey(struct gfximage *dest, int x, int y, struct gfximage *src, struct gfxrect *srect);

void gfx_swapbuffers(int vsync);
void gfx_waitvsync(void);

#endif	/* GFX_H_ */
