/*
 * sdl_window_shim.c — renderer-neutral SDL window glue.
 *
 * Window creation/capture + native-handle plumbing shared by every SDL-based
 * machismo port (bgfx, Sugar, Gothic). See sdl_window_shim.h for the rationale;
 * this code was previously embedded in bgfx_shim.c and is otherwise unchanged.
 */
#include "sdl_window_shim.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

/* SDL window flag bits (SDL2 values; identical under SDL3/SDL2-compat). */
#define SDL_WINDOW_FULLSCREEN         0x00000001
#define SDL_WINDOW_FULLSCREEN_DESKTOP 0x00001001
#define SDL_WINDOW_ALLOW_HIGHDPI      0x00002000
#define SDL_WINDOW_METAL              0x20000000

typedef struct {
	uint8_t major;
	uint8_t minor;
	uint8_t patch;
} SDL_version_t;

/* SDL2 function pointers — resolved at runtime via dlsym (SDL2 is already
 * loaded by the trampoline). */
static void* (*sdl_GetWindowFromID)(uint32_t id) = NULL;
static int   (*sdl_GetWindowWMInfo)(void*, void* info) = NULL;
static void  (*sdl_GetVersion)(SDL_version_t* ver) = NULL;
static void  (*sdl_GetWindowSize)(void*, int*, int*) = NULL;
static void  (*sdl_GL_GetDrawableSize)(void*, int*, int*) = NULL;

/* The SDL window captured by sdl_create_window_wrapper. */
static void* captured_sdl_window = NULL;

static void resolve_sdl_funcs(void)
{
	if (sdl_GetWindowFromID) return;

	sdl_GetWindowFromID    = dlsym(RTLD_DEFAULT, "SDL_GetWindowFromID");
	sdl_GetWindowWMInfo    = dlsym(RTLD_DEFAULT, "SDL_GetWindowWMInfo");
	sdl_GetVersion         = dlsym(RTLD_DEFAULT, "SDL_GetVersion");
	sdl_GetWindowSize      = dlsym(RTLD_DEFAULT, "SDL_GetWindowSize");
	sdl_GL_GetDrawableSize = dlsym(RTLD_DEFAULT, "SDL_GL_GetDrawableSize");

	if (!sdl_GetWindowFromID || !sdl_GetWindowWMInfo) {
		fprintf(stderr, "sdl_window_shim: warning: SDL2 functions not found, "
		        "cannot get native window handle\n");
	}
}

void* sdl_window_get_captured(void)
{
	return captured_sdl_window;
}

void* sdl_window_from_id(uint32_t id)
{
	resolve_sdl_funcs();
	return sdl_GetWindowFromID ? sdl_GetWindowFromID(id) : NULL;
}

void sdl_window_get_size(void* window, int* w, int* h)
{
	*w = 0; *h = 0;
	resolve_sdl_funcs();
	if (sdl_GetWindowSize)
		sdl_GetWindowSize(window, w, h);
}

void sdl_window_get_drawable_size(void* window, int* w, int* h)
{
	*w = 0; *h = 0;
	resolve_sdl_funcs();
	if (sdl_GL_GetDrawableSize)
		sdl_GL_GetDrawableSize(window, w, h);
	if (*w <= 0 || *h <= 0)
		sdl_window_get_size(window, w, h);
}

int sdl_window_fill_wminfo(void* window, sdl_wminfo_t* out)
{
	resolve_sdl_funcs();
	if (!sdl_GetWindowWMInfo || !sdl_GetVersion || !window)
		return 0;
	memset(out, 0, sizeof(*out));
	sdl_GetVersion((SDL_version_t*)out);   /* version gate for WMInfo */
	return sdl_GetWindowWMInfo(window, out) ? 1 : 0;
}

int sdl_window_is_kmsdrm_env(void)
{
	const char *video_drv = getenv("SDL_VIDEODRIVER");
	return (video_drv && strcmp(video_drv, "KMSDRM") == 0)
	    || (!getenv("DISPLAY") && !getenv("WAYLAND_DISPLAY"));
}

void* sdl_create_window_wrapper(const char* title, int x, int y, int w, int h, unsigned int flags)
{
	typedef void* (*sdl_create_window_fn)(const char*, int, int, int, int, unsigned int);
	static sdl_create_window_fn real_fn = NULL;
	if (!real_fn)
		real_fn = dlsym(RTLD_DEFAULT, "SDL_CreateWindow");
	if (!real_fn) {
		fprintf(stderr, "sdl_window_shim: SDL_CreateWindow not found\n");
		return NULL;
	}

	/* Apple-Silicon Mac builds create a Metal window (SDL_WINDOW_METAL); native
	 * Linux SDL has no Metal backend, so SDL_CreateWindow with that flag fails
	 * and returns NULL — which silently strands any engine that waits on its
	 * window handle (Mina's Gothic engine deadlocks here). We replace the Metal
	 * renderer with EGL/GLES built from the window's native handle, so the flag
	 * is never wanted on Linux: strip it unconditionally. */
	if (flags & SDL_WINDOW_METAL) {
		flags &= ~(unsigned int)SDL_WINDOW_METAL;
		fprintf(stderr, "sdl_window_shim: stripped SDL_WINDOW_METAL (no Metal on Linux)\n");
	}
	/* On KMSDRM, fullscreen is the only valid mode for EGL surface creation. */
	if (sdl_window_is_kmsdrm_env()) {
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		fprintf(stderr, "sdl_window_shim: KMSDRM detected, forcing fullscreen\n");
	}
	/* Keep ALLOW_HIGHDPI — the game was built for Retina displays and handles
	 * HiDPI itself. Stripping it causes a drawable/logical size mismatch on
	 * HiDPI screens (renders at logical size into a physical-size surface,
	 * showing only a quarter of the frame). */
	void* win = real_fn(title, x, y, w, h, flags);
	if (win) {
		captured_sdl_window = win;
		int ww = 0, wh = 0;
		sdl_window_get_drawable_size(win, &ww, &wh);
		fprintf(stderr, "sdl_window_shim: captured SDL window %p (%dx%d, flags=0x%x)\n",
		        win, ww, wh, flags);
	}
	return win;
}

int sdl_set_window_fullscreen_wrapper(void* window, unsigned int flags)
{
	/* Block fullscreen transitions. On macOS, fullscreen changes the display
	 * mode to match the game's resolution (e.g. 960x540). On Linux/Wayland the
	 * display mode can't change, so fullscreen gives a native-res surface while
	 * the game's viewports/cameras stay at the internal resolution — broken. */
	(void)window;
	fprintf(stderr, "sdl_window_shim: blocked SDL_SetWindowFullscreen(0x%x)\n", flags);
	return 0;
}
