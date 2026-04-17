/*
 * sugar::gfx::_shader_flip bypass (older Sugar engine / SDL2, e.g. Wratch's Den).
 *
 * The old Sugar engine attaches a `shader_ctx` to every window on creation
 * and routes flips through `_shader_flip` — even when the user passes the
 * `shaderless` game arg, which only skips loading the user-supplied stretch
 * / CRT shaders. The window's shader_ctx stays non-NULL (pointing at an
 * uncompiled default), so `_shader_flip` still issues a `glDrawElements`
 * with no program bound. gl4es falls through to its fixed-pipeline
 * emulator, fpe->prog is 0, and the draw crashes in realize_glenv.
 *
 * We can't compile the real stretch shader under gl4es because it uses
 * GLSL 1.50 `usampler2D` which gl4es's translator doesn't handle. Instead,
 * bypass the shader path entirely: replace `_shader_flip` with a tail call
 * to `_flip` (the non-shader blit path).
 */

#define _GNU_SOURCE
#include "stubs/apple_abi.h"
#include "loader.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>

typedef void (*flip_fn_t)(void *window);
static flip_fn_t p_flip = NULL;

/* window_t->shader_ctx lives at offset 0x80 in the old Sugar engine
 * (Wratch's Den). _flip itself checks this field and tail-calls
 * _shader_flip when non-NULL, so we must clear it before delegating,
 * or we recurse infinitely through _flip → _shader_flip → _flip. */
#define SUGAR_WINDOW_SHADER_CTX_OFFSET 0x80

MACHO_FUNC(_ZN5sugar3gfx12_shader_flipEPNS0_8window_tE, void, void *window)
{
	if (window)
		*(void *volatile *)((uint8_t *)window +
		                    SUGAR_WINDOW_SHADER_CTX_OFFSET) = NULL;
	if (p_flip)
		p_flip(window);
}

__attribute__((constructor))
static void sugar_shader_bypass_init(void)
{
	struct load_results *lr =
		(struct load_results *)dlsym(RTLD_DEFAULT, "machismo_load_results");
	typedef uintptr_t (*resolver_fn_t)(void *mh, uintptr_t slide,
	                                   const char *name);
	resolver_fn_t resolver_lookup_symbol =
		(resolver_fn_t)dlsym(RTLD_DEFAULT, "resolver_lookup_symbol");
	if (!lr || !resolver_lookup_symbol || !lr->mh) {
		fprintf(stderr, "sugar_shader_bypass: machismo internals unavailable\n");
		return;
	}

	uintptr_t addr = resolver_lookup_symbol((void *)lr->mh, lr->slide,
		"__ZN5sugar3gfx5_flipEPNS0_8window_tE");
	if (addr) {
		p_flip = (flip_fn_t)addr;
		fprintf(stderr,
		        "sugar_shader_bypass: _shader_flip redirected to _flip @ %p\n",
		        (void *)addr);
	} else {
		fprintf(stderr, "sugar_shader_bypass: _flip not found\n");
	}
}
