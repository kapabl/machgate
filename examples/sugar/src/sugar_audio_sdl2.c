/*
 * sugar::audio::_mp3_fromdata detour (older Sugar engine / SDL2 builds,
 * e.g. Wratch's Den). Caches the decoded PCM via audio_cache.
 *
 * _mp3_fromdata is the dispatched entry from _get_sample for .mp3 files —
 * it resolves the logical filename via sugar::file::getprj, pulls bytes
 * out of data.sgr, and feeds _mp3_fromrawdata. Only falls back to
 * _mp3_fromfile for loose files (not used in packaged builds).
 *
 * Signature: (std::string, sample_t*) -> sample_t*
 */

#define _GNU_SOURCE
#include "stubs/apple_abi.h"
#include "loader.h"
#include "audio_cache.h"

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>

static audio_load_fn_t call_original = NULL;
static uint32_t       *use_freq_ptr  = NULL;

MACHO_FUNC(
	_ZN5sugar5audio13_mp3_fromdataENSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEPNS0_8sample_tE,
	int64_t, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4)
{
	(void)arg3; (void)arg4;
	if (!call_original || !use_freq_ptr) {
		fprintf(stderr, "sugar_patches_sdl2: _mp3_fromdata detour not initialized\n");
		return 0;
	}
	uint32_t use_freq = *use_freq_ptr;
	char key[33];
	if (audio_cache_probe(arg1, arg2, use_freq, "_mp3_fromdata", key))
		return arg2;
	int64_t ret = call_original(arg1, arg2, 0, 0);
	if (ret == 0)
		return 0;
	audio_cache_store(arg1, arg2, use_freq, "_mp3_fromdata", key);
	return ret;
}

__attribute__((constructor))
static void sugar_audio_sdl2_init(void)
{
	struct load_results *lr =
		(struct load_results *)dlsym(RTLD_DEFAULT, "machismo_load_results");
	typedef uintptr_t (*resolver_fn_t)(void *mh, uintptr_t slide,
	                                   const char *name);
	resolver_fn_t resolver_lookup_symbol =
		(resolver_fn_t)dlsym(RTLD_DEFAULT, "resolver_lookup_symbol");
	if (!lr || !resolver_lookup_symbol || !lr->mh) {
		fprintf(stderr, "sugar_patches_sdl2: machismo internals unavailable\n");
		return;
	}

	use_freq_ptr = (uint32_t *)resolver_lookup_symbol(
		(void *)lr->mh, lr->slide, "__ZN5sugar5audio10__use_freqE");
	if (!use_freq_ptr) {
		fprintf(stderr, "sugar_patches_sdl2: __use_freq not found\n");
		return;
	}

	uintptr_t addr = resolver_lookup_symbol((void *)lr->mh, lr->slide,
		"__ZN5sugar5audio13_mp3_fromdataENSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEPNS0_8sample_tE");
	if (addr)
		call_original = audio_cache_build_detour(addr);

	fprintf(stderr,
	        "sugar_patches_sdl2: ready; _mp3_fromdata=%p(->%p) __use_freq@%p=%u\n",
	        (void *)addr, (void *)call_original,
	        (void *)use_freq_ptr, *use_freq_ptr);
}
