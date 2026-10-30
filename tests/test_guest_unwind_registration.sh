#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
FIXTURE="tests/fixtures/dynamic_guest_unwind"

[ -f "$FIXTURE" ] || bash tests/fixtures/build_fixtures.sh

MACHGATE_SYSTEM_SHIM_LIB="$BUILD_DIR/libsystem_shim.so" \
	timeout -k 2 10 "$BUILD_DIR/machgate" "$FIXTURE"
