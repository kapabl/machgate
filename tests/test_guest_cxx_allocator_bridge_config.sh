#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
FIXTURE="tests/fixtures/guest_cxx_allocator_bridge"

[ -f "$FIXTURE" ] || bash tests/fixtures/build_fixtures.sh

output="$(
    MACHGATE_SYSTEM_SHIM_LIB="$BUILD_DIR/libsystem_shim.so" \
    MACHGATE_TRACE_CXX_ALLOCATORS=1 \
        "$BUILD_DIR/machgate" "$FIXTURE" 2>&1
)"

grep -Fq "machgate: configured 1 guest C++ allocator hook" <<<"$output"
grep -Fq "machgate_shim_guest_operator_" scripts/libcxx_allocator_overlay.c
