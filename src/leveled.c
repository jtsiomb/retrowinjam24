#include <stdio.h>
#include "game.h"
#include "level.h"
#include "screen.h"
#include "font.h"
#include "rend.h"

#define TILE_HALF_YSZ	(TILE_YSZ >> 1)

#define PAN_SPEED	64

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

	render_init();
	rend.flags = REND_HOVER;

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

static void eddraw(void)
{
	edupdate();
	vscr_to_cell(mouse_x + rend.pan.x, mouse_y + rend.pan.y, &rend.hovertile.x, &rend.hovertile.y);

	render_view();

	if(showdbg) {
		char buf[64];
		struct fontdraw fdr;
		gfx_imgstart(gfx_back);
		fnt_initdraw(&fdr, gfx_back->pixels, gfx_back->width, gfx_back->height, gfx_back->pitch);
		fdr.colorbase = 240;
		fdr.colorshift = 4;
		fnt_position(&fdr, 0, 20);
		sprintf(buf, "tiles: %d/%d (%d blits)", rend.stat_ntiles, lvl.size << lvl.shift,
				rend.stat_nblits);
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
