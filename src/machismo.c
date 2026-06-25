/*
 * Machismo — Mach-O Loader for aarch64 Linux.
 *
 * Based on Darling's mldr, stripped of darlingserver dependencies,
 * adapted for aarch64, with simplified library handling.
 *
 * Copyright (C) 2017 Lubos Dolezel (original Darling code)
 * Modified for standalone aarch64 use.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include "macho_defs.h"
#include <dlfcn.h>
#include <endian.h>
#include <pthread.h>
#include "commpage.h"
#include "loader.h"
#include "resolver.h"
#include "trampoline.h"
#include "gdb_jit.h"
#include "patcher.h"
#include "config.h"
#include "bgfx_shim.h"
#include "sdl_window_shim.h"
#include "eh_frame.h"
#include "dylib_loader.h"
#include "isa_emul.h"
#include "syscall/syscall_gate.h"
#include "vm_interpose.h"
#include <sys/resource.h>
#include <pthread.h>

/* VM_PROT_* and SEG_DATA are defined in macho_defs.h */

#ifndef PAGE_SIZE
#	define PAGE_SIZE	4096
#endif
#define PAGE_ALIGN(x) ((x) & ~(PAGE_SIZE-1))
#define MAIN_LSE_POOL_MIN_SIZE (2 * 1024 * 1024)
#define MAIN_LSE_POOL_SLACK_SIZE (64 * 1024)
#define ARM64_DIRECT_BRANCH_RANGE (128 * 1024 * 1024)
#define ARM64_MRS_TPIDRRO_EL0 0xd53bd060u
#define ARM64_MRS_TPIDR_EL0   0xd53bd040u

static void load64(int fd, bool expect_dylinker, struct load_results* lr);
static void load_fat(int fd, cpu_type_t cpu, bool expect_dylinker, char** argv, struct load_results* lr);
static void load(const char* path, cpu_type_t cpu, bool expect_dylinker, char** argv, struct load_results* lr);
static void fixup_darwin_pthread_data(struct load_results* lr);
static void setup_tlv_image(struct load_results* lr);
static int native_prot(int prot);
static void setup_space(struct load_results* lr, bool is_64_bit);
static size_t align_page_size(size_t size);
static size_t estimate_main_lse_pool_size(struct load_results* lr);
static uint32_t* allocate_main_lse_pool(uintptr_t text_begin,
                                        uintptr_t text_end,
                                        size_t pool_size,
                                        struct load_results* lr);
static void start_thread(struct load_results* lr);
static void setup_stack64(const char* filepath, struct load_results* lr);
static void lc_main_return(int status) __attribute__((noreturn));
static void trace_lc_main_abi(struct load_results* lr);
void machgate_trace_guest_address(const char* label, uintptr_t address);

typedef void (*shim_note_init_context_fn)(const char*, int, int, uintptr_t);
typedef void (*shim_clear_init_context_fn)(void);

static void note_init_context(const char* kind, int index, int total,
                              uintptr_t address)
{
	trampoline_note_init_context(kind, index, total, address);

	shim_note_init_context_fn shim_note =
		(shim_note_init_context_fn)dlsym(RTLD_DEFAULT,
		                                 "machgate_shim_note_init_context");
	if (shim_note)
		shim_note(kind, index, total, address);
}

static void clear_init_context(void)
{
	trampoline_clear_init_context();

	shim_clear_init_context_fn shim_clear =
		(shim_clear_init_context_fn)dlsym(RTLD_DEFAULT,
		                                  "machgate_shim_clear_init_context");
	if (shim_clear)
		shim_clear();
}

/* UUID of the main executable */
uint8_t exe_uuid[16];

__attribute__((used))
struct load_results machismo_load_results = {0};

void* __machismo_main_stack_top = NULL;
size_t __machismo_main_stack_size = 0;
int __machismo_guest_argc = 0;
char** __machismo_guest_argv = NULL;
char** __machismo_guest_envp = NULL;
const char* __machismo_guest_executable_path = NULL;

void machgate_trace_guest_address(const char* label, uintptr_t address)
{
	if (!address) {
		fprintf(stderr, "machgate: guest context %s=(nil)\n",
		        label ? label : "address");
		return;
	}

	struct mach_header_64* header = (struct mach_header_64*)machismo_load_results.mh;
	if (!header)
		return;

	uint8_t* cmd_ptr = (uint8_t*)(header + 1);
	for (uint32_t i = 0; i < header->ncmds; i++) {
		struct load_command* lc = (struct load_command*)cmd_ptr;
		if (lc->cmd != LC_SEGMENT_64) {
			cmd_ptr += lc->cmdsize;
			continue;
		}

		struct segment_command_64* seg = (struct segment_command_64*)lc;
		uintptr_t seg_start = seg->vmaddr + machismo_load_results.slide;
		uintptr_t seg_end = seg_start + seg->vmsize;
		if (address < seg_start || address >= seg_end) {
			cmd_ptr += lc->cmdsize;
			continue;
		}

		const char* section_name = "<none>";
		uint64_t section_fileoff = seg->fileoff + (address - seg_start);
		struct section_64* sect = (struct section_64*)(seg + 1);
		for (uint32_t section_index = 0; section_index < seg->nsects; section_index++, sect++) {
			uintptr_t section_start = sect->addr + machismo_load_results.slide;
			uintptr_t section_end = section_start + sect->size;
			if (address >= section_start && address < section_end) {
				section_name = sect->sectname;
				section_fileoff = sect->offset + (address - section_start);
				break;
			}
		}

		uint32_t insn = 0;
		if ((address & 3u) == 0 && address + sizeof(insn) <= seg_end)
			insn = *(uint32_t*)address;

		fprintf(stderr,
		        "machgate: guest context %s=%p vmaddr=0x%lx segment=%.*s section=%.*s fileoff=0x%llx insn=0x%08x\n",
		        label ? label : "address", (void*)address,
		        (unsigned long)(address - machismo_load_results.slide),
		        16, seg->segname, 16, section_name,
		        (unsigned long long)section_fileoff, insn);
		return;
	}

	fprintf(stderr, "machgate: guest context %s=%p outside main Mach-O image\n",
	        label ? label : "address", (void*)address);
}

static size_t align_page_size(size_t size)
{
	size_t page_mask = (size_t)PAGE_SIZE - 1;
	return (size + page_mask) & ~page_mask;
}

static int rewrite_tpidrro_read(uint32_t *instruction)
{
	if ((*instruction & 0xffffffe0u) != ARM64_MRS_TPIDRRO_EL0)
		return 0;

	*instruction = ARM64_MRS_TPIDR_EL0 | (*instruction & 0x1fu);
	return 1;
}

static size_t estimate_main_lse_pool_size(struct load_results* lr)
{
	if (!lr->mh)
		return MAIN_LSE_POOL_MIN_SIZE;

	struct mach_header_64* mh = (struct mach_header_64*)lr->mh;
	uint8_t* cmds = (uint8_t*)(mh + 1);
	uint32_t p = 0;
	size_t estimated_size = 0;

	for (uint32_t i = 0; i < mh->ncmds && p < mh->sizeofcmds; i++) {
		struct load_command* lc = (struct load_command*)(cmds + p);
		if (lc->cmd == LC_SEGMENT_64) {
			struct segment_command_64* seg = (struct segment_command_64*)lc;
			if (seg->maxprot & VM_PROT_EXECUTE) {
				struct section_64* sect = (struct section_64*)(seg + 1);
				for (uint32_t s = 0; s < seg->nsects; s++, sect++) {
					if (!(sect->flags & S_ATTR_SOME_INSTRUCTIONS) &&
					    !(sect->flags & S_ATTR_PURE_INSTRUCTIONS))
						continue;
					uint32_t* code = (uint32_t*)(sect->addr + lr->slide);
					estimated_size += isa_emul_estimate_pool_size(code, sect->size);
				}
			}
		}
		p += lc->cmdsize;
	}

	if (estimated_size > 0)
		estimated_size += MAIN_LSE_POOL_SLACK_SIZE;
	if (estimated_size < MAIN_LSE_POOL_MIN_SIZE)
		estimated_size = MAIN_LSE_POOL_MIN_SIZE;

	return align_page_size(estimated_size);
}

static uint32_t* allocate_main_lse_pool(uintptr_t text_begin,
                                        uintptr_t text_end,
                                        size_t pool_size,
                                        struct load_results* lr)
{
	if (text_begin && text_end > text_begin) {
#ifdef MAP_FIXED_NOREPLACE
		uintptr_t aligned_size = align_page_size(pool_size);
		uintptr_t span = text_end - text_begin;

		if (span + aligned_size < ARM64_DIRECT_BRANCH_RANGE && text_begin > aligned_size) {
			uintptr_t page_mask = (uintptr_t)PAGE_SIZE - 1;
			uintptr_t highest = (text_begin - aligned_size) & ~page_mask;
			uintptr_t lowest = 0;
			if (text_end > ARM64_DIRECT_BRANCH_RANGE + aligned_size)
				lowest = (text_end - ARM64_DIRECT_BRANCH_RANGE - aligned_size) & ~page_mask;

			for (uintptr_t preferred = highest; preferred >= lowest; preferred -= PAGE_SIZE) {
				void* mapped = mmap((void*)preferred, aligned_size,
				                    PROT_READ | PROT_WRITE | PROT_EXEC,
				                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
				                    -1, 0);
				if (mapped != MAP_FAILED) {
					fprintf(stderr, "machgate: LSE island pool at %p (before __TEXT, %zu KB)\n",
					        mapped, aligned_size / 1024);
					return (uint32_t*)mapped;
				}
				if (preferred < PAGE_SIZE)
					break;
			}
		}
#endif
	}

	uint32_t* result = (uint32_t*)machismo_pool_alloc(lr, pool_size);
	if (result) {
		fprintf(stderr, "machgate: LSE island pool at %p (adjacent to segments, %zu KB)\n",
		        result, pool_size / 1024);
	}
	return result;
}

int machismo_verbose = 0;

int main(int argc, char** argv, char** envp)
{
	char *filename;

	signal(SIGPIPE, SIG_IGN);

	machgate_ensure_vm_interpose(argv, envp);

	machismo_load_results.kernfd = -1;
	machismo_load_results.argc = argc;
	machismo_load_results.argv = argv;

	while (envp[machismo_load_results.envc] != NULL) {
		++machismo_load_results.envc;
	}
	machismo_load_results.envp = envp;

	/* Parse machismo flags before the binary path */
	int arg_idx = 1;
	while (arg_idx < argc && argv[arg_idx][0] == '-') {
		if (strcmp(argv[arg_idx], "-v") == 0 || strcmp(argv[arg_idx], "--verbose") == 0) {
			machismo_verbose = 1;
		} else if (strcmp(argv[arg_idx], "--") == 0) {
			arg_idx++;
			break;
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[arg_idx]);
			fprintf(stderr, "Usage: %s [-v] <mach-o-binary> [args...]\n", argv[0]);
			return 1;
		}
		arg_idx++;
	}

	if (arg_idx >= argc)
	{
		fprintf(stderr, "Usage: %s [-v] <mach-o-binary> [args...]\n", argv[0]);
		return 1;
	}

	filename = argv[arg_idx];

	/* Load the Mach-O binary */
	load(filename, 0, false, argv, &machismo_load_results);

	/* Adjust argv: remove machismo args so the loaded binary sees its path as argv[0] */
	machismo_load_results.argc = argc - arg_idx;
	machismo_load_results.argv = argv + arg_idx;
	__machismo_guest_argc = (int)machismo_load_results.argc;
	__machismo_guest_argv = machismo_load_results.argv;
	__machismo_guest_envp = machismo_load_results.envp;
	__machismo_guest_executable_path = filename;

	/* Load config file — look next to the binary, or use MACHISMO_CONFIG */
	machismo_config_t cfg = {0};
	{
		const char* cfg_path = getenv("MACHISMO_CONFIG");
		int cfg_loaded = 0;

		if (cfg_path) {
			/* MACHISMO_CONFIG=none disables config loading (for tests) */
			if (strcmp(cfg_path, "none") != 0)
				cfg_loaded = (config_load(cfg_path, &cfg) == 0);
		} else {
			char conf_buf[4096];

			/* 1. Look for machismo.conf in the current working directory */
			cfg_loaded = (config_load("machismo.conf", &cfg) == 0);
			if (cfg_loaded) cfg_path = "machismo.conf";

			/* 2. Look for machismo.conf next to the game binary */
			if (!cfg_loaded) {
				const char* binary = machismo_load_results.argv[0];
				const char* slash = strrchr(binary, '/');
				if (slash) {
					size_t dirlen = slash - binary;
					snprintf(conf_buf, sizeof(conf_buf), "%.*s/machismo.conf",
					         (int)dirlen, binary);
				} else {
					snprintf(conf_buf, sizeof(conf_buf), "machismo.conf");
				}
				cfg_loaded = (config_load(conf_buf, &cfg) == 0);
				if (cfg_loaded)
					cfg_path = conf_buf;
			}
		}

		if (cfg_loaded) {
			fprintf(stderr, "machgate: loaded config from %s\n", cfg_path);
		} else {
			/* Fallback to env vars for backward compat */
			const char* map = getenv("MACHISMO_DYLIB_MAP");
			if (map) cfg.dylib_map = strdup(map);
			const char* pat = getenv("MACHISMO_PATCHES");
			if (pat) cfg.patches = strdup(pat);
			/* Legacy single trampoline from env */
			const char* tlib = getenv("MACHISMO_TRAMPOLINE_LIB");
			if (tlib) {
				cfg.trampolines[0].lib = strdup(tlib);
				const char* tpfx = getenv("MACHISMO_TRAMPOLINE_PREFIX");
				cfg.trampolines[0].prefixes[0] = strdup(tpfx ? tpfx : "_SDL_");
				cfg.trampolines[0].num_prefixes = 1;
				cfg.num_trampolines = 1;
			}
		}
	}

	/* LSE emulation state — shared between main exe and dylib patching */
	uint32_t *lse_pool_cur = NULL;
	uint32_t *lse_pool_end = NULL;
	int lse_total = 0;

	/* Patch ARMv8.1+ instructions for ARMv8.0 compatibility.
	 * Apple Silicon (A12+) uses LDAPR (ARMv8.3 RCPC) and LDADD/SWP/CAS
	 * (ARMv8.1 LSE atomics). Older cores (Cortex-A35/A53/A55) lack these.
	 *
	 * RCPC: replaced in-place with full-barrier equivalents (LDAR).
	 * LSE: replaced with B to islands containing LDXR/STXR loops.
	 *
	 * Must run before any Mach-O code executes. */
	if (machismo_load_results.mh) {
		struct mach_header_64* mh = (struct mach_header_64*)machismo_load_results.mh;
		uint8_t* cmds = (uint8_t*)(mh + 1);
		uint32_t p = 0;
		int rcpc_fixed = 0;
		int tpidr_fixed = 0;

		/* Allocate island pool for LSE emulation (near __TEXT for B range). */
		uintptr_t text_begin = 0;
		uintptr_t text_end = 0;
		uint32_t lp = 0;
		for (uint32_t li = 0; li < mh->ncmds && lp < mh->sizeofcmds; li++) {
			struct load_command* llc = (struct load_command*)(cmds + lp);
			if (llc->cmd == LC_SEGMENT_64) {
				struct segment_command_64* lseg = (struct segment_command_64*)llc;
				if (lseg->maxprot & VM_PROT_EXECUTE) {
					struct section_64* lsect = (struct section_64*)(lseg + 1);
					for (uint32_t s = 0; s < lseg->nsects; s++, lsect++) {
						if (!(lsect->flags & S_ATTR_SOME_INSTRUCTIONS) &&
						    !(lsect->flags & S_ATTR_PURE_INSTRUCTIONS))
							continue;
						uintptr_t sect_start = lsect->addr + machismo_load_results.slide;
						uintptr_t sect_end = sect_start + lsect->size;
						if (!text_begin || sect_start < text_begin)
							text_begin = sect_start;
						if (sect_end > text_end)
							text_end = sect_end;
					}
				}
			}
			lp += llc->cmdsize;
		}
		size_t lse_pool_size = estimate_main_lse_pool_size(&machismo_load_results);
		uint32_t* lse_pool = allocate_main_lse_pool(text_begin, text_end,
		                                            lse_pool_size,
		                                            &machismo_load_results);
		if (!lse_pool) {
			fprintf(stderr, "machgate: WARNING: LSE pool alloc failed — LSE atomics will SIGILL\n");
		}

		lse_pool_cur = lse_pool;
		lse_pool_end = lse_pool ? lse_pool + lse_pool_size / 4 : NULL;
		for (uint32_t i = 0; i < mh->ncmds && p < mh->sizeofcmds; i++) {
			struct load_command* lc = (struct load_command*)(cmds + p);
			if (lc->cmd == LC_SEGMENT_64) {
				struct segment_command_64* seg = (struct segment_command_64*)lc;
				if (seg->maxprot & VM_PROT_EXECUTE) {
					uint32_t* code = (uint32_t*)(seg->vmaddr + machismo_load_results.slide);
					/* Make writable for patching */
					mprotect(code, seg->vmsize, PROT_READ | PROT_WRITE | PROT_EXEC);

					/* Only patch actual code sections (__text), NOT data
					 * sections like __cstring, __const, __gcc_except_tab, etc.
					 * Data sections can contain byte patterns that look like
					 * LSE/RCPC instructions but are really string constants. */
					struct section_64* sect = (struct section_64*)(seg + 1);
					for (uint32_t s = 0; s < seg->nsects; s++, sect++) {
						if (!(sect->flags & S_ATTR_SOME_INSTRUCTIONS) &&
						    !(sect->flags & S_ATTR_PURE_INSTRUCTIONS))
							continue;
						uint32_t* scode = (uint32_t*)(sect->addr + machismo_load_results.slide);
						size_t scount = sect->size / 4;

						/* RCPC: in-place downgrade */
						for (size_t j = 0; j < scount; j++) {
							if ((scode[j] & 0x3FFFFC00) == 0x38BFC000) {
								scode[j] = (scode[j] & 0xC00003FF) | 0x08DFFC00;
								rcpc_fixed++;
							}
							if (rewrite_tpidrro_read(&scode[j])) {
								tpidr_fixed++;
							}
						}

						/* LSE: replace with B to island */
						if (lse_pool_cur) {
							int n = isa_emul_patch(scode, sect->size,
							                       &lse_pool_cur, lse_pool_end);
							lse_total += n;
						}
					}

					mprotect(code, seg->vmsize, native_prot(seg->initprot));
					__builtin___clear_cache((char*)code, (char*)code + seg->vmsize);
				}
			}
			p += lc->cmdsize;
		}
		if (lse_pool)
			__builtin___clear_cache((char*)lse_pool, (char*)lse_pool + lse_pool_size);
		if (rcpc_fixed > 0)
			fprintf(stderr, "machgate: downgraded %d ARMv8.3 RCPC instructions to ARMv8.0\n", rcpc_fixed);
		if (tpidr_fixed > 0)
			fprintf(stderr, "machgate: rewrote %d Darwin TPIDRRO reads to Linux TPIDR reads\n", tpidr_fixed);
		if (lse_total > 0)
			fprintf(stderr, "machgate: patched %d ARMv8.1 LSE atomics with LDXR/STXR islands\n", lse_total);
	}

	/* Resolve chained fixups — patch GOT with native Linux .so addresses.
	 * The resolver handles MACHO: entries in dylib_map.conf by loading
	 * Mach-O dylibs and looking up symbols in their LC_SYMTAB.
	 * Order: resolve main exe first (which triggers dylib loading),
	 * then resolve each loaded dylib's own fixups. */
	if (machismo_load_results.mh && cfg.dylib_map) {
		resolver_resolve_fixups((void*)machismo_load_results.mh,
		                        machismo_load_results.slide, cfg.dylib_map);

		/* Resolve chained fixups for loaded Mach-O dylibs.
		 * Each dylib has its own LC_DYLD_CHAINED_FIXUPS that need patching.
		 * Dependencies between dylibs are handled because they're all
		 * already loaded — the resolver finds them via MACHO: entries. */
		for (int i = 0; i < g_num_macho_dylibs; i++) {
			struct macho_dylib_info *mdi = &g_macho_dylibs[i];
			fprintf(stderr, "machgate: resolving fixups for Mach-O dylib '%s'\n", mdi->path);
			resolver_resolve_fixups((void*)mdi->mh, mdi->slide, cfg.dylib_map);
		}

		/* Fix macOS pthread signatures in dylib __DATA segments.
		 * Same scan as for the main exe — find 0x32AAABA7 patterns
		 * and reinitialize as PTHREAD_MUTEX_RECURSIVE. */
		for (int i = 0; i < g_num_macho_dylibs; i++) {
			struct macho_dylib_info *mdi = &g_macho_dylibs[i];
			struct mach_header_64 *mh = mdi->mh;
			uint8_t *lcmds = (uint8_t*)(mh + 1);
			uint32_t lp = 0;
			for (uint32_t li = 0; li < mh->ncmds && lp < mh->sizeofcmds; li++) {
				struct segment_command_64 *seg = (struct segment_command_64*)&lcmds[lp];
				if (seg->cmd == LC_SEGMENT_64 &&
				    (seg->initprot & 2) /* writable */) {
					uintptr_t seg_start = seg->vmaddr + mdi->slide;
					uintptr_t seg_end = seg_start + seg->vmsize;
					uint32_t *scan = (uint32_t*)seg_start;
					int fixed = 0;
					while ((uintptr_t)scan + 4 <= seg_end) {
						if (*scan == 0x32AAABA7) {
							pthread_mutexattr_t attr;
							pthread_mutexattr_init(&attr);
							pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
							pthread_mutex_init((pthread_mutex_t*)scan, &attr);
							pthread_mutexattr_destroy(&attr);
							fixed++;
							scan = (uint32_t*)((uint8_t*)scan + sizeof(pthread_mutex_t));
						} else {
							scan++;
						}
					}
					if (fixed > 0)
						fprintf(stderr, "machgate: fixed %d macOS pthread sigs in dylib '%s' segment %s\n",
						        fixed, mdi->path, seg->segname);
				}
				lp += seg->cmdsize;
			}
		}

		/* Downgrade RCPC and LSE instructions in loaded Mach-O dylibs.
		 * Each dylib needs its own island pool near its __TEXT since
		 * the main pool is too far away for a ±128MB B instruction. */
		for (int i = 0; i < g_num_macho_dylibs; i++) {
			struct macho_dylib_info *mdi = &g_macho_dylibs[i];
			struct mach_header_64 *dmh = mdi->mh;
			uint8_t *dcmds = (uint8_t*)(dmh + 1);
			uint32_t dp = 0;
			int dfix = 0;
			int dtpidr = 0;

			/* Use the dylib's preallocated adjacent pool for LSE islands */
			size_t dylib_pool_size = mdi->pool_size;
			uint32_t *dylib_lse_pool = (uint32_t *)mdi->pool_base;
			uint32_t *dylib_lse_cur = dylib_lse_pool;
			uint32_t *dylib_lse_end = dylib_lse_pool
				? dylib_lse_pool + dylib_pool_size / 4 : NULL;

			dp = 0;
			int dln = 0;
			for (uint32_t di = 0; di < dmh->ncmds && dp < dmh->sizeofcmds; di++) {
				struct load_command *dlc = (struct load_command*)(dcmds + dp);
				if (dlc->cmd == LC_SEGMENT_64) {
					struct segment_command_64 *dseg = (struct segment_command_64*)dlc;
					if (dseg->maxprot & VM_PROT_EXECUTE) {
						uint32_t *code = (uint32_t*)(dseg->vmaddr + mdi->slide);
						size_t cnt = dseg->vmsize / 4;
						mprotect(code, dseg->vmsize, PROT_READ | PROT_WRITE | PROT_EXEC);
						for (size_t dj = 0; dj < cnt; dj++) {
							if ((code[dj] & 0x3FFFFC00) == 0x38BFC000) {
								code[dj] = (code[dj] & 0xC00003FF) | 0x08DFFC00;
								dfix++;
							}
							if (rewrite_tpidrro_read(&code[dj])) {
								dtpidr++;
							}
						}
						if (dylib_lse_cur) {
							int ln = isa_emul_patch(code, dseg->vmsize,
							                        &dylib_lse_cur, dylib_lse_end);
							dln += ln;
							lse_total += ln;
						}
						mprotect(code, dseg->vmsize, native_prot(dseg->initprot));
						__builtin___clear_cache((char*)code, (char*)code + dseg->vmsize);
					}
				}
				dp += dlc->cmdsize;
			}
			if (dylib_lse_pool)
				__builtin___clear_cache((char*)dylib_lse_pool,
					(char*)dylib_lse_pool + dylib_pool_size);
			if (dfix > 0)
				fprintf(stderr, "machgate: downgraded %d RCPC instructions in dylib '%s'\n",
				        dfix, mdi->path);
			if (dtpidr > 0)
				fprintf(stderr, "machgate: rewrote %d Darwin TPIDRRO reads in dylib '%s'\n",
				        dtpidr, mdi->path);
			if (dln > 0)
				fprintf(stderr, "machgate: patched %d LSE atomics in dylib '%s' (pool at %p)\n",
				        dln, mdi->path, dylib_lse_pool);
		}

		/* Run static initializers for loaded Mach-O dylibs.
		 * Must be after fixup resolution (function pointers in
		 * __mod_init_func need to be rebased). */
		for (int i = 0; i < g_num_macho_dylibs; i++) {
			dylib_loader_run_inits(&g_macho_dylibs[i]);
		}
	}

	/* Fix macOS pthread objects and set up TLV before the __DATA guard
	 * locks down pages (the pthread scan reads all writable segments). */
	if (machismo_load_results.mh) {
		fixup_darwin_pthread_data(&machismo_load_results);
		setup_tlv_image(&machismo_load_results);
	}

	/* Register Mach-O exception handling frames with system unwinder.
	 * Must be after resolver (GOT entries for personality functions must be patched)
	 * but before any Mach-O code runs (__init_offsets, trampolines, _main). */
	if (machismo_load_results.mh) {
		eh_frame_register_macho((void*)machismo_load_results.mh,
		                        machismo_load_results.slide);
		/* NOTE: Do NOT register eh_frame for loaded Mach-O dylibs yet.
		 * The _dl_find_object interposition uses a single text range —
		 * registering dylibs would overwrite the main exe's range and
		 * break C++ exception handling for the main binary. */
	}

	/* Apply trampolines from config */
	if (machismo_load_results.mh) {
		/* Give the trampoline system its share of the adjacent pool (1MB) */
		size_t tramp_pool_size = 1024 * 1024;
		void* tramp_pool = machismo_pool_alloc(
			&machismo_load_results, tramp_pool_size);
		if (tramp_pool) {
			trampoline_set_pool(tramp_pool, tramp_pool_size);
		} else {
			fprintf(stderr, "machgate: WARNING: no pool space for trampoline islands\n");
		}

		for (int i = 0; i < cfg.num_trampolines; i++) {
			machismo_trampoline_config_t* tc = &cfg.trampolines[i];

			/* Override mode: exact-symbol matching from an override .so */
			if (tc->override_lib) {
				void* oh = dlopen(tc->override_lib, RTLD_LAZY);
				if (oh) {
					trampoline_patch_overrides(
						(void*)machismo_load_results.mh,
						machismo_load_results.slide, oh,
						tc->match_local);
				} else {
					fprintf(stderr, "machgate: warning: cannot load override lib %s: %s\n",
					        tc->override_lib, dlerror());
				}
				if (!tc->lib) continue;  /* override-only section, no prefix patching */
			}

			if (!tc->lib || tc->num_prefixes == 0) continue;

			trampoline_override_t* overrides = NULL;
			int num_overrides = 0;

			/* SDL-specific: capture window + resize bgfx on fullscreen */
			for (int j = 0; j < tc->num_prefixes; j++) {
				if (strcmp(tc->prefixes[j], "_SDL_") == 0) {
					static trampoline_override_t sdl_ov[] = {
						{ "_SDL_CreateWindow", NULL },
						{ "_SDL_SetWindowFullscreen", NULL },
					};
					sdl_ov[0].target = (void*)sdl_create_window_wrapper;
					sdl_ov[1].target = (void*)sdl_set_window_fullscreen_wrapper;
					overrides = sdl_ov;
					num_overrides = 2;
					break;
				}
			}

			/* bgfx-specific: set up init, frame, and diagnostic wrappers */
			if (tc->init_wrapper) {
				void* h = dlopen(tc->lib, RTLD_LAZY | RTLD_GLOBAL);
				if (h) {
					bgfx_shim_set_real_init(dlsym(h, "bgfx_init"));
					bgfx_shim_set_real_frame(dlsym(h, "bgfx_frame"));
					bgfx_shim_set_real_reset(dlsym(h, "bgfx_reset"));
					bgfx_shim_set_real_set_view_rect(dlsym(h, "bgfx_set_view_rect"));
					bgfx_shim_set_real_set_view_transform(dlsym(h, "bgfx_set_view_transform"));
					bgfx_shim_set_real_get_caps(dlsym(h, "bgfx_get_caps"));
					bgfx_shim_set_renderer(tc->renderer);
					static trampoline_override_t bgfx_ov[] = {
						{ "_bgfx_init", NULL },
						{ "__ZN4bgfx4initERKNS_4InitE", NULL },
						{ "_bgfx_frame", NULL },
						{ "__ZN4bgfx5frameEb", NULL },
						{ "_bgfx_reset", NULL },
						{ "__ZN4bgfx5resetEjjjNS_13TextureFormat4EnumE", NULL },
						{ "_bgfx_set_view_rect", NULL },
						{ "__ZN4bgfx11setViewRectEttttt", NULL },
						{ "_bgfx_set_view_transform", NULL },
						{ "__ZN4bgfx16setViewTransformEtPKvS1_", NULL },
						{ "_bgfx_get_caps", NULL },
						{ "__ZN4bgfx7getCapsEv", NULL },
						{ "__ZN4bgfx7Encoder12setTransformEPKvt", NULL },
						{ "__ZN4bgfx7Encoder6submitEtNS_13ProgramHandleEjh", NULL },
					};
					bgfx_ov[0].target = (void*)bgfx_init_wrapper;
					bgfx_ov[1].target = (void*)bgfx_init_wrapper;
					bgfx_ov[2].target = (void*)bgfx_frame_wrapper;
					bgfx_ov[3].target = (void*)bgfx_frame_wrapper;
					bgfx_ov[4].target = (void*)bgfx_reset_wrapper;
					bgfx_ov[5].target = (void*)bgfx_reset_wrapper;
					bgfx_ov[6].target = (void*)bgfx_set_view_rect_wrapper;
					bgfx_ov[7].target = (void*)bgfx_set_view_rect_wrapper;
					bgfx_ov[8].target = (void*)bgfx_set_view_transform_wrapper;
					bgfx_ov[9].target = (void*)bgfx_set_view_transform_wrapper;
					bgfx_ov[10].target = (void*)bgfx_get_caps_wrapper;
					bgfx_ov[11].target = (void*)bgfx_get_caps_wrapper;
					bgfx_shim_set_real_encoder_set_transform(
						dlsym(h, "_ZN4bgfx7Encoder12setTransformEPKvt"));
					bgfx_ov[12].target = (void*)bgfx_encoder_set_transform_wrapper;
					bgfx_shim_set_real_encoder_submit(
						dlsym(h, "_ZN4bgfx7Encoder6submitEtNS_13ProgramHandleEjh"));
					bgfx_ov[13].target = (void*)bgfx_encoder_submit_wrapper;
					overrides = bgfx_ov;
					num_overrides = 14;
				} else {
					fprintf(stderr, "machgate: warning: cannot load %s: %s\n",
					        tc->lib, dlerror());
				}
			}

			trampoline_patch_lib((void*)machismo_load_results.mh,
			                     machismo_load_results.slide,
			                     tc->lib,
			                     (const char**)tc->prefixes,
			                     tc->num_prefixes,
			                     overrides, num_overrides);
		}

		/* Guard __DATA pages against stale library state access.
		 * Collect all non-STUB prefixes for the data guard. */
		const char* guard_prefixes[64];
		int num_guard_prefixes = 0;
		for (int i = 0; i < cfg.num_trampolines; i++) {
			machismo_trampoline_config_t* tc2 = &cfg.trampolines[i];
			if (!tc2->lib || strcmp(tc2->lib, "STUB") == 0) continue;
			for (int j = 0; j < tc2->num_prefixes; j++) {
				if (num_guard_prefixes < 64)
					guard_prefixes[num_guard_prefixes++] = tc2->prefixes[j];
			}
		}
		if (num_guard_prefixes > 0) {
			trampoline_guard_stale_data((void*)machismo_load_results.mh,
			                            machismo_load_results.slide,
			                            guard_prefixes, num_guard_prefixes);
		}
	}

	/* Register Mach-O symbols with GDB for backtraces */
	if (machismo_load_results.mh) {
		gdb_jit_register_macho((void*)machismo_load_results.mh,
		                       machismo_load_results.slide);
	}
	for (int i = 0; i < g_num_macho_dylibs; i++) {
		gdb_jit_register_macho((void*)g_macho_dylibs[i].mh,
		                       g_macho_dylibs[i].slide);
	}

	/* Apply game-specific binary patches. Any failure (missing symbol,
	 * unique pattern drift, expect mismatch) is fatal — better to crash
	 * at startup than to run the game with random memory corruption. */
	if (cfg.patches) {
		if (patcher_apply(cfg.patches,
		                  (void*)machismo_load_results.mh,
		                  machismo_load_results.slide) < 0) {
			fprintf(stderr, "machgate: patcher failed — aborting\n");
			abort();
		}
	}

	if (machismo_load_results.mh) {
		if (syscall_gate_patch(&machismo_load_results) < 0) {
			fprintf(stderr, "machgate: syscall gate patching failed — aborting\n");
			abort();
		}
	}

	if (machismo_load_results.mh && getenv("MACHGATE_TRACE_SIGNALS")) {
		trampoline_install_signal_diagnostics((void*)machismo_load_results.mh,
		                                      machismo_load_results.slide);
	}

	/* Set up the Mach-O stack layout */
	setup_stack64(filename, &machismo_load_results);

	start_thread(&machismo_load_results);

	__builtin_unreachable();
}

void load(const char* path, cpu_type_t forced_arch, bool expect_dylinker, char** argv, struct load_results* lr)
{
	int fd;
	uint32_t magic;

	fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno));
		exit(1);
	}

	if (read(fd, &magic, sizeof(magic)) != sizeof(magic))
	{
		fprintf(stderr, "Cannot read the file header of %s.\n", path);
		exit(1);
	}

	if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64)
	{
		lseek(fd, 0, SEEK_SET);
		load64(fd, expect_dylinker, lr);
	}
	else if (magic == MH_MAGIC || magic == MH_CIGAM)
	{
		fprintf(stderr, "32-bit Mach-O not supported on aarch64: %s\n", path);
		exit(1);
	}
	else if (magic == FAT_MAGIC || magic == FAT_CIGAM)
	{
		lseek(fd, 0, SEEK_SET);
		load_fat(fd, forced_arch, expect_dylinker, argv, lr);
	}
	else
	{
		fprintf(stderr, "Unknown file format: %s (magic: 0x%08x)\n", path, magic);
		exit(1);
	}

	close(fd);
}

static void load_fat(int fd, cpu_type_t forced_arch, bool expect_dylinker, char** argv, struct load_results* lr) {
	struct fat_header fhdr;
	struct fat_arch best_arch = {0};

	best_arch.cputype = CPU_TYPE_ANY;

	if (read(fd, &fhdr, sizeof(fhdr)) != sizeof(fhdr))
	{
		fprintf(stderr, "Cannot read fat file header.\n");
		exit(1);
	}

	const bool swap = fhdr.magic == FAT_CIGAM;

#define SWAP32(x) x = __bswap_32(x)

	if (swap)
		SWAP32(fhdr.nfat_arch);

	uint32_t i;
	for (i = 0; i < fhdr.nfat_arch; i++)
	{
		struct fat_arch arch;

		if (read(fd, &arch, sizeof(arch)) != sizeof(arch))
		{
			fprintf(stderr, "Cannot read fat_arch header.\n");
			exit(1);
		}

		if (swap)
		{
			SWAP32(arch.cputype);
			SWAP32(arch.cpusubtype);
			SWAP32(arch.offset);
			SWAP32(arch.size);
			SWAP32(arch.align);
		}

		if (!forced_arch)
		{
			/* On aarch64, prefer ARM64 slice */
			if (arch.cputype == CPU_TYPE_ARM64)
				best_arch = arch;
		}
		else
		{
			if (arch.cputype == forced_arch)
				best_arch = arch;
		}
	}

	if (best_arch.cputype == CPU_TYPE_ANY)
	{
		fprintf(stderr, "No arm64 architecture found in fat binary.\n");
		exit(1);
	}

	if (lseek(fd, best_arch.offset, SEEK_SET) == -1)
	{
		fprintf(stderr, "Cannot seek to selected arch in fat binary.\n");
		exit(1);
	}

	if (best_arch.cputype & CPU_ARCH_ABI64) {
		load64(fd, expect_dylinker, lr);
	} else {
		fprintf(stderr, "32-bit slices not supported on aarch64.\n");
		exit(1);
	}
}

/* Include the 64-bit loader template */
#define GEN_64BIT
#include "loader.c"
#include "stack.c"
#undef GEN_64BIT

int native_prot(int prot)
{
	int protOut = 0;

	if (prot & VM_PROT_READ)
		protOut |= PROT_READ;
	if (prot & VM_PROT_WRITE)
		protOut |= PROT_WRITE;
	if (prot & VM_PROT_EXECUTE)
		protOut |= PROT_EXEC;

	return protOut;
}

static void setup_space(struct load_results* lr, bool is_64_bit) {
	commpage_setup(is_64_bit);

	lr->stack_top = commpage_address(true);

	struct rlimit limit;
	getrlimit(RLIMIT_STACK, &limit);
	unsigned long size = PAGE_SIZE * 4096;
	if (limit.rlim_cur != RLIM_INFINITY && limit.rlim_cur < size) {
		size = limit.rlim_cur;
	}

	if (mmap((void*)(lr->stack_top - size), size, PROT_READ | PROT_WRITE,
	         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_GROWSDOWN, -1, 0) == MAP_FAILED) {
		fprintf(stderr, "Failed to allocate stack of %lu bytes: %d (%s)\n", size, errno, strerror(errno));
		exit(1);
	}

	__machismo_main_stack_top = (void*)lr->stack_top;
	__machismo_main_stack_size = size;

	lr->kernfd = -1;
}

/*
 * Fix macOS pthread objects in the Mach-O binary.
 *
 * macOS statically initializes pthread_mutex_t with signature 0x32AAABA7
 * at offset 0. Linux PTHREAD_MUTEX_INITIALIZER is all zeros. Since native
 * Linux libraries (libc++, etc.) call glibc's pthread_mutex_lock directly
 * through their own ELF PLT/GOT, we can't intercept at the function level.
 * We must fix the data in-place before execution.
 *
 * Strategy: use LC_SYMTAB to find symbols in __DATA whose names contain
 * "mutex", "Mutex", "cond", "rwlock", or "once". Check for macOS pthread
 * signatures at those addresses and zero-initialize them for Linux.
 */

#define DARWIN_MUTEX_SIG      0x32AAABA7L  /* normal     PTHREAD_MUTEX_INITIALIZER */
#define DARWIN_RMUTEX_SIG     0x32AAABA1L  /* errorcheck PTHREAD_ERRORCHECK_MUTEX_INITIALIZER */
#define DARWIN_RECURSIVE_MUTEX_SIG 0x32AAABA2L /* recursive PTHREAD_RECURSIVE_MUTEX_INITIALIZER */
#define DARWIN_FIRSTFIT_MUTEX_SIG  0x32AAABA3L /* firstfit */
#define DARWIN_COND_SIG       0x3CB0B1BBL
#define DARWIN_RWLOCK_SIG     0x2DA8B3B4L
#define DARWIN_ONCE_SIG       0x30B1BCBAL

/* Try to fix a macOS pthread signature at the given address.
 * Returns 1 if fixed, 0 if no macOS signature found. */
static int try_fixup_pthread_at(uintptr_t addr) {
	long *sig = (long *)addr;
	switch (*sig) {
	case DARWIN_MUTEX_SIG:
	case DARWIN_RMUTEX_SIG:
	case DARWIN_RECURSIVE_MUTEX_SIG:
	case DARWIN_FIRSTFIT_MUTEX_SIG: {
		/* Initialize as recursive mutex for macOS compatibility.
		 * macOS game code may re-lock from the same thread. */
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		memset(sig, 0, 64);
		pthread_mutex_init((pthread_mutex_t*)sig, &attr);
		pthread_mutexattr_destroy(&attr);
		return 1;
	}
	case DARWIN_COND_SIG:
		memset(sig, 0, 48);
		pthread_cond_init((pthread_cond_t*)sig, NULL);
		return 1;
	case DARWIN_RWLOCK_SIG:
		memset(sig, 0, 64);
		pthread_rwlock_init((pthread_rwlock_t*)sig, NULL);
		return 1;
	case DARWIN_ONCE_SIG:
		memset(sig, 0, 16);
		return 1;
	}
	return 0;
}


static void fixup_darwin_pthread_data(struct load_results* lr) {
	struct mach_header_64* mh = (struct mach_header_64*)lr->mh;
	uint8_t* cmds = (uint8_t*)(mh + 1);
	uint32_t p = 0;
	int fixed = 0;

	/* Scan all writable DATA segments for macOS pthread signatures.
	 * The signatures (0x32AAABA7 etc.) are stored as 8-byte-aligned longs
	 * and are distinctive enough that false positives are negligible. */
	for (uint32_t i = 0; i < mh->ncmds && p < mh->sizeofcmds; i++) {
		struct load_command* lc = (struct load_command*)&cmds[p];
		if (lc->cmd == LC_SEGMENT_64) {
			struct segment_command_64* seg = (struct segment_command_64*)lc;
			if (seg->initprot & 0x02) { /* VM_PROT_WRITE */
				uintptr_t base = seg->vmaddr + lr->slide;
				/* Only scan the file-backed portion (not BSS) */
				size_t scan_size = seg->filesize;
				for (size_t off = 0; off + sizeof(long) <= scan_size; off += sizeof(long)) {
					if (try_fixup_pthread_at(base + off))
						fixed++;
				}
			}
		}
		p += lc->cmdsize;
	}

	if (fixed > 0)
		fprintf(stderr, "machgate: fixed %d macOS pthread objects in __DATA\n", fixed);
}

/*
 * Set up Mach-O thread-local variable (TLV) image info.
 * The __DATA,__thread_data section contains initial values for TLVs,
 * and __DATA,__thread_bss is the zero-initialized portion.
 * These are needed by _tlv_bootstrap() in libsystem_shim.so.
 */
static void setup_tlv_image(struct load_results* lr)
{
	struct mach_header_64* mh = (struct mach_header_64*)lr->mh;
	uint8_t* cmds = (uint8_t*)(mh + 1);
	uint32_t p = 0;

	void* tlv_image_base = NULL;
	size_t tlv_image_size = 0;
	size_t tlv_bss_size = 0;

	for (uint32_t i = 0; i < mh->ncmds && p < mh->sizeofcmds; i++) {
		struct load_command* lc = (struct load_command*)&cmds[p];
		if (lc->cmd == LC_SEGMENT_64) {
			struct segment_command_64* seg = (struct segment_command_64*)lc;
			struct section_64* sect = (struct section_64*)(seg + 1);
			for (uint32_t s = 0; s < seg->nsects; s++, sect++) {
				/* S_THREAD_LOCAL_REGULAR = 0x11 */
				if ((sect->flags & 0xff) == 0x11) {
					tlv_image_base = (void*)(sect->addr + lr->slide);
					tlv_image_size = sect->size;
				}
				/* S_THREAD_LOCAL_ZEROFILL = 0x12 */
				if ((sect->flags & 0xff) == 0x12) {
					tlv_bss_size = sect->size;
				}
			}
		}
		p += lc->cmdsize;
	}

	if (tlv_image_size || tlv_bss_size) {
		fprintf(stderr, "machgate: TLV image at %p, size %zu + %zu bss\n",
		        tlv_image_base, tlv_image_size, tlv_bss_size);
	}

	/* Set globals in libsystem_shim.so — try multiple paths since
	 * the resolver may have loaded it with a relative or absolute path */
	const char* shim_paths[] = {
		"./libsystem_shim.so", "libsystem_shim.so",
		"libs/libsystem_shim.so", "./libs/libsystem_shim.so",
		NULL
	};
	void* shim = NULL;
	for (int i = 0; shim_paths[i]; i++) {
		shim = dlopen(shim_paths[i], RTLD_NOLOAD | RTLD_LAZY);
		if (shim) break;
	}
	if (shim) {
		void** p_base = (void**)dlsym(shim, "__tlv_image_base");
		size_t* p_size = (size_t*)dlsym(shim, "__tlv_image_size");
		size_t* p_bss = (size_t*)dlsym(shim, "__tlv_bss_size");
		if (p_base) *p_base = tlv_image_base;
		if (p_size) *p_size = tlv_image_size;
		if (p_bss) *p_bss = tlv_bss_size;
		dlclose(shim);
		fprintf(stderr, "machgate: TLV info set in shim\n");
	} else {
		fprintf(stderr, "machgate: WARNING: cannot find libsystem_shim.so for TLV (%s)\n",
		        dlerror());
	}
}

/*
 * Run C++ static initializers from __init_offsets section.
 * This section contains 32-bit relative offsets from the Mach-O base
 * to void(void) initializer functions. On macOS, dyld runs these before main().
 */
static void run_init_offsets(struct load_results* lr) {
	struct mach_header_64* mh = (struct mach_header_64*)lr->mh;
	uint8_t* cmds = (uint8_t*)(mh + 1);
	uint32_t p = 0;

	for (uint32_t i = 0; i < mh->ncmds && p < mh->sizeofcmds; i++) {
		struct load_command* lc = (struct load_command*)&cmds[p];
		if (lc->cmd == LC_SEGMENT_64) {
			struct segment_command_64* seg = (struct segment_command_64*)lc;
			struct section_64* sect = (struct section_64*)(seg + 1);
			for (uint32_t s = 0; s < seg->nsects; s++, sect++) {
				/* S_INIT_FUNC_OFFSETS = 0x16 */
				if ((sect->flags & 0xff) == 0x16) {
					uint32_t* offsets = (uint32_t*)(lr->mh + sect->addr - seg->vmaddr + sect->offset
							+ (lr->slide - 0) /* offset within mapped segment */);
					/* offsets are at the mapped section address */
					offsets = (uint32_t*)(sect->addr + lr->slide);
					int count = sect->size / sizeof(uint32_t);
					fprintf(stderr, "machgate: running %d C++ static initializers from __init_offsets\n", count);
					typedef void (*init_func_t)(void);
					extern int machismo_verbose;
					for (int j = 0; j < count; j++) {
						uintptr_t func_addr = lr->mh + offsets[j];
						init_func_t func = (init_func_t)func_addr;
						if (machismo_verbose)
							fprintf(stderr, "machgate: init[%d/%d] at 0x%lx\n", j, count, func_addr);
						note_init_context("__init_offsets", j, count, func_addr);
						func();
						clear_init_context();
					}
					fprintf(stderr, "machgate: static initializers complete\n");
				}
				/* S_MOD_INIT_FUNC_POINTERS = 0x09 */
				if ((sect->flags & 0xff) == 0x09) {
					uintptr_t* funcs = (uintptr_t*)(sect->addr + lr->slide);
					int count = sect->size / sizeof(uintptr_t);
					fprintf(stderr, "machgate: running %d static constructors from __mod_init_func\n", count);
					typedef void (*init_func_t)(void);
					extern int machismo_verbose;
					for (int j = 0; j < count; j++) {
						if (machismo_verbose)
							fprintf(stderr, "machgate: mod_init[%d/%d] at 0x%lx\n", j, count, funcs[j]);
						note_init_context("__mod_init_func", j, count, funcs[j]);
						((init_func_t)funcs[j])();
						clear_init_context();
					}
					fprintf(stderr, "machgate: __mod_init_func constructors complete\n");
				}
			}
		}
		p += lc->cmdsize;
	}
}

static void lc_main_return(int status)
{
	_exit(status);
}

static FILE* open_trace_file(void)
{
	const char* trace_file = getenv("MACHGATE_EXECVE_TRACE_FILE");

	if (!trace_file || !*trace_file)
		return NULL;
	return fopen(trace_file, "a");
}

static void trace_lc_main_abi(struct load_results* lr)
{
	if (!getenv("MACHGATE_TRACE_LCMAIN"))
		return;

	FILE* trace_file = open_trace_file();
	const char* guest_cookie = NULL;
	const char* host_cookie = getenv("PACKER_WRAP_COOKIE");
	char** env_from_argv = lr->argv + lr->argc + 1;
	char** apple_from_argv = env_from_argv;
	for (size_t env_index = 0; lr->envp && lr->envp[env_index]; env_index++) {
		if (strncmp(lr->envp[env_index], "PACKER_WRAP_COOKIE=", 19) == 0) {
			guest_cookie = lr->envp[env_index] + 19;
			break;
		}
	}
	while (*apple_from_argv)
		apple_from_argv++;
	apple_from_argv++;

	fprintf(stderr, "machgate: lcmain abi stack=%p argc=%zu argv=%p envp=%p applep=%p\n",
	        (void*)lr->stack_top, lr->argc, (void*)lr->argv, (void*)lr->envp,
	        (void*)lr->applep);
	fprintf(stderr, "machgate: lcmain abi derived envp=%p applep=%p\n",
	        (void*)env_from_argv, (void*)apple_from_argv);
	fprintf(stderr, "machgate: lcmain abi argv0=%p '%s' argv1=%p '%s'\n",
	        lr->argv[0], lr->argv[0] ? lr->argv[0] : "",
	        lr->argc > 1 ? lr->argv[1] : NULL,
	        lr->argc > 1 && lr->argv[1] ? lr->argv[1] : "");
	fprintf(stderr, "machgate: lcmain abi env0=%p '%s' apple0=%p '%s'\n",
	        lr->envp[0], lr->envp[0] ? lr->envp[0] : "",
	        lr->applep[0], lr->applep[0] ? lr->applep[0] : "");
	fprintf(stderr, "machgate: lcmain abi PACKER_WRAP_COOKIE guest=%d host=%d\n",
	        guest_cookie ? 1 : 0, host_cookie ? 1 : 0);
	if (trace_file) {
		fprintf(trace_file,
		        "machgate: lcmain pid=%d argc=%zu argv0='%s' argv1='%s' guest-cookie=%d host-cookie=%d\n",
		        (int)getpid(), lr->argc,
		        lr->argv[0] ? lr->argv[0] : "",
		        lr->argc > 1 && lr->argv[1] ? lr->argv[1] : "",
		        guest_cookie ? 1 : 0, host_cookie ? 1 : 0);
		fclose(trace_file);
	}
}

static void start_thread(struct load_results* lr) {
	if (lr->lc_main) {
		/* pthread fixup and TLV setup are done earlier in main(),
		 * before the __DATA guard locks down pages. */

		/* Run C++ static initializers before main */
		run_init_offsets(lr);

		/* chdir to the binary's directory so the game finds its data files */
		{
			char* binary_path = strdup(lr->argv[0] ? lr->argv[0] : "");
			char* slash = strrchr(binary_path, '/');
			if (slash) {
				*slash = '\0';
				if (chdir(binary_path) == 0)
					fprintf(stderr, "machgate: chdir to '%s'\n", binary_path);
			}
			free(binary_path);
		}

		fprintf(stderr, "machgate: calling _main at %p (argc=%zu)\n",
				(void*)lr->entry_point, lr->argc);
		trace_lc_main_abi(lr);

#ifdef __aarch64__
		__asm__ volatile(
			"mov x0, %x[argc]\n"
			"mov x1, %x[argv]\n"
			"mov x2, %x[envp]\n"
			"mov x3, %x[applep]\n"
			"mov x29, #0\n"
			"mov x30, %x[return_address]\n"
			"mov sp, %x[stack_top]\n"
			"br %x[entry_point]\n"
			:
			: [argc] "r"((uintptr_t)lr->argc),
			  [argv] "r"(lr->argv),
			  [envp] "r"(lr->envp),
			  [applep] "r"(lr->applep),
			  [return_address] "r"(lc_main_return),
			  [stack_top] "r"(lr->stack_top),
			  [entry_point] "r"(lr->entry_point)
			: "x0", "x1", "x2", "x3", "memory");
		__builtin_unreachable();
#else
		typedef int (*main_func_t)(int, char**, char**, char**);
		main_func_t entry = (main_func_t)lr->entry_point;
		int ret = entry((int)lr->argc, lr->argv, lr->envp, lr->applep);
		_exit(ret);
#endif
	}

	/* LC_UNIXTHREAD: set up stack and jump directly */
#ifdef __aarch64__
	__asm__ volatile(
		"mov x29, #0\n"
		"mov x30, #0\n"
		"mov sp, %1\n"
		"br %0"
		::
		"r"(lr->entry_point),
		"r"(lr->stack_top)
		:
	);
#elif defined(__x86_64__)
	__asm__ volatile(
		"mov %1, %%rsp\n"
		"jmpq *%0"
		::
		"m"(lr->entry_point),
		"r"(lr->stack_top)
		:
	);
#else
#       error Unsupported platform!
#endif
}
