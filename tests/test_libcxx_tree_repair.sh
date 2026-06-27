#!/bin/bash
set -e
cd "$(dirname "$0")/.."
MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"

[ -x "$BUILD_DIR/test_libcxx_tree_repair" ] || {
    echo "SKIP: test_libcxx_tree_repair not built for this architecture"
    exit 0
}
[ -f "$BUILD_DIR/libsystem_shim.so" ] || { echo "libsystem_shim.so not built"; exit 1; }

LD_PRELOAD="$BUILD_DIR/libsystem_shim.so" \
LD_LIBRARY_PATH="$BUILD_DIR" \
"$BUILD_DIR/test_libcxx_tree_repair" >/dev/null 2>/dev/null
