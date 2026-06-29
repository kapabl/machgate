#!/bin/bash
set -euo pipefail

ROOT="${MACHGATE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
MACHGATE_C="$ROOT/src/machgate.c"
OVERLAY="$ROOT/scripts/libcxx_allocator_overlay.c"

bridge_calls="$(
    grep -F "configure_guest_cxx_allocator_hooks(&machgate_load_results)" "$MACHGATE_C" |
        wc -l
)"
bridge_gates="$(
    grep -F "MACHGATE_GUEST_CXX_ALLOCATOR_BRIDGE" "$MACHGATE_C" |
        wc -l
)"

if [ "$bridge_calls" -ne 2 ] || [ "$bridge_gates" -lt 2 ]; then
    echo "guest C++ allocator bridge must be opt-in on both bootstrap paths" >&2
    exit 1
fi

if grep -Fq "machgate_shim_guest_operator_" "$OVERLAY"; then
    echo "native libc++ allocator overlay must not call guest C++ operator hooks" >&2
    exit 1
fi
