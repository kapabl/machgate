#!/usr/bin/env python3
"""
Generate a pcm_cache manifest from a host-warmed Sugar PCM cache.

For each OGG file in a data.sgr extract, we:
  1. Compute the runtime cache key: FNV-1a64(filename) -> chain -> FNV-1a64(freq)
  2. Read the matching <key>.pcm file from the host warm cache
  3. Extract the 24 stable sample_t bytes (offsets 8..31 within sample_t,
     which is offsets 40..63 in the cache file)
  4. Emit one manifest line: "<key> <freq> <ogg_filename> <sample_t_24_hex>"

patch.bash on the target device then loops over manifest entries:
  - oggdec --raw the matching OGG to S16 stereo PCM
  - pcm_cache_write builds the proper SGPC cache file

This script runs once on the dev host, committing the generated manifest to
the port tree. Re-run when new music tracks are added.

Usage:
    scripts/gen_pcm_cache_manifest.py \\
        --music-dir ~/darling/shotgun_king/extracted/assets/music \\
        --cache-dir ~/darling/shotgun_king/shotgun_king.app/Contents/MacOS/.machgate-pcm-cache \\
        --freq 48000 \\
        --out port/shotgunking/patch/cache_meta.txt
"""

import argparse
import os
import struct
import sys
from pathlib import Path

FNV_OFFSET = 0xcbf29ce484222325
FNV_PRIME  = 0x100000001b3
MASK64     = 0xffffffffffffffff


def fnv1a64(data: bytes, seed: int = FNV_OFFSET) -> int:
    h = seed
    for b in data:
        h ^= b
        h = (h * FNV_PRIME) & MASK64
    return h


def cache_key(filename: str, use_freq: int) -> str:
    h1 = fnv1a64(filename.encode("utf-8"))
    # sizeof(uint32_t) = 4, little-endian byte order as laid out in C memory
    freq_bytes = struct.pack("<I", use_freq)
    h2 = fnv1a64(freq_bytes, h1)
    return f"{h1:016x}{h2:016x}"


def extract_sample_t_stable(cache_path: Path) -> bytes:
    """Return the 24 stable bytes (sample_t offsets 8..31)."""
    with open(cache_path, "rb") as f:
        f.seek(40)  # cache-file offset 40 = sample_t offset 8
        return f.read(24)


def extract_pcm_bytes(cache_path: Path) -> int:
    """Return the pcm_bytes field from the header.

    This is whatever malloc_usable_size() returned when the engine stored
    the cache — typically larger than the raw decoded PCM because glibc's
    allocator rounds up. We preserve it because the engine's sample_t may
    reference frames past the raw decode length (block-aligned padding),
    and ogg_cache_try_load enforces file_size == 16384 + pcm_bytes exactly.
    """
    with open(cache_path, "rb") as f:
        f.seek(16)  # struct ogg_cache_header.pcm_bytes offset
        return int.from_bytes(f.read(8), "little")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--music-dir", required=True, type=Path,
                    help="directory containing .ogg files (e.g. assets/music/ from a data.sgr extract)")
    ap.add_argument("--cache-dir", required=True, type=Path,
                    help="host .machgate-pcm-cache dir warmed by a prior run")
    ap.add_argument("--freq", required=True, type=int,
                    help="use_freq value (typically 48000)")
    ap.add_argument("--out", required=True, type=Path,
                    help="output manifest path")
    ap.add_argument("--prefix", default="assets/music/",
                    help="filename prefix used at runtime when calling _load_ogg "
                         "(this prefix is what's hashed, not the absolute path)")
    args = ap.parse_args()

    oggs = sorted(args.music_dir.glob("*.ogg"))
    if not oggs:
        print(f"no .ogg files in {args.music_dir}", file=sys.stderr)
        return 1

    lines = []
    lines.append(f"# pcm_cache manifest (freq={args.freq})")
    lines.append(f"# fields: <key_hex_32> <use_freq> <pcm_bytes> <ogg_filename> <sample_t_stable_hex_48>")
    missing = 0
    for ogg in oggs:
        runtime_name = args.prefix + ogg.name
        key = cache_key(runtime_name, args.freq)
        cache_file = args.cache_dir / f"{key}.pcm"
        if not cache_file.exists():
            print(f"WARNING: no cache file for {runtime_name} (expected {cache_file})", file=sys.stderr)
            missing += 1
            continue
        stable = extract_sample_t_stable(cache_file)
        pcm_bytes = extract_pcm_bytes(cache_file)
        lines.append(f"{key} {args.freq} {pcm_bytes} {ogg.name} {stable.hex()}")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text("\n".join(lines) + "\n")
    print(f"wrote {args.out} ({len(oggs) - missing}/{len(oggs)} entries)", file=sys.stderr)
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
