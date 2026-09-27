#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
FIXTURE="tests/fixtures/dynamic_chained_libsystem_call"

[ -f "$FIXTURE" ] || bash tests/fixtures/build_fixtures.sh

output="$(
    MACHGATE_SYSTEM_SHIM_LIB="$BUILD_DIR/libsystem_shim.so" \
        "$BUILD_DIR/machgate" "$FIXTURE" 2>/dev/null
)"

[ "$output" = "dynamic-ok" ]
