#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
FIXTURE="tests/fixtures/dynamic_compiler_divti3"
PROVIDER="$(pwd)/tests/fixtures/libmachgate_test_provider.so"
MAP_FILE="$(mktemp)"
trap 'rm -f "$MAP_FILE"' EXIT

[ -f "$FIXTURE" ] || bash tests/fixtures/build_fixtures.sh

printf 'libMachGateFixture = %s\n' "$PROVIDER" > "$MAP_FILE"

status=0
MACHGATE_DYLIB_MAP="$MAP_FILE" \
    "$BUILD_DIR/machgate" "$FIXTURE" 2>/dev/null || status=$?

[ "$status" -eq 70 ]
