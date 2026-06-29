#!/bin/bash
set -euo pipefail

ROOT="${MACHGATE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
MACHGATE_C="$ROOT/src/machgate.c"
OVERLAY="$ROOT/scripts/libcxx_allocator_overlay.c"

bridge_calls="$(grep -Fc "configure_guest_cxx_allocator_hooks(&machgate_load_results)" "$MACHGATE_C" || true)"

if [ "$bridge_calls" -ne 0 ]; then
    echo "guest C++ allocator bridge must not be wired into Mach-O bootstrap" >&2
    exit 1
fi

if grep -Fq "machgate_shim_guest_operator_" "$OVERLAY"; then
    echo "native libc++ allocator overlay must not call guest C++ operator hooks" >&2
    exit 1
fi
