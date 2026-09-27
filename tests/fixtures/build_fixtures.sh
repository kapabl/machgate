#!/bin/bash
# Build test Mach-O fixtures using the LLVM cross-compilation toolchain.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

LLVM_MC="${LLVM_MC:-$(command -v llvm-mc || command -v llvm-mc-18 || true)}"
LD64_LLD="${LD64_LLD:-$(command -v ld64.lld || command -v ld64.lld-18 || true)}"
LLVM_LIPO="${LLVM_LIPO:-$(command -v llvm-lipo || command -v llvm-lipo-18 || true)}"
CLANGXX="${CLANGXX:-$(command -v clang++ || command -v clang++-18 || true)}"

[ -n "$LLVM_MC" ] || { echo "llvm-mc not found" >&2; exit 1; }
[ -n "$LD64_LLD" ] || { echo "ld64.lld not found" >&2; exit 1; }
[ -n "$LLVM_LIPO" ] || { echo "llvm-lipo not found" >&2; exit 1; }
[ -n "$CLANGXX" ] || { echo "clang++ not found" >&2; exit 1; }

echo "Building test fixtures..."

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

# --- cpp_many_ctors: 707 C++ static constructors, Darwin exit(0) on success ---
{
    cat <<'EOF'
extern "C" int main(int argc, char **argv, char **envp);
static volatile unsigned long long sink;
#define CTOR(N) __attribute__((constructor)) static void ctor_##N(void) { sink += (unsigned long long)(N + 1); }
EOF
    for constructor_index in $(seq 0 706); do
        printf 'CTOR(%s)\n' "$constructor_index"
    done
    cat <<'EOF'
extern "C" int main(int argc, char **argv, char **envp) {
    (void)argc;
    (void)argv;
    (void)envp;
    register unsigned long x0 __asm__("x0") = (sink == 250278ULL) ? 0 : 43;
    register unsigned long x16 __asm__("x16") = 1;
    __asm__ volatile("svc #0x80" : : "r"(x0), "r"(x16) : "memory");
    __builtin_unreachable();
}
EOF
} > cpp_many_ctors.cpp
"$CLANGXX" -target arm64-apple-macos11 -fno-exceptions -fno-rtti -nostdlib \
    -c cpp_many_ctors.cpp -o cpp_many_ctors.o
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 -o cpp_many_ctors -e _main cpp_many_ctors.o
rm -f cpp_many_ctors.o cpp_many_ctors.cpp
echo "  Built cpp_many_ctors"

# --- guest_cxx_allocator_bridge: executable-owned operator new definition ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o guest_cxx_allocator_bridge.o guest_cxx_allocator_bridge.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o guest_cxx_allocator_bridge -e _main guest_cxx_allocator_bridge.o
rm -f guest_cxx_allocator_bridge.o
echo "  Built guest_cxx_allocator_bridge"

# --- dynamic_libsystem_call: classic libSystem import binding ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o libsystem_fixture.o libsystem_fixture.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -dylib -install_name /usr/lib/libSystem.B.dylib \
    -o libsystem_fixture.dylib libsystem_fixture.o
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o dynamic_libsystem_call.o dynamic_libsystem_call.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o dynamic_libsystem_call -e _main dynamic_libsystem_call.o \
    libsystem_fixture.dylib
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -fixup_chains -o dynamic_chained_libsystem_call -e _main \
    dynamic_libsystem_call.o libsystem_fixture.dylib
rm -f libsystem_fixture.o dynamic_libsystem_call.o
echo "  Built dynamic_libsystem_call"
echo "  Built dynamic_chained_libsystem_call"

# --- dynamic_mapped_provider: non-libSystem native provider mapping ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o mapped_provider_stub.o mapped_provider_stub.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -dylib -install_name /usr/lib/libMachGateFixture.dylib \
    -o mapped_provider_stub.dylib mapped_provider_stub.o
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o dynamic_mapped_provider.o dynamic_mapped_provider.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o dynamic_mapped_provider -e _main dynamic_mapped_provider.o \
    mapped_provider_stub.dylib
rm -f mapped_provider_stub.o dynamic_mapped_provider.o
echo "  Built dynamic_mapped_provider"


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

# Create fat binary from an x86_64 decoy and the ARM64 Darwin fixture
"$LLVM_LIPO" -create -output fat_binary fat_stub_x86 darwin_exit42
rm -f fat_stub_x86
echo "  Built fat_binary"

# --- MachGate trampoline fixture ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o machgate_trampoline_target.o machgate_trampoline_target.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o machgate_trampoline_target -e _main machgate_trampoline_target.o
rm -f machgate_trampoline_target.o
echo "  Built machgate_trampoline_target"

# --- native MachGate trampoline provider ---
gcc -shared -fPIC -o libmachgate_test_provider.so machgate_native_provider.c
echo "  Built libmachgate_test_provider.so"

echo "All fixtures built."
