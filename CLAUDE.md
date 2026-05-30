# Machismo — Mach-O Loader (loader/runtime internals)

> This is the **machismo loader** submodule. It is consumed by the
> **machismo-ports** workspace (one level up): see that repo's top-level
> `CLAUDE.md` for the workspace layout, per-game status, binary analyses, the
> Bullseye build environment, and the port-promotion process. This file covers
> machismo's own internals. Public-facing overview: `README.md`.

## What machismo is

Loads and runs **Apple-Silicon arm64 Mach-O binaries on aarch64 Linux** with no
CPU emulation: map the Mach-O, set up the macOS-style memory/stack layout, then
redirect the binary's library dependencies to native Linux `.so`s (or to the
game's own dylibs loaded as Mach-O). The ARM instructions execute natively.
Originally extracted from [Darling](https://github.com/darlinghq/darling); now a
standalone, self-contained CMake project.

```
Mach-O game binary (arm64) ── GOT ──▶ Resolver (chained-fixup / dyld-info patcher)
                                          │
        ┌───────────────┬─────────────────┼──────────────┬───────────────┐
   native .so      Apple-ABI libc++   libsystem_shim   Mach-O dylibs   trampolines
   (SDL2, …)        (.so.1)            (→ glibc)        (SFML, …)       (SDL2/bgfx/steam)
```

## Build

```bash
cd machismo && cmake -S . -B build && cmake --build build   # loader + generic shims + tests
./scripts/build-libcxx.sh    # Apple-ABI libc++  → build-libcxx/lib/libc++.so.1
./scripts/build-luajit.sh    # custom LuaJIT     → build-luajit/lib/libluajit-5.1.so.2
./scripts/build-gl4es.sh     # GL→GLES           → build-gl4es/gl4es/lib/libGL.so.1
./scripts/build-bgfx.sh      # NecroDancer renderer
./scripts/build-sfml.sh      # NecroDancer SFML
bash tests/run_tests.sh      # test suite
```

machismo builds the **loader** + the **generic** shims (`libsystem_shim`,
`libgalaxy_shim`, and the parked `libobjc_shim`/`libgl_shim`). The **per-game**
override libraries (`libgame_patches`, `libsugar_patches_*`, `pcm_cache_write`)
build from the workspace umbrella, not here — their source lives in
`../ports/`, configs remain under `examples/`.

## What's built (loader/runtime)

### Loader
Fat-binary support, segment mapping, `LC_MAIN`/`LC_UNIXTHREAD` entry, Mach-O
stack layout, commpage emulation, static-initializer execution, pthread signature
fixup. `MAP_FIXED_NOREPLACE` fallback for kernels < 4.17. `entry_override` config
key bypasses native entry (custom runners).
- **`__init_offsets`** (section 0x16) — 32-bit relative offsets (NecroDancer).
- **`__mod_init_func`** (section 0x09) — absolute 64-bit ptrs (Pluralys, 3,907 ctors).
- Files: `src/machismo.c`, `src/loader.c`, `src/stack.c`, `src/commpage.c`, `src/macho_defs.h`

### Resolver
Patches GOT entries via `dylib_map.conf`, C++ ctor/dtor ABI adapters (skip >8 float
params), Mach-O self-symbol lookup (ordinal -1/-3 + RTLD_DEFAULT fallback), exec
stub pool. Two fixup formats:
- **`LC_DYLD_CHAINED_FIXUPS`** — format 6 (PTR_64_OFFSET) + format 2 (PTR_64). NecroDancer, Sugar.
- **`LC_DYLD_INFO_ONLY`** — rebase/bind opcode interpreter, ULEB128/SLEB128, three-pass bind. Pluralys.
- Public API: `resolver_lookup_symbol(mh, slide, name)` for post-load lookup.
- Files: `src/resolver.c`, `src/resolver.h`

### Mach-O dylib loader
Loads `.dylib` deps as Mach-O when native substitutes have ABI mismatches (SFML
audio/system, libnecrolevel). `MACHO:` prefix in `dylib_map.conf`. Maps segments,
parses `LC_SYMTAB`, resolves chained fixups, runs initializers. Preserves leading
underscore. **Limitation:** `_dl_find_object` hook supports only one text range →
dylib eh_frame disabled. Files: `src/dylib_loader.{c,h}`

### Trampoline system
Redirects statically-linked functions to native `.so` via 4-byte branch islands.
Multi-library, configured in `machismo.conf`. STUB mode, override mechanism, C++
mangling fallback (`y`↔`m`), `__DATA` stale-access guard. SDL2: 825 patched; bgfx:
605 + 236 trapped; Steam: 240 → return-0. Files: `src/trampoline.{c,h}`

### Other runtime pieces
- **GDB JIT symbols** — in-memory ELF, 40,930 symbols via GDB JIT iface. `src/gdb_jit.{c,h}`
- **C++ exceptions** — compact-unwind → DWARF `.eh_frame` (19,193 FDEs); interposes `_dl_find_object` (GCC 15). `src/eh_frame.{c,h}`
- **bgfx renderer** — native bgfx, GLES 3.1; KMSDRM auto-detect + EGL context sharing (SDL2<2.0.16 fallback); Mali opts. `src/bgfx_shim.c`, `scripts/build-bgfx.sh`
- **libSystem shim** — Apple `libSystem.B` → glibc: mach time, stdio/errno globals, math, BSD string, pthread, dispatch, TLV bootstrap, HOME rewrite, **zero-init malloc wrapper**, readdir/stat ABI xlate, CommonCrypto stubs. `src/shim/libsystem_shim.c`
- **Apple-ABI libc++** — `_LIBCPP_ABI_ALTERNATE_STRING_LAYOUT`; runtime macOS mutex+condvar signature detection (build-time patch `patches/libcxx-darwin-pthread-compat.patch`, no LLVM fork). `scripts/build-libcxx.sh`
- **libgalaxy shim** — minimal GOG Galaxy SDK; checks `goggame-*.info` for DLC ownership. `src/shim/libgalaxy_shim.c`
- **Config** — INI `machismo.conf` (`[general]`, `[trampoline.*]`); auto-discovered next to binary; `MACHISMO_CONFIG`/`MACHISMO_HOME` overrides. `src/config.{c,h}`
- **Binary patcher** — instruction-level patches from config (online-service stubs, null-vtable guards, Lua bridge stubs). `src/patcher.{c,h}`
- **LSE atomic emulation** — ARMv8.1 LSE → LDXR/STXR branch islands + ARMv8.3 RCPC→LDAR downgrade (879 RCPC + 4132 LSE) for Cortex-A35. `src/lse_emul.{c,h}`
- **Lua entity optimization** — C metamethods replace NecroDancer's interpreted sol2 `__index`/`__newindex`. `src/lua_entity_opt.{c,h}`
- **Custom LuaJIT** — unstripped, `jit.p`/`jit.v`, Cortex-A35 tuning, `-DLUAJIT_USE_PERFTOOLS` → `/tmp/perf-<PID>.map`. `scripts/build-luajit.sh`, `extern/LuaJIT`
- **SGR archive tooling** — `tools/sgr_extract.c` / `tools/sgr_repack.c` round-trip the Sugar `.sgr` format (xoshiro128 XOR + zlib + per-entry XOR trailer).
- **SFML** — audio/system as Mach-O dylibs (vtable ABI match); graphics/window/network native with Apple-ABI libc++. `scripts/build-sfml.sh`
- **Parked (gmloader-yyc branch):** `libobjc_shim`, `libgl_shim`, the GameMaker YYC runner (`src/gm/gm_runner.c`).

## What's NOT built
- **Multi-range eh_frame for dylibs** — `_dl_find_object` hook supports one Mach-O text range only; dylib exception handling disabled. Needs extension to multiple ranges.

## Key ABI differences

| Issue | macOS ARM64 | Linux ARM64 | Fix |
|-------|-------------|-------------|-----|
| Constructor return | returns `this` | void | ABI adapter (skip >8 float params) |
| `std::string` layout | alternate SSO | standard SSO | Apple-ABI libc++ |
| `pthread_mutex_t` | sig `0x32AAABA7` | zeros | three-layer detect & fixup |
| `malloc` zero-init | zeroed pages | maybe stale | `malloc → calloc` wrapper in shim |
| Weak symbols | search main exe | dlsym only | Mach-O symbol-table lookup |
| `__init_offsets` | run by dyld | not automatic | explicit execution |
| TLV descriptors | `_tlv_bootstrap` | `__thread` | custom impl |
| Exceptions | compact unwind | DWARF `.eh_frame` | converter + `_dl_find_object` hook |
| `uint64_t` mangling | `y` | `m` | fallback `y`↔`m` |
| SFML vtable layout | custom virtuals | stock 2.5.1 | load original dylibs as Mach-O |
| `pthread_cond_t` | sig `0x3CB0B1BB` | zeros | detect in libc++ condvar + shim |
| RCPC (LDAPR) | ARMv8.3 | ARMv8.0 on A35 | in-place bit flip → LDAR |
| LSE atomics | ARMv8.1 | ARMv8.0 on A35 | branch islands w/ LDXR/STXR |

## Branches
- `sugar-port` — active (Sugar engine work)
- `master` — GitHub release (LLVM trunk)
- `bullseye` — Debian Bullseye builds (LLVM 15.0.7, GCC); handheld-compatible. **Local-only / unpushed.**
- `gmloader-yyc` — parked GameMaker YYC runner. **Local-only / unpushed.**
- `kloptops-pr1` — upstream PR branch (tracks `kloptops/master`)

Remotes: `origin` = `bmdhacks/machismo`, `kloptops` = `kloptops/machismo`.

## Tooling
Mach-O: `llvm-mc`, `ld64.lld`, `llvm-lipo`, `llvm-otool`, `llvm-nm`. Decompiler:
rizin 0.7.4 + rz-ghidra. See the workspace CLAUDE.md + RE memories for analysis tooling.
