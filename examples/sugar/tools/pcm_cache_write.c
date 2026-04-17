/*
 * pcm_cache_write — install-time helper that builds a Sugar PCM cache file
 * from raw S16 stereo PCM on stdin plus metadata on argv.
 *
 * Usage:
 *   pcm_cache_write <output_path> <use_freq> <pcm_bytes> <sample_t_hex>
 *     <sample_t_hex> = 48 hex chars, the 24 stable bytes of sample_t
 *                      (offsets 8..31 inclusive; offsets 0..7 are the heap
 *                      PCM pointer which the engine overwrites on cache
 *                      hit so we can zero it out here).
 *
 * File format (see ogg_cache.h):
 *   offset 0:     magic 'SGPC'           (4 bytes)
 *   offset 4:     version = 2            (4 bytes)
 *   offset 8:     use_freq               (4 bytes)
 *   offset 12:    sample_t_size = 32     (4 bytes)
 *   offset 16:    pcm_bytes              (8 bytes)
 *   offset 24:    reserved = 0           (8 bytes)
 *   offset 32:    sample_t               (32 bytes: 8 zero + 24 from hex)
 *   offset 16384: PCM data               (pcm_bytes)
 *
 * Why this is installable instead of runtime-generated: at runtime the
 * engine's _load_ogg does the decode+resample+snapshot in one shot and our
 * cache wrapper captures the sample_t it produced. At install time we can't
 * easily replicate that (sample_t layout is reverse-engineered and only
 * partially understood), but the stable 24 bytes happen to be content-
 * agnostic enough that we can snapshot them once on a reference machine
 * and ship them as opaque blobs alongside each OGG.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAGIC           0x43504753u   /* 'SGPC' little-endian */
#define VERSION         2u
#define SAMPLE_T_BYTES  32u
#define PCM_OFFSET      16384u

struct header {
	uint32_t magic;
	uint32_t version;
	uint32_t use_freq;
	uint32_t sample_t_size;
	uint64_t pcm_bytes;
	uint64_t reserved;
};

static int hex2bin(const char *hex, uint8_t *out, size_t out_len)
{
	if (strlen(hex) != out_len * 2) return -1;
	for (size_t i = 0; i < out_len; i++) {
		char buf[3] = { hex[2*i], hex[2*i+1], 0 };
		char *end = NULL;
		unsigned long v = strtoul(buf, &end, 16);
		if (*end) return -1;
		out[i] = (uint8_t)v;
	}
	return 0;
}

static int write_all(int fd, const void *buf, size_t n)
{
	const uint8_t *p = buf;
	while (n > 0) {
		ssize_t w = write(fd, p, n);
		if (w < 0) { if (errno == EINTR) continue; return -1; }
		p += w;
		n -= (size_t)w;
	}
	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 5) {
		fprintf(stderr,
		        "usage: %s <output_path> <use_freq> <pcm_bytes> <sample_t_hex_48>\n"
		        "       raw PCM on stdin\n", argv[0]);
		return 2;
	}

	const char *out_path       = argv[1];
	uint32_t    use_freq       = (uint32_t)strtoul(argv[2], NULL, 10);
	uint64_t    expected_pcm   = strtoull(argv[3], NULL, 10);
	const char *sample_t_hex   = argv[4];

	uint8_t sample_t_bytes[SAMPLE_T_BYTES] = {0};
	/* Leave offsets 0..7 zeroed (the PCM ptr; runtime overwrites it anyway). */
	if (hex2bin(sample_t_hex, sample_t_bytes + 8, 24) != 0) {
		fprintf(stderr, "pcm_cache_write: sample_t_hex must be 48 hex chars\n");
		return 2;
	}

	/* Write to tmp + rename for atomicity — partial writes from a
	 * killed install don't leave a corrupt entry the engine would
	 * load later. */
	char tmp_path[4096];
	if (snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", out_path) >= (int)sizeof(tmp_path)) {
		fprintf(stderr, "pcm_cache_write: output path too long\n");
		return 1;
	}

	int fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		fprintf(stderr, "pcm_cache_write: open(%s): %s\n", tmp_path, strerror(errno));
		return 1;
	}

	struct header hdr = {
		.magic         = MAGIC,
		.version       = VERSION,
		.use_freq      = use_freq,
		.sample_t_size = SAMPLE_T_BYTES,
		.pcm_bytes     = expected_pcm,
		.reserved      = 0,
	};
	if (write_all(fd, &hdr, sizeof(hdr)) != 0 ||
	    write_all(fd, sample_t_bytes, SAMPLE_T_BYTES) != 0)
		goto io_fail;

	/* Pad with zeros to PCM_OFFSET. */
	off_t cur = sizeof(hdr) + SAMPLE_T_BYTES;
	static const uint8_t zeros[4096] = {0};
	while (cur < PCM_OFFSET) {
		size_t chunk = (size_t)(PCM_OFFSET - cur);
		if (chunk > sizeof(zeros)) chunk = sizeof(zeros);
		if (write_all(fd, zeros, chunk) != 0) goto io_fail;
		cur += (off_t)chunk;
	}

	/* Stream PCM from stdin, padding with zeros if stdin ends before we
	 * reach expected_pcm, truncating if it's longer. Engine-built caches
	 * include allocator padding (malloc_usable_size rounded up) and
	 * sample_t may reference frames past the real decode length — so
	 * we always write exactly expected_pcm bytes, zero-filled as needed. */
	uint64_t pcm_written = 0;
	int      stdin_eof = 0;
	uint8_t  buf[65536];
	while (pcm_written < expected_pcm) {
		size_t want = (size_t)(expected_pcm - pcm_written);
		if (want > sizeof(buf)) want = sizeof(buf);
		ssize_t r = 0;
		if (!stdin_eof) {
			r = read(0, buf, want);
			if (r < 0) {
				if (errno == EINTR) continue;
				fprintf(stderr, "pcm_cache_write: read stdin: %s\n", strerror(errno));
				goto io_fail;
			}
			if (r == 0) stdin_eof = 1;
		}
		if (stdin_eof) {
			memset(buf, 0, want);
			r = (ssize_t)want;
		}
		if (write_all(fd, buf, (size_t)r) != 0) goto io_fail;
		pcm_written += (uint64_t)r;
	}
	/* Drain any remaining stdin data (truncation path) so the caller's
	 * pipe doesn't SIGPIPE on close. */
	if (!stdin_eof) {
		while (read(0, buf, sizeof(buf)) > 0) { /* discard */ }
	}

	if (close(fd) != 0) {
		fprintf(stderr, "pcm_cache_write: close: %s\n", strerror(errno));
		unlink(tmp_path);
		return 1;
	}

	if (rename(tmp_path, out_path) != 0) {
		fprintf(stderr, "pcm_cache_write: rename(%s → %s): %s\n",
		        tmp_path, out_path, strerror(errno));
		unlink(tmp_path);
		return 1;
	}

	return 0;

io_fail:
	fprintf(stderr, "pcm_cache_write: write %s failed: %s\n", tmp_path, strerror(errno));
	close(fd);
	unlink(tmp_path);
	return 1;
}
