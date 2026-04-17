/*
 * Shared helpers for Sugar audio-loader detours.
 *
 * Each Sugar engine generation uses a different decoder entry point
 * (_mp3_fromfile on SDL2 builds, _load_ogg on SDL3 builds) but both write
 * their decoded PCM into the same sample_t layout, so the cache code is
 * identical. These helpers are linked into both libsugar_patches_sdl2.so
 * and libsugar_patches_sdl3.so.
 */

#ifndef SUGAR_AUDIO_CACHE_H
#define SUGAR_AUDIO_CACHE_H

#include <stddef.h>
#include <stdint.h>

typedef int64_t (*audio_load_fn_t)(int64_t arg1, int64_t arg2,
                                   int64_t arg3, int64_t arg4);

/* Cache probe. Returns 1 on hit (sample_t populated, ready to return),
 * 0 on miss. Always writes the 33-byte cache key so the caller can reuse
 * it for store. `tag` is a log prefix like "_load_ogg". */
int audio_cache_probe(int64_t string_ptr, int64_t sample_t_ptr,
                      uint32_t use_freq, const char *tag,
                      char key_out[33]);

/* Cache store. Writes the decoded PCM to disk (if large enough) and then
 * swaps the heap allocation for an mmap'd view of the cache file so the
 * kernel can reclaim pages under pressure. */
void audio_cache_store(int64_t string_ptr, int64_t sample_t_ptr,
                       uint32_t use_freq, const char *tag,
                       const char key[33]);

/* Build an RWX trampoline that replays the original first instruction of
 * `target_addr` and then jumps to target_addr+4. The machismo override
 * trampoline will have overwritten that first 4 bytes with a B into our
 * MACHO_FUNC by the time we're called, so we must read the instruction
 * out before that happens (from our ctor). Returns NULL on failure. */
audio_load_fn_t audio_cache_build_detour(uintptr_t target_addr);

#endif /* SUGAR_AUDIO_CACHE_H */
