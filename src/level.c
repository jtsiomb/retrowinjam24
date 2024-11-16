#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "level.h"
#include "game.h"

int create_level(struct level *lvl, int xsz, int ysz)
{
	int ncells;

	lvl->width = xsz;
	lvl->height = ysz;
	ncells = xsz * ysz;

	lvl->xshift = 0;
	while(xsz) {
		lvl->xshift++;
		xsz >>= 1;
	}

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

struct levelcell *get_levelcell(struct level *lvl, int cx, int cy)
{
	if(cx < 0 || cx >= lvl->width) return 0;
	if(cy < 0 || cy >= lvl->height) return 0;

	return lvl->cell + (cy << lvl->xshift) + cy;
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


void vscr_to_cell(int sx, int sy, int *cx, int *cy)
{
	int ev_cx, ev_cy, odd_cx, odd_cy, x, y, dx, dy, even_dsq, odd_dsq;

	/* calculate even grid coordinates, and squared distance from sx,sy */
	ev_cx = (sx + TILE_XSZ / 2) / TILE_XSZ;
	ev_cy = (sy + TILE_YSZ / 2) / TILE_YSZ;
	cell_to_vscr(ev_cx, ev_cy, &x, &y);
	dx = sx - x;
	dy = sy - y;
	even_dsq = dx * dx + dy * dy;

	/* calculate odd grid coordinates, and squared distance from sx,sy */
	odd_cx = sx / TILE_XSZ;
	odd_cy = sy / TILE_YSZ;
	cell_to_vscr(odd_cx, odd_cy, &x, &y);
	dx = sx - x;
	dy = sy - y;
	odd_dsq = dx * dx + dy * dy;

	if(even_dsq < odd_dsq) {
		*cx = ev_cx;
		*cy = ev_cy;
	} else {
		*cx = odd_cx;
		*cy = odd_cy;
	}
}

void cell_to_vscr(int cx, int cy, int *sx, int *sy)
{
	int x = cx * TILE_XSZ;
	if(cy & 1) x += TILE_XSZ / 2;
	*sx = x;
	*sy = cy * (TILE_YSZ / 2);
}
