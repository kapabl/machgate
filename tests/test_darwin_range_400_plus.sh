#!/bin/bash
# Test: representative Darwin ARM64 syscalls in the 400+ range.
set -e
cd "$(dirname "$0")/.."
MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"
FIXTURE_DIR="$MACHISMO_ROOT/tests/fixtures"
FIXTURE="$FIXTURE_DIR/darwin_range_400_plus"

LLVM_MC="${LLVM_MC:-$(command -v llvm-mc || command -v llvm-mc-18 || true)}"
LD64_LLD="${LD64_LLD:-$(command -v ld64.lld || command -v ld64.lld-18 || true)}"

[ -n "$LLVM_MC" ] || { echo "llvm-mc not found" >&2; exit 1; }
[ -n "$LD64_LLD" ] || { echo "ld64.lld not found" >&2; exit 1; }

rm -f "$FIXTURE" "$FIXTURE_DIR/darwin_range_400_plus.o"
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o "$FIXTURE_DIR/darwin_range_400_plus.o" \
    "$FIXTURE_DIR/darwin_range_400_plus.s"
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o "$FIXTURE" -e _main "$FIXTURE_DIR/darwin_range_400_plus.o"
rm -f "$FIXTURE_DIR/darwin_range_400_plus.o"

if ! nm "$BUILD_DIR/machgate" 2>/dev/null | grep -q 'syscall_range_400_plus_dispatch'; then
    echo "SKIP: syscall_range_400_plus_dispatch is not linked into machgate yet"
    exit 0
fi

rm -f "$FIXTURE_DIR/range_400.sock" \
      "$FIXTURE_DIR/range_400_rename_src.tmp" \
      "$FIXTURE_DIR/range_400_rename_dst.tmp" \
      "$FIXTURE_DIR/range_400_rename_excl_src.tmp" \
      "$FIXTURE_DIR/range_400_clonefileat.tmp" \
      "$FIXTURE_DIR/range_400_fclonefileat.tmp" \
      "$FIXTURE_DIR/range_400_guarded.tmp" \
      "$FIXTURE_DIR/range_400_guarded_dprotected.tmp"
printf "src" > "$FIXTURE_DIR/range_400_rename_src.tmp"
printf "excl" > "$FIXTURE_DIR/range_400_rename_excl_src.tmp"
output="$("$BUILD_DIR/machgate" tests/fixtures/darwin_range_400_plus 2>/dev/null)"
[ "$output" = "range400 0123" ]
[ ! -e "$FIXTURE_DIR/range_400_rename_src.tmp" ]
[ -e "$FIXTURE_DIR/range_400_rename_dst.tmp" ]
[ -e "$FIXTURE_DIR/range_400_rename_excl_src.tmp" ]
[ -e "$FIXTURE_DIR/range_400_clonefileat.tmp" ]
[ -e "$FIXTURE_DIR/range_400_fclonefileat.tmp" ]
rm -f "$FIXTURE_DIR/range_400.sock" \
      "$FIXTURE_DIR/range_400_rename_src.tmp" \
      "$FIXTURE_DIR/range_400_rename_dst.tmp" \
      "$FIXTURE_DIR/range_400_rename_excl_src.tmp" \
      "$FIXTURE_DIR/range_400_clonefileat.tmp" \
      "$FIXTURE_DIR/range_400_fclonefileat.tmp" \
      "$FIXTURE_DIR/range_400_guarded.tmp" \
      "$FIXTURE_DIR/range_400_guarded_dprotected.tmp"
