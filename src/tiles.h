#ifndef TILES_H_
#define TILES_H_

#include "gfx.h"

struct tile;

struct tileset {
	char *name;
	struct tile *tiles;
	int num_tiles;
	struct gfximage *img;
};

struct tile {
	char *name;
	int xoffs, yoffs;
	int xsz, ysz;
};

int load_tileset(struct tileset *tset, const char *fname);
void destroy_tileset(struct tileset *tset);

void blit_tile(struct gfximage *dest, int x, int y, struct tileset *tset, int idx);

#endif	/* TILES_H_ */
