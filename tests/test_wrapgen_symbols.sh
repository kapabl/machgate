#!/bin/bash
# Test: wrapgen output contains expected symbol names from libz
set -e
cd "$(dirname "$0")/.."
MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"

tmpdir=$(mktemp -d)
trap "rm -rf $tmpdir" EXIT

LIBZ="${LIBZ:-}"
if [ -z "$LIBZ" ]; then
    for candidate in /usr/lib64/libz.so /usr/lib/aarch64-linux-gnu/libz.so /lib/aarch64-linux-gnu/libz.so /usr/lib/x86_64-linux-gnu/libz.so /lib/x86_64-linux-gnu/libz.so; do
        if [ -f "$candidate" ]; then
            LIBZ="$candidate"
            break
        fi
    done
fi

[ -n "$LIBZ" ] || { echo "libz.so not found" >&2; exit 1; }

"$BUILD_DIR/wrapgen" "$LIBZ" "$tmpdir/wrapper.c" "$tmpdir/wrapper.h"

# libz should have these well-known symbols
grep -q "compress" "$tmpdir/wrapper.c"
grep -q "uncompress" "$tmpdir/wrapper.c"
grep -q "inflate" "$tmpdir/wrapper.c"
grep -q "deflate" "$tmpdir/wrapper.c"
grep -q "adler32" "$tmpdir/wrapper.c"
