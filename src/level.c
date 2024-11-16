#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "level.h"
#include "game.h"

int create_level(struct level *lvl, int sz)
{
	int ncells;

	lvl->size = sz;
	lvl->shift = 0;
	while(sz > 1) {
		lvl->shift++;
		sz >>= 1;
	}
	ncells = lvl->size << lvl->shift;

	if(!(lvl->cell = malloc(ncells * sizeof *lvl->cell))) {
		fprintf(stderr, "failed to allocate level cells (%dx%d)\n", lvl->size, lvl->size);
		return -1;
	}

	/* for now set everything to -1, because we only have tiles */
	memset(lvl->cell, 0xff, ncells * sizeof *lvl->cell);

	lvl->tset = 0;
	return 0;
}

void destroy_level(struct level *lvl)
{
	free(lvl->cell);
	if(lvl->tset) {
		free_tileset(lvl->tset);
	}
}

struct levelcell *get_levelcell(struct level *lvl, int cx, int cy)
{
	if(cx < 0 || cx >= lvl->size) return 0;
	if(cy < 0 || cy >= lvl->size) return 0;

	return lvl->cell + (cy << lvl->shift) + cy;
}

struct levelcell *get_levelcell_vscr(struct level *lvl, int sx, int sy)
{
	int cx, cy;
	vscr_to_cell(sx, sy, &cx, &cy);
	return get_levelcell(lvl, cx, cy);
}

int load_level(struct level *lvl, const char *fname)
{
	return -1;
}

int save_level(struct level *lvl, const char *fname)
{
	return -1;
}

/* implicit in these conversions is the tile size: 128x64 */
void vscr_to_grid(int sx, int sy, int32_t *gridx, int32_t *gridy)
{
	sx <<= 2;
	sy <<= 3;
	*gridx = (sy + sx) >> 1;
	*gridy = (sy - sx) >> 1;
}

void grid_to_vscr(int32_t gridx, int32_t gridy, int *sx, int *sy)
{
	*sx = (gridx - gridy) >> 2;
	*sy = (gridx + gridy) >> 3;
}

void vscr_to_cell(int sx, int sy, int *cx, int *cy)
{
	int32_t gx, gy;
	vscr_to_grid(sx, sy, &gx, &gy);
	*cx = gx >> 8;
	*cy = gy >> 8;
}
