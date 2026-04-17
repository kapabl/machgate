/*
 * Shared Sugar audio-cache helpers. See audio_cache.h.
 */

#define _GNU_SOURCE
#include "stubs/apple_abi.h"
#include "audio_cache.h"
#include "ogg_cache.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* Apple-ABI std::string accessor that handles both SSO and long forms. */
static const char *apple_str_data(const apple_string_t *s, size_t *out_len)
{
	if (s->cap & ((uint64_t)1 << 63)) {
		if (out_len) *out_len = s->size;
		return s->data;
	}
	/* Short (SSO): data stored inline, length in the 24th byte. */
	const uint8_t *bytes = (const uint8_t *)s;
	if (out_len) *out_len = (size_t)bytes[23];
	return (const char *)s;
}

/* Only cache files above this threshold. Small SFX don't benefit and would
 * just litter the cache directory. 256 KB ≈ 1.5 s stereo S16 @ 44.1 kHz. */
#define MIN_CACHE_BYTES (256u * 1024u)

extern size_t malloc_usable_size(void *ptr);

int audio_cache_probe(int64_t string_ptr, int64_t sample_t_ptr,
                      uint32_t use_freq, const char *tag,
                      char key_out[33])
{
	size_t filename_len = 0;
	const char *filename =
		apple_str_data((const apple_string_t *)string_ptr, &filename_len);
	ogg_cache_key(filename, filename_len, use_freq, key_out);

	void *cached_pcm = NULL;
	uint64_t cached_bytes = 0;
	uint8_t cached_sample_t[OGG_CACHE_SAMPLE_T_BYTES];
	if (!ogg_cache_try_load(key_out, use_freq, &cached_pcm, &cached_bytes,
	                        cached_sample_t))
		return 0;

	memcpy((void *)sample_t_ptr, cached_sample_t, OGG_CACHE_SAMPLE_T_BYTES);
	*(void **)((uint8_t *)sample_t_ptr + 0) = cached_pcm;
	(void)filename; (void)filename_len; (void)cached_bytes; (void)tag;
	return 1;
}

void audio_cache_store(int64_t string_ptr, int64_t sample_t_ptr,
                       uint32_t use_freq, const char *tag,
                       const char key[33])
{
	void *pcm = *(void **)((uint8_t *)sample_t_ptr + 0);
	if (!pcm)
		return;

	/* Trust the allocator over our reverse-engineered layout guesses. */
	size_t pcm_bytes = malloc_usable_size(pcm);
	if (pcm_bytes < MIN_CACHE_BYTES)
		return;

	size_t filename_len = 0;
	const char *filename =
		apple_str_data((const apple_string_t *)string_ptr, &filename_len);

	/* Snapshot the whole sample_t (minus the PCM ptr) for faithful replay */
	uint8_t sample_t_snap[OGG_CACHE_SAMPLE_T_BYTES];
	memcpy(sample_t_snap, (void *)sample_t_ptr, OGG_CACHE_SAMPLE_T_BYTES);

	if (ogg_cache_store(key, use_freq, sample_t_snap, pcm,
	                    (uint64_t)pcm_bytes) != 0) {
		fprintf(stderr, "sugar_patches: cache_store failed for %.*s\n",
		        (int)filename_len, filename);
		return;
	}

	fprintf(stderr, "sugar_patches: %s cached: %.*s (%zu bytes)\n",
	        tag, (int)filename_len, filename, pcm_bytes);

	/*
	 * Swap the heap PCM for an mmap'd view of the cache file so the
	 * kernel can reclaim its pages under memory pressure.
	 */
	void *mapped = NULL;
	uint64_t mapped_bytes = 0;
	uint8_t discard[OGG_CACHE_SAMPLE_T_BYTES];
	if (ogg_cache_try_load(key, use_freq, &mapped, &mapped_bytes, discard)) {
		free(pcm);
		*(void **)((uint8_t *)sample_t_ptr + 0) = mapped;
	}
}

audio_load_fn_t audio_cache_build_detour(uintptr_t target_addr)
{
	uint32_t orig_insn;
	memcpy(&orig_insn, (const void *)target_addr, sizeof(orig_insn));

	/* Refuse to relocate PC-relative branches. At audio loader entries
	 * we expect a stack-setup (sub sp / stp x29, x30) — not a branch. */
	uint32_t top6 = orig_insn >> 26;
	if (top6 == 0x05 || top6 == 0x25 ||
	    top6 == 0x15 || top6 == 0x35) {
		fprintf(stderr, "sugar_patches: refusing to detour: entry instr "
		        "0x%08x is a branch\n", orig_insn);
		return NULL;
	}

	long pgsz = sysconf(_SC_PAGESIZE);
	if (pgsz <= 0) pgsz = 16384;
	void *page = mmap(NULL, (size_t)pgsz,
	                  PROT_READ | PROT_WRITE | PROT_EXEC,
	                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (page == MAP_FAILED) {
		page = mmap(NULL, (size_t)pgsz, PROT_READ | PROT_WRITE,
		            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (page == MAP_FAILED) {
			fprintf(stderr, "sugar_patches: mmap trampoline failed: %s\n",
			        strerror(errno));
			return NULL;
		}
	}

	uint32_t *code = (uint32_t *)page;
	code[0] = orig_insn;
	code[1] = 0x58000050;      /* ldr x16, #8 */
	code[2] = 0xd61f0200;      /* br  x16     */
	*(uint64_t *)&code[3] = (uint64_t)(target_addr + 4);

	mprotect(page, (size_t)pgsz, PROT_READ | PROT_EXEC);
	__builtin___clear_cache((char *)page, (char *)page + 32);

	return (audio_load_fn_t)page;
}
