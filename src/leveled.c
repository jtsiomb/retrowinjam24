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
static int pan_x, pan_y;

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
		if(create_level(&lvl, 16, 32) == -1) {
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
		if(pan_y >= (lvl.height - 1) * TILE_YSZ) {
			pan_y = (lvl.height - 1) * TILE_YSZ;
		}
	}
	if(navkey_state[2]) {	/* left */
		pan_x -= speed;
		if(pan_x < 0) pan_x = 0;
	}
	if(navkey_state[3]) {	/* right */
		pan_x += speed;
		if(pan_x >= (lvl.width - 1) * TILE_XSZ) {
			pan_x = (lvl.width - 1) * TILE_XSZ;
		}
	}
	return 1;
}

static void eddraw(void)
{
	int startx, x, y, start_cellx, cellx, celly, hoverx, hovery, tile;
	struct levelcell *cell;

	edupdate();

	vscr_to_cell(mouse_x + pan_x, mouse_y + pan_y, &hoverx, &hovery);
	printf("\rPAN(%d,%d) M(%d,%d) %d,%d -> cell %d,%d           ", pan_x, pan_y,
			mouse_x, mouse_y, mouse_x + pan_x, mouse_y + pan_y, hoverx, hovery);
	fflush(stdout);

	vscr_to_cell(pan_x, pan_y, &start_cellx, &celly);

	if(celly & 1) {
		startx = -(pan_x % TILE_XSZ);
	} else {
		startx = -(pan_x % TILE_XSZ) - TILE_XSZ / 2;
	}
	startx = 0;
	y = 0;//-(pan_y % TILE_YSZ);// - TILE_YSZ / 2;

	/* x,y are tile centers, offset of blit coordinates */
	startx -= TILE_XSZ / 2;
	y -= TILE_YSZ / 2;

	while(y + TILE_YSZ < YRES) {
		x = startx;
		cellx = start_cellx;
		if(celly & 1) x += TILE_XSZ / 2;	/* stagger odd rows */
		while(x + TILE_XSZ < XRES) {
			if((cell = get_levelcell(&lvl, cellx, celly))) {
				tile = cell->ftile > 0 ? cell->ftile : 0;
				blit_tile(gfx_back, x, y, lvl.tset, tile);
			}
			if(cellx == hoverx && celly == hovery) {
				blit_tile(gfx_back, x, y, lvl.tset, 1);
			}

			x += TILE_XSZ;
			cellx++;
		}

		y += TILE_YSZ / 2;
		celly++;
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
