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

	struct levelcell *cell;		/* linear size x size array of cells */
};

int create_level(struct level *lvl, int xsz, int ysz);
void destroy_level(struct level *lvl);

int load_level(struct level *lvl, const char *fname);
int save_level(struct level *lvl, const char *fname);


#endif	/* LEVEL_H_ */
