#include <stdio.h>
#include <windows.h>
#include <ddraw.h>
#include "game.h"
#include "gfx.h"
#include "options.h"
#include "logger.h"

#define WCNAME	"w32jamwin"

HWND win;

static int win_width, win_height;
static int quit;

static int create_win(int width, int height, const char *title);
static LRESULT CALLBACK handle_msg(HWND win, unsigned int msg, WPARAM wparam, LPARAM lparam);


int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hprev, char *cmdline, int showcmd)
{
	int res;
	MSG msg;
	RECT rect;
	unsigned long start_time, msec, nframes;

	log_start("game.log");
	load_options("game.cfg");

	if(create_win(opt.xres, opt.yres, "win32 retro jam 2024") == -1) {
		return 1;
	}
	GetWindowRect(win, &rect);

	if(game_init() == -1) {
		MessageBox(win, "failed to initialize game, see game.log for details", "fatal", MB_OK);
		return 1;
	}

	nframes = 0;
	start_time = timeGetTime();
	for(;;) {
		while(PeekMessage(&msg, win, 0, 0, PM_REMOVE)) {
			if(msg.message == WM_QUIT) goto end;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if(quit) goto end;
		}

		game_draw();
		nframes++;
	}

end:
	msec = timeGetTime() - start_time;
	printf("shutting down, avg fps: %.2f\n", (float)nframes / ((float)msec / 1000.0f));
	game_cleanup();
	log_stop();
	if(win) DestroyWindow(win);
	UnregisterClass(WCNAME, hinst);
	return 0;
}

static int create_win(int width, int height, const char *title)
{
	static int wcreg_done;
	unsigned int wstyle;
	RECT rect;

	HINSTANCE hinst = GetModuleHandle(0);

	if(!wcreg_done) {
		WNDCLASSEX wc = {0};
		wc.cbSize = sizeof wc;
		wc.hInstance = hinst;
		wc.lpszClassName = WCNAME;
		wc.hIcon = LoadIcon(hinst, IDI_APPLICATION);
		wc.hIconSm = wc.hIcon;
		wc.hCursor = LoadCursor(hinst, IDC_ARROW);
		wc.hbrBackground = GetStockObject(BLACK_BRUSH);
		wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
		wc.lpfnWndProc = handle_msg;

		if(!RegisterClassEx(&wc)) {
			MessageBox(0, "failed to register window class", "fatal", MB_OK);
			return -1;
		}
		wcreg_done = 1;
	}

	if(opt.fullscreen) {
		wstyle = WS_POPUP;
	} else {
		wstyle = WS_OVERLAPPEDWINDOW;

		rect.left = rect.top = 0;
		rect.right = width;
		rect.bottom = height;
		AdjustWindowRect(&rect, wstyle, 0);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	wstyle = opt.fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
	if(!(win = CreateWindow(WCNAME, title, wstyle, 20, 20, width, height, 0, 0, hinst, 0))) {
		MessageBox(0, "failed to create window", "fatal", MB_OK);
		return -1;
	}
	ShowWindow(win, 1);

	return 0;
}

static LRESULT CALLBACK handle_msg(HWND win, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg) {
	case WM_CLOSE:
		DestroyWindow(win);
		win = 0;
		return 0;
	case WM_DESTROY:
		quit = 1;
		PostQuitMessage(0);
		return 0;

	case WM_KEYDOWN:
		quit = 1;
		return 0;

	default:
		break;
	}

	return DefWindowProc(win, msg, wparam, lparam);
}
