#ifndef LEVEL_H_
#define LEVEL_H_

struct levelcell {
	int ftile, wtile[2];	/* floor and wall tiles -1 for none */

	/* TODO players, NPCs, items, etc */
};

struct level {
	struct tileset *tset;
	int size;					/* let's keep it pow2 for now */

	struct levelcell *cell;		/* linear size x size array of cells */
};

int load_level(struct level *lvl, const char *fname);


#endif	/* LEVEL_H_ */
