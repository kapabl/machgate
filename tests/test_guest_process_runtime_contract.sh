#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
[ -x "$BUILD_DIR/machgate" ] || exit 1

readelf --dyn-syms --wide "$BUILD_DIR/machgate" |
    awk '$7 != "UND" && $8 == "machgate_guest_process_runtime_state_v1" { found = 1 }
         END { exit !found }'

if readelf --dyn-syms --wide "$BUILD_DIR/machgate" |
    awk '$7 != "UND" && $8 ~ /^__machgate_guest_|^__machgate_main_stack_/ { found = 1 }
         END { exit !found }'; then
    printf '%s\n' "legacy Guest Process globals remain exported" >&2
    exit 1
fi

