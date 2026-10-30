#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
FIXTURE="tests/fixtures/darwin_file_variadic_abi"

[ -f "$FIXTURE" ] || bash tests/fixtures/build_fixtures.sh

MACHGATE_SYSTEM_SHIM_LIB="$BUILD_DIR/libsystem_shim.so" \
    "$BUILD_DIR/machgate" "$FIXTURE" >/dev/null 2>&1
