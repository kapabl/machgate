#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
FIXTURE_DIR="tests/fixtures"
FIXTURE="$FIXTURE_DIR/dynamic_inherited_resolver"
PROVIDER="$(pwd)/$FIXTURE_DIR/libinherited_resolver_provider.so"
MAP_FILE="$(mktemp)"
trap 'rm -f "$MAP_FILE"' EXIT

[ -f "$FIXTURE" ] && [ -f "$PROVIDER" ] || \
    bash "$FIXTURE_DIR/build_fixtures.sh"

defines_symbol()
{
    local symbol="$1"
    readelf --dyn-syms --wide "$PROVIDER" | awk -v expected="$symbol" '
        $7 != "UND" && $8 == expected { found = 1 }
        END { exit !found }
    '
}

requires_symbol()
{
    local symbol="$1"
    readelf --dyn-syms --wide "$PROVIDER" | awk -v expected="$symbol" '
        $7 == "UND" {
            name = $8
            sub(/@.*/, "", name)
            if (name == expected) found = 1
        }
        END { exit !found }
    '
}

defines_symbol getrlimit
requires_symbol machgate_resolve_guest_import_v1
readelf --dynamic "$PROVIDER" | grep -q '\[libsystem_shim\.so\]'

printf 'libMachGateInheritedResolverFixture = %s\n' "$PROVIDER" > "$MAP_FILE"

status=0
LD_LIBRARY_PATH="$BUILD_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    MACHGATE_DYLIB_MAP="$MAP_FILE" \
    "$BUILD_DIR/machgate" "$FIXTURE" || status=$?
[ "$status" -eq 73 ]
