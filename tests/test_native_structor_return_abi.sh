#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
CLASSIC_FIXTURE="tests/fixtures/dynamic_structor_return"
CHAINED_FIXTURE="tests/fixtures/dynamic_chained_structor_return"
PROVIDER="tests/fixtures/libnative_structor_provider.so"
MAP_FILE="$(mktemp)"
trap 'rm -f "$MAP_FILE"' EXIT

[ -f "$CLASSIC_FIXTURE" ] && [ -f "$CHAINED_FIXTURE" ] && \
    [ -f "$PROVIDER" ] || bash tests/fixtures/build_fixtures.sh

printf 'libMachGateStructorFixture = %s\n' "$(pwd)/$PROVIDER" > "$MAP_FILE"

for fixture in "$CLASSIC_FIXTURE" "$CHAINED_FIXTURE"; do
    MACHGATE_DYLIB_MAP="$MAP_FILE" \
        "$BUILD_DIR/machgate" "$fixture"
done
