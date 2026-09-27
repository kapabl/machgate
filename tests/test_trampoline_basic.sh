#!/bin/bash
# Test: trampoline patcher can redirect a Mach-O function to a native .so
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
[ -f tests/fixtures/machgate_trampoline_target ] || bash tests/fixtures/build_fixtures.sh

# Without trampoline: exits with the guest implementation's result.
status=0
"$BUILD_DIR/machgate" tests/fixtures/machgate_trampoline_target 2>/dev/null || status=$?
[ $status -eq 41 ]

# With trampoline: exits with the native provider's result.
status=0
MACHGATE_TRAMPOLINE_LIB=tests/fixtures/libmachgate_test_provider.so \
MACHGATE_TRAMPOLINE_PREFIX=_machgate_ \
    "$BUILD_DIR/machgate" tests/fixtures/machgate_trampoline_target 2>/dev/null || status=$?
[ $status -eq 99 ]
