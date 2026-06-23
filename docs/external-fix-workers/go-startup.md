# Worker C: Go LC_MAIN startup and null SIGSEGV diagnostics

Scope: null SIGSEGV diagnostics and Go LC_MAIN startup investigation for
`fzf`, `gum`, `shfmt`, `yq`, and `just`.

## Current status

- `fzf`: RECLASSIFIED. Go LC_MAIN reaches `_main`, then faults at
  `__TEXT,__text` PC `0x100066420`, fileoff `0x66420`, instruction
  `0x3980001b`, with `x0=NULL`.
- `gum`: RECLASSIFIED. Go LC_MAIN reaches `_main`, then faults at
  `__TEXT,__text` PC `0x10005f1e0`, fileoff `0x5f1e0`, instruction
  `0x3980001b`, with `x0=NULL`.
- `shfmt`: RECLASSIFIED. Go LC_MAIN reaches `_main`, then faults at
  `__TEXT,__text` PC `0x1000624d0`, fileoff `0x624d0`, instruction
  `0x3980001b`, with `x0=NULL`.
- `yq`: RECLASSIFIED. Original target matches the Go LC_MAIN failure pattern:
  PC `0x10006a6d0`, fileoff `0x6a6d0`, instruction `0x3980001b`, with
  `x0=NULL`.
- `just`: RECLASSIFIED. Original target is not the Go `x0=NULL` pattern. It
  reaches Rust startup/signal setup, then faults at PC `0x100287af4`,
  fileoff `0x287af4`, instruction `0xf940010a`, with `x8=NULL`.

No target is fixed by this pass. The loop stopped after attempt 1 because the
Go-family targets now have concrete PC/instruction evidence, and original
`yq`/`just` also have actionable PC evidence.

## Attempt 0: reproduce baseline

Command shape:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'cmake -S . -B build-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && cmake --build build-arm64 --parallel && MACHGATE_RUN_EXTERNAL=1 MACHGATE_EXTERNAL_MANIFEST=/tmp/worker-c-manifest.txt BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-c-attempt0 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-c-attempt0 MACHGATE_EXTERNAL_TIMEOUT=10 bash tests/test_external_macho_cli.sh'
```

Result: FAILED-0 baseline, 0/5 passed.

Evidence:

- Logs: `tests/external/logs/worker-c-attempt0/`
- `fzf`, `gum`, `shfmt`, and `yq` reached `machismo: calling _main`, then
  qemu reported `SIGSEGV` with `si_addr=NULL`.
- `just` reached `_main`, performed early Rust signal/altstack/thread setup,
  then qemu reported `SIGSEGV` with `si_addr=NULL`.

Changed files: none.

## Attempt 1: install gated SIGSEGV diagnostics

Patch:

- Added `trampoline_install_signal_diagnostics()` in `src/trampoline.c` and
  `src/trampoline.h`.
- Expanded the SIGSEGV handler to log signal code, fault address, PC, LR,
  SP/FP, registers `x0` through `x8`, Mach-O segment/section/file offset, and
  previous/current/next instruction words when `MACHGATE_TRACE_SIGNALS=1`.
- Installed the diagnostic handler from `src/machismo.c` when
  `MACHGATE_TRACE_SIGNALS=1`.

Build:

- Full build was blocked by an unrelated `src/shim/libsystem_shim.c`
  `CFDataCreate` implicit declaration/conflicting type error, outside Worker C
  scope.
- `cmake --build build-arm64 --target machismo --parallel` completed for the
  owned diagnostic path.
- Follow-up sanity check after the worker doc update: both
  `cmake --build build-arm64 --target machismo --parallel` and
  `cmake --build build-arm64 --parallel` reported `ninja: no work to do` in
  the ARM64 Docker container.

Command shape:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'MACHGATE_RUN_EXTERNAL=1 MACHGATE_TRACE_SIGNALS=1 MACHGATE_EXTERNAL_MANIFEST=/tmp/worker-c-manifest.txt BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-c-attempt1 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-c-attempt1 MACHGATE_EXTERNAL_TIMEOUT=10 bash tests/test_external_macho_cli.sh'
```

Result: PARTIAL/RECLASSIFIED, 0/5 passed. Diagnostics produced the requested
guest PC and instruction evidence.

Evidence:

- Logs: `tests/external/logs/worker-c-attempt1/`
- `fzf.err`: PC `0x100066420`, LR `0x1000663fc`, fileoff `0x66420`,
  instruction `0x3980001b`, prev `0x5400024a`, next `0xf8617802`,
  `x0=NULL`, `x8=0x40`.
- `gum.err`: PC `0x10005f1e0`, LR `0x10005f1bc`, fileoff `0x5f1e0`,
  instruction `0x3980001b`, prev `0x5400024a`, next `0xf8617802`,
  `x0=NULL`, `x8=0x40`.
- `shfmt.err`: PC `0x1000624d0`, LR `0x1000624ac`, fileoff `0x624d0`,
  instruction `0x3980001b`, prev `0x5400024a`, next `0xf8617802`,
  `x0=NULL`, `x8=0x40`.
- `yq.err`: PC `0x10006a6d0`, LR `0x10006a6ac`, fileoff `0x6a6d0`,
  instruction `0x3980001b`, prev `0x5400024a`, next `0xf8617802`,
  `x0=NULL`, `x8=0x40`.
- `just.err`: PC `0x100287af4`, LR `0x100287ac4`, fileoff `0x287af4`,
  instruction `0xf940010a`, prev `0xf9416129`, next `0xaa0803e0`,
  `x8=NULL`.

Changed files:

- `src/trampoline.c`
- `src/trampoline.h`
- `src/machismo.c`
- `docs/external-fix-workers/go-startup.md`

## Next blocker

The Go-family crash is now narrowed to LC_MAIN startup state, not missing
symbol binding. The next fix needs to determine why Go startup receives
`x0=NULL` at the first faulting runtime path. The likely area is LC_MAIN launch
ABI setup: direct C-call entry versus Darwin process entry expectations,
guest stack use, and the shape of `argc`, `argv`, `envp`, and `apple` vectors.

`just` is a separate Rust startup/TLV/signal bootstrap failure: it dereferences
through `x8=NULL` after early signal and alternate-stack setup.
