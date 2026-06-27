#!/bin/bash
# Test: resolver can parse NecroDancer's chained fixups and report stats
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"

NECRO_BIN="../necrodancer/depot_247086/NecroDancerSP.app/Contents/MacOS/NecroDancer"

if [ ! -f "$NECRO_BIN" ]; then
    echo "SKIP: NecroDancer binary not found"
    exit 0
fi

output=$(MACHGATE_DYLIB_MAP=dylib_map.conf LD_LIBRARY_PATH="$BUILD_DIR" "$BUILD_DIR/machgate" "$NECRO_BIN" 2>&1 || true)

# Resolver should have found dylibs and chained fixups
echo "$output" | grep -q "found 38 dylibs"
echo "$output" | grep -q "chained fixups"
echo "$output" | grep -q "imports_count=3704"
echo "$output" | grep -q "pointer_format=6"

# Should have resolved some binds and applied rebases
echo "$output" | grep -q "binds resolved"
echo "$output" | grep -q "rebases"
