#include <stdio.h>
#include "game.h"
#include "level.h"
#include "screen.h"

#define TILE_HALF_YSZ	(TILE_YSZ >> 1)

#define PAN_SPEED	32

static int edstart(void);
static void edstop(void);
static void eddraw(void);
static void edkeyb(int key, int press);
static void edbutton(int bn, int st, int x, int y);
static void edmotion(int x, int y);

static struct screen edscreen = {
	"leveled",
	edstart,
	edstop,
	eddraw,
	edkeyb,
	edbutton,
	edmotion
};

static int mouse_x, mouse_y;
static int pan_x, pan_y, pan_min_x, pan_max_x, pan_max_y;

int init_leveled(void)
{
	return add_screen(&edscreen);
}

static int edstart(void)
{
	int i;
	struct tileset *tset;
	struct gfxcolor pal[256];

	if(!lvl.cell) {
		if(create_level(&lvl, 16) == -1) {
			return -1;
		}
		if(!(tset = alloc_tileset())) {
			destroy_level(&lvl);
			return -1;
		}
		if(load_tileset(tset, "data/test.set") == -1) {
			destroy_level(&lvl);
			return -1;
		}
		lvl.tset = tset;
	} else {
		tset = lvl.tset;
	}

	pan_x = pan_y = 0;
	pan_max_x = lvl.size * TILE_XSZ / 2 - XRES;
	pan_min_x = -(lvl.size * TILE_XSZ / 2);
	pan_max_y = lvl.size * TILE_YSZ - YRES;

	for(i=0; i<tset->img->ncolors; i++) {
		pal[i].r = tset->img->cmap[i].r;
		pal[i].g = tset->img->cmap[i].g;
		pal[i].b = tset->img->cmap[i].b;
	}
	gfx_setcolors(0, tset->img->ncolors, pal);

	return 0;
}

static void edstop(void)
{
}

static int navkey_state[4];

static int edupdate(void)
{
	int speed;
	unsigned int dt_msec;
	static unsigned long prev_upd;

	dt_msec = time_msec - prev_upd;
	if(dt_msec < 33) return 0;
	prev_upd = time_msec;

	speed = (PAN_SPEED * dt_msec) >> 8;

	if(navkey_state[0]) {	/* up */
		pan_y -= speed;
		if(pan_y < 0) pan_y = 0;
	}
	if(navkey_state[1]) {	/* down */
		pan_y += speed;
		if(pan_y >= pan_max_y) pan_y = pan_max_y - 1;
	}
	if(navkey_state[2]) {	/* left */
		pan_x -= speed;
		if(pan_x < pan_min_x) pan_x = pan_min_x;
	}
	if(navkey_state[3]) {	/* right */
		pan_x += speed;
		if(pan_x >= pan_max_x) pan_x = pan_max_x - 1;
	}
	return 1;
}

static void eddraw(void)
{
	int i, j, x, y, x0, y0, x1, y1, tile, hoverx, hovery;
	struct levelcell *cell;

	edupdate();

	vscr_to_cell(mouse_x + pan_x, mouse_y + pan_y, &hoverx, &hovery);

	for(i=0; i<lvl.size; i++) {
		for(j=0; j<lvl.size; j++) {
			cell_to_vscr(j, i, &x, &y);
			x -= pan_x;
			y -= pan_y;
			x0 = x - TILE_XSZ / 2;
			x1 = x + TILE_XSZ / 2;
			y0 = y;
			y1 = y + TILE_YSZ;

			if(x0 >= XRES || x1 < 0 || y0 >= YRES || y1 < 0) continue;

			cell = get_levelcell(&lvl, j, i);
			tile = cell->ftile > 0 ? cell->ftile : 0;
			blit_tile(gfx_back, x0, y0, lvl.tset, tile);

			if(j == hoverx && i == hovery) {
				blit_tile(gfx_back, x0, y0, lvl.tset, 1);
			}
		}
	}
}

static void edkeyb(int key, int press)
{
	if(key >= KEY_UP && key <= KEY_RIGHT) {
		navkey_state[key - KEY_UP] = press;
	}
}

static void edbutton(int bn, int st, int x, int y)
{
	mouse_x = x;
	mouse_y = y;
}

static void edmotion(int x, int y)
{
	mouse_x = x;
	mouse_y = y;
}
