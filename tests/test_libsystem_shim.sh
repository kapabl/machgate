#!/bin/bash
# Test: libsystem_shim.so can be loaded and key Apple-specific symbols resolve
set -e
cd "$(dirname "$0")/.."
MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"

[ -f "$BUILD_DIR/libsystem_shim.so" ] || { echo "libsystem_shim.so not built"; exit 1; }

# Test that the .so loads without errors
BUILD_DIR="$BUILD_DIR" LD_LIBRARY_PATH="$BUILD_DIR" python3 -c "
import ctypes, sys, os

build_dir = os.environ['BUILD_DIR']
lib = ctypes.CDLL(os.path.join(build_dir, 'libsystem_shim.so'))

# mach_absolute_time should return a nonzero nanosecond timestamp
lib.mach_absolute_time.restype = ctypes.c_uint64
t1 = lib.mach_absolute_time()
t2 = lib.mach_absolute_time()
assert t1 > 0, f'mach_absolute_time returned 0'
assert t2 >= t1, f'mach_absolute_time went backwards: {t2} < {t1}'

# mach_timebase_info should return {1, 1}
class TimebaseInfo(ctypes.Structure):
    _fields_ = [('numer', ctypes.c_uint32), ('denom', ctypes.c_uint32)]
info = TimebaseInfo()
lib.mach_timebase_info(ctypes.byref(info))
assert info.numer == 1, f'timebase numer={info.numer}, expected 1'
assert info.denom == 1, f'timebase denom={info.denom}, expected 1'

# __error should return a valid pointer (errno location)
lib.__error.restype = ctypes.POINTER(ctypes.c_int)
p = lib.__error()
assert p, '__error returned NULL'

# __sincosf_stret should compute sin/cos correctly
class Float2(ctypes.Structure):
    _fields_ = [('sinval', ctypes.c_float), ('cosval', ctypes.c_float)]
lib.__sincosf_stret.restype = Float2
result = lib.__sincosf_stret(ctypes.c_float(0.0))
assert abs(result.sinval - 0.0) < 1e-6, f'sin(0)={result.sinval}'
assert abs(result.cosval - 1.0) < 1e-6, f'cos(0)={result.cosval}'

# __exp10f
lib.__exp10f.restype = ctypes.c_float
val = lib.__exp10f(ctypes.c_float(2.0))
assert abs(val - 100.0) < 0.01, f'__exp10f(2)={val}, expected 100'

# _NSGetExecutablePath should return something via /proc/self/exe
buf = ctypes.create_string_buffer(1024)
bufsize = ctypes.c_uint32(1024)
ret = lib._NSGetExecutablePath(buf, ctypes.byref(bufsize))
assert ret == 0, f'_NSGetExecutablePath returned {ret}'
path = buf.value.decode()
assert len(path) > 0, '_NSGetExecutablePath returned empty path'

# stdio globals should be non-null after constructor runs
# (ctypes can't easily check FILE* globals, but we can check they exist)
# Just verify the symbols exist
assert hasattr(lib, '__stdinp'), 'missing __stdinp'
assert hasattr(lib, '__stdoutp'), 'missing __stdoutp'
assert hasattr(lib, '__stderrp'), 'missing __stderrp'

# memset_pattern4
buf = ctypes.create_string_buffer(16)
pattern = (ctypes.c_uint8 * 4)(0xDE, 0xAD, 0xBE, 0xEF)
lib.memset_pattern4(buf, pattern, 16)
assert buf.raw == b'\\xDE\\xAD\\xBE\\xEF' * 4, f'memset_pattern4 failed: {buf.raw.hex()}'

lib.posix_memalign.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_size_t, ctypes.c_size_t]
lib.posix_memalign.restype = ctypes.c_int
lib.machgate_shim_memalign.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
lib.machgate_shim_memalign.restype = ctypes.c_void_p
lib.malloc_zone_memalign.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t]
lib.malloc_zone_memalign.restype = ctypes.c_void_p
lib.malloc.argtypes = [ctypes.c_size_t]
lib.malloc.restype = ctypes.c_void_p
lib.calloc.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
lib.calloc.restype = ctypes.c_void_p
lib.realloc.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
lib.realloc.restype = ctypes.c_void_p
lib.free.argtypes = [ctypes.c_void_p]
lib.malloc_size.argtypes = [ctypes.c_void_p]
lib.malloc_size.restype = ctypes.c_size_t

heap_ptr = lib.malloc(72)
assert heap_ptr, 'malloc failed'
assert lib.malloc_size(heap_ptr) >= 72, 'malloc_size returned too little for malloc allocation'
lib.free(heap_ptr)
assert lib.malloc_size(heap_ptr) == 0, 'malloc_size did not clear after free'

calloc_ptr = lib.calloc(3, 17)
assert calloc_ptr, 'calloc failed'
assert lib.malloc_size(calloc_ptr) >= 51, 'malloc_size returned too little for calloc allocation'
realloc_ptr = lib.realloc(calloc_ptr, 123)
assert realloc_ptr, 'realloc failed'
assert lib.malloc_size(realloc_ptr) >= 123, 'malloc_size returned too little after realloc'
lib.free(realloc_ptr)
assert lib.malloc_size(realloc_ptr) == 0, 'malloc_size did not clear after realloc/free'

aligned = ctypes.c_void_p()
assert lib.posix_memalign(ctypes.byref(aligned), 64, 129) == 0, 'posix_memalign failed'
assert aligned.value and aligned.value % 64 == 0, f'posix_memalign returned unaligned pointer {aligned.value:#x}'
assert lib.malloc_size(aligned) >= 129, 'malloc_size returned too little for aligned allocation'
lib.free(aligned)
assert lib.malloc_size(aligned) == 0, 'malloc_size did not clear after aligned free'

memalign_ptr = lib.machgate_shim_memalign(64, 72)
assert memalign_ptr and memalign_ptr % 64 == 0, f'memalign returned unaligned pointer {memalign_ptr:#x}'
assert lib.malloc_size(memalign_ptr) >= 72, 'malloc_size returned too little for memalign allocation'
lib.free(memalign_ptr)
assert lib.malloc_size(memalign_ptr) == 0, 'malloc_size did not clear after memalign free'

zone_memalign_ptr = lib.malloc_zone_memalign(None, 64, 72)
assert zone_memalign_ptr and zone_memalign_ptr % 64 == 0, f'malloc_zone_memalign returned unaligned pointer {zone_memalign_ptr:#x}'
assert lib.malloc_size(zone_memalign_ptr) >= 72, 'malloc_size returned too little for malloc_zone_memalign allocation'
lib.free(zone_memalign_ptr)
assert lib.malloc_size(zone_memalign_ptr) == 0, 'malloc_size did not clear after malloc_zone_memalign free'

# Darwin ctype masks must match Apple's <ctype.h>. Boost.Test validates
# names such as auto_start_dbg through std::isalnum, which reaches ___maskrune.
lib.__maskrune.argtypes = [ctypes.c_int, ctypes.c_ulong]
lib.__maskrune.restype = ctypes.c_int
CTYPE_A = 0x00000100
CTYPE_D = 0x00000400
CTYPE_P = 0x00002000
CTYPE_S = 0x00004000
CTYPE_U = 0x00008000
for ch in b'auto_start_dbg':
    valid = bool(lib.__maskrune(ch, CTYPE_A | CTYPE_D)) or chr(ch) in '+_?'
    assert valid, f'Boost.Test parameter character {chr(ch)!r} classified invalid'
assert lib.__maskrune(ord('Z'), CTYPE_A | CTYPE_U), 'uppercase alpha mask failed'
assert lib.__maskrune(ord('7'), CTYPE_D), 'digit mask failed'
assert lib.__maskrune(ord('_'), CTYPE_P), 'punctuation mask failed'
assert lib.__maskrune(ord(' '), CTYPE_S), 'space mask failed'

# Darwin pthread_once_t starts with a signature value that glibc's pthread_once
# does not understand. The shim must consume that state itself so C++/libc++
# one-time initialization does not fall through to host pthread semantics.
CALLBACK = ctypes.CFUNCTYPE(None)
once_state = ctypes.c_int(0x30B1BCBA)
once_calls = ctypes.c_int(0)

@CALLBACK
def once_init():
    once_calls.value += 1

lib.pthread_once.argtypes = [ctypes.c_void_p, CALLBACK]
lib.pthread_once.restype = ctypes.c_int
assert lib.pthread_once(ctypes.byref(once_state), once_init) == 0, 'pthread_once first call failed'
assert lib.pthread_once(ctypes.byref(once_state), once_init) == 0, 'pthread_once second call failed'
assert once_calls.value == 1, f'pthread_once init count={once_calls.value}, expected 1'
assert once_state.value == 2, f'pthread_once state={once_state.value:#x}, expected done state 2'

print('All libsystem_shim symbol tests passed')
"
