#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "level.h"
#include "game.h"
#include "treestor.h"
#include "util.h"

static int read_map(struct level *lvl, struct ts_node *tsn);
static int proc_map(struct level *lvl);

int create_level(struct level *lvl, int sz)
{
	int ncells;

	lvl->size = sz;
	lvl->shift = calc_shift(sz);
	ncells = sz << lvl->shift;

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

	return lvl->cell + (cy << lvl->shift) + cx;
}

struct levelcell *get_levelcell_vscr(struct level *lvl, int sx, int sy)
{
	int cx, cy;
	vscr_to_cell(sx, sy, &cx, &cy);
	return get_levelcell(lvl, cx, cy);
}

int load_level(struct level *lvl, const char *fname)
{
	int size;
	const char *tsetfile;
	struct ts_node *ts, *tsn;
	struct tileset *tset = 0;

	if(!(ts = ts_load(fname))) {
		fprintf(stderr, "failed to load: %s\n", fname);
		return -1;
	}
	if(strcmp(ts->name, "level") != 0) {
		fprintf(stderr, "invalid level file: %s\n", fname);
		goto err;
	}

	if(!(size = ts_get_attr_int(ts, "size", 0))) {
		fprintf(stderr, "load_level(%s): missing size attribute\n", fname);
		goto err;
	}
	if(!(tsetfile = ts_get_attr_str(ts, "tileset", 0))) {
		fprintf(stderr, "load_level(%s): missing tileset attribute\n", fname);
		goto err;
	}
	if(!(tset = alloc_tileset())) {
		fprintf(stderr, "failed to allocate tileset\n");
		goto err;
	}
	if(load_tileset(tset, tsetfile) == -1) {
		goto err;
	}
	if(create_level(lvl, size) == -1) {
		goto err;
	}
	lvl->tset = tset;

	if(!(tsn = ts_get_child(ts, "map"))) {
		fprintf(stderr, "load_level(%s): missing floor data\n", fname);
		goto err;
	}
	if(read_map(lvl, tsn) == -1) {
		goto err;
	}

	ts_free_tree(ts);
	return 0;

err:
	ts_free_tree(ts);
	free_tileset(tset);
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


/* special temporary tile types */
#define TILE_DOORWAY	128

static int read_map(struct level *lvl, struct ts_node *tsn)
{
	int i, j, tid, inval_tid, rowcount;
	struct ts_attr *tsattr;
	const char *str;
	struct levelcell *cell;

	if((inval_tid = find_tile_id(lvl->tset, "invalid")) == -1) {
		inval_tid = 0;
	}

	cell = lvl->cell;
	rowcount = 0;
	tsattr = tsn->attr_list;
	while(tsattr) {
		if(strcmp(tsattr->name, "row") == 0) {
			str = tsattr->val.str;
			for(i=0; i<lvl->size; i++) {
				if(!*str) {
					tid = TILE_UNKNOWN;
				} else {
					switch(*str++) {
					case '#':
						tid = TILE_SOLID;
						break;
					case 'p':
						lvl->spawnx = i;
						lvl->spawny = rowcount;
					case ' ':
						tid = TILE_FLOOR;
						cell->flags |= CELL_WALK;
						break;
					case 'd':
						tid = TILE_DOORWAY;
						break;
					}
				}
				cell->ftile = tid;
				cell++;
			}
			rowcount++;
		}
		tsattr = tsattr->next;
	}

	if(rowcount < lvl->size) {
		for(i=rowcount; i<lvl->size; i++) {
			for(j=0; j<lvl->size; j++) {
				cell->ftile = inval_tid;
				cell++;
			}
		}
	}

	proc_map(lvl);
	return 0;
}


static int proc_map(struct level *lvl)
{
	int i, j;
	struct levelcell *cell;

	/* first pass: assign wall tiles */
	cell = lvl->cell;
	for(i=0; i<lvl->size; i++) {
		for(j=0; j<lvl->size; j++) {
			switch(cell->ftile) {
			case TILE_FLOOR:
				if(j == 0 || cell[-1].ftile == TILE_SOLID) {
					cell->wtile[0] = pick_tile(lvl->tset, TILE_LWALL);
				}
				if(i == 0 || cell[-lvl->size].ftile == TILE_SOLID) {
					cell->wtile[1] = pick_tile(lvl->tset, TILE_RWALL);
				}
				break;

			case TILE_DOORWAY:
				if(j > 0 && cell[-1].ftile != TILE_SOLID) {
					cell->wtile[0] = pick_tile(lvl->tset, TILE_LCDOOR);
				}
				if(i > 0 && cell[-lvl->size].ftile != TILE_SOLID) {
					cell->wtile[1] = pick_tile(lvl->tset, TILE_RCDOOR);
				}

			default:
				break;
			}
			cell++;
		}
	}

	/* second pass: assign floor tiles */
	cell = lvl->cell;
	for(i=0; i<lvl->size; i++) {
		for(j=0; j<lvl->size; j++) {
			switch(cell->ftile) {
			case TILE_FLOOR:
			case TILE_DOORWAY:
				cell->ftile = pick_tile(lvl->tset, TILE_FLOOR);
				break;

			case TILE_SOLID:
				cell->ftile = pick_tile(lvl->tset, TILE_SOLID);
				break;

			default:
				break;
			}
			cell++;
		}
	}

	return 0;
}
