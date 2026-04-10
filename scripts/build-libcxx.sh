#!/bin/bash
# Build Apple-ABI-compatible libc++ for aarch64 Linux.
#
# This produces a libc++.so.1 with Apple's std::string layout
# (_LIBCPP_ABI_ALTERNATE_STRING_LAYOUT) so Mach-O binaries compiled
# against Apple's libc++ get the correct memory layout.
#
# Uses bmdhacks/llvm-project fork (darwin-abi-compat branch) which
# includes Darwin ABI compatibility patches:
#   - pthread_mutex_t padded to 64 bytes (matching macOS)
#   - Darwin mutex/condvar signature detection and reinitialization
#   - fpos<mbstate_t> padded to 136 bytes (macOS mbstate_t = 128 bytes)
#
# Compiler auto-detection: uses clang if available (>= 15), falls back to GCC.
#
# Usage: ./scripts/build-libcxx.sh
# Output: build-libcxx/lib/libc++.so.1

set -e

# Always work from the project root
cd "$(dirname "$0")/.."

BUILD_DIR=build-libcxx
SRC_DIR=extern/llvm-project

if [ ! -d "$SRC_DIR/libcxx" ]; then
    echo "Error: $SRC_DIR/libcxx not found. Run: git submodule update --init extern/llvm-project"
    exit 1
fi

# Detect compiler: prefer clang >= 15, fall back to GCC
if command -v clang++ >/dev/null 2>&1; then
    CLANG_VER=$(clang++ --version | head -1 | grep -oP '\d+' | head -1)
    if [ "$CLANG_VER" -ge 15 ] 2>/dev/null; then
        CC_TO_USE=clang
        CXX_TO_USE=clang++
        echo "Using clang $CLANG_VER"
    else
        CC_TO_USE=gcc
        CXX_TO_USE=g++
        echo "Using GCC (clang too old: $CLANG_VER)"
    fi
else
    CC_TO_USE=gcc
    CXX_TO_USE=g++
    echo "Using GCC (clang not found)"
fi

# Verify we're on the darwin-abi-compat branch (not upstream LLVM)
BRANCH=$(cd "$SRC_DIR" && git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "detached")
if [ "$BRANCH" != "darwin-abi-compat" ]; then
    echo "Switching to darwin-abi-compat branch..."
    (cd "$SRC_DIR" && git checkout darwin-abi-compat)
fi

echo "Configuring Apple-ABI libc++ build..."

cmake -G Ninja -S "$SRC_DIR/runtimes" -B "$BUILD_DIR" \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
    -DLIBCXXABI_USE_LLVM_UNWINDER=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="$CC_TO_USE" \
    -DCMAKE_CXX_COMPILER="$CXX_TO_USE" \
    -DLIBCXX_ABI_VERSION=1 \
    -DLIBCXX_ABI_DEFINES="_LIBCPP_ABI_ALTERNATE_STRING_LAYOUT;_LIBCPP_ABI_DARWIN_MBSTATE_COMPAT" \
    -DLIBCXX_ENABLE_SHARED=ON \
    -DLIBCXX_ENABLE_STATIC=OFF \
    -DLIBCXX_INCLUDE_TESTS=OFF \
    -DLIBCXX_INCLUDE_BENCHMARKS=OFF \
    -DLIBCXXABI_ENABLE_SHARED=ON \
    -DLIBCXXABI_ENABLE_STATIC=OFF \
    -DLIBCXXABI_INCLUDE_TESTS=OFF

echo ""
echo "Configuration complete. To build:"
echo "  cd $BUILD_DIR && ninja cxx cxxabi"
echo ""
echo "Output will be in $BUILD_DIR/lib/"
