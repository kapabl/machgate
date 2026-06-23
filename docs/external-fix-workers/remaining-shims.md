# Worker H: Remaining Shim Surface

Owned source scope:

- `src/shim/libsystem_shim.c`
- `src/shim/libsystem_shim.ver`
- `docs/external-fix-workers/remaining-shims.md`

Targets:

- `sqlite3`
- `bun`
- `nvim`
- `node`
- representative skipped `Security` / `libresolv` / `libiconv` cases

Status: `RECLASSIFIED` after 3 attempts.

Exit criteria met: repeated Worker H-owned `resolver: symbol ... not found`
imports for the four target rows are now implemented or safely stubbed, and
`sqlite3` reclassified from missing malloc-zone/fsctl/gethostuuid imports plus
`Error: out of memory` to post-`_main` `SIGILL`.

No pthread, signal, or TLV functions were edited.

## Implemented Surface

Added minimal CLI-safe shims in `src/shim/libsystem_shim.c`:

- malloc zone: `malloc_default_zone`, `malloc_create_zone`,
  `malloc_set_zone_name`, `malloc_zone_malloc`, `malloc_zone_realloc`,
  `malloc_zone_free`, `malloc_size`
- Darwin helpers: `fsctl`, `gethostuuid`, `srandomdev`, `atexit`,
  `__darwin_check_fd_set_overflow`, `sscanf_l`
- CoreFoundation additions: `CFArraySetValueAtIndex`, `CFDictionaryCreate`,
  `CFDictionaryContainsKey`, `CFDictionaryGetValue`, `CFEqual`,
  `CFNumberGetValue`, `CFStringGetCString`,
  `CFStringGetMaximumSizeForEncoding`, `CFTimeZoneCopyDefault`,
  `kCFAllocatorDefault`, `kCFBooleanTrue`, `kCFTypeArrayCallBacks`,
  `kCFTypeDictionaryKeyCallBacks`, `kCFTypeDictionaryValueCallBacks`
- CommonCrypto safe stubs: `CCCryptorCreateWithMode`, `CCCryptorReset`,
  `CCCryptorUpdate`, `CCHmacInit`, `CCHmacUpdate`, `CCHmacFinal`,
  `CCKeyDerivationPBKDF`
- Blocks/MIG: `_NSConcreteMallocBlock`, `_Block_copy`, `_Block_release`,
  `NDR_record`
- Mach host/thread/semaphore stubs: `mach_host_self`, `mach_thread_self`,
  `mach_continuous_time`, `host_statistics`, `host_processor_info`,
  `task_policy_set`, `thread_info`, `vm_deallocate`, `semaphore_create`,
  `semaphore_destroy`, `semaphore_signal`, `semaphore_wait`,
  `semaphore_timedwait`
- socket/process-adjacent unsupported stubs: `recvmsg_x`, `sendmsg_x`,
  `posix_spawn_file_actions_addinherit_np`

`src/shim/libsystem_shim.ver` was not changed; the new symbols are exported by
the existing build/link setup.

Skipped dylib decision:

- The external harness still maps `Security`, `libresolv`, and `libiconv` to
  `SKIP`. Worker H did not edit the harness or canonical manifests. The shim
  additions cover symbols imported from mapped `libSystem.B` and
  `CoreFoundation`; skipped dylib rows remain documented as later mapping or
  dedicated dylib-shim work.

## Attempt 1

Status: `FAILED-1` baseline reproduction.

Command:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'set -euo pipefail
awk -F"|" '\''BEGIN{print "# Worker H remaining shim surface"} /^#/ || $1=="sqlite3" || $1=="bun" || $1=="nvim" || $1=="node" {print}'\'' tests/external/worker_manifests/worker-5.txt > /tmp/worker-h-attempt1.txt
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/tmp/worker-h-attempt1.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-h-attempt1 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-h-attempt1 MACHGATE_EXTERNAL_TIMEOUT=30 bash tests/test_external_macho_cli.sh'
```

Result:

- `0/4 external Mach-O CLI probes passed, 4 failed`
- `sqlite3`: status 1; missing `atexit`, `fsctl`, `gethostuuid`,
  malloc-zone APIs, `srandomdev`; reaches `_main` and prints
  `Error: out of memory`
- `node`: status 124; missing CF dictionary/string/constants surface;
  `Security` skipped
- `bun`: status 139; missing CommonCrypto, Blocks, and `NDR_record`;
  `libresolv` skipped
- `nvim`: status 139; missing Mach host/thread/semaphore/spawn helpers;
  `CoreServices` and `libiconv` skipped

Changed files after attempt:

- none

Next cause:

- Implement the repeated mapped-dylib missing imports in `libsystem_shim.c`.

## Attempt 2

Status: `PARTIAL`.

Patch:

- Added malloc-zone, Darwin helper, CoreFoundation dictionary/string/constant,
  CommonCrypto, Blocks, Mach host/thread/semaphore, and socket/process-adjacent
  stubs.

Build:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'cmake --build build-arm64 --parallel'
```

Run command:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'set -euo pipefail
awk -F"|" '\''BEGIN{print "# Worker H remaining shim surface"} /^#/ || $1=="sqlite3" || $1=="bun" || $1=="nvim" || $1=="node" {print}'\'' tests/external/worker_manifests/worker-5.txt > /tmp/worker-h-attempt2.txt
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/tmp/worker-h-attempt2.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-h-attempt2 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-h-attempt2 MACHGATE_EXTERNAL_TIMEOUT=30 bash tests/test_external_macho_cli.sh'
```

Result:

- `0/4 external Mach-O CLI probes passed, 4 failed`
- `sqlite3`: `resolver: done - 193 binds resolved, 0 stubbed, 0 failed`;
  reclassified to post-`_main` `SIGILL`
- `node`: no Worker H CF missing imports remain visible before timeout;
  `Security` remains skipped
- `bun`: no Worker H CommonCrypto/Blocks missing imports remain; still
  `SIGSEGV`; `libicucore`, `libresolv`, and `libc++` remain skipped/no mapping
- `nvim`: first-wave Mach/semaphore/spawn imports resolved; second wave
  exposed `sendmsg_x`, `sscanf_l`, `task_policy_set`, `thread_info`,
  `vm_deallocate`

Changed files after attempt:

- `src/shim/libsystem_shim.c`

Next cause:

- Implement nvim's second-wave mapped `libSystem.B` imports.

## Attempt 3

Status: `RECLASSIFIED`.

Patch:

- Added `sendmsg_x`, `sscanf_l`, `task_policy_set`, `thread_info`, and
  `vm_deallocate`.

Build:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'cmake --build build-arm64 --parallel'
```

Run command:

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'set -euo pipefail
awk -F"|" '\''BEGIN{print "# Worker H remaining shim surface"} /^#/ || $1=="sqlite3" || $1=="bun" || $1=="nvim" || $1=="node" {print}'\'' tests/external/worker_manifests/worker-5.txt > /tmp/worker-h-attempt3.txt
MACHGATE_RUN_EXTERNAL=1 BUILD_DIR=/work/build-arm64 MACHGATE_EXTERNAL_MANIFEST=/tmp/worker-h-attempt3.txt MACHGATE_EXTERNAL_LOGS=/work/tests/external/logs/worker-h-attempt3 MACHGATE_EXTERNAL_WORK=/work/tests/external/work/worker-h-attempt3 MACHGATE_EXTERNAL_TIMEOUT=30 bash tests/test_external_macho_cli.sh'
```

Result:

- `0/4 external Mach-O CLI probes passed, 4 failed`
- `rg -o "resolver: symbol '[^']+' not found" tests/external/logs/worker-h-attempt3/{sqlite3,node,bun,nvim}.err` produced no rows
- `sqlite3`: `resolver: done - 193 binds resolved, 0 stubbed, 0 failed`;
  post-`_main` `SIGILL`
- `node`: timeout remains; logs still show many LSE out-of-range warnings,
  `Security` skipped, and `libc++` no mapping
- `bun`: `resolver: done - 561 binds resolved, 3 stubbed, 475 failed`;
  still `SIGSEGV`; `libicucore`, `libresolv`, and `libc++` remain skipped/no
  mapping; LSE pool still reports exhausted after 32774 patches
- `nvim`: `resolver: done - 407 binds resolved, 0 stubbed, 7 failed`;
  no symbol-not-found lines; still crashes during EH-frame registration;
  `CoreServices`, `libiconv`, and `libutil` remain skipped/no mapping

Changed files after attempt:

- `src/shim/libsystem_shim.c`
- `docs/external-fix-workers/remaining-shims.md`

Next cause:

- `sqlite3`: post-entry `SIGILL`, likely outside remaining shim import surface.
- `node`: large-binary LSE range/timeout plus skipped `Security` and unmapped
  `libc++`.
- `bun`: LSE pool exhaustion and unmapped `libicucore`/`libc++`; `libresolv`
  remains skipped by harness.
- `nvim`: EH-frame/runtime crash plus skipped `CoreServices`/`libiconv` and
  unmapped `libutil`.

## Verification

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'BUILD_DIR=/work/build-arm64 bash tests/run_tests.sh'
```

Result:

- `27/27 passed, 0 failed`

```sh
docker run --rm --platform linux/arm64 \
  -v /home/kapablanka/repos/machgate:/work -w /work \
  machgate-arm64-toolchain bash -lc 'BUILD_DIR=/work/build-arm64 bash tests/test_libsystem_shim.sh'
```

Result:

- `All libsystem_shim symbol tests passed`
