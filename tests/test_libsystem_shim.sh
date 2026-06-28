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
lib.memalign.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
lib.memalign.restype = ctypes.c_void_p
lib.aligned_alloc.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
lib.aligned_alloc.restype = ctypes.c_void_p
lib.valloc.argtypes = [ctypes.c_size_t]
lib.valloc.restype = ctypes.c_void_p
lib.machgate_shim_memalign.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
lib.machgate_shim_memalign.restype = ctypes.c_void_p
lib.malloc_zone_memalign.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t]
lib.malloc_zone_memalign.restype = ctypes.c_void_p
lib.malloc_default_zone.restype = ctypes.c_void_p
lib.malloc_create_zone.argtypes = [ctypes.c_size_t, ctypes.c_uint]
lib.malloc_create_zone.restype = ctypes.c_void_p
lib.malloc_get_zone_name.argtypes = [ctypes.c_void_p]
lib.malloc_get_zone_name.restype = ctypes.c_char_p
lib.malloc_zone_from_ptr.argtypes = [ctypes.c_void_p]
lib.malloc_zone_from_ptr.restype = ctypes.c_void_p
lib.malloc_zone_malloc.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
lib.malloc_zone_malloc.restype = ctypes.c_void_p
lib.malloc_zone_calloc.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t]
lib.malloc_zone_calloc.restype = ctypes.c_void_p
lib.malloc_zone_realloc.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
lib.malloc_zone_realloc.restype = ctypes.c_void_p
lib.malloc_zone_free.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
lib.malloc_zone_size.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
lib.malloc_zone_size.restype = ctypes.c_size_t
lib.malloc_zone_valloc.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
lib.malloc_zone_valloc.restype = ctypes.c_void_p
lib.malloc_zone_claimed_address.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
lib.malloc_zone_claimed_address.restype = ctypes.c_int
lib.malloc.argtypes = [ctypes.c_size_t]
lib.malloc.restype = ctypes.c_void_p
lib.calloc.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
lib.calloc.restype = ctypes.c_void_p
lib.realloc.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
lib.realloc.restype = ctypes.c_void_p
lib.free.argtypes = [ctypes.c_void_p]
lib.malloc_size.argtypes = [ctypes.c_void_p]
lib.malloc_size.restype = ctypes.c_size_t
operator_new = getattr(lib, '_Znwm')
operator_new.argtypes = [ctypes.c_size_t]
operator_new.restype = ctypes.c_void_p
operator_new_aligned = getattr(lib, '_ZnwmSt11align_val_t')
operator_new_aligned.argtypes = [ctypes.c_size_t, ctypes.c_size_t]
operator_new_aligned.restype = ctypes.c_void_p
operator_delete_sized = getattr(lib, '_ZdlPvm')
operator_delete_sized.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
operator_delete_aligned_sized = getattr(lib, '_ZdlPvmSt11align_val_t')
operator_delete_aligned_sized.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t]

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

direct_memalign_ptr = lib.memalign(64, 72)
assert direct_memalign_ptr and direct_memalign_ptr % 64 == 0, f'direct memalign returned unaligned pointer {direct_memalign_ptr:#x}'
assert lib.malloc_size(direct_memalign_ptr) >= 72, 'malloc_size returned too little for direct memalign allocation'
lib.free(direct_memalign_ptr)
assert lib.malloc_size(direct_memalign_ptr) == 0, 'malloc_size did not clear after direct memalign free'

direct_aligned_ptr = lib.aligned_alloc(64, 72)
assert direct_aligned_ptr and direct_aligned_ptr % 64 == 0, f'aligned_alloc returned unaligned pointer {direct_aligned_ptr:#x}'
assert lib.malloc_size(direct_aligned_ptr) >= 72, 'malloc_size returned too little for aligned_alloc allocation'
lib.free(direct_aligned_ptr)
assert lib.malloc_size(direct_aligned_ptr) == 0, 'malloc_size did not clear after aligned_alloc free'

valloc_ptr = lib.valloc(72)
assert valloc_ptr, 'valloc failed'
assert lib.malloc_size(valloc_ptr) >= 72, 'malloc_size returned too little for valloc allocation'
lib.free(valloc_ptr)
assert lib.malloc_size(valloc_ptr) == 0, 'malloc_size did not clear after valloc free'

zone_memalign_ptr = lib.malloc_zone_memalign(None, 64, 72)
assert zone_memalign_ptr and zone_memalign_ptr % 64 == 0, f'malloc_zone_memalign returned unaligned pointer {zone_memalign_ptr:#x}'
assert lib.malloc_size(zone_memalign_ptr) >= 72, 'malloc_size returned too little for malloc_zone_memalign allocation'
lib.free(zone_memalign_ptr)
assert lib.malloc_size(zone_memalign_ptr) == 0, 'malloc_size did not clear after malloc_zone_memalign free'

default_zone = lib.malloc_default_zone()
assert default_zone, 'malloc_default_zone returned NULL'
assert lib.malloc_create_zone(0, 0) == default_zone, 'malloc_create_zone did not return the default zone'
assert lib.malloc_get_zone_name(default_zone), 'malloc_get_zone_name returned NULL'

zone_ptr = lib.malloc_zone_malloc(default_zone, 72)
assert zone_ptr, 'malloc_zone_malloc failed'
assert lib.malloc_zone_size(default_zone, zone_ptr) >= 72, 'malloc_zone_size returned too little'
assert lib.malloc_zone_from_ptr(zone_ptr) == default_zone, 'malloc_zone_from_ptr did not return default zone'
assert lib.malloc_zone_claimed_address(default_zone, zone_ptr) == 1, 'malloc_zone_claimed_address did not claim live allocation'
zone_realloc_ptr = lib.malloc_zone_realloc(default_zone, zone_ptr, 144)
assert zone_realloc_ptr, 'malloc_zone_realloc failed'
assert lib.malloc_zone_size(default_zone, zone_realloc_ptr) >= 144, 'malloc_zone_size returned too little after realloc'
lib.malloc_zone_free(default_zone, zone_realloc_ptr)
assert lib.malloc_zone_size(default_zone, zone_realloc_ptr) == 0, 'malloc_zone_size did not clear after zone free'

zone_calloc_ptr = lib.malloc_zone_calloc(default_zone, 2, 36)
assert zone_calloc_ptr, 'malloc_zone_calloc failed'
assert lib.malloc_zone_size(default_zone, zone_calloc_ptr) >= 72, 'malloc_zone_size returned too little for zone calloc'
lib.malloc_zone_free(default_zone, zone_calloc_ptr)

zone_valloc_ptr = lib.malloc_zone_valloc(default_zone, 72)
assert zone_valloc_ptr, 'malloc_zone_valloc failed'
assert lib.malloc_zone_size(default_zone, zone_valloc_ptr) >= 72, 'malloc_zone_size returned too little for zone valloc'
lib.malloc_zone_free(default_zone, zone_valloc_ptr)

new_ptr = operator_new(72)
assert new_ptr, 'operator new failed'
assert lib.malloc_size(new_ptr) >= 72, 'malloc_size returned too little for operator new allocation'
operator_delete_sized(new_ptr, 72)
assert lib.malloc_size(new_ptr) == 0, 'malloc_size did not clear after sized operator delete'

aligned_new_ptr = operator_new_aligned(72, 64)
assert aligned_new_ptr and aligned_new_ptr % 64 == 0, f'aligned operator new returned unaligned pointer {aligned_new_ptr:#x}'
assert lib.malloc_size(aligned_new_ptr) >= 72, 'malloc_size returned too little for aligned operator new allocation'
operator_delete_aligned_sized(aligned_new_ptr, 72, 64)
assert lib.malloc_size(aligned_new_ptr) == 0, 'malloc_size did not clear after sized aligned operator delete'

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
