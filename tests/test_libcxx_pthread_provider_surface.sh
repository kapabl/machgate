#!/bin/bash
set -euo pipefail

ROOT="${MACHGATE_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
LIBCXX_DIR="${LIBCXX_DIR:-$ROOT/build-libcxx/lib}"
LIBCXX_INCLUDE="${LIBCXX_INCLUDE:-$ROOT/build-libcxx/include/c++/v1}"
REQUIRE="${MACHGATE_REQUIRE_LIBCXX_PTHREAD_ABI:-0}"

skip_or_fail()
{
    if [ "$REQUIRE" = "1" ]; then
        echo "$1" >&2
        exit 1
    fi
    echo "SKIP: $1"
    exit 0
}

[ -f "$LIBCXX_DIR/libc++.so.1" ] || skip_or_fail "$LIBCXX_DIR/libc++.so.1 not built"
[ -f "$BUILD_DIR/libsystem_shim.so" ] || skip_or_fail "$BUILD_DIR/libsystem_shim.so not built"
[ -d "$LIBCXX_INCLUDE" ] || skip_or_fail "$LIBCXX_INCLUDE not built"

provider_imports="$({
    readelf --wide --dyn-syms "$LIBCXX_DIR/libc++.so.1" |
        awk '$7 == "UND" { name = $8; sub(/@.*/, "", name); print name }'
} | sort -u)"

shim_exports="$({
    readelf --wide --dyn-syms "$BUILD_DIR/libsystem_shim.so" |
        awk '$7 != "UND" { name = $8; sub(/@.*/, "", name); print name }'
} | sort -u)"

provider_symbols=(
    machgate_darwin_pthread_mutex_init
    machgate_darwin_pthread_mutex_destroy
    machgate_darwin_pthread_mutex_lock
    machgate_darwin_pthread_mutex_trylock
    machgate_darwin_pthread_mutex_unlock
    machgate_darwin_pthread_cond_signal
    machgate_darwin_pthread_cond_broadcast
    machgate_darwin_pthread_cond_wait
    machgate_darwin_pthread_cond_timedwait
    machgate_darwin_pthread_cond_destroy
)

for symbol in "${provider_symbols[@]}"; do
    grep -Fxq "$symbol" <<<"$provider_imports" || {
        echo "$LIBCXX_DIR/libc++.so.1 does not import $symbol" >&2
        exit 1
    }
    grep -Fxq "$symbol" <<<"$shim_exports" || {
        echo "$BUILD_DIR/libsystem_shim.so does not export $symbol" >&2
        exit 1
    }
done

grep -Fxq machgate_darwin_pthread_cond_clockwait <<<"$shim_exports" || {
    echo "$BUILD_DIR/libsystem_shim.so does not export machgate_darwin_pthread_cond_clockwait" >&2
    exit 1
}

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT
"${CXX:-clang++}" \
    -std=c++17 \
    -nostdinc++ \
    -I "$LIBCXX_INCLUDE" \
    -c "$ROOT/tests/fixtures/libcxx_pthread_clockwait_probe.cpp" \
    -o "$tmpdir/libcxx_pthread_clockwait_probe.o"
if ! nm -u "$tmpdir/libcxx_pthread_clockwait_probe.o" |
    awk '{ print $NF }' | grep -Fxq machgate_darwin_pthread_cond_clockwait; then
    echo "$LIBCXX_INCLUDE does not route condition clockwait to the shim" >&2
    exit 1
fi

ordinary_symbols=(
    pthread_mutex_init
    pthread_mutex_destroy
    pthread_mutex_lock
    pthread_mutex_trylock
    pthread_mutex_unlock
    pthread_cond_signal
    pthread_cond_broadcast
    pthread_cond_wait
    pthread_cond_timedwait
    pthread_cond_destroy
    pthread_cond_clockwait
)

for symbol in "${ordinary_symbols[@]}"; do
    if grep -Fxq "$symbol" <<<"$provider_imports"; then
        echo "$LIBCXX_DIR/libc++.so.1 still imports ordinary $symbol" >&2
        exit 1
    fi
done
