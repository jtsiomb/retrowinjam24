#include <stdio.h>
#include <SDL.h>
#include "game.h"
#include "options.h"
#include "logger.h"

extern int fbscale;

unsigned long time_msec;
static unsigned long start_time;

static int quit;

static void handle_event(SDL_Event *ev);
static int translate_sdlkey(SDLKey sym);

static const char *wintext = "win32 retro jam 2024 (cross-platform SDL build)";


int main(int argc, char **argv)
{
	unsigned long msec, nframes;

#ifdef _WIN32
	log_start("game.log");
#endif
	load_options("game.cfg");

#ifdef _WIN32
	if(opt.fullscreen) {
		putenv("SDL_VIDEODRIVER=directx");
	}
#endif

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

void game_quit(void)
{
	quit = 1;
}

unsigned long game_getmsec(void)
{
	return SDL_GetTicks() - start_time;
}

static void handle_event(SDL_Event *ev)
{
	int key;

	switch(ev->type) {
	case SDL_QUIT:
		quit = 1;
		break;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		if((key = translate_sdlkey(ev->key.keysym.sym)) >= 0) {
			game_keyboard(key, ev->key.state);
		}
		break;

	case SDL_MOUSEMOTION:
		game_mousemove(ev->motion.x / fbscale, ev->motion.y / fbscale);
		break;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		game_mousebtn(ev->button.button - SDL_BUTTON_LEFT, ev->button.state,
				ev->button.x / fbscale, ev->button.y / fbscale);
		break;

	default:
		break;
	}
}

static int translate_sdlkey(SDLKey sym)
{
	switch(sym) {
	case SDLK_LEFT: return KEY_LEFT;
	case SDLK_RIGHT: return KEY_RIGHT;
	case SDLK_UP: return KEY_UP;
	case SDLK_DOWN: return KEY_DOWN;
	case SDLK_INSERT: return KEY_INSERT;
	case SDLK_HOME: return KEY_HOME;
	case SDLK_END: return KEY_END;
	case SDLK_PAGEUP: return KEY_PGUP;
	case SDLK_PAGEDOWN: return KEY_PGDN;
	default:
		break;
	}

	if(sym >= SDLK_F1 && sym <= SDLK_F12) {
		return sym - SDLK_F1 + KEY_F1;
	}

	if(sym >= 0 && sym < 128) {
		return sym;
	}
	return -1;
}
