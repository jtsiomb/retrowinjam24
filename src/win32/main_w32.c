#include <stdio.h>
#include <windows.h>
#include <ddraw.h>
#include "game.h"
#include "gfx.h"
#include "options.h"
#include "logger.h"

#define WCNAME	"w32jamwin"

HWND win;
unsigned long time_msec;
static unsigned long start_time;

static int win_width, win_height;
static int quit;

static int create_win(int width, int height, const char *title);
static LRESULT CALLBACK handle_msg(HWND win, unsigned int msg, WPARAM wparam, LPARAM lparam);

static int translate_vkey(int vkey);


int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hprev, char *cmdline, int showcmd)
{
	MSG msg;
	RECT rect;
	unsigned long msec, nframes;

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

		time_msec = game_getmsec();
		game_draw();
		nframes++;
	}

end:
	msec = game_getmsec();
	printf("shutting down, avg fps: %.2f\n", (float)nframes / ((float)msec / 1000.0f));
	game_cleanup();
	log_stop();
	if(win) DestroyWindow(win);
	UnregisterClass(WCNAME, hinst);
	return 0;
}

void game_quit(void)
{
	quit = 1;
}

unsigned long game_getmsec(void)
{
	return timeGetTime() - start_time;
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
	BringWindowToTop(win);
	SetForegroundWindow(win);

	return 0;
}

static LRESULT CALLBACK handle_msg(HWND win, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	int key;

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
	case WM_SYSKEYDOWN:
		key = translate_vkey(wparam);
		game_keyboard(key, 1);
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		key = translate_vkey(wparam);
		game_keyboard(key, 0);
		break;

	case WM_LBUTTONDOWN:
		game_mousebtn(0, 1, lparam & 0xffff, lparam >> 16);
		break;
	case WM_MBUTTONDOWN:
		game_mousebtn(1, 1, lparam & 0xffff, lparam >> 16);
		break;
	case WM_RBUTTONDOWN:
		game_mousebtn(2, 1, lparam & 0xffff, lparam >> 16);
		break;
	case WM_LBUTTONUP:
		game_mousebtn(0, 0, lparam & 0xffff, lparam >> 16);
		break;
	case WM_MBUTTONUP:
		game_mousebtn(1, 0, lparam & 0xffff, lparam >> 16);
		break;
	case WM_RBUTTONUP:
		game_mousebtn(2, 0, lparam & 0xffff, lparam >> 16);
		break;

	case WM_MOUSEMOVE:
		game_mousemove(lparam & 0xffff, lparam >> 16);
		break;


	default:
		break;
	}

	return DefWindowProc(win, msg, wparam, lparam);
}


#ifndef VK_OEM_1
#define VK_OEM_1	0xba
#define VK_OEM_2	0xbf
#define VK_OEM_3	0xc0
#define VK_OEM_4	0xdb
#define VK_OEM_5	0xdc
#define VK_OEM_6	0xdd
#define VK_OEM_7	0xde
#endif

static int translate_vkey(int vkey)
{
	switch(vkey) {
	case VK_PRIOR: return KEY_PGUP;
	case VK_NEXT: return KEY_PGDN;
	case VK_END: return KEY_END;
	case VK_HOME: return KEY_HOME;
	case VK_LEFT: return KEY_LEFT;
	case VK_UP: return KEY_UP;
	case VK_RIGHT: return KEY_RIGHT;
	case VK_DOWN: return KEY_DOWN;
	case VK_OEM_1: return ';';
	case VK_OEM_2: return '/';
	case VK_OEM_3: return '`';
	case VK_OEM_4: return '[';
	case VK_OEM_5: return '\\';
	case VK_OEM_6: return ']';
	case VK_OEM_7: return '\'';
	default:
		break;
	}

	if(vkey >= 'A' && vkey <= 'Z') {
		vkey += 32;
	} else if(vkey >= VK_F1 && vkey <= VK_F12) {
		vkey -= VK_F1 + KEY_F1;
	}

	return vkey;
}
