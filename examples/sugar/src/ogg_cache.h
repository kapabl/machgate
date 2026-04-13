/*
 * PCM cache for Sugar engine _load_ogg replacement.
 *
 * Caches the post-converted interleaved S16 PCM buffer produced by
 * sugar::audio::_load_ogg to a file so subsequent loads skip the
 * stb_vorbis decode + resample and mmap the PCM directly.
 */

#ifndef OGG_CACHE_H
#define OGG_CACHE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* On-disk header. Little-endian aarch64. Bump version on any layout change. */
#define OGG_CACHE_MAGIC   0x43504753u  /* 'SGPC' — Sugar PCM Cache */
#define OGG_CACHE_VERSION 2u
#define OGG_CACHE_SAMPLE_T_BYTES 32u   /* sugar::audio::sample_t size we snapshot */

/* PCM data starts at this fixed offset in the cache file so mmap() can
 * use it directly. Must be a multiple of every page size we may run on:
 * 4 KB (most ARM handhelds), 8 KB, and 16 KB (Apple Silicon / Asahi). */
#define OGG_CACHE_PCM_OFFSET 16384u

struct ogg_cache_header {
	uint32_t magic;
	uint32_t version;
	uint32_t use_freq;      /* __use_freq at cache time (sanity check on load) */
	uint32_t sample_t_size; /* always OGG_CACHE_SAMPLE_T_BYTES */
	uint64_t pcm_bytes;     /* size of the PCM data section */
	uint64_t reserved;
	/* followed by: uint8_t sample_t_bytes[sample_t_size]; */
	/* followed by: uint8_t pcm[pcm_bytes]; */
};

/*
 * Compute a cache key for a (filename, use_freq) tuple. Writes a
 * NUL-terminated lowercase hex string to out (must be at least 33 bytes).
 * filename should be the game's logical name (as passed to _load_ogg),
 * not a resolved disk path — this keeps keys stable across machines.
 */
void ogg_cache_key(const char *filename, size_t filename_len,
                   uint32_t use_freq, char out[33]);

/*
 * Resolve the cache file path for a given key. Returns the full path in
 * out (at least 512 bytes). Ensures the cache directory exists.
 * Returns 0 on success, -1 if the cache dir can't be created.
 */
int ogg_cache_path(const char key[33], char *out, size_t outlen);

/*
 * Try to load a cached PCM for the given key. On hit, returns 1 and fills:
 *   - out_pcm: MAP_PRIVATE mmap of the PCM region (registered with
 *              mmap_registry so free() routes to munmap())
 *   - out_sample_t: OGG_CACHE_SAMPLE_T_BYTES of snapshotted sample_t state
 *   - out_pcm_bytes: size of the PCM region
 * On miss or error, returns 0.
 */
int ogg_cache_try_load(const char key[33], uint32_t expected_freq,
                       void **out_pcm, uint64_t *out_pcm_bytes,
                       uint8_t out_sample_t[OGG_CACHE_SAMPLE_T_BYTES]);

/*
 * Write a cache entry. sample_t is a 32-byte snapshot of the post-
 * _load_ogg sample_t struct; pcm is the decoded PCM buffer of pcm_bytes.
 * Written atomically via <path>.tmp + rename. Returns 0 on success.
 */
int ogg_cache_store(const char key[33], uint32_t use_freq,
                    const uint8_t sample_t_bytes[OGG_CACHE_SAMPLE_T_BYTES],
                    const void *pcm, uint64_t pcm_bytes);

#ifdef __cplusplus
}
#endif

#endif /* OGG_CACHE_H */
