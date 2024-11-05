#include <string.h>
#include "gfx.h"
#include "imago2.h"

int gfx_loadimg(struct gfximage *img, const char *fname)
{
	int i, j;
	struct img_pixmap pixmap;
	struct img_colormap *cmap;
	unsigned char *dptr, *sptr;

	img_init(&pixmap);
	if(img_load(&pixmap, fname) == -1) {
		return -1;
	}

	if(pixmap.fmt != IMG_FMT_IDX8) {
		img_convert(&pixmap, IMG_FMT_IDX8);
	}
	cmap = img_colormap(&pixmap);

	if(gfx_imginit(img, pixmap.width, pixmap.height, 8) == -1) {
		img_destroy(&pixmap);
		return -1;
	}

	gfx_imgstart(img);
	dptr = img->pixels;
	sptr = pixmap.pixels;

	for(i=0; i<pixmap.height; i++) {
		for(j=0; j<pixmap.width; j++) {
			dptr[j] = sptr[j];
		}
		dptr += img->pitch;
		sptr += pixmap.width;
	}
	gfx_imgend(img);
;
	img->ncolors = cmap->ncolors;

	for(i=0; i<cmap->ncolors; i++) {
		img->cmap[i].r = cmap->color[i].r;
		img->cmap[i].g = cmap->color[i].g;
		img->cmap[i].b = cmap->color[i].b;
	}

	img_destroy(&pixmap);
	return 0;
}


int gfx_saveimg(struct gfximage *img, const char *fname)
{
	int i, j, res;
	unsigned char *sptr, *dptr;
	struct img_pixmap pixmap;
	struct img_colormap *dpal;

	if(!gfx_imgstart(img)) {
		return -1;
	}
	sptr = img->pixels;

	img_init(&pixmap);
	img_set_pixels(&pixmap, img->width, img->height, IMG_FMT_IDX8, 0);
	dptr = pixmap.pixels;
	dpal = img_colormap(&pixmap);

	for(i=0; i<img->height; i++) {
		for(j=0; j<img->width; j++) {
			dptr[j] = sptr[j];
		}
		dptr += pixmap.width;
		sptr += img->pitch;
	}

	for(i=0; i<img->ncolors; i++) {
		dpal->color[i].r = img->cmap[i].r;
		dpal->color[i].g = img->cmap[i].g;
		dpal->color[i].b = img->cmap[i].b;
	}
	dpal->ncolors = img->ncolors;
	gfx_imgend(img);

	res = img_save(&pixmap, fname);

	img_destroy(&pixmap);
	return res;
}
