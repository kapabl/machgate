/*
 * PCM cache for Sugar engine _load_ogg replacement. See ogg_cache.h.
 */

#define _GNU_SOURCE
#include "ogg_cache.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define CACHE_DIR_NAME ".machismo-pcm-cache"

/*
 * FNV-1a 64-bit. Small, allocation-free, plenty of entropy for a
 * (filename + freq) key — collisions would only cause cache misses, never
 * correctness issues (header validates rate/channels on load).
 */
static uint64_t fnv1a64(const void *data, size_t len, uint64_t seed)
{
	const uint8_t *p = (const uint8_t *)data;
	uint64_t h = seed;
	for (size_t i = 0; i < len; i++) {
		h ^= p[i];
		h *= 0x100000001b3ULL;
	}
	return h;
}

void ogg_cache_key(const char *filename, size_t filename_len,
                   uint32_t use_freq, char out[33])
{
	uint64_t h1 = fnv1a64(filename, filename_len, 0xcbf29ce484222325ULL);
	uint64_t h2 = fnv1a64(&use_freq, sizeof(use_freq), h1);
	snprintf(out, 33, "%016lx%016lx",
	         (unsigned long)h1, (unsigned long)h2);
}

/*
 * The cache directory lives at <cwd>/.machismo-pcm-cache/. machismo's
 * loader chdir's to the game binary's directory on startup (see
 * machismo.c:934), so for Shotgun King this resolves to
 * shotgun_king.app/Contents/MacOS/.machismo-pcm-cache/ on dev and to
 * gamedata/<app>/Contents/MacOS/.machismo-pcm-cache/ in a port layout.
 */
int ogg_cache_path(const char key[33], char *out, size_t outlen)
{
	/* Ensure cache dir exists (ok if it already does) */
	if (mkdir(CACHE_DIR_NAME, 0755) != 0 && errno != EEXIST) {
		fprintf(stderr, "ogg_cache: mkdir(%s) failed: %s\n",
		        CACHE_DIR_NAME, strerror(errno));
		return -1;
	}
	int n = snprintf(out, outlen, "%s/%s.pcm", CACHE_DIR_NAME, key);
	if (n < 0 || (size_t)n >= outlen)
		return -1;
	return 0;
}

int ogg_cache_try_load(const char key[33], uint32_t expected_freq,
                       void **out_pcm, uint64_t *out_pcm_bytes,
                       uint8_t out_sample_t[OGG_CACHE_SAMPLE_T_BYTES])
{
	char path[512];
	if (ogg_cache_path(key, path, sizeof(path)) != 0)
		return 0;

	int fd = open(path, O_RDONLY);
	if (fd < 0)
		return 0;

	struct ogg_cache_header hdr;
	ssize_t r = read(fd, &hdr, sizeof(hdr));
	if (r != (ssize_t)sizeof(hdr) ||
	    hdr.magic != OGG_CACHE_MAGIC ||
	    hdr.version != OGG_CACHE_VERSION ||
	    hdr.use_freq != expected_freq ||
	    hdr.sample_t_size != OGG_CACHE_SAMPLE_T_BYTES ||
	    hdr.pcm_bytes == 0) {
		close(fd);
		return 0;
	}

	r = read(fd, out_sample_t, OGG_CACHE_SAMPLE_T_BYTES);
	if (r != (ssize_t)OGG_CACHE_SAMPLE_T_BYTES) {
		close(fd);
		return 0;
	}

	/* Validate file size matches header layout (PCM at fixed offset) */
	struct stat st;
	uint64_t want_size = (uint64_t)OGG_CACHE_PCM_OFFSET + hdr.pcm_bytes;
	if (fstat(fd, &st) != 0 || (uint64_t)st.st_size != want_size) {
		close(fd);
		return 0;
	}

	/* Map PCM region only, at page-aligned file offset. MAP_PRIVATE so
	 * the kernel can evict pages under memory pressure and refault. */
	void *mapped = mmap(NULL, (size_t)hdr.pcm_bytes, PROT_READ,
	                    MAP_PRIVATE, fd, (off_t)OGG_CACHE_PCM_OFFSET);
	close(fd);
	if (mapped == MAP_FAILED) {
		fprintf(stderr, "ogg_cache: mmap failed: %s\n", strerror(errno));
		return 0;
	}

	/* Register with shim's mmap registry so free() routes to munmap() */
	typedef void (*registry_fn)(void *, size_t);
	static registry_fn reg = NULL;
	if (!reg)
		reg = (registry_fn)dlsym(RTLD_DEFAULT, "mmap_registry_add");
	if (reg)
		reg(mapped, (size_t)hdr.pcm_bytes);

	*out_pcm = mapped;
	*out_pcm_bytes = hdr.pcm_bytes;
	return 1;
}

int ogg_cache_store(const char key[33], uint32_t use_freq,
                    const uint8_t sample_t_bytes[OGG_CACHE_SAMPLE_T_BYTES],
                    const void *pcm, uint64_t pcm_bytes)
{
	char path[512];
	if (ogg_cache_path(key, path, sizeof(path)) != 0)
		return -1;

	char tmp[512 + 16];
	int n = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
	if (n < 0 || (size_t)n >= sizeof(tmp))
		return -1;

	int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		fprintf(stderr, "ogg_cache: open(%s) failed: %s\n",
		        tmp, strerror(errno));
		return -1;
	}

	struct ogg_cache_header hdr = {
		.magic = OGG_CACHE_MAGIC,
		.version = OGG_CACHE_VERSION,
		.use_freq = use_freq,
		.sample_t_size = OGG_CACHE_SAMPLE_T_BYTES,
		.pcm_bytes = pcm_bytes,
		.reserved = 0,
	};

	ssize_t w = write(fd, &hdr, sizeof(hdr));
	if (w != (ssize_t)sizeof(hdr))
		goto fail;

	w = write(fd, sample_t_bytes, OGG_CACHE_SAMPLE_T_BYTES);
	if (w != (ssize_t)OGG_CACHE_SAMPLE_T_BYTES)
		goto fail;

	/* Pad out to the fixed PCM offset so mmap can use a page-aligned
	 * file offset at load time. ftruncate + lseek avoids writing zeros. */
	if (ftruncate(fd, OGG_CACHE_PCM_OFFSET) != 0)
		goto fail;
	if (lseek(fd, OGG_CACHE_PCM_OFFSET, SEEK_SET) == (off_t)-1)
		goto fail;

	const uint8_t *p = (const uint8_t *)pcm;
	uint64_t remaining = pcm_bytes;
	while (remaining > 0) {
		size_t chunk = remaining > (1u << 20) ? (1u << 20) : (size_t)remaining;
		w = write(fd, p, chunk);
		if (w <= 0)
			goto fail;
		p += w;
		remaining -= (uint64_t)w;
	}

	if (close(fd) != 0)
		goto fail_no_close;
	if (rename(tmp, path) != 0) {
		fprintf(stderr, "ogg_cache: rename(%s -> %s) failed: %s\n",
		        tmp, path, strerror(errno));
		unlink(tmp);
		return -1;
	}
	return 0;

fail:
	close(fd);
fail_no_close:
	unlink(tmp);
	return -1;
}
