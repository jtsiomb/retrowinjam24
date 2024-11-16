#ifndef LEVEL_H_
#define LEVEL_H_

#include "tiles.h"

struct levelcell {
	int ftile, wtile[2];	/* floor and wall tiles -1 for none */

	/* TODO players, NPCs, items, etc */
};

struct level {
	struct tileset *tset;
	int width, height;			/* let's keep width pow2 for now */
	int xshift;

	struct levelcell *cell;		/* linear size x size array of cells */
};

int create_level(struct level *lvl, int xsz, int ysz);
void destroy_level(struct level *lvl);

/* get the cell cx,cy */
struct levelcell *get_levelcell(struct level *lvl, int cx, int cy);
/* get the cell at virtual screen coordinates sx, sy */
struct levelcell *get_levelcell_vscr(struct level *lvl, int sx, int sy);

int load_level(struct level *lvl, const char *fname);
int save_level(struct level *lvl, const char *fname);

void vscr_to_cell(int sx, int sy, int *cx, int *cy);
void cell_to_vscr(int cx, int cy, int *sx, int *sy);

#endif	/* LEVEL_H_ */
