#include <stdio.h>
#include <string.h>
#include "screen.h"

struct screen *screens[MAX_SCR];
int num_screens;
struct screen *curscr;


int add_screen(struct screen *scr)
{
	if(num_screens >= MAX_SCR) {
		fprintf(stderr, "failed to add screen: %s. too many screens\n", scr->name);
		return -1;
	}
	screens[num_screens++] = scr;
	return 0;
}

struct screen *find_screen(const char *name)
{
	int i;

	for(i=0; i<num_screens; i++) {
		if(strcmp(screens[i]->name, name) == 0) {
			return screens[i];
		}
	}
	return 0;
}

void start_screen(struct screen *scr)
{
	if(scr->start() == -1) {
		fprintf(stderr, "failed to start screen: %s\n", scr->name);
		return;
	}
	if(curscr) {
		curscr->stop();
	}
	curscr = scr;
}
