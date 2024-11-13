#ifndef SCREEN_H_
#define SCREEN_H_

struct screen {
	const char *name;

	int (*start)(void);
	void (*stop)(void);
	void (*draw)(void);

	void (*keyb)(int key, int press);
	void (*mbutton)(int bn, int st, int x, int y);
	void (*mmotion)(int x, int y);
};

#define MAX_SCR		8
extern struct screen *screens[MAX_SCR];
extern int num_screens;
extern struct screen *curscr;

int add_screen(struct screen *scr);
struct screen *find_screen(const char *name);
void start_screen(struct screen *scr);

/* screen init functions */
int init_leveled(void);

#endif	/* SCREEN_H_ */
