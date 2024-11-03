#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tiles.h"
#include "treestor.h"

int load_tileset(struct tileset *tset, const char *fname)
{
	int res = -1;
	struct ts_node *tsroot;
	const char *str;

	if(!(tsroot = ts_load(fname))) {
		fprintf(stderr, "failed to open %s\n", fname);
		return -1;
	}
	if(strcmp(tsroot->name, "tileset") != 0) {
		fprintf(stderr, "%s is not a valid tileset file\n", fname);
		goto end;
	}

	if(!(str = ts_get_attr_str(tsroot, "image", 0))) {
		fprintf(stderr, "tileset without an image attribute is invalid\n", fname);
		goto end;
	}
	if(!(tset->img = malloc(sizeof *tset->img))) {
		perror("failed to allocate tileset image");
		goto end;
	}
	if(gfx_loadimg(tset->img, str) == -1) {
		goto end;
	}

	res = 0;
end:
	ts_free_tree(tsroot);
	return res;
}

void destroy_tileset(struct tileset *tset)
{
	gfx_imgdestroy(tset->img);
	free(tset->tiles);
}
