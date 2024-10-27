#include <stdio.h>
#include <SDL.h>
#include "game.h"
#include "options.h"

unsigned long time_msec;
static unsigned long start_time;

static int win_width, win_height;
static int quit;

static void handle_event(SDL_Event *ev);

static const char *wintext = "win32 retro jam 2024 (cross-platform version)";


int main(void)
{
	unsigned long msec, nframes;

	load_options("game.cfg");

	if(game_init() == -1) {
		return 1;
	}
	SDL_WM_SetCaption(wintext, wintext);

	nframes = 0;
	start_time = SDL_GetTicks();
	for(;;) {
		SDL_Event ev;
		while(SDL_PollEvent(&ev)) {
			handle_event(&ev);
			if(quit) goto end;
		}

		time_msec = game_getmsec();
		game_draw();
		nframes++;
	}

end:
	msec = game_getmsec();
	printf("shutting down, avg fps: %.2f\n", (float)nframes / ((float)msec / 1000.0f));
	game_cleanup();
	return 0;
}

unsigned long game_getmsec(void)
{
	return SDL_GetTicks() - start_time;
}

static void handle_event(SDL_Event *ev)
{
	switch(ev->type) {
	case SDL_QUIT:
		quit = 1;
		break;

	case SDL_KEYDOWN:
		quit = 1;
		break;

	default:
		break;
	}
}
