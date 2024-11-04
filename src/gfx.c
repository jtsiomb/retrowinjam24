#include <string.h>
#include "gfx.h"
#include "imago2.h"

int gfx_loadimg(struct gfximage *img, const char *fname)
{
	int i;
	struct img_pixmap pixmap;
	struct img_colormap *cmap;

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
	img->pixels = pixmap.pixels;
	img->ncolors = cmap->ncolors;

	for(i=0; i<cmap->ncolors; i++) {
		img->cmap[i].r = cmap->color[i].r;
		img->cmap[i].g = cmap->color[i].g;
		img->cmap[i].b = cmap->color[i].b;
	}

	return 0;
}
