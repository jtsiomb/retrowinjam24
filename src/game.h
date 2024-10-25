#ifndef GAME_H_
#define GAME_H_

extern unsigned long time_msec;		/* defined in main*.c */


int game_init(void);
void game_cleanup(void);

void game_draw(void);

void game_keyboard(int key, int press);
void game_mousebtn(int bn, int st, int x, int y);
void game_mousemove(int x, int y);

/* defined in main*.c */
unsigned long game_getmsec(void);

#endif	/* GAME_H_ */
