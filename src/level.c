#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "level.h"

int create_level(struct level *lvl, int xsz, int ysz)
{
	int ncells;

	lvl->width = xsz;
	lvl->height = ysz;
	ncells = xsz * ysz;

	if(!(lvl->cell = malloc(ncells * sizeof *lvl->cell))) {
		fprintf(stderr, "failed to allocate level cells (%dx%d)\n", xsz, ysz);
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

int load_level(struct level *lvl, const char *fname)
{
	return -1;
}

int save_level(struct level *lvl, const char *fname)
{
	return -1;
}
