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

# Darwin-padded synchronization layouts: the vendored libc++ routes ordinary
# pthread operations through Darwin-sized mutex and condition-variable types,
# and the shim's pthread exports implement the Darwin ABI for those objects.
header_threading="$LIBCXX_INCLUDE/__threading_support"
for marker in _LIBCPP_DARWIN_MUTEX_SIZE __machgate_rebuild_darwin_mutex __machgate_rebuild_darwin_cond; do
    grep -q "$marker" "$header_threading" || {
        echo "$header_threading does not carry the Darwin $marker surface" >&2
        exit 1
    }
done

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
)

for symbol in "${ordinary_symbols[@]}"; do
    grep -Fxq "$symbol" <<<"$provider_imports" || {
        echo "$LIBCXX_DIR/libc++.so.1 does not import $symbol" >&2
        exit 1
    }
    grep -Fxq "$symbol" <<<"$shim_exports" || {
        echo "$BUILD_DIR/libsystem_shim.so does not export $symbol" >&2
        exit 1
    }
done

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT
cat > "$tmpdir/darwin_layout_probe.cpp" <<'EOF'
#include <mutex>

static_assert(sizeof(std::__libcpp_mutex_t) == _LIBCPP_DARWIN_MUTEX_SIZE,
              "libc++ mutex storage must match the Darwin pthread_mutex_t size");

int main()
{
    std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);
    return 0;
}
EOF
if ! "${CXX:-clang++}" -std=c++17 -nostdinc++ -I "$LIBCXX_INCLUDE" \
        -fsyntax-only "$tmpdir/darwin_layout_probe.cpp" 2>/dev/null; then
    "${CXX:-clang++}" -std=c++17 -nostdinc++ -I "$LIBCXX_INCLUDE" \
        -fsyntax-only "$tmpdir/darwin_layout_probe.cpp"
    echo "$LIBCXX_INCLUDE does not preserve the Darwin mutex storage size" >&2
    exit 1
fi
