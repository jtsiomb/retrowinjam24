#ifndef GAME_H_
#define GAME_H_

int game_init(void);
void game_cleanup(void);

void game_draw(void);

void game_keyboard(int key, int press);
void game_mousebtn(int bn, int st, int x, int y);
void game_mousemove(int x, int y);

#endif	/* GAME_H_ */
