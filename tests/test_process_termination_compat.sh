#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
PROBE="tests/fixtures/process_termination_probe"

[ -f "$PROBE" ] || cc -O2 -o "$PROBE" \
    tests/fixtures/process_termination_probe.c -ldl

exit_output="$(LD_LIBRARY_PATH="$BUILD_DIR" \
    "$PROBE" "$BUILD_DIR/libsystem_shim.so" exit)"
[ "$exit_output" = "buffered" ]

immediate_exit_output="$(LD_LIBRARY_PATH="$BUILD_DIR" \
    "$PROBE" "$BUILD_DIR/libsystem_shim.so" _exit)"
[ -z "$immediate_exit_output" ]

GUEST_RETURN_FIXTURE="tests/fixtures/darwin_guest_return_buffering"
[ -f "$GUEST_RETURN_FIXTURE" ] || bash tests/fixtures/build_fixtures.sh

set +e
guest_return_output="$(
    MACHGATE_SYSTEM_SHIM_LIB="$BUILD_DIR/libsystem_shim.so" \
        "$BUILD_DIR/machgate" "$GUEST_RETURN_FIXTURE" 2>/dev/null
)"
guest_return_status=$?
set -e

[ "$guest_return_status" -eq 23 ]
[ "$guest_return_output" = "buffered guest return" ]
