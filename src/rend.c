#include <string.h>
#include "rend.h"
#include "game.h"
#include "tiles.h"


struct renderstate rend;

void render_init(void)
{
	memset(&rend, 0, sizeof rend);

	reset_view();
}

void reset_view(void)
{
	rend.pan.x = rend.pan.y = 0;
	rend.pan_max.x = lvl.size * TILE_XSZ / 2 - XRES;
	rend.pan_min.x = -(lvl.size * TILE_XSZ / 2);
	rend.pan_min.y = 0;
	rend.pan_max.y = lvl.size * TILE_YSZ - YRES;
}

void pan_view(int dx, int dy)
{
	rend.pan.x += dx;
	rend.pan.y += dy;

	if(rend.pan.y < 0) rend.pan.y = 0;
	if(rend.pan.y >= rend.pan_max.y) rend.pan.y = rend.pan_max.y - 1;
	if(rend.pan.x < rend.pan_min.x) rend.pan.x = rend.pan_min.x;
	if(rend.pan.x >= rend.pan_max.x) rend.pan.x = rend.pan_max.x - 1;
}

void render_view(void)
{
	int i, j, x, y, x0, y0, x1, y1, tid;
	struct levelcell *cell;

	rend.stat_nblits = rend.stat_ntiles = 0;

	for(i=0; i<lvl.size; i++) {
		for(j=0; j<lvl.size; j++) {
			cell_to_vscr(j, i, &x, &y);
			x -= rend.pan.x;
			y -= rend.pan.y;
			x0 = x - TILE_XSZ / 2;
			x1 = x + TILE_XSZ / 2;
			y0 = y;
			y1 = y + TILE_YSZ;

			if(x0 >= XRES || x1 < 0 || y0 >= YRES + 96 || y1 < 0) continue;

			rend.stat_ntiles++;

			cell = get_levelcell(&lvl, j, i);
			if(cell->wtile[0] > 0) {
				blit_tile(gfx_back, x0, y0 + TILE_YSZ / 2, lvl.tset, cell->wtile[0]);
				rend.stat_nblits++;
			}
			if(cell->wtile[1] > 0) {
				blit_tile(gfx_back, x0 + TILE_XSZ / 2, y0 + TILE_YSZ / 2, lvl.tset, cell->wtile[1]);
				rend.stat_nblits++;
			}

			tid = cell->ftile > 0 ? cell->ftile : 0;
			y = lvl.tset->tiles[tid].type == TILE_SOLID ? y1 - lvl.tset->wallheight : y1;
			if(y < YRES + TILE_YSZ) {
				blit_tile(gfx_back, x0, y, lvl.tset, tid);
				rend.stat_nblits++;
			}
		}
	}

	if((rend.flags & REND_HOVER) && rend.hovertile.x >= 0 && rend.hovertile.y >= 0 &&
			rend.hovertile.x < lvl.size && rend.hovertile.y < lvl.size) {
		cell_to_vscr(rend.hovertile.x, rend.hovertile.y, &x, &y);
		x -= rend.pan.x;
		y -= rend.pan.y;
		x0 = x - TILE_XSZ / 2;
		x1 = x + TILE_XSZ / 2;
		y0 = y;
		y1 = y + TILE_YSZ;

		if(x0 < XRES && x1 >= 0 && y0 < YRES && y1 >= 0) {
			blit_tile(gfx_back, x0, y1, lvl.tset, 1);
		}
	}
}
