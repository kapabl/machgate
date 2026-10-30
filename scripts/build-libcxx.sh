#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
libcxx_source_dir="${MACHGATE_LIBCXX_SOURCE_DIR:-$project_root/extern/llvm-project}"
libcxx_build_dir="${MACHGATE_LIBCXX_BUILD_DIR:-$project_root/build-libcxx}"
allocator_overlay="$project_root/scripts/libcxx_allocator_overlay.c"
system_shim="${MACHGATE_SHIM_LIB:-}"

if [ ! -d "$libcxx_source_dir/libcxx" ]; then
    echo "Apple-ABI libc++ source not found: $libcxx_source_dir/libcxx" >&2
    echo "Run: git submodule update --init extern/llvm-project" >&2
    exit 1
fi

if command -v clang >/dev/null 2>&1 && command -v clang++ >/dev/null 2>&1; then
    c_compiler=clang
    cxx_compiler=clang++
else
    c_compiler=gcc
    cxx_compiler=g++
fi

if ! grep -q "_LIBCPP_ABI_DARWIN_CTYPE_COMPAT" \
    "$libcxx_source_dir/libcxx/include/__locale"; then
    ctype_patch="$(mktemp)"
    cat > "$ctype_patch" <<'CTYPE_PATCH_EOF'
diff --git a/libcxx/include/__config b/libcxx/include/__config
index c97cbae96f..e599f4380d 100644
--- a/libcxx/include/__config
+++ b/libcxx/include/__config
@@ -755 +755 @@ _LIBCPP_BEGIN_NAMESPACE_STD _LIBCPP_END_NAMESPACE_STD
-#  if defined(__APPLE__) || defined(__FreeBSD__)
+#  if defined(__APPLE__) || defined(__FreeBSD__) || defined(_LIBCPP_ABI_DARWIN_CTYPE_COMPAT)
diff --git a/libcxx/include/__locale b/libcxx/include/__locale
index 40f9a3ff57..19f8b8ebd4 100644
--- a/libcxx/include/__locale
+++ b/libcxx/include/__locale
@@ -423,0 +424,13 @@ public:
+#elif defined(_LIBCPP_ABI_DARWIN_CTYPE_COMPAT)
+    typedef __uint32_t mask;
+    static const mask space  = 0x00004000;
+    static const mask print  = 0x00040000;
+    static const mask cntrl  = 0x00000200;
+    static const mask upper  = 0x00008000;
+    static const mask lower  = 0x00001000;
+    static const mask alpha  = 0x00000100;
+    static const mask digit  = 0x00000400;
+    static const mask punct  = 0x00002000;
+    static const mask xdigit = 0x00010000;
+    static const mask blank  = 0x00020000;
+    static const mask __regex_word = 0x80;
diff --git a/libcxx/src/locale.cpp b/libcxx/src/locale.cpp
index 6b454274e1..a0b0c52c17 100644
--- a/libcxx/src/locale.cpp
+++ b/libcxx/src/locale.cpp
@@ -55,0 +56,10 @@ _LIBCPP_PUSH_MACROS
+#if defined(_LIBCPP_ABI_DARWIN_CTYPE_COMPAT)
+struct _RuneLocale {
+    char __pad[60];
+    unsigned int __runetype[256];
+    int __maplower[256];
+    int __mapupper[256];
+};
+extern "C" _RuneLocale _DefaultRuneLocale;
+#endif
+
@@ -1193 +1203 @@ ctype<char>::classic_table() noexcept
-#if defined(__APPLE__) || defined(__FreeBSD__)
+#if defined(_LIBCPP_ABI_DARWIN_CTYPE_COMPAT) || defined(__APPLE__) || defined(__FreeBSD__)
CTYPE_PATCH_EOF
    git -C "$libcxx_source_dir" apply --unidiff-zero "$ctype_patch"
    rm -f "$ctype_patch"
fi

required_markers=(
    "libcxx/include/__ios/fpos.h:_LIBCPP_ABI_DARWIN_MBSTATE_COMPAT"
    "libcxx/include/__locale:_LIBCPP_ABI_DARWIN_CTYPE_COMPAT"
    "libcxx/include/__threading_support:_LIBCPP_DARWIN_MUTEX_SIZE"
)
for requirement in "${required_markers[@]}"; do
    relative_path="${requirement%%:*}"
    marker="${requirement#*:}"
    if ! grep -q "$marker" "$libcxx_source_dir/$relative_path"; then
        echo "Apple-ABI libc++ marker missing: $marker" >&2
        exit 1
    fi
done

if [ -z "$system_shim" ]; then
    for candidate in \
        "$project_root/build/libsystem_shim.so" \
        "$project_root/build-arm64/libsystem_shim.so" \
        "$project_root/build-arm64-release/libsystem_shim.so" \
        "$project_root/build-shim/libsystem_shim.so"; do
        if [ -f "$candidate" ]; then
            system_shim="$candidate"
            break
        fi
    done
fi
if [ ! -f "$system_shim" ]; then
    echo "libsystem_shim.so not found; build MachGate or set MACHGATE_SHIM_LIB" >&2
    exit 1
fi

mkdir -p "$libcxx_build_dir"
allocator_overlay_object="$libcxx_build_dir/machgate_libcxx_allocator_overlay.o"
"$c_compiler" -fPIC -O2 -c "$allocator_overlay" \
    -o "$allocator_overlay_object"

wrapped_symbols=(
    malloc free calloc realloc posix_memalign memalign aligned_alloc valloc
    _Znwm _Znam _ZnwmRKSt9nothrow_t _ZnamRKSt9nothrow_t
    _ZnwmSt11align_val_t _ZnamSt11align_val_t
    _ZnwmSt11align_val_tRKSt9nothrow_t _ZnamSt11align_val_tRKSt9nothrow_t
    _ZdlPv _ZdaPv _ZdlPvm _ZdaPvm
    _ZdlPvRKSt9nothrow_t _ZdaPvRKSt9nothrow_t
    _ZdlPvSt11align_val_t _ZdaPvSt11align_val_t
    _ZdlPvmSt11align_val_t _ZdaPvmSt11align_val_t
    _ZdlPvSt11align_val_tRKSt9nothrow_t _ZdaPvSt11align_val_tRKSt9nothrow_t
)
shared_linker_flags="$allocator_overlay_object -L$(dirname "$system_shim") -lsystem_shim"
for symbol in "${wrapped_symbols[@]}"; do
    shared_linker_flags+=" -Wl,--wrap=$symbol"
done

cmake -G Ninja \
    -S "$libcxx_source_dir/runtimes" \
    -B "$libcxx_build_dir" \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
    -DLIBCXXABI_USE_LLVM_UNWINDER=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="$c_compiler" \
    -DCMAKE_CXX_COMPILER="$cxx_compiler" \
    -DLIBCXX_ABI_VERSION=1 \
    -DLIBCXX_ABI_DEFINES="_LIBCPP_ABI_ALTERNATE_STRING_LAYOUT;_LIBCPP_ABI_DARWIN_MBSTATE_COMPAT;_LIBCPP_ABI_DARWIN_CTYPE_COMPAT" \
    -DLIBCXX_ENABLE_SHARED=ON \
    -DLIBCXX_ENABLE_STATIC=OFF \
    -DLIBCXX_INCLUDE_TESTS=OFF \
    -DLIBCXX_INCLUDE_BENCHMARKS=OFF \
    -DLIBCXXABI_ENABLE_SHARED=ON \
    -DLIBCXXABI_ENABLE_STATIC=OFF \
    -DLIBCXXABI_INCLUDE_TESTS=OFF \
    -DCMAKE_SHARED_LINKER_FLAGS="$shared_linker_flags"

cmake --build "$libcxx_build_dir" --target cxx cxxabi --parallel

echo "Built $libcxx_build_dir/lib/libc++.so.1"
echo "Built $libcxx_build_dir/lib/libc++abi.so.1"
