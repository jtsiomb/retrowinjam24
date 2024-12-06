#include <stdio.h>
#include "game.h"
#include "level.h"
#include "screen.h"
#include "font.h"

#define TILE_HALF_YSZ	(TILE_YSZ >> 1)

#define PAN_SPEED	64

static int edstart(void);
static void edstop(void);
static void eddraw(void);
static void edkeyb(int key, int press);
static void edbutton(int bn, int st, int x, int y);
static void edmotion(int x, int y);

static void reset_view(void);
static void pan_view(int dx, int dy);

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

	gfx_imgdebug(tset->img);

	reset_view();

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
	int speed, dx, dy;
	unsigned int dt_msec;
	static unsigned long prev_upd;

	dt_msec = time_msec - prev_upd;
	if(dt_msec < 33) return 0;
	prev_upd = time_msec;

	speed = (PAN_SPEED * dt_msec) >> 8;

	dx = dy = 0;
	if(navkey_state[0]) {	/* up */
		dy -= speed;
	}
	if(navkey_state[1]) {	/* down */
		dy += speed;
	}
	if(navkey_state[2]) {	/* left */
		dx -= speed;
	}
	if(navkey_state[3]) {	/* right */
		dx += speed;
	}
	if(dx | dy) {
		pan_view(dx, dy);
	}
	return 1;
}

static int frmstat_nblits, frmstat_ntiles;

static void eddraw(void)
{
	int i, j, x, y, x0, y0, x1, y1, tid, hoverx, hovery;
	struct levelcell *cell;

	frmstat_nblits = frmstat_ntiles = 0;

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

			if(x0 >= XRES || x1 < 0 || y0 >= YRES + 96 || y1 < 0) continue;

			frmstat_ntiles++;

			cell = get_levelcell(&lvl, j, i);
			if(cell->wtile[0] > 0) {
				blit_tile(gfx_back, x0, y0 + TILE_YSZ / 2, lvl.tset, cell->wtile[0]);
				frmstat_nblits++;
			}
			if(cell->wtile[1] > 0) {
				blit_tile(gfx_back, x0 + TILE_XSZ / 2, y0 + TILE_YSZ / 2, lvl.tset, cell->wtile[1]);
				frmstat_nblits++;
			}

			tid = cell->ftile > 0 ? cell->ftile : 0;
			y = lvl.tset->tiles[tid].type == TILE_SOLID ? y1 - lvl.tset->wallheight : y1;
			if(y < YRES + TILE_YSZ) {
				blit_tile(gfx_back, x0, y, lvl.tset, tid);
				frmstat_nblits++;

				if(j == hoverx && i == hovery) {
					blit_tile(gfx_back, x0, y1, lvl.tset, 1);
				}
			}
		}
	}

	if(showdbg) {
		char buf[64];
		struct fontdraw fdr;
		gfx_imgstart(gfx_back);
		fnt_initdraw(&fdr, gfx_back->pixels, gfx_back->width, gfx_back->height, gfx_back->pitch);
		fdr.colorbase = 240;
		fdr.colorshift = 4;
		fnt_position(&fdr, 0, 20);
		sprintf(buf, "tiles: %d/%d (%d blits)", frmstat_ntiles, lvl.size << lvl.shift,
				frmstat_nblits);
		fnt_drawstr(fnt, &fdr, buf);
		gfx_imgend(gfx_back);
	}
}

static void edkeyb(int key, int press)
{
	struct level newlvl;

	if(key >= KEY_UP && key <= KEY_RIGHT) {
		navkey_state[key - KEY_UP] = press;
	}

	switch(key) {
	case 'l':
		if(press) {
			if(load_level(&newlvl, "data/test.lvl") != -1) {
				destroy_level(&lvl);
				lvl = newlvl;
				reset_view();
			}
		}
		break;

	case 'a':
		navkey_state[2] = press;
		break;
	case 's':
		navkey_state[1] = press;
		break;
	case 'd':
		navkey_state[3] = press;
		break;
	case 'w':
		navkey_state[0] = press;
		break;
	default:
		break;
	}
}

static int bnstate[3];

static void edbutton(int bn, int st, int x, int y)
{
	mouse_x = x;
	mouse_y = y;

	if(bn < 3) {
		bnstate[bn] = st;
	}
}

static void edmotion(int x, int y)
{
	int dx, dy;
	dx = x - mouse_x;
	dy = y - mouse_y;
	mouse_x = x;
	mouse_y = y;

	if(bnstate[1]) {
		pan_view(-dx, -dy);
	}
}

static void reset_view(void)
{
	pan_x = pan_y = 0;
	pan_max_x = lvl.size * TILE_XSZ / 2 - XRES;
	pan_min_x = -(lvl.size * TILE_XSZ / 2);
	pan_max_y = lvl.size * TILE_YSZ - YRES;
}

static void pan_view(int dx, int dy)
{
	pan_x += dx;
	pan_y += dy;

	if(pan_y < 0) pan_y = 0;
	if(pan_y >= pan_max_y) pan_y = pan_max_y - 1;
	if(pan_x < pan_min_x) pan_x = pan_min_x;
	if(pan_x >= pan_max_x) pan_x = pan_max_x - 1;
}
