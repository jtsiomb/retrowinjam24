#include <stdio.h>
#include "game.h"
#include "level.h"
#include "screen.h"
#include "font.h"
#include "rend.h"
#include "dynarr.h"

#define TILE_HALF_YSZ	(TILE_YSZ >> 1)

#define PAN_SPEED	64

static int edstart(void);
static void edstop(void);
static void eddraw(void);
static void edkeyb(int key, int press);
static void edbutton(int bn, int st, int x, int y);
static void edmotion(int x, int y);

static void paint(int cx, int cy, int mode);

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

enum brush_type { BRUSH_FLOOR, BRUSH_LWALL, BRUSH_RWALL, NUM_BRUSH_TYPES };
static int brush[3];
static int *typetiles[3];
static int cur_brush_type;

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

		/* create per-type tile arrays */
		for(i=0; i<3; i++) {
			typetiles[i] = dynarr_alloc_ordie(0, sizeof *typetiles[i]);
			brush[i] = 0;
		}
		cur_brush_type = BRUSH_FLOOR;

		for(i=0; i<tset->num_tiles; i++) {
			switch(tset->tiles[i].type) {
			case TILE_FLOOR:
				dynarr_push_ordie(typetiles[BRUSH_FLOOR], &i);
				break;
			case TILE_LWALL:
			case TILE_LCDOOR:
			case TILE_LODOOR:
				dynarr_push_ordie(typetiles[BRUSH_LWALL], &i);
				break;
			case TILE_RWALL:
			case TILE_RCDOOR:
			case TILE_RODOOR:
				dynarr_push_ordie(typetiles[BRUSH_RWALL], &i);
				break;
			default:
				break;
			}
		}

	} else {
		tset = lvl.tset;
	}

	gfx_imgdebug(tset->img);

	render_init();
	rend.flags |= REND_HOVER;
	rend.flags &= ~REND_SHADE;

	for(i=1; i<tset->img->ncolors; i++) {
		pal[i].r = tset->img->cmap[i].r;
		pal[i].g = tset->img->cmap[i].g;
		pal[i].b = tset->img->cmap[i].b;
	}
	gfx_setcolors(1, tset->img->ncolors - 1, pal + 1);

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
	struct gfxrect rect;

	edupdate();

	render_view();

	if(!dynarr_empty(typetiles[cur_brush_type])) {
		int brushtile = brush[cur_brush_type];
		rect.x = XRES - TILE_XSZ - 4;
		rect.y = 0;
		rect.width = TILE_XSZ + 4;
		rect.height = lvl.tset->wallheight + TILE_YSZ / 2 + 4;
		gfx_fill(gfx_back, 0, &rect);
		blit_tile(gfx_back, rect.x + 2, rect.y + rect.height - 2, lvl.tset,
				typetiles[cur_brush_type][brushtile]);
	}

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
	int sz;

	if(key >= KEY_UP && key <= KEY_RIGHT) {
		navkey_state[key - KEY_UP] = press;
	}

	switch(key) {
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

	case '1':
		cur_brush_type = press ? BRUSH_LWALL : BRUSH_FLOOR;
		break;
	case '2':
		cur_brush_type = press ? BRUSH_RWALL : BRUSH_FLOOR;
		break;
	default:
		break;
	}

	if(!press) return;

	switch(key) {
	case 'l':
		if(load_level(&newlvl, "data/test.lvl") != -1) {
			destroy_level(&lvl);
			lvl = newlvl;
			reset_view();
		}
		break;

	case 'z':
		if(rend.empty_tile) {
			rend.empty_tile = 0;
		} else {
			rend.empty_tile = pick_tile(lvl.tset, TILE_SOLID);
		}
		break;

	case ']':
		sz = dynarr_size(typetiles[cur_brush_type]);
		brush[cur_brush_type] = (brush[cur_brush_type] + 1) % sz;
		break;
	case '[':
		sz = dynarr_size(typetiles[cur_brush_type]);
		brush[cur_brush_type] = (brush[cur_brush_type] - 1 + sz) % sz;
		break;

	default:
		break;
	}
}

static int bnstate[3];

static void edbutton(int bn, int st, int x, int y)
{
	int sz;

	mouse_x = x;
	mouse_y = y;

	if(bn < 3) {
		bnstate[bn] = st;
	}

	if(st) {
		switch(bn) {
		case 0:
			paint(rend.hovertile.x, rend.hovertile.y, 1);
			break;

		case 2:
			paint(rend.hovertile.x, rend.hovertile.y, 0);
			break;

		case 3:
			sz = dynarr_size(typetiles[cur_brush_type]);
			brush[cur_brush_type] = (brush[cur_brush_type] + 1) % sz;
			break;
		case 4:
			sz = dynarr_size(typetiles[cur_brush_type]);
			brush[cur_brush_type] = (brush[cur_brush_type] - 1 + sz) % sz;
			break;
		}
	}
}

static void edmotion(int x, int y)
{
	int dx, dy;
	dx = x - mouse_x;
	dy = y - mouse_y;
	mouse_x = x;
	mouse_y = y;

	vscr_to_cell(x + rend.pan.x, y + rend.pan.y, &rend.hovertile.x, &rend.hovertile.y);

	if(bnstate[0]) {
		paint(rend.hovertile.x, rend.hovertile.y, 1);
	}
	if(bnstate[1]) {
		pan_view(-dx, -dy);
	}
	if(bnstate[2]) {
		paint(rend.hovertile.x, rend.hovertile.y, 0);
	}
}

static void paint(int cx, int cy, int mode)
{
	int tid, brushtype;
	struct levelcell *cell;

	if((cell = get_levelcell(&lvl, rend.hovertile.x, rend.hovertile.y))) {
		brushtype = brush[cur_brush_type];

		if(cur_brush_type == BRUSH_FLOOR) {
			tid = mode ? typetiles[cur_brush_type][brushtype] : pick_tile(lvl.tset, TILE_SOLID);
			cell->ftile = tid;
		} else {
			tid = mode ? typetiles[cur_brush_type][brushtype] : 0;
			cell->wtile[cur_brush_type - BRUSH_LWALL] = tid;
		}
	}
}
