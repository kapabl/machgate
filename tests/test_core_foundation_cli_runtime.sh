#!/bin/bash
set -euo pipefail

cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"

[ -f "$BUILD_DIR/libsystem_shim.so" ] || {
    echo "libsystem_shim.so not built"
    exit 1
}

BUILD_DIR="$BUILD_DIR" LD_LIBRARY_PATH="$BUILD_DIR" python3 - <<'PY'
import ctypes
import os

library = ctypes.CDLL(os.path.join(os.environ['BUILD_DIR'], 'libsystem_shim.so'))

boolean_false = ctypes.c_void_p.in_dll(library, 'kCFBooleanFalse').value
boolean_true = ctypes.c_void_p.in_dll(library, 'kCFBooleanTrue').value
run_loop_default_mode = ctypes.c_void_p.in_dll(
    library, 'kCFRunLoopDefaultMode'
).value

assert boolean_false
assert boolean_true
assert boolean_false != boolean_true

library.CFBooleanGetTypeID.restype = ctypes.c_ulong
library.CFGetTypeID.argtypes = [ctypes.c_void_p]
library.CFGetTypeID.restype = ctypes.c_ulong
library.CFBooleanGetValue.argtypes = [ctypes.c_void_p]
library.CFBooleanGetValue.restype = ctypes.c_ubyte

boolean_type = library.CFBooleanGetTypeID()
assert library.CFGetTypeID(boolean_false) == boolean_type
assert library.CFBooleanGetValue(boolean_false) == 0
assert library.CFBooleanGetValue(boolean_true) == 1

assert run_loop_default_mode
library.CFStringGetCString.argtypes = [
    ctypes.c_void_p,
    ctypes.c_char_p,
    ctypes.c_long,
    ctypes.c_uint32,
]
library.CFStringGetCString.restype = ctypes.c_ubyte
buffer = ctypes.create_string_buffer(64)
assert library.CFStringGetCString(
    run_loop_default_mode, buffer, len(buffer), 0
) == 1
assert buffer.value == b'kCFRunLoopDefaultMode'

os_status_domain = ctypes.c_void_p.in_dll(
    library, 'kCFErrorDomainOSStatus'
).value
assert os_status_domain

library.CFErrorCreate.argtypes = [
    ctypes.c_void_p,
    ctypes.c_void_p,
    ctypes.c_long,
    ctypes.c_void_p,
]
library.CFErrorCreate.restype = ctypes.c_void_p
library.CFErrorGetCode.argtypes = [ctypes.c_void_p]
library.CFErrorGetCode.restype = ctypes.c_long
library.CFErrorGetDomain.argtypes = [ctypes.c_void_p]
library.CFErrorGetDomain.restype = ctypes.c_void_p

error = library.CFErrorCreate(None, os_status_domain, -9800, None)
assert error
assert library.CFErrorGetCode(error) == -9800
assert library.CFErrorGetDomain(error) == os_status_domain

for unsupported_symbol in [
    'CFRunLoopGetCurrent',
    'CFRunLoopRun',
    'FSEventStreamCopyDescription',
    'FSEventStreamCreate',
    'OBJC_CLASS_$_NSDate',
]:
    assert not hasattr(library, unsupported_symbol)
PY
