# ASan-instrumented guest support

## Scope

MachGate can load and run ARM64 Mach-O command-line executables built with
`-fsanitize=address` by mapping the guest's ASan runtime dependency to a Linux
aarch64 compiler-rt ASan runtime through the existing provider map. This is a
runtime-mapping capability built on the documented provider machinery; it adds
no new loader or syscall surface.

## How it works

An ASan-instrumented guest links against
`@rpath/libclang_rt.asan_osx_dynamic.dylib` and imports the platform-stable
compiler interface (`__asan_init`, `__asan_version_mismatch_check_v8`,
`__asan_report_*`, `__asan_stack_malloc_*`, image-globals registration, and
companions). The dylib map routes that dependency to a composite provider:

- `libasan_darwin_surface.so` exports correct no-ops for the Mach-O-only
  `__asan_register_image_globals` and `__asan_unregister_image_globals`
  entry points, which the Linux runtime leaves unimplemented because ELF
  objects use the ELF-globals path.
- The surface library carries a `DT_NEEDED` dependency on the real
  `libclang_rt.asan-aarch64.so`, and `dlsym` on the mapped provider follows
  that dependency, so every portable `__asan_*` import resolves to the real
  runtime.

Because registered image globals are not poisoned under the no-op surface,
global-buffer-overflow detection is unavailable for such guests. Stack,
heap-through-shim-allocator, and report-path behavior remain governed by the
real runtime.

## Requirements

- a Linux aarch64 `libclang_rt.asan-aarch64.so` provider (LLVM compiler-rt,
  same compiler major as the instrumentation), recorded in the map through
  the surface library's `DT_NEEDED` and `rpath`;
- `ASAN_OPTIONS=verify_asan_link_order=0` in the MachGate process
  environment, because a guest-mapped runtime can never be the first library
  in the host link order;
- the fixture-built surface adapter `tests/fixtures/libasan_darwin_surface.so`
  (`tests/fixtures/asan_darwin_surface.c`), built by
  `tests/fixtures/build_fixtures.sh` when `MACHGATE_ASAN_PROVIDER` names the
  runtime.

## Behavioral test

`tests/test_libclang_rt_asan_provider.sh` validates the provider's architecture
and exports, the surface adapter's exports, and runs the instrumented
`darwin_asan_probe` fixture end to end under the mapped provider. Without the
provider the test skips; with `MACHGATE_REQUIRE_ASAN_PROVIDER=1` it fails.

ASan-instrumented guests remain subject to the documented fail-closed provider
policy: without a mapped provider, an instrumented guest fails at launch on
its unresolved `__asan_*` imports.
