#ifndef TILES_H_
#define TILES_H_

#include "gfx.h"

struct tile;

enum tiletype {
	TILE_UNKNOWN,
	/* floor tile types */
	TILE_SOLID,
	TILE_FLOOR,
	/* wall tile types */
	TILE_LWALL, TILE_RWALL,
	TILE_LCDOOR, TILE_RCDOOR,
	TILE_LODOOR, TILE_RODOOR
};

struct tileset {
	char *name;
	struct tile *tiles;
	int num_tiles;
	struct gfximage *img;
};

struct tile {
	char *name;
	enum tiletype type;
	int xoffs, yoffs;
	int xsz, ysz;
};

struct tileset *alloc_tileset(void);
void free_tileset(struct tileset *tset);

int load_tileset(struct tileset *tset, const char *fname);
void destroy_tileset(struct tileset *tset);

int find_tile_id(struct tileset *tset, const char *name);

/* pick a random tile of the specified type */
int pick_tile(struct tileset *tset, enum tiletype type);

/* coordinates are bottom-left */
void blit_tile(struct gfximage *dest, int x, int y, struct tileset *tset, int idx);

#endif	/* TILES_H_ */
