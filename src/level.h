#ifndef LEVEL_H_
#define LEVEL_H_

#include "tiles.h"
#include "types.h"

/* levelcell flags */
enum {
	CELL_WALK	= 1
};

typedef struct vec2i {
	int x, y;
} vec2i;

struct levelcell {
	unsigned int flags;
	int ftile, wtile[2];	/* floor and wall tiles -1 for none */

	/* TODO players, NPCs, items, etc */
};

struct level {
	struct tileset *tset;
	int size;			/* size x size level (keep size pow2) */
	int shift;			/* shift needed to multiply by size */

	vec2i spawn;

	struct levelcell *cell;		/* linear size x size array of cells */
};

int create_level(struct level *lvl, int sz);
void destroy_level(struct level *lvl);

/* get the cell cx,cy */
struct levelcell *get_levelcell(struct level *lvl, int cx, int cy);
/* get the cell at virtual screen coordinates sx, sy */
struct levelcell *get_levelcell_vscr(struct level *lvl, int sx, int sy);

int load_level(struct level *lvl, const char *fname);
int save_level(struct level *lvl, const char *fname);

/* conversion between virtual screen and grid coordinates (24.8 fixed point) */
void vscr_to_grid(int sx, int sy, int32_t *gridx, int32_t *gridy);
void grid_to_vscr(int32_t gridx, int32_t gridy, int *sx, int *sy);

/* conversion between virtual screen and cell indices */
void vscr_to_cell(int sx, int sy, int *cx, int *cy);
#define cell_to_vscr(cx, cy, sx, sy) grid_to_vscr((cx) << 8, (cy) << 8, sx, sy)

#endif	/* LEVEL_H_ */
