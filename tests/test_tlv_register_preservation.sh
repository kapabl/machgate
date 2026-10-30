#!/bin/bash
# Test: Darwin TLV resolvers preserve guest caller registers other than x0.
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_DIR="${BUILD_DIR:-$(pwd)/build}"
probe_directory="$(mktemp -d)"
trap 'rm -rf "$probe_directory"' EXIT

cc -shared -fPIC -o "$probe_directory/libtlv_register_probe.so" \
    tests/fixtures/tlv_register_probe.S

BUILD_DIR="$BUILD_DIR" PROBE_DIR="$probe_directory" \
LD_LIBRARY_PATH="$BUILD_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
python3 - <<'PY'
import ctypes
import os


class TlvDescriptor(ctypes.Structure):
    _fields_ = [
        ("resolver", ctypes.c_void_p),
        ("key", ctypes.c_size_t),
        ("offset", ctypes.c_size_t),
    ]


shim = ctypes.CDLL(os.path.join(os.environ["BUILD_DIR"], "libsystem_shim.so"))
probe = ctypes.CDLL(
    os.path.join(os.environ["PROBE_DIR"], "libtlv_register_probe.so")
)
probe.machgate_probe_tlv_registers.argtypes = [
    ctypes.c_void_p,
    ctypes.POINTER(TlvDescriptor),
]
probe.machgate_probe_tlv_registers.restype = ctypes.c_int
descriptor = TlvDescriptor(None, 0, 0)
resolver = ctypes.cast(shim._tlv_bootstrap, ctypes.c_void_p)
assert probe.machgate_probe_tlv_registers(resolver, ctypes.byref(descriptor)) == 0
PY
