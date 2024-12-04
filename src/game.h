#ifndef GAME_H_
#define GAME_H_

#include "level.h"

#define XRES	640
#define YRES	480

#define TILE_XSZ	128
#define TILE_YSZ	64

extern unsigned long time_msec;		/* defined in main*.c */

extern struct level lvl;
extern struct font *fnt;
extern int showdbg;

/* special keys */
enum {
	KEY_BACKSP = 8,
	KEY_ESC = 27,
	KEY_DEL = 127,

	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
	KEY_INSERT, KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
	KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12
};


int game_init(void);
void game_cleanup(void);

void game_draw(void);

void game_keyboard(int key, int press);
void game_mousebtn(int bn, int st, int x, int y);
void game_mousemove(int x, int y);

/* defined in main*.c */
void game_quit(void);
unsigned long game_getmsec(void);

#endif	/* GAME_H_ */
