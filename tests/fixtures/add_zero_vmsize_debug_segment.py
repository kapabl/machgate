#!/usr/bin/env python3
import struct
import sys


LC_SEGMENT_64 = 0x19
MACH_HEADER_64_SIZE = 32
SEGMENT_COMMAND_64_SIZE = 72


path = sys.argv[1]
with open(path, "rb") as executable:
    image = bytearray(executable.read())

command_count, command_bytes = struct.unpack_from("<II", image, 16)
command_end = MACH_HEADER_64_SIZE + command_bytes
debug_payload = b"MachGate debug fixture\0"
debug_offset = len(image)
segment = struct.pack(
    "<II16sQQQQiiII",
    LC_SEGMENT_64,
    SEGMENT_COMMAND_64_SIZE,
    b"__DWARF".ljust(16, b"\0"),
    0,
    0,
    debug_offset,
    len(debug_payload),
    0,
    0,
    0,
    0,
)

if any(image[command_end : command_end + len(segment)]):
    raise SystemExit("Mach-O header padding is too small for debug segment")

image[command_end : command_end + len(segment)] = segment
struct.pack_into("<II", image, 16, command_count + 1, command_bytes + len(segment))
image.extend(debug_payload)

with open(path, "wb") as executable:
    executable.write(image)
