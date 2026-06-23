#!/bin/bash
# Download and run external ARM64 macOS CLI Mach-O binaries through MachGate.
set -u

cd "$(dirname "$0")/.."

if [ "${MACHGATE_RUN_EXTERNAL:-0}" != "1" ]; then
    echo "SKIP: set MACHGATE_RUN_EXTERNAL=1 to run external Mach-O CLI tests"
    exit 0
fi

MACHISMO_ROOT="${MACHISMO_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHISMO_ROOT/build}"
MANIFEST="${MACHGATE_EXTERNAL_MANIFEST:-$MACHISMO_ROOT/tests/external/arm64_macho_cli_manifest.txt}"
CACHE_DIR="${MACHGATE_EXTERNAL_CACHE:-$MACHISMO_ROOT/tests/external/cache}"
WORK_DIR="${MACHGATE_EXTERNAL_WORK:-$MACHISMO_ROOT/tests/external/work}"
LOG_DIR="${MACHGATE_EXTERNAL_LOGS:-$MACHISMO_ROOT/tests/external/logs}"
TIMEOUT="${MACHGATE_EXTERNAL_TIMEOUT:-10}"
CONFIG_DIR="$WORK_DIR/config"
LIBCXX_DYLIB="${MACHGATE_EXTERNAL_LIBCXX:-}"

if [ -z "$LIBCXX_DYLIB" ] && [ "${MACHGATE_EXTERNAL_MAP_LIBCXX:-0}" = "1" ]; then
    LIBCXX_DYLIB="$MACHISMO_ROOT/build-libcxx/lib/libc++.so.1"
fi

mkdir -p "$CACHE_DIR" "$WORK_DIR" "$LOG_DIR" "$CONFIG_DIR"

[ -x "$BUILD_DIR/machismo" ] || { echo "machismo not found at $BUILD_DIR/machismo" >&2; exit 1; }
[ -f "$MANIFEST" ] || { echo "manifest not found: $MANIFEST" >&2; exit 1; }

if [ -n "$LIBCXX_DYLIB" ]; then
    [ -f "$LIBCXX_DYLIB" ] || { echo "libc++ mapping target not found: $LIBCXX_DYLIB" >&2; exit 1; }
    LIBCXX_DIR="$(cd "$(dirname "$LIBCXX_DYLIB")" && pwd)"
    LIBCXX_DYLIB="$LIBCXX_DIR/$(basename "$LIBCXX_DYLIB")"
    export LD_LIBRARY_PATH="$BUILD_DIR:$LIBCXX_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
else
    export LD_LIBRARY_PATH="$BUILD_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

cat > "$CONFIG_DIR/dylib_map.conf" <<EOF
libSystem.B = $BUILD_DIR/libsystem_shim.so
libcurl.4 = libcurl.so.4
libz.1 = libz.so
libobjc = STUB
CoreFoundation = $BUILD_DIR/libsystem_shim.so
$(if [ -n "$LIBCXX_DYLIB" ]; then echo "libc++.1 = $LIBCXX_DYLIB"; fi)
libicucore = $BUILD_DIR/libsystem_shim.so
libiconv = libc.so.6
libresolv = $BUILD_DIR/libsystem_shim.so
Security = $BUILD_DIR/libsystem_shim.so
Foundation = SKIP
IOKit = $BUILD_DIR/libsystem_shim.so
SystemConfiguration = SKIP
AppKit = SKIP
CoreServices = $BUILD_DIR/libsystem_shim.so
OpenDirectory = SKIP
EOF

cat > "$CONFIG_DIR/machismo.conf" <<EOF
[general]
dylib_map = $CONFIG_DIR/dylib_map.conf
EOF

download_file()
{
    local url="$1"
    local output="$2"

    if [ -f "$output" ]; then
        return 0
    fi

    if command -v curl >/dev/null 2>&1; then
        curl -L --fail --retry 3 --connect-timeout 20 -o "$output" "$url"
        return
    fi

    wget -O "$output" "$url"
}

extract_binary()
{
    local archive="$1"
    local binary_path="$2"
    local dest="$3"
    local extract_dir="$4"

    rm -rf "$extract_dir"
    mkdir -p "$extract_dir"

    if [ "$binary_path" = "." ]; then
        cp "$archive" "$dest"
        chmod +x "$dest"
        return 0
    fi

    case "$archive" in
        *.tar.gz|*.tgz)
            tar -xzf "$archive" -C "$extract_dir"
            ;;
        *.zip)
            unzip -q "$archive" -d "$extract_dir"
            ;;
        *)
            echo "unsupported archive type: $archive" >&2
            return 1
            ;;
    esac

    if [ "$binary_path" = "cmake-4.3.3-macos-universal/CMake.app/Contents/bin/cmake" ]; then
        ln -sf "$extract_dir/$binary_path" "$dest"
    else
        cp "$extract_dir/$binary_path" "$dest"
    fi
    chmod +x "$dest"
}

run_with_diagnostics()
{
    local name="$1"
    local binary="$2"
    local args="$3"
	local out_log="$LOG_DIR/$name.out"
	local err_log="$LOG_DIR/$name.err"
	local strace_log="$LOG_DIR/$name.strace"
	local qemu_strace_log="$LOG_DIR/$name.qemu-strace"

	rm -f "$out_log" "$err_log" "$strace_log" "$qemu_strace_log"

	if command -v timeout >/dev/null 2>&1; then
		timeout_cmd=(timeout -k 2 "$TIMEOUT")
	else
		timeout_cmd=()
	fi

    MACHISMO_CONFIG="$CONFIG_DIR/machismo.conf" \
        "${timeout_cmd[@]}" "$BUILD_DIR/machismo" "$binary" $args >"$out_log" 2>"$err_log"
    local status=$?
    if [ "$status" -eq 0 ]; then
        echo "PASS: external $name"
        return 0
    fi

    echo "FAIL: external $name status=$status"
    echo "  stdout: $out_log"
    echo "  stderr: $err_log"

	if command -v strace >/dev/null 2>&1; then
		MACHISMO_CONFIG="$CONFIG_DIR/machismo.conf" \
			"${timeout_cmd[@]}" strace -f -o "$strace_log" "$BUILD_DIR/machismo" "$binary" $args >"$out_log.strace-run" 2>"$err_log.strace-run" || true
		echo "  strace: $strace_log"
	fi

	QEMU_STRACE=1 MACHISMO_CONFIG="$CONFIG_DIR/machismo.conf" \
		"${timeout_cmd[@]}" "$BUILD_DIR/machismo" "$binary" $args >"$out_log.qemu-strace-run" 2>"$qemu_strace_log" || true
	echo "  qemu-strace: $qemu_strace_log"

	return 1
}

pass=0
fail=0
total=0

while IFS='|' read -r name url expected_sha binary_path args; do
    case "$name" in
        ""|\#*) continue ;;
    esac

    total=$((total + 1))
    asset="$CACHE_DIR/${url##*/}"
    binary="$WORK_DIR/$name"
    extract_dir="$WORK_DIR/$name.extract"

    if ! download_file "$url" "$asset"; then
        echo "FAIL: external $name download failed"
        fail=$((fail + 1))
        continue
    fi

    actual_sha="$(sha256sum "$asset" | awk '{print $1}')"
    if [ "$expected_sha" = "TO_FILL" ]; then
        echo "FAIL: external $name checksum is not pinned; actual sha256=$actual_sha"
        fail=$((fail + 1))
        continue
    fi

    if [ "$actual_sha" != "$expected_sha" ]; then
        echo "FAIL: external $name checksum mismatch"
        echo "  expected: $expected_sha"
        echo "  actual:   $actual_sha"
        fail=$((fail + 1))
        continue
    fi

    if ! extract_binary "$asset" "$binary_path" "$binary" "$extract_dir"; then
        echo "FAIL: external $name extraction failed"
        fail=$((fail + 1))
        continue
    fi

    if run_with_diagnostics "$name" "$binary" "$args"; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
    fi
done < "$MANIFEST"

echo "---"
echo "$pass/$total external Mach-O CLI probes passed, $fail failed"
[ "$fail" -eq 0 ]
