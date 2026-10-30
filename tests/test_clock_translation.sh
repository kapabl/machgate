#!/bin/bash
# Test: shim time functions translate Darwin clock ids for guests.
# Rust guests call clock_gettime with Darwin ids such as CLOCK_UPTIME_RAW
# (8); an untranslated pass-through reaches Linux's CLOCK_REALTIME_ALARM,
# which fails with EINVAL for unprivileged processes on strict kernels.
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f "$BUILD_DIR/libsystem_shim.so" ] || { echo "libsystem_shim.so not built" >&2; exit 1; }

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT
gcc -O0 -o "$tmpdir/clock_translation_probe" \
    tests/fixtures/clock_translation_probe.c -ldl

output="$(cd "$BUILD_DIR" && LD_LIBRARY_PATH="$BUILD_DIR" "$tmpdir/clock_translation_probe")"
[ "$output" = "clock-translation-ok" ]
