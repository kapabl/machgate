#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
SHIM="$BUILD_DIR/libsystem_shim.so"
FIXTURE="tests/fixtures/dynamic_resource_limit_imports"

defines_symbol()
{
    local symbol="$1"
    readelf --dyn-syms --wide "$SHIM" | awk -v expected="$symbol" '
        $7 != "UND" {
            name = $8
            sub(/@.*/, "", name)
            if (name == expected) found = 1
        }
        END { exit !found }
    '
}

! defines_symbol getrlimit
! defines_symbol setrlimit
defines_symbol machgate_darwin_getrlimit
defines_symbol machgate_darwin_setrlimit
defines_symbol machgate_resolve_guest_import_v1

[ -f "$FIXTURE" ] || bash tests/fixtures/build_fixtures.sh

MACHGATE_SYSTEM_SHIM_LIB="$SHIM" \
    "$BUILD_DIR/machgate" "$FIXTURE"
