#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tiles.h"
#include "treestor.h"
#include "util.h"

static enum tiletype parse_type(const char *str);


struct tileset *alloc_tileset(void)
{
	return calloc(1, sizeof(struct tileset));
}

void free_tileset(struct tileset *tset)
{
	if(!tset) return;
	destroy_tileset(tset);
	free(tset);
}

int load_tileset(struct tileset *tset, const char *fname)
{
	struct ts_node *tsroot, *tsnode;
	const char *str;
	struct tile *tile;
	float *vec;
	float defsize[4] = {8, 8};

	if(!(tsroot = ts_load(fname))) {
		fprintf(stderr, "failed to open %s\n", fname);
		return -1;
	}
	if(strcmp(tsroot->name, "tileset") != 0) {
		fprintf(stderr, "%s is not a valid tileset file\n", fname);
		ts_free_tree(tsroot);
		return -1;
	}

	memset(tset, 0, sizeof *tset);

	if(!(str = ts_get_attr_str(tsroot, "image", 0))) {
		fprintf(stderr, "%s: tileset without an image attribute is invalid\n", fname);
		goto err;
	}
	if(!(tset->img = malloc(sizeof *tset->img))) {
		perror("failed to allocate tileset image");
		goto err;
	}
	if(gfx_loadimg(tset->img, str) == -1) {
		goto err;
	}
	gfx_imgkey(tset->img, ts_get_attr_int(tsroot, "colorkey", 0));
	tset->name = strdup(ts_get_attr_str(tsroot, "name", "unknown"));

	tset->wallheight = ts_get_attr_int(tsroot, "wallheight", 0);

	if((vec = ts_get_attr_vec(tsroot, "defsize", 0))) {
		defsize[0] = vec[0];
		defsize[1] = vec[1];
	}

	if(!(tset->tiles = malloc(tsroot->child_count * sizeof *tset->tiles))) {
		perror("failed to allocate tiles array");
		goto err;
	}
	tile = tset->tiles;

	tsnode = tsroot->child_list;
	while(tsnode) {
		if(strcmp(tsnode->name, "tile") == 0) {
			tile->name = strdup(ts_get_attr_str(tsnode, "name", "unknown"));
			tile->type = parse_type(ts_get_attr_str(tsnode, "type", 0));
			if((vec = ts_get_attr_vec(tsnode, "offs", 0))) {
				tile->xoffs = (int)vec[0];
				tile->yoffs = (int)vec[1];
			}
			vec = ts_get_attr_vec(tsnode, "size", defsize);
			tile->xsz = (int)vec[0];
			tile->ysz = (int)vec[1];
			tile->pickprio = ts_get_attr_int(tsnode, "pick", 0);
			tile++;
		}
		tsnode = tsnode->next;
	}
	tset->num_tiles = tile - tset->tiles;

	ts_free_tree(tsroot);
	return 0;

err:
	ts_free_tree(tsroot);
	free(tset->name);
	free(tset->tiles);
	return -1;
}

void destroy_tileset(struct tileset *tset)
{
	if(tset->img) {
		gfx_imgdestroy(tset->img);
	}
	free(tset->tiles);
	free(tset->name);
}

int find_tile_id(struct tileset *tset, const char *name)
{
	int i;
	for(i=0; i<tset->num_tiles; i++) {
		if(strcmp(tset->tiles[i].name, name) == 0) {
			return i;
		}
	}
	return -1;
}

int pick_tile(struct tileset *tset, enum tiletype type)
{
	unsigned int i, pick, cdf, num = 0;
	struct {int tid, cdf;} *pool = alloca(tset->num_tiles * sizeof *pool);

	for(i=0; i<tset->num_tiles; i++) {
		if(tset->tiles[i].type == type) {
			pool[num].tid = i;
			pool[num].cdf = (num ? pool[num - 1].cdf : 0) + tset->tiles[i].pickprio;
			num++;
		}
	}

	if(!num) return -1;

	pick = rand() & 0xffff;

	for(i=0; i<num - 1; i++) {
		cdf = (pool[i].cdf << 16) / pool[num - 1].cdf;
		if(pick < cdf) return pool[i].tid;
	}
	return pool[num - 1].tid;
}

void blit_tile(struct gfximage *dest, int x, int y, struct tileset *tset, int idx)
{
	struct gfxrect rect;
	struct tile *tile = tset->tiles + idx;

	rect.x = tile->xoffs;
	rect.y = tile->yoffs;
	rect.width = tile->xsz;
	rect.height = tile->ysz;

	gfx_blitkey(dest, x, y - tile->ysz, tset->img, &rect);
}


/* XXX these must correspond to enum tiletype in tiles.h */
static const char *typestr[] = {
	"unknown", "solid", "floor",
	"lwall", "rwall",
	"lcdoor", "rcdoor",
	"lodoor", "rodoor",
	0
};

static enum tiletype parse_type(const char *str)
{
	int i;

	if(!str) return TILE_UNKNOWN;

	for(i=0; typestr[i]; i++) {
		if(strcmp(str, typestr[i]) == 0) {
			return i;
		}
	}
	return TILE_UNKNOWN;
}
