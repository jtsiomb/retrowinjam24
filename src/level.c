#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "level.h"
#include "game.h"
#include "treestor.h"

static int read_map(struct level *lvl, struct ts_node *tsn);

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

	if(!(tsn = ts_get_child(ts, "floor"))) {
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

static const char *symmap[256];
static int numsym;

static int find_symtile(struct tileset *tset, int c)
{
	int i;
	for(i=0; i<numsym; i++) {
		if(symmap[i][0] == c) {
			return find_tile_id(tset, symmap[i] + 1);
		}
	}
	return -1;
}

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
	numsym = 0;
	tsattr = tsn->attr_list;
	while(tsattr) {
		if(strcmp(tsattr->name, "sym") == 0) {
			if(numsym >= sizeof symmap / sizeof *symmap) {
				fprintf(stderr, "load_level: warning: ignoring excessive symbol mappings\n");
			} else {
				symmap[numsym++] = tsattr->val.str;
			}

		} else if(strcmp(tsattr->name, "row") == 0) {

			str = tsattr->val.str;
			for(i=0; i<lvl->size; i++) {
				if(!*str) {
					tid = inval_tid;
				} else {
					if((tid = find_symtile(lvl->tset, *str)) == -1) {
						tid = inval_tid;
					}
					str++;
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
	return 0;
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
