#!/bin/bash
# Test: wrapgen parses aarch64 ELF and emits valid C
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"

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

# Output file should exist and contain C code
[ -f "$tmpdir/wrapper.c" ]
grep -q "elfcalls" "$tmpdir/wrapper.c"
grep -q "dlsym_fatal" "$tmpdir/wrapper.c"
grep -q "dlopen_fatal" "$tmpdir/wrapper.c"
