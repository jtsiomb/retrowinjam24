#include "config.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "game.h"
#include "gfx.h"
#include "tiles.h"
#include "options.h"
#include "screen.h"
#include "font.h"

struct level lvl;
struct font *fnt;
int showdbg;

static unsigned int nframes;
static char fpsbuf[32];

int game_init(void)
{
	int i, vmidx;

	if(gfx_init() == -1) {
		return -1;
	}

	if(opt.fullscreen) {
		if((vmidx = gfx_findmode(opt.xres, opt.yres, 8, 0)) == -1) {
			fprintf(stderr, "failed to find suitable video mode, falling back to windowed\n");
			opt.fullscreen = 0;
			goto windowed;
		}
		printf("found video mode %d (%dx%d 8bpp), switching ...\n", vmidx, opt.xres, opt.yres);
		if(gfx_setmode(vmidx) == -1) {
			fprintf(stderr, "failed to set video mode, falling back to windowed\n");
			opt.fullscreen = 0;
			goto windowed;
		}
		printf("setting up %dx%d fullscreen\n", opt.xres, opt.yres);
		if(gfx_setup(opt.xres, opt.yres, 8, GFX_FULLSCREEN) == -1) {
			fprintf(stderr, "graphics setup failed\n");
			goto err;
		}
	} else {
windowed:
		printf("setting up %dx%d windowed\n", opt.xres, opt.yres);
		if(gfx_setup(opt.xres, opt.yres, 8, GFX_WINDOWED) == -1) {
			fprintf(stderr, "graphics setup failed\n");
			goto err;
		}
	}

	if(!(fnt = fnt_load("data/test.fnt"))) {
		fprintf(stderr, "failed to load font\n");
		goto err;
	}

	for(i=0; i<16; i++) {
		int val = i << 4;
		gfx_setcolor(i + 240, val, val, val);
	}
	gfx_setcolor(0, 0, 0, 0);
	gfx_setcolor(255, 0xff, 0xff, 0xff);

	if(init_leveled() == -1) return -1;
	start_screen(screens[0]);

	return 0;

err:
	gfx_destroy();
	return -1;
}

void game_cleanup(void)
{
	fnt_free(fnt);
	gfx_destroy();
}

static void update_fps(void)
{
	static unsigned long prev_fps_upd;
	float fps, dt;
	unsigned int interv = time_msec - prev_fps_upd;

	if(interv < 1000 || nframes < 20) return;

	dt = (float)interv / 1000.0f;
	fps = (float)nframes / dt;

	sprintf(fpsbuf, "%.1f fps", fps);

	nframes = 0;
	prev_fps_upd = time_msec;
}

void game_draw(void)
{
	struct fontdraw fdr;
	int i, xsz;
	unsigned char *ptr;

	update_fps();

	gfx_fill(gfx_back, 4, 0);

	curscr->draw();

	ptr = gfx_imgstart(gfx_back);
	xsz = fnt_strwidth(fnt, fpsbuf);
	for(i=0; i<fnt->line_advance; i++) {
		memset(ptr, 240, xsz);
		ptr += gfx_back->pitch;
	}
	fnt_initdraw(&fdr, gfx_back->pixels, gfx_back->width, gfx_back->height, gfx_back->pitch);
	fdr.colorbase = 240;
	fdr.colorshift = 4;
	fnt_drawstr(fnt, &fdr, fpsbuf);
	gfx_imgend(gfx_back);

	gfx_swapbuffers(1);
	nframes++;
}


void game_keyboard(int key, int press)
{
	if(press) {
		switch(key) {
		case 27:
			game_quit();
			return;

		case '`':
			showdbg ^= 1;
			break;

		default:
			break;
		}
	}

	curscr->keyb(key, press);
}

void game_mousebtn(int bn, int st, int x, int y)
{
	curscr->mbutton(bn, st, x, y);
}

void game_mousemove(int x, int y)
{
	curscr->mmotion(x, y);
}
