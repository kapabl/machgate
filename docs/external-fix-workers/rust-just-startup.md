# Worker G: Rust just startup

Scope: original `just` startup, limited to pthread/signal/TLV shim code plus
optional trampoline diagnostics.

## Current status

- `just`: FIXED after attempt 2. `just --version` prints `just 1.53.0`.
- Root cause: Rust startup kept a TLV descriptor pointer live in `x8` across
  the Mach-O TLV thunk call. The C shim `_tlv_bootstrap` clobbered `x8`.
- Fix: preserve `x8` in a narrow ARM64 `_tlv_bootstrap` wrapper and keep the
  previous C TLV allocation/copy behavior in `_tlv_bootstrap_impl`.

## Attempt 1: reproduce and classify fault site

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail; cmake -S . -B build-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON; cmake --build build-arm64 --parallel; mkdir -p /work/tests/external/work/worker-g; awk -F"|" '\''/^#/ || NF < 5 {next} $1 == "just" {print}'\'' tests/external/arm64_macho_cli_manifest.txt > /work/tests/external/work/worker-g/just-manifest.txt; MACHGATE_RUN_EXTERNAL=1 MACHGATE_TRACE_SIGNALS=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/work/worker-g/just-manifest.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-g-attempt1 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-g-attempt1 MACHGATE_EXTERNAL_TIMEOUT=12 bash tests/test_external_macho_cli.sh'
```

Result: FAILED-1, `0/1 external Mach-O CLI probes passed`.

Evidence:

- Logs: `tests/external/logs/worker-g-attempt1/`
- `just.err`: PC `0x100287af4`, LR `0x100287ac4`, fileoff `0x287af4`,
  instruction `0xf940010a`, `x8=NULL`.
- Disassembly maps the PC to
  `std::sys::pal::unix::stack_overflow::thread_info::set_current_info`.
- The fault follows Rust startup signal/alt-stack setup and
  `pthread_threadid_np`.
- The faulting instruction is `ldr x10, [x8]` after a call through the
  Mach-O TLV thunk. The guest appears to keep a TLV descriptor pointer live in
  `x8` across `_tlv_bootstrap`; the C shim implementation does not preserve
  `x8`.

Changed files:

- `docs/external-fix-workers/rust-just-startup.md`

Touched functions:

- None.

Next suspected cause:

- `_tlv_bootstrap` needs a narrow ARM64 ABI wrapper that preserves `x8` while
  delegating existing TLV allocation/copy behavior to C.

## Attempt 2: preserve x8 across TLV bootstrap

Command:

```sh
docker run --rm --platform linux/arm64 -v /home/kapablanka/repos/machgate:/work -w /work machgate-arm64-toolchain bash -lc 'set -euo pipefail; cmake --build build-arm64 --parallel; MACHGATE_RUN_EXTERNAL=1 MACHGATE_TRACE_SIGNALS=1 MACHGATE_TRACE_SHIM=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/work/tests/external/work/worker-g/just-manifest.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-g-attempt2 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-g-attempt2 MACHGATE_EXTERNAL_TIMEOUT=12 bash tests/test_external_macho_cli.sh'
```

Result: FIXED, `1/1 external Mach-O CLI probes passed`.

Evidence:

- Logs: `tests/external/logs/worker-g-attempt2/`
- `just.out`: `just 1.53.0`
- `just.err`: Rust startup reaches `pthread_get_stackaddr_np`,
  `pthread_get_stacksize_np`, `sigaction`, `sigaltstack`,
  `pthread_threadid_np`, `pthread_setname_np`, and exits successfully.

Changed files:

- `docs/external-fix-workers/rust-just-startup.md`
- `src/shim/libsystem_shim.c`

Touched functions:

- `_tlv_bootstrap`
- `_tlv_bootstrap_impl`

Next suspected cause:

- None for Worker G. The target passed; no edits were made to pthread or
  signal functions.
