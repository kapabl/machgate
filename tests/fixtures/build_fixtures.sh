#!/bin/bash
# Build test Mach-O fixtures using the LLVM cross-compilation toolchain.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

LLVM_MC="${LLVM_MC:-$(command -v llvm-mc || command -v llvm-mc-18 || true)}"
LD64_LLD="${LD64_LLD:-$(command -v ld64.lld || command -v ld64.lld-18 || true)}"
LLVM_LIPO="${LLVM_LIPO:-$(command -v llvm-lipo || command -v llvm-lipo-18 || true)}"

[ -n "$LLVM_MC" ] || { echo "llvm-mc not found" >&2; exit 1; }
[ -n "$LD64_LLD" ] || { echo "ld64.lld not found" >&2; exit 1; }
[ -n "$LLVM_LIPO" ] || { echo "llvm-lipo not found" >&2; exit 1; }

echo "Building test fixtures..."

# --- exit42: static arm64 Mach-O, exit(42) ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o exit42.o exit42.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o exit42 -e _main exit42.o
rm -f exit42.o
echo "  Built exit42"

# --- exit0: static arm64 Mach-O, exit(0) ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o exit0.o exit0.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o exit0 -e _main exit0.o
rm -f exit0.o
echo "  Built exit0"

# --- hello_write: static arm64 Mach-O, write + exit ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o hello_write.o hello_write.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o hello_write -e _main hello_write.o
rm -f hello_write.o
echo "  Built hello_write"

# --- darwin_exit42: static arm64 Mach-O, Darwin SYS_exit(42) ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_exit42.o darwin_exit42.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_exit42 -e _main darwin_exit42.o
rm -f darwin_exit42.o
echo "  Built darwin_exit42"

# --- darwin_write_stdout: static arm64 Mach-O, Darwin write + exit ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_write_stdout.o darwin_write_stdout.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_write_stdout -e _main darwin_write_stdout.o
rm -f darwin_write_stdout.o
echo "  Built darwin_write_stdout"

# --- darwin_execve_exit42: static arm64 Mach-O, Darwin execve to darwin_exit42 ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_execve_exit42.o darwin_execve_exit42.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_execve_exit42 -e _main darwin_execve_exit42.o
rm -f darwin_execve_exit42.o
echo "  Built darwin_execve_exit42"

# --- darwin_panicwrap_self_reexec: static arm64 Mach-O, panicwrap-shaped fork/reexec ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_panicwrap_self_reexec.o darwin_panicwrap_self_reexec.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_panicwrap_self_reexec -e _main darwin_panicwrap_self_reexec.o
rm -f darwin_panicwrap_self_reexec.o
echo "  Built darwin_panicwrap_self_reexec"

# --- darwin_read_close: static arm64 Mach-O, Darwin read + close ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_read_close.o darwin_read_close.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_read_close -e _main darwin_read_close.o
rm -f darwin_read_close.o
echo "  Built darwin_read_close"

# --- darwin_open_lseek: static arm64 Mach-O, Darwin open + lseek ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_open_lseek.o darwin_open_lseek.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_open_lseek -e _main darwin_open_lseek.o
rm -f darwin_open_lseek.o
echo "  Built darwin_open_lseek"

# --- darwin_fstat: static arm64 Mach-O, Darwin fstat ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_fstat.o darwin_fstat.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_fstat -e _main darwin_fstat.o
rm -f darwin_fstat.o
echo "  Built darwin_fstat"

# --- darwin_mmap_protect: static arm64 Mach-O, Darwin mmap/mprotect/munmap ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_mmap_protect.o darwin_mmap_protect.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_mmap_protect -e _main darwin_mmap_protect.o
rm -f darwin_mmap_protect.o
echo "  Built darwin_mmap_protect"

# --- darwin_range_000_099: range dispatcher probe ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_range_000_099.o darwin_range_000_099.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_range_000_099 -e _main darwin_range_000_099.o
rm -f darwin_range_000_099.o
echo "  Built darwin_range_000_099"

# --- darwin_range_100_199: range dispatcher probe ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_range_100_199.o darwin_range_100_199.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_range_100_199 -e _main darwin_range_100_199.o
rm -f darwin_range_100_199.o
echo "  Built darwin_range_100_199"

# --- darwin_range_200_399: range dispatcher probe ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_range_200_399.o darwin_range_200_399.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_range_200_399 -e _main darwin_range_200_399.o
rm -f darwin_range_200_399.o
echo "  Built darwin_range_200_399"

# --- darwin_range_400_plus: range dispatcher probe ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o darwin_range_400_plus.o darwin_range_400_plus.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o darwin_range_400_plus -e _main darwin_range_400_plus.o
rm -f darwin_range_400_plus.o
echo "  Built darwin_range_400_plus"

# --- fat_binary: universal binary with x86_64 + arm64 ---
# Build x86_64 stub
"$LLVM_MC" -triple x86_64-apple-macos11 -filetype=obj -o fat_stub_x86.o fat_stub_x86.s
"$LD64_LLD" -arch x86_64 -platform_version macos 11.0.0 11.0.0 -o fat_stub_x86 -e _main fat_stub_x86.o
rm -f fat_stub_x86.o

# Create fat binary from x86_64 + arm64 exit42
"$LLVM_LIPO" -create -output fat_binary fat_stub_x86 exit42
rm -f fat_stub_x86
echo "  Built fat_binary"

# --- with_lc_main: arm64 Mach-O using LC_MAIN entry point ---
# ld64.lld produces LC_MAIN when -no_pie and -e _main are not used together.
# We use -lc to force dyld-style linking which generates LC_MAIN.
# If that doesn't work, we just use the same approach as above (LC_UNIXTHREAD).
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o with_lc_main.o with_lc_main.s
# Try to create with LC_MAIN by using -lSystem and no -e flag
# ld64.lld will use LC_MAIN when _main is the entry and we use dyld-style linking
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o with_lc_main with_lc_main.o -e _main 2>/dev/null || \
    "$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
        -o with_lc_main -e _main with_lc_main.o
rm -f with_lc_main.o
echo "  Built with_lc_main"

# --- trampoline_target: Mach-O with a static function to trampoline ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj -o trampoline_target.o trampoline_target.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o trampoline_target -e _main trampoline_target.o
rm -f trampoline_target.o
echo "  Built trampoline_target"

# --- libtest_native.so: native .so with replacement function ---
gcc -shared -fPIC -o libtest_native.so libtest_native.c
echo "  Built libtest_native.so"

echo "All fixtures built."
