#!/bin/bash
set -euo pipefail

ROOT="${MACHGATE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
PROVIDER="${MACHGATE_ASAN_PROVIDER:-/home/kapablanka/machgate-asan-aarch64/libclang_rt.asan-aarch64.so}"
REQUIRE="${MACHGATE_REQUIRE_ASAN_PROVIDER:-0}"

skip_or_fail()
{
    if [ "$REQUIRE" = "1" ]; then
        echo "$1" >&2
        exit 1
    fi
    echo "SKIP: $1"
    exit 0
}

[ -f "$BUILD_DIR/machgate" ] || skip_or_fail "$BUILD_DIR/machgate not built"
[ -f "$BUILD_DIR/libsystem_shim.so" ] || skip_or_fail "$BUILD_DIR/libsystem_shim.so not built"
[ -f "$ROOT/tests/fixtures/darwin_asan_probe" ] || skip_or_fail "tests/fixtures/darwin_asan_probe not built"
[ -f "$PROVIDER" ] || skip_or_fail "ASan provider not built: $PROVIDER"
SURFACE="$ROOT/tests/fixtures/libasan_darwin_surface.so"
[ -f "$SURFACE" ] || skip_or_fail "Darwin ASan surface adapter not built: $SURFACE"
command -v readelf >/dev/null 2>&1 || skip_or_fail "readelf is required to validate the ASan provider"
readelf -h "$PROVIDER" | grep -Eq 'Machine:[[:space:]]+AArch64' || {
    echo "ASan provider is not AArch64: $PROVIDER" >&2
    exit 1
}

provider_exports="$({
    readelf --wide --dyn-syms "$PROVIDER" |
        awk '$7 != "UND" { name = $8; sub(/@.*/, "", name); print name }'
} | sort -u)"

for symbol in __asan_init __asan_version_mismatch_check_v8 __asan_report_store1 \
    __asan_stack_malloc_0 __asan_handle_no_return \
    __asan_option_detect_stack_use_after_return; do
    grep -Fxq "$symbol" <<<"$provider_exports" || {
        echo "$PROVIDER does not export $symbol" >&2
        exit 1
    }
done

surface_exports="$({
    readelf --wide --dyn-syms "$SURFACE" |
        awk '$7 != "UND" { name = $8; sub(/@.*/, "", name); print name }'
} | sort -u)"

for symbol in __asan_register_image_globals __asan_unregister_image_globals; do
    grep -Fxq "$symbol" <<<"$surface_exports" || {
        echo "$SURFACE does not export $symbol" >&2
        exit 1
    }
done

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT
asan_lib_dir="$(cd "$(dirname "$PROVIDER")" && pwd)"
cat > "$tmpdir/dylib_map.conf" <<EOF
libSystem.B = $BUILD_DIR/libsystem_shim.so
libclang_rt.asan_osx_dynamic.dylib = $SURFACE
libclang_rt.asan_osx_dynamic = $SURFACE
EOF

status=0
ASAN_OPTIONS=verify_asan_link_order=0 \
LD_LIBRARY_PATH="$asan_lib_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
MACHGATE_DYLIB_MAP="$tmpdir/dylib_map.conf" \
    "$BUILD_DIR/machgate" "$ROOT/tests/fixtures/darwin_asan_probe" || status=$?
[ "$status" -eq 42 ] || {
    echo "ASan-instrumented guest exited $status, expected 42" >&2
    exit 1
}
