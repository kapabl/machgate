#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
FIXTURE="tests/fixtures/darwin_variadic_abi"

[ -f "$FIXTURE" ] || bash tests/fixtures/build_fixtures.sh

output="$(
    MACHGATE_SYSTEM_SHIM_LIB="$BUILD_DIR/libsystem_shim.so" \
        "$BUILD_DIR/machgate" "$FIXTURE" 2>/dev/null
)"

expected=$'printf:7:clean:3.5\nvfprintf:9000000001:room:2.25\nvsnprintf:42:safe:1.5\nsnprintf:8:direct:0.5'
[ "$output" = "$expected" ]
