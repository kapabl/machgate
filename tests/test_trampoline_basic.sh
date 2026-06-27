#!/bin/bash
# Test: trampoline patcher can redirect a Mach-O function to a native .so
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/trampoline_target ] || bash tests/fixtures/build_fixtures.sh

# Without trampoline: exits 42 (original static function)
# MACHGATE_CONFIG=none prevents picking up machgate.conf from CWD
status=0
MACHGATE_CONFIG=none "$BUILD_DIR/machgate" tests/fixtures/trampoline_target 2>/dev/null || status=$?
[ $status -eq 42 ]

# With trampoline: should exit 99 (native .so function called instead)
status=0
MACHGATE_CONFIG=none \
MACHGATE_TRAMPOLINE_LIB=tests/fixtures/libtest_native.so \
MACHGATE_TRAMPOLINE_PREFIX=_fake_lib_ \
    "$BUILD_DIR/machgate" tests/fixtures/trampoline_target 2>/dev/null || status=$?
[ $status -eq 99 ]
