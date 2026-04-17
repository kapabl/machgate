#!/bin/bash
#
# Build gl4es — desktop OpenGL → GLES translation layer.
# Used by Sugar engine games (Shotgun King, etc.) to run on GLES.
#
# gl4es provides a libGL.so.1 that translates GL 2.x/3.x/4.x calls
# to OpenGL ES 2.0/3.0. GLEW and other desktop GL code works transparently.
#
# Environment variables at runtime:
#   LIBGL_GL=32      — report GL 3.2 to the application
#   LIBGL_ES=2       — target GLES 2.0 backend
#   LIBGL_NOERROR=1  — suppress GL error checking (performance)
#
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MACHISMO_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$MACHISMO_DIR/build-gl4es"
GL4ES_SRC="$BUILD_DIR/gl4es"

echo "Building gl4es in $BUILD_DIR..."

mkdir -p "$BUILD_DIR"

# Clone if not present
if [ ! -d "$GL4ES_SRC" ]; then
    echo "Cloning gl4es..."
    git clone --depth 1 https://github.com/ptitSeb/gl4es.git "$GL4ES_SRC"
    # Teach pixel_convert's color map about GL_RED_INTEGER / GL_RG_INTEGER so
    # Sugar-engine palette-indexed framebuffer uploads (usampler2D textures)
    # don't bail in swizzle_texture with "pixel conversion, anticipated abort".
    (cd "$GL4ES_SRC" && patch -p1 < "$MACHISMO_DIR/patches/gl4es-red-integer.patch")
fi

# Build
cd "$GL4ES_SRC"
mkdir -p build && cd build
# NOX11: no X11/GLX — prevents EGL init in constructor (KMSDRM compat)
# GLX_STUBS: provide glX* symbols as no-op stubs
# EGL_WRAPPER: build libEGL.so.1 shim for SDL_VIDEO_EGL_DRIVER
# GBM: disabled unless libgbm-dev is installed (optional, for DRM surfaces)
cmake .. \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DNOX11=ON \
    -DGLX_STUBS=ON \
    -DEGL_WRAPPER=ON \
    -DGBM=OFF \
    -DCMAKE_C_FLAGS="-Wno-int-conversion -Wno-return-mismatch -Wno-incompatible-pointer-types"
make -j$(nproc)

echo ""
echo "gl4es built successfully."
echo "Library: $GL4ES_SRC/lib/libGL.so.1"
echo ""
echo "Usage: set LD_LIBRARY_PATH to include the gl4es lib directory"
echo "  export LD_LIBRARY_PATH=$GL4ES_SRC/lib:\$LD_LIBRARY_PATH"
