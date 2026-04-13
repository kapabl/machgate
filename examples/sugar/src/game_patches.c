/*
 * Sugar engine game function replacements.
 *
 * Compiled as libsugar_patches.so — loaded by machismo's override trampoline.
 * Each exported function replaces the Mach-O function with the same mangled
 * name.
 *
 * Config: [trampoline.game] override_lib = ./libsugar_patches.so
 */

#define _GNU_SOURCE
#include "stubs/apple_abi.h"
#include "loader.h"
#include "ogg_cache.h"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* ====================================================================
 * Detour: call the original _load_ogg after trampoline_patch_overrides
 * has replaced it with a branch to us.
 *
 * machismo's override trampoline writes a single 4-byte B instruction at
 * the target function's entry. We read + save the original first 4 bytes
 * from our .so's constructor (which runs during dlopen from
 * trampoline_patch_overrides, BEFORE the symbol-iteration phase patches
 * anything). Then we build a small RWX trampoline:
 *
 *   [ saved 4 bytes (original instr) ]
 *   [ ldr x16, #4                    ]  ; = 0x58000030
 *   [ br  x16                        ]  ; = 0xd61f0200
 *   [ .quad <orig + 4>               ]
 *
 * Executing the trampoline runs the original instruction (which is a
 * stack-setup at _load_ogg's entry — PC-independent, safe to relocate)
 * and then jumps back to the original function body just past where the
 * B instruction was written.
 * ==================================================================== */

typedef int64_t (*load_ogg_fn_t)(int64_t arg1, int64_t arg2,
                                 int64_t arg3, int64_t arg4);

static load_ogg_fn_t call_original_load_ogg = NULL;
static uintptr_t load_ogg_addr = 0;
static uint32_t *use_freq_ptr = NULL;

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

/* malloc_usable_size for sizing the heap-allocated PCM so we don't need to
 * guess the sample_t channel layout. Declared here to avoid dragging in
 * <malloc.h> on toolchains that route free() through our shim. */
extern size_t malloc_usable_size(void *ptr);

/*
 * sugar::audio::_load_ogg replacement.
 *
 * Intercepts _load_ogg, checks the PCM cache, and on miss calls the
 * original implementation and stores its output. Returns whatever the
 * original would have returned (sample_t* on success, 0 on failure).
 */
MACHO_FUNC(
	_ZN5sugar5audio9_load_oggENSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEPNS0_8sample_tEbPNS0_6spec_tE,
	int64_t, int64_t arg1, int64_t arg2, int64_t arg3, int64_t arg4)
{
	if (!call_original_load_ogg || !use_freq_ptr) {
		fprintf(stderr, "sugar_patches: _load_ogg detour not initialized\n");
		return 0;
	}

	/* Resolve filename + target freq for cache keying */
	size_t filename_len = 0;
	const char *filename =
		apple_str_data((const apple_string_t *)arg1, &filename_len);

	uint32_t use_freq = *use_freq_ptr;
	char key[33];
	ogg_cache_key(filename, filename_len, use_freq, key);

	/*
	 * Cache hit path — restore the snapshotted sample_t, then swap in
	 * the mmap'd PCM pointer. Skip the decode entirely.
	 */
	void *cached_pcm = NULL;
	uint64_t cached_bytes = 0;
	uint8_t cached_sample_t[OGG_CACHE_SAMPLE_T_BYTES];
	if (ogg_cache_try_load(key, use_freq, &cached_pcm, &cached_bytes,
	                       cached_sample_t)) {
		memcpy((void *)arg2, cached_sample_t, OGG_CACHE_SAMPLE_T_BYTES);
		*(void **)((uint8_t *)arg2 + 0) = cached_pcm;
		fprintf(stderr, "sugar_patches: _load_ogg cache hit: %.*s (%llu PCM bytes)\n",
		        (int)filename_len, filename,
		        (unsigned long long)cached_bytes);
		return arg2;
	}

	/* Cache miss: call the real decoder */
	int64_t ret = call_original_load_ogg(arg1, arg2, arg3, arg4);
	if (ret == 0)
		return 0;  /* original failed — don't cache */

	void *pcm = *(void **)((uint8_t *)arg2 + 0);
	if (!pcm)
		return ret;

	/* Trust the allocator over our reverse-engineered layout guesses. */
	size_t pcm_bytes = malloc_usable_size(pcm);
	if (pcm_bytes < MIN_CACHE_BYTES)
		return ret;

	/* Snapshot the whole sample_t (minus the PCM ptr) for faithful replay */
	uint8_t sample_t_snap[OGG_CACHE_SAMPLE_T_BYTES];
	memcpy(sample_t_snap, (void *)arg2, OGG_CACHE_SAMPLE_T_BYTES);

	if (ogg_cache_store(key, use_freq, sample_t_snap, pcm,
	                    (uint64_t)pcm_bytes) != 0) {
		fprintf(stderr, "sugar_patches: cache_store failed for %.*s\n",
		        (int)filename_len, filename);
		return ret;
	}

	fprintf(stderr, "sugar_patches: _load_ogg cached: %.*s (%zu bytes)\n",
	        (int)filename_len, filename, pcm_bytes);

	/*
	 * Swap the heap PCM for an mmap'd view of the cache file so the
	 * kernel can reclaim its pages under memory pressure. This is where
	 * we actually save resident memory on the first run.
	 */
	void *mapped = NULL;
	uint64_t mapped_bytes = 0;
	uint8_t discard[OGG_CACHE_SAMPLE_T_BYTES];
	if (ogg_cache_try_load(key, use_freq, &mapped, &mapped_bytes, discard)) {
		free(pcm);
		*(void **)((uint8_t *)arg2 + 0) = mapped;
		fprintf(stderr, "sugar_patches: _load_ogg mmap-swapped: %.*s (%llu bytes)\n",
		        (int)filename_len, filename,
		        (unsigned long long)mapped_bytes);
	}

	return ret;
}

/* ====================================================================
 * Constructor — runs during dlopen() from trampoline_patch_overrides,
 * BEFORE the symbol-iteration loop rewrites any function prologues.
 * This is our one chance to read _load_ogg's original first 4 bytes.
 * ==================================================================== */

static int build_detour(uintptr_t target_addr)
{
	/* Read the original first instruction while it's still intact. */
	uint32_t orig_insn;
	memcpy(&orig_insn, (const void *)target_addr, sizeof(orig_insn));

	/* Sanity: bail if this looks like something we can't relocate
	 * (PC-relative branch / conditional branch / adr / adrp). At
	 * _load_ogg's entry we expect a stack-setup (sub sp, sp, #imm)
	 * which is encoded as 0x100xxxxx under the data-processing group. */
	uint32_t top6 = orig_insn >> 26;
	if (top6 == 0x05 || top6 == 0x25 ||   /* unconditional B / BL */
	    top6 == 0x15 || top6 == 0x35) {
		fprintf(stderr, "sugar_patches: refusing to detour: entry instr "
		        "0x%08x is a branch\n", orig_insn);
		return -1;
	}

	/* Allocate a 16-byte RWX trampoline slot. Using anon mmap here
	 * rather than machismo's island pool keeps us self-contained. */
	long pgsz = sysconf(_SC_PAGESIZE);
	if (pgsz <= 0) pgsz = 16384;
	void *page = mmap(NULL, (size_t)pgsz,
	                  PROT_READ | PROT_WRITE | PROT_EXEC,
	                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (page == MAP_FAILED) {
		/* Fall back to RW + mprotect → RX (some hardened kernels
		 * reject direct RWX mappings). */
		page = mmap(NULL, (size_t)pgsz, PROT_READ | PROT_WRITE,
		            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (page == MAP_FAILED) {
			fprintf(stderr, "sugar_patches: mmap trampoline failed: %s\n",
			        strerror(errno));
			return -1;
		}
	}

	uint32_t *code = (uint32_t *)page;
	code[0] = orig_insn;       /* relocated original instruction */
	code[1] = 0x58000050;      /* ldr x16, #8 (load .quad at code[3]) */
	code[2] = 0xd61f0200;      /* br  x16                             */
	*(uint64_t *)&code[3] = (uint64_t)(target_addr + 4);

	/* Switch to RX if we mapped RW */
	mprotect(page, (size_t)pgsz, PROT_READ | PROT_EXEC);
	__builtin___clear_cache((char *)page, (char *)page + 32);

	call_original_load_ogg = (load_ogg_fn_t)page;
	return 0;
}

__attribute__((constructor))
static void sugar_patches_init(void)
{
	/*
	 * We run during dlopen() from trampoline_patch_overrides, which fires
	 * BEFORE gdb_jit_register_macho — so gdb_jit_lookup_name can't see
	 * game symbols yet. Use resolver_lookup_symbol which reads the Mach-O
	 * symbol table directly. The Mach-O is fully loaded by this point.
	 *
	 * Mach-O symbol names have a leading underscore (Darwin convention);
	 * the Itanium mangled name `_ZN5sugar...` becomes `__ZN5sugar...` in
	 * the symbol table.
	 */
	struct load_results *lr =
		(struct load_results *)dlsym(RTLD_DEFAULT, "machismo_load_results");
	typedef uintptr_t (*resolver_fn_t)(void *mh, uintptr_t slide,
	                                   const char *name);
	resolver_fn_t resolver_lookup_symbol =
		(resolver_fn_t)dlsym(RTLD_DEFAULT, "resolver_lookup_symbol");
	if (!lr || !resolver_lookup_symbol || !lr->mh) {
		fprintf(stderr, "sugar_patches: machismo internals unavailable — "
		        "caching disabled\n");
		return;
	}

	load_ogg_addr = resolver_lookup_symbol((void *)lr->mh, lr->slide,
		"__ZN5sugar5audio9_load_oggENSt3__112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEEPNS0_8sample_tEbPNS0_6spec_tE");
	if (!load_ogg_addr) {
		fprintf(stderr, "sugar_patches: _load_ogg symbol not found — "
		        "caching disabled\n");
		return;
	}

	use_freq_ptr = (uint32_t *)resolver_lookup_symbol(
		(void *)lr->mh, lr->slide, "__ZN5sugar5audio10__use_freqE");
	if (!use_freq_ptr) {
		fprintf(stderr, "sugar_patches: __use_freq symbol not found — "
		        "caching disabled\n");
		return;
	}

	if (build_detour(load_ogg_addr) != 0) {
		call_original_load_ogg = NULL;
		return;
	}

	fprintf(stderr,
	        "sugar_patches: ready; _load_ogg=%p detour=%p __use_freq@%p=%u\n",
	        (void *)load_ogg_addr, (void *)call_original_load_ogg,
	        (void *)use_freq_ptr, *use_freq_ptr);
}
