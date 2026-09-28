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

# --- darwin_zero_vmsize_debug: debug bytes in a nonmapped segment ---
"$CLANGXX" -target arm64-apple-macos11 -x c -g -nostdlib \
    -c darwin_zero_vmsize_debug.c -o darwin_zero_vmsize_debug.o
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -headerpad 0x100 -o darwin_zero_vmsize_debug -e _main \
    darwin_zero_vmsize_debug.o
python3 add_zero_vmsize_debug_segment.py darwin_zero_vmsize_debug
rm -f darwin_zero_vmsize_debug.o
echo "  Built darwin_zero_vmsize_debug"

# --- darwin_tpidrro: Darwin read-only thread-pointer register ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o darwin_tpidrro.o darwin_tpidrro.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o darwin_tpidrro -e _main darwin_tpidrro.o
rm -f darwin_tpidrro.o
echo "  Built darwin_tpidrro"

# --- darwin_tsd_context: isolated Darwin thread-specific storage ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o tlv_context_provider_stub.o tlv_context_provider_stub.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -dylib -install_name /usr/lib/libSystem.B.dylib \
    -o tlv_context_provider_stub.dylib tlv_context_provider_stub.o
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o darwin_tsd_context.o darwin_tsd_context.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o darwin_tsd_context -e _main darwin_tsd_context.o \
    tlv_context_provider_stub.dylib
"$CLANGXX" -target arm64-apple-macos11 -x c -O0 -fno-builtin \
    -fno-stack-protector -nostdlib -c darwin_large_tlv.c \
    -o darwin_large_tlv.o
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o darwin_large_tlv -e _main darwin_large_tlv.o \
    tlv_context_provider_stub.dylib
"$CLANGXX" -target arm64-apple-macos11 -x c -O0 -fno-builtin \
    -fno-stack-protector -nostdlib -c darwin_gapped_tlv.c \
    -o darwin_gapped_tlv.o
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o darwin_gapped_tlv -e _main darwin_gapped_tlv.o \
    tlv_context_provider_stub.dylib
rm -f tlv_context_provider_stub.o darwin_tsd_context.o darwin_large_tlv.o \
    darwin_gapped_tlv.o
echo "  Built darwin_tsd_context"
echo "  Built darwin_large_tlv"
echo "  Built darwin_gapped_tlv"

# --- darwin_launch_context: LC_MAIN argv/env/apple vector contract ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o darwin_launch_context.o darwin_launch_context.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o darwin_launch_context -e _main darwin_launch_context.o
rm -f darwin_launch_context.o
echo "  Built darwin_launch_context"

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

# --- darwin_init_offsets: LLVM offset-based static initializer table ---
"$CLANGXX" -target arm64-apple-macos11 -x c -fno-builtin \
    -fno-stack-protector -nostdlib -c darwin_init_offsets.c \
    -o darwin_init_offsets.o
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -init_offsets -o darwin_init_offsets -e _main darwin_init_offsets.o
rm -f darwin_init_offsets.o
echo "  Built darwin_init_offsets"

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

"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o dynamic_flat_provider_precedence.o dynamic_flat_provider_precedence.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -flat_namespace -o dynamic_flat_provider_precedence -e _main \
    dynamic_flat_provider_precedence.o libsystem_fixture.dylib
rm -f dynamic_flat_provider_precedence.o
echo "  Built dynamic_flat_provider_precedence"

# --- darwin_variadic_abi: Darwin stack varargs and char-pointer va_list ---
"$CLANGXX" -target arm64-apple-macos11 -x c -O0 -fno-builtin \
    -fno-stack-protector -nostdlib -c darwin_variadic_abi.c \
    -o darwin_variadic_abi.o
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o darwin_variadic_abi -e _main darwin_variadic_abi.o \
    libsystem_fixture.dylib
rm -f darwin_variadic_abi.o
echo "  Built darwin_variadic_abi"

# --- darwin_file_variadic_abi: Darwin fcntl/ioctl stack arguments ---
"$CLANGXX" -target arm64-apple-macos11 -x c -O0 -fno-builtin \
    -fno-stack-protector -nostdlib -c darwin_file_variadic_abi.c \
    -o darwin_file_variadic_abi.o
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o darwin_file_variadic_abi -e _main darwin_file_variadic_abi.o \
    libsystem_fixture.dylib
rm -f darwin_file_variadic_abi.o
echo "  Built darwin_file_variadic_abi"

# --- darwin_guest_return_buffering: normal return flushes host stdio ---
"$CLANGXX" -target arm64-apple-macos11 -x c -O0 -fno-builtin \
    -fno-stack-protector -nostdlib -c darwin_guest_return_buffering.c \
    -o darwin_guest_return_buffering.o
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o darwin_guest_return_buffering -e _main \
    darwin_guest_return_buffering.o libsystem_fixture.dylib
rm -f darwin_guest_return_buffering.o
echo "  Built darwin_guest_return_buffering"

# --- darwin_task_policy: explicit unsupported current-task category policy ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o darwin_task_policy.o darwin_task_policy.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o darwin_task_policy -e _main darwin_task_policy.o \
    libsystem_fixture.dylib
rm -f darwin_task_policy.o
echo "  Built darwin_task_policy"

# --- dynamic_mapped_provider: non-libSystem native provider mapping ---
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o mapped_provider_stub.o mapped_provider_stub.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -dylib -install_name /usr/lib/libMachGateFixture.dylib \
    -o mapped_provider_stub.dylib mapped_provider_stub.o
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -dylib -install_name /usr/lib/libMachGateFixture.9.dylib \
    -o mapped_provider_versioned_stub.dylib mapped_provider_stub.o
"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o dynamic_mapped_provider.o dynamic_mapped_provider.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o dynamic_mapped_provider -e _main dynamic_mapped_provider.o \
    mapped_provider_stub.dylib
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o dynamic_versioned_provider -e _main dynamic_mapped_provider.o \
    mapped_provider_versioned_stub.dylib
rm -f mapped_provider_stub.o dynamic_mapped_provider.o
echo "  Built dynamic_mapped_provider"
echo "  Built dynamic_versioned_provider"

for fixture in dynamic_lazy_unused dynamic_lazy_called dynamic_skipped_data \
    dynamic_stubbed_function dynamic_partial_provider \
    dynamic_partial_used_data dynamic_regular_lazy_unused \
    dynamic_regular_lazy_called dynamic_compiler_runtime \
    dynamic_compiler_floattidf; do
    "$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
        -o "$fixture.o" "$fixture.s"
    "$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
        -o "$fixture" -e _main "$fixture.o" mapped_provider_stub.dylib
    rm -f "$fixture.o"
    echo "  Built $fixture"
done

for fixture in dynamic_lazy_unused dynamic_lazy_called; do
    chained_fixture="${fixture/dynamic_/dynamic_chained_}"
    "$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
        -o "$chained_fixture.o" "$fixture.s"
    "$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
        -fixup_chains -o "$chained_fixture" -e _main \
        "$chained_fixture.o" mapped_provider_stub.dylib
    rm -f "$chained_fixture.o"
    echo "  Built $chained_fixture"
done

"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o dynamic_chained_data_missing.o dynamic_partial_used_data.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -fixup_chains -o dynamic_chained_data_missing -e _main \
    dynamic_chained_data_missing.o mapped_provider_stub.dylib
rm -f dynamic_chained_data_missing.o
echo "  Built dynamic_chained_data_missing"

"$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
    -o dynamic_guest_definition.o dynamic_guest_definition.s
"$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
    -o dynamic_guest_definition -e _main dynamic_guest_definition.o
rm -f dynamic_guest_definition.o
echo "  Built dynamic_guest_definition"

backward_bind_cases=(
    "1:16944:47624"
    "2:4296:37696"
    "3:12056:24008"
)
for backward_bind_case in "${backward_bind_cases[@]}"; do
    IFS=: read -r case_name backward_target final_a_offset <<< "$backward_bind_case"
    "$LLVM_MC" -triple arm64-apple-macos11 -filetype=obj \
        --defsym "BACKWARD_TARGET=$backward_target" \
        --defsym "FINAL_A_OFFSET=$final_a_offset" \
        -o "dynamic_backward_bind_$case_name.o" dynamic_backward_bind.s
    "$LD64_LLD" -arch arm64 -platform_version macos 11.0.0 11.0.0 \
        -o "dynamic_backward_bind_$case_name" -e _main \
        "dynamic_backward_bind_$case_name.o" mapped_provider_stub.dylib
    rm -f "dynamic_backward_bind_$case_name.o"
    echo "  Built dynamic_backward_bind_$case_name"
done


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

gcc -O2 -o process_termination_probe process_termination_probe.c -ldl
echo "  Built process_termination_probe"

echo "All fixtures built."
