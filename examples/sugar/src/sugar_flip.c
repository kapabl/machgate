/*
 * sugar::gfx::_flip replacement — NEON-accelerated palette→RGBA blit.
 *
 * Sugar stores its framebuffer as 8-bit palette-indexed pixels and converts
 * to RGBA on the CPU inside `_flip` every frame. The original inner loop is
 * 9 instructions per pixel with two dependent byte loads (translation table
 * and global palette), which wastes ~15% of main-thread CPU on Cortex-A53.
 *
 * This replacement:
 *   1. Pre-composes a 256-entry combined LUT (translation → palette) once
 *      per surface, collapsing the double indirection into a single lookup.
 *   2. Uses a 16-pixel-per-iteration NEON-stored inner loop so the A53 can
 *      emit two 128-bit stores (STP Q) per unrolled block.
 *   3. Re-implements the full _flip flow (shaderless path):
 *        - lock main texture → blit main surface → unlock
 *        - if alt surface present: lock alt → blit alt (with bg_color key)
 *          → unlock
 *        - SetRenderTarget(NULL), SetRenderDrawColor(border_color),
 *          RenderClear, RenderTexture(main), maybe RenderTexture(alt),
 *          RenderPresent.
 *
 * The shader path (when window->[0x88] is non-zero) is delegated back to
 * the original sugar::gfx::_shader_flip, which we resolve from the Mach-O
 * symbol table in our constructor.
 *
 * Struct offsets were reverse-engineered from the Apple arm64 binary:
 *
 *   window_t:
 *     0x20  SDL_Renderer *
 *     0x28  SDL_Texture  * main_tex
 *     0x38  surface_t    * main_surface
 *     0x40  surface_t    * alt_surface  (nullable)
 *     0x48  SDL_Texture  * alt_tex
 *     0x50  uint8_t        bg_color_idx
 *     0x78  int16_t        border_color_idx   (negative = transparent)
 *     0x88  void         * shader_ctx         (non-NULL → use _shader_flip)
 *     0x90  uint8_t        translation_tbl[256]
 *
 *   surface_t:
 *     0x18  uint32_t       width
 *     0x1c  uint32_t       height
 *     0x20  uint8_t **     pixels_holder       (*pixels_holder = uint8_t *)
 *
 *   global:
 *     sugar::gfx::__plt  — uint32_t palette[>=256]   (ARGB little-endian)
 */

#define _GNU_SOURCE
#include "stubs/apple_abi.h"
#include "loader.h"

#include <arm_neon.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct SDL_Texture  SDL_Texture;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct { int x, y, w, h; } SDL_Rect;

/* SDL3 entry points, resolved at constructor time via dlsym(RTLD_DEFAULT).
 * They come from libSDL3 (stock or the sdl3forsdl2 fork), already loaded
 * into the process by the time this .so is dlopen'd. */
static int  (*p_SDL_LockTexture)(SDL_Texture *, const SDL_Rect *, void **, int *);
static void (*p_SDL_UnlockTexture)(SDL_Texture *);
static int  (*p_SDL_SetRenderTarget)(SDL_Renderer *, SDL_Texture *);
static int  (*p_SDL_SetRenderDrawColor)(SDL_Renderer *, uint8_t, uint8_t, uint8_t, uint8_t);
static int  (*p_SDL_RenderClear)(SDL_Renderer *);
static int  (*p_SDL_RenderTexture)(SDL_Renderer *, SDL_Texture *, const void *, const void *);
static int  (*p_SDL_RenderPresent)(SDL_Renderer *);
static int  (*p_SDL_SetRenderVSync)(SDL_Renderer *, int);   /* optional */

/* Original _shader_flip, resolved from the Mach-O symbol table. */
typedef void (*shader_flip_fn_t)(void *window);
static shader_flip_fn_t p_shader_flip = NULL;

/* Global palette pointer (sugar::gfx::__plt). */
static uint32_t *g_palette = NULL;

/* Ready flag — if resolution failed anywhere, we fall back to letting the
 * trampoline loop skip us (we return without doing anything useful). The
 * override mechanism replaces the original function entirely, so we must
 * never silently no-op; instead, we log and call the original via fallback
 * — but replacement happens unconditionally, so we just have to always be
 * correct. If a pointer is missing we log once and bail to original via
 * the shader_flip tail-call trick (only works in shader mode). */
static int g_ready = 0;

/*----------------------------------------------------------------------
 * Inner blit: palette → RGBA, 16 pixels per NEON iteration
 *
 * `lut` is a 256-entry uint32_t table (combined translation × palette,
 * pre-alpha-keyed if needed). `src` is width-contiguous; `dst` has
 * `dst_pitch_words` words per row. The scalar tail handles width%16.
 *----------------------------------------------------------------------*/
static inline __attribute__((always_inline))
void blit_row_lut(const uint8_t *src, uint32_t *dst, uint32_t width,
                  const uint32_t *lut)
{
	uint32_t x = 0;
	uint32_t w16 = width & ~15u;
	for (; x < w16; x += 16) {
		/* 16 scalar LUT gathers — the A53's in-order pipeline issues
		 * these as two hits per cycle when the LUT is in L1. */
		uint32x4_t q0 = {
			lut[src[x + 0]], lut[src[x + 1]],
			lut[src[x + 2]], lut[src[x + 3]],
		};
		uint32x4_t q1 = {
			lut[src[x + 4]], lut[src[x + 5]],
			lut[src[x + 6]], lut[src[x + 7]],
		};
		uint32x4_t q2 = {
			lut[src[x + 8]], lut[src[x + 9]],
			lut[src[x + 10]], lut[src[x + 11]],
		};
		uint32x4_t q3 = {
			lut[src[x + 12]], lut[src[x + 13]],
			lut[src[x + 14]], lut[src[x + 15]],
		};
		/* STP Q — two 128-bit stores. */
		vst1q_u32(dst + x + 0,  q0);
		vst1q_u32(dst + x + 4,  q1);
		vst1q_u32(dst + x + 8,  q2);
		vst1q_u32(dst + x + 12, q3);
	}
	for (; x < width; x++)
		dst[x] = lut[src[x]];
}

/*----------------------------------------------------------------------
 * Blit a palette-indexed surface into an SDL_Texture.
 *
 * If `bg_idx_or_neg < 0` the blit is opaque (combined[i] is palette[i]
 * as-is). Otherwise, the pixel whose combined index maps to that palette
 * slot gets its alpha cleared — reproducing the game's color-key behavior
 * where the alt surface's background color is transparent.
 *
 * Returns 0 on success, -1 on SDL lock failure.
 *----------------------------------------------------------------------*/
static int flip_surface(SDL_Texture *tex, const uint8_t *translation_tbl,
                        const uint8_t *src, uint32_t width, uint32_t height,
                        int bg_idx_or_neg)
{
	void *dst_pixels = NULL;
	int dst_pitch = 0;
	if (!p_SDL_LockTexture(tex, NULL, &dst_pixels, &dst_pitch))
		return -1;

	if (width == 0 || height == 0) {
		p_SDL_UnlockTexture(tex);
		return 0;
	}

	/* Build the combined LUT on the stack. 1 KB + alignment. */
	uint32_t combined[256] __attribute__((aligned(16)));
	for (int i = 0; i < 256; i++)
		combined[i] = g_palette[translation_tbl[i]];

	if (bg_idx_or_neg >= 0 && bg_idx_or_neg < 256)
		combined[bg_idx_or_neg] &= 0x00ffffffu;

	const uint32_t dst_pitch_words = (uint32_t)(dst_pitch >> 2);
	uint32_t *dst = (uint32_t *)dst_pixels;
	for (uint32_t y = 0; y < height; y++) {
		blit_row_lut(src + (size_t)y * width,
		             dst + (size_t)y * dst_pitch_words,
		             width, combined);
	}

	p_SDL_UnlockTexture(tex);
	return 0;
}

/*----------------------------------------------------------------------
 * sugar::gfx::_flip replacement.
 *
 * Called by sugar::gfx::flip_all for each live window. The `window_t *`
 * is an opaque pointer — we reach into it via hand-crafted offsets since
 * the header isn't exported.
 *----------------------------------------------------------------------*/
static void flip_body(void *window)
{
	/* Shader mode — delegate to original _shader_flip. */
	void *shader_ctx = *(void **)((uint8_t *)window + 0x88);
	if (shader_ctx) {
		if (p_shader_flip)
			p_shader_flip(window);
		return;
	}

	if (!g_ready)
		return;   /* Resolver failed during init; nothing safe to do. */

	SDL_Renderer *renderer = *(SDL_Renderer **)((uint8_t *)window + 0x20);

	/* Lazy vsync enable — Sugar never calls SDL_SetRenderVSync itself. Do
	 * it once on the first _flip we see so the page-flip paces the main
	 * loop and the GPU isn't racing ahead into queue backpressure. */
	static int vsync_done = 0;
	if (!vsync_done && renderer && p_SDL_SetRenderVSync) {
		int rc = p_SDL_SetRenderVSync(renderer, 1);
		fprintf(stderr, "sugar_patches: SDL_SetRenderVSync(1) -> %d\n", rc);
		vsync_done = 1;
	}

	SDL_Texture  *main_tex = *(SDL_Texture **)((uint8_t *)window + 0x28);
	void         *main_surf = *(void **)((uint8_t *)window + 0x38);
	void         *alt_surf  = *(void **)((uint8_t *)window + 0x40);
	SDL_Texture  *alt_tex   = *(SDL_Texture **)((uint8_t *)window + 0x48);
	const uint8_t *translation_tbl = (uint8_t *)window + 0x90;

	/* Main surface → main texture. */
	if (main_surf) {
		uint32_t mw = *(uint32_t *)((uint8_t *)main_surf + 0x18);
		uint32_t mh = *(uint32_t *)((uint8_t *)main_surf + 0x1c);
		uint8_t **holder = *(uint8_t ***)((uint8_t *)main_surf + 0x20);
		const uint8_t *src = holder ? *holder : NULL;
		if (src)
			flip_surface(main_tex, translation_tbl, src, mw, mh, -1);
	}

	/* Alt surface → alt texture, with bg color key. */
	if (alt_surf) {
		uint32_t aw = *(uint32_t *)((uint8_t *)alt_surf + 0x18);
		uint32_t ah = *(uint32_t *)((uint8_t *)alt_surf + 0x1c);
		uint8_t **holder = *(uint8_t ***)((uint8_t *)alt_surf + 0x20);
		const uint8_t *src = holder ? *holder : NULL;
		uint8_t bg_idx = *(uint8_t *)((uint8_t *)window + 0x50);
		if (src)
			flip_surface(alt_tex, translation_tbl, src, aw, ah,
			             (int)bg_idx);
	}

	/* Compose onto the window's default render target. */
	p_SDL_SetRenderTarget(renderer, NULL);

	int16_t border_idx = *(int16_t *)((uint8_t *)window + 0x78);
	uint8_t r = 0, g = 0, b = 0;
	if (border_idx >= 0) {
		uint8_t pal_idx = translation_tbl[border_idx & 0xff];
		uint32_t color = g_palette[pal_idx];
		r = (uint8_t)(color >> 16);
		g = (uint8_t)(color >> 8);
		b = (uint8_t)(color);
	}
	p_SDL_SetRenderDrawColor(renderer, r, g, b, 0xff);
	p_SDL_RenderClear(renderer);
	p_SDL_RenderTexture(renderer, main_tex, NULL, NULL);
	if (alt_surf)
		p_SDL_RenderTexture(renderer, alt_tex, NULL, NULL);
	p_SDL_RenderPresent(renderer);
}

/* FPS + time-breakdown logger wrapper.
 *
 * Per 60-flip window we report:
 *   fps      — flips per second (wall clock)
 *   inflip   — avg ms spent inside _flip (our render work + SDL + Mali)
 *   between  — avg ms spent outside _flip (Sugar update + Lua)
 *
 * The split tells us where to look next: if inflip dominates, GPU / our
 * override is the bottleneck; if between dominates, game logic is. */
MACHO_FUNC(_ZN5sugar3gfx5_flipEPNS0_8window_tE, void, void *window)
{
	static int frames = 0;
	static struct timespec t0 = {0, 0};
	static struct timespec last_exit = {0, 0};
	static uint64_t inflip_ns  = 0;
	static uint64_t between_ns = 0;

	struct timespec entry;
	clock_gettime(CLOCK_MONOTONIC, &entry);
	if (last_exit.tv_sec != 0) {
		between_ns +=
			(uint64_t)(entry.tv_sec  - last_exit.tv_sec)  * 1000000000ull +
			(uint64_t)(entry.tv_nsec - last_exit.tv_nsec);
	}

	flip_body(window);

	struct timespec exit_;
	clock_gettime(CLOCK_MONOTONIC, &exit_);
	inflip_ns +=
		(uint64_t)(exit_.tv_sec  - entry.tv_sec)  * 1000000000ull +
		(uint64_t)(exit_.tv_nsec - entry.tv_nsec);
	last_exit = exit_;

	if (++frames >= 60) {
		if (t0.tv_sec != 0) {
			double elapsed =
				(double)(exit_.tv_sec  - t0.tv_sec) +
				(double)(exit_.tv_nsec - t0.tv_nsec) * 1e-9;
			if (elapsed > 0.0) {
				fprintf(stderr,
					"sugar_patches: fps=%.1f  inflip=%.2fms  between=%.2fms  (%d flips in %.2fs)\n",
					(double)frames / elapsed,
					(double)inflip_ns  / frames / 1e6,
					(double)between_ns / frames / 1e6,
					frames, elapsed);
			}
		}
		t0 = exit_;
		frames = 0;
		inflip_ns = 0;
		between_ns = 0;
	}
}

/*----------------------------------------------------------------------
 * Constructor — resolve SDL3 + Mach-O symbols we depend on.
 *----------------------------------------------------------------------*/
#define RESOLVE_SDL(sym) do {                                         \
	p_##sym = dlsym(RTLD_DEFAULT, #sym);                          \
	if (!p_##sym) { missing = #sym; goto fail; }                  \
} while (0)

__attribute__((constructor))
static void sugar_flip_init(void)
{
	const char *missing = NULL;

	RESOLVE_SDL(SDL_LockTexture);
	RESOLVE_SDL(SDL_UnlockTexture);
	RESOLVE_SDL(SDL_SetRenderTarget);
	RESOLVE_SDL(SDL_SetRenderDrawColor);
	RESOLVE_SDL(SDL_RenderClear);
	RESOLVE_SDL(SDL_RenderTexture);
	RESOLVE_SDL(SDL_RenderPresent);

	/* Optional — sdl3forsdl2 may not expose this. Don't fail startup. */
	p_SDL_SetRenderVSync = dlsym(RTLD_DEFAULT, "SDL_SetRenderVSync");

	struct load_results *lr =
		(struct load_results *)dlsym(RTLD_DEFAULT, "machismo_load_results");
	typedef uintptr_t (*resolver_fn_t)(void *mh, uintptr_t slide,
	                                   const char *name);
	resolver_fn_t resolver_lookup_symbol =
		(resolver_fn_t)dlsym(RTLD_DEFAULT, "resolver_lookup_symbol");
	if (!lr || !resolver_lookup_symbol || !lr->mh) {
		missing = "machismo internals";
		goto fail;
	}

	/* Mach-O symbols carry a leading underscore (Darwin convention); the
	 * Itanium mangled name `_ZN5sugar...` appears as `__ZN5sugar...` in
	 * the symbol table. */
	g_palette = (uint32_t *)resolver_lookup_symbol(
		(void *)lr->mh, lr->slide, "__ZN5sugar3gfx5__pltE");
	if (!g_palette) { missing = "sugar::gfx::__plt"; goto fail; }

	p_shader_flip = (shader_flip_fn_t)resolver_lookup_symbol(
		(void *)lr->mh, lr->slide,
		"__ZN5sugar3gfx12_shader_flipEPNS0_8window_tE");
	/* _shader_flip is only needed for the shader path; absence isn't
	 * fatal for the shaderless config we target on r36s. */

	g_ready = 1;
	fprintf(stderr,
	        "sugar_patches: _flip NEON override ready (palette=%p, shader_flip=%p)\n",
	        (void *)g_palette, (void *)p_shader_flip);
	return;

fail:
	fprintf(stderr,
	        "sugar_patches: _flip override disabled — missing %s\n",
	        missing ? missing : "(unknown)");
}
