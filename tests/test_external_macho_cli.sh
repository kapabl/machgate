#!/bin/bash
# Download and run external ARM64 macOS CLI Mach-O binaries through MachGate.
set -u

cd "$(dirname "$0")/.."

if [ "${MACHGATE_RUN_EXTERNAL:-0}" != "1" ]; then
    echo "SKIP: set MACHGATE_RUN_EXTERNAL=1 to run external Mach-O CLI tests"
    exit 0
fi

MACHGATE_ROOT="${MACHGATE_ROOT:-$(pwd)}"
BUILD_DIR="${BUILD_DIR:-$MACHGATE_ROOT/build}"
MANIFEST="${MACHGATE_EXTERNAL_MANIFEST:-$MACHGATE_ROOT/tests/external/extended_arm64_macho_cli_manifest.txt}"
CACHE_DIR="${MACHGATE_EXTERNAL_CACHE:-$MACHGATE_ROOT/tests/external/cache}"
WORK_DIR="${MACHGATE_EXTERNAL_WORK:-$MACHGATE_ROOT/tests/external/work}"
LOG_DIR="${MACHGATE_EXTERNAL_LOGS:-$MACHGATE_ROOT/tests/external/logs}"
TIMEOUT="${MACHGATE_EXTERNAL_TIMEOUT:-120}"
CONFIG_DIR="$WORK_DIR/config"
LIBCXX_DYLIB="${MACHGATE_EXTERNAL_LIBCXX:-}"
REQUIRE_LIBCXX="${MACHGATE_EXTERNAL_REQUIRE_LIBCXX:-0}"
MACHGATE_ARGS=()

if [ "${MACHGATE_EXTERNAL_VERBOSE:-0}" = "1" ]; then
    MACHGATE_ARGS=(-v)
fi

if [ -z "$LIBCXX_DYLIB" ] && [ "${MACHGATE_EXTERNAL_MAP_LIBCXX:-0}" = "1" ]; then
    LIBCXX_DYLIB="$MACHGATE_ROOT/build-libcxx/lib/libc++.so.1"
fi

mkdir -p "$CACHE_DIR" "$WORK_DIR" "$LOG_DIR" "$CONFIG_DIR"

[ -x "$BUILD_DIR/machgate" ] || { echo "machgate not found at $BUILD_DIR/machgate" >&2; exit 1; }
[ -f "$MANIFEST" ] || { echo "manifest not found: $MANIFEST" >&2; exit 1; }

if [ "$REQUIRE_LIBCXX" = "1" ] && [ -z "$LIBCXX_DYLIB" ]; then
    echo "MACHGATE_EXTERNAL_LIBCXX must name an ARM64 libc++.so.1 provider" >&2
    exit 1
fi

if [ -n "$LIBCXX_DYLIB" ]; then
    [ -f "$LIBCXX_DYLIB" ] || { echo "libc++ mapping target not found: $LIBCXX_DYLIB" >&2; exit 1; }
    LIBCXX_DIR="$(cd "$(dirname "$LIBCXX_DYLIB")" && pwd)"
    LIBCXX_DYLIB="$LIBCXX_DIR/$(basename "$LIBCXX_DYLIB")"
    LIBCXX_ABI_DYLIB="$LIBCXX_DIR/libc++abi.so.1"
    [ -f "$LIBCXX_ABI_DYLIB" ] || { echo "libc++abi provider not found beside libc++: $LIBCXX_ABI_DYLIB" >&2; exit 1; }
    command -v readelf >/dev/null 2>&1 || { echo "readelf is required to validate the libc++ provider" >&2; exit 1; }
    readelf -h "$LIBCXX_DYLIB" | grep -Eq 'Machine:[[:space:]]+AArch64' || {
        echo "libc++ provider is not ARM64: $LIBCXX_DYLIB" >&2
        exit 1
    }
    readelf -h "$LIBCXX_ABI_DYLIB" | grep -Eq 'Machine:[[:space:]]+AArch64' || {
        echo "libc++abi provider is not ARM64: $LIBCXX_ABI_DYLIB" >&2
        exit 1
    }
    export LD_LIBRARY_PATH="$BUILD_DIR:$LIBCXX_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
else
    export LD_LIBRARY_PATH="$BUILD_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

cat > "$CONFIG_DIR/dylib_map.conf" <<EOF
libSystem.B = $BUILD_DIR/libsystem_shim.so
libcurl.4 = libcurl.so.4
libz.1 = libz.so
libobjc = STUB
libobjc.A = SKIP
CoreFoundation = PARTIAL:$BUILD_DIR/libsystem_shim.so
$(if [ -n "$LIBCXX_DYLIB" ]; then echo "libc++.1 = $LIBCXX_DYLIB"; fi)
libicucore = $BUILD_DIR/libsystem_shim.so
libicucore.A = SKIP
libiconv = libc.so.6
libiconv.2 = libc.so.6
libresolv = $BUILD_DIR/libsystem_shim.so
libresolv.9 = $BUILD_DIR/libsystem_shim.so
Security = $BUILD_DIR/libsystem_shim.so
Foundation = SKIP
IOKit = PARTIAL:$BUILD_DIR/libsystem_shim.so
SystemConfiguration = SKIP
AppKit = SKIP
CoreServices = PARTIAL:$BUILD_DIR/libsystem_shim.so
OpenDirectory = SKIP
EOF

export MACHGATE_DYLIB_MAP="$CONFIG_DIR/dylib_map.conf"

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

    "${timeout_cmd[@]}" "$BUILD_DIR/machgate" "${MACHGATE_ARGS[@]}" "$binary" $args >"$out_log" 2>"$err_log"
    local status=$?
    if [ "$status" -eq 0 ]; then
        echo "PASS: external $name"
        return 0
    fi

    echo "FAIL: external $name status=$status"
    echo "  stdout: $out_log"
    echo "  stderr: $err_log"

	if command -v strace >/dev/null 2>&1; then
		"${timeout_cmd[@]}" strace -f -o "$strace_log" "$BUILD_DIR/machgate" "${MACHGATE_ARGS[@]}" "$binary" $args >"$out_log.strace-run" 2>"$err_log.strace-run" || true
		echo "  strace: $strace_log"
	fi

	QEMU_STRACE=1 \
		"${timeout_cmd[@]}" "$BUILD_DIR/machgate" "${MACHGATE_ARGS[@]}" "$binary" $args >"$out_log.qemu-strace-run" 2>"$qemu_strace_log" || true
	echo "  qemu-strace: $qemu_strace_log"

	return "$status"
}

pass=0
fail=0
crash=0
non_crash_fail=0
harness_fail=0
total=0
RESULTS_FILE="$LOG_DIR/results.tsv"
printf 'probe\tstatus\tclassification\tstdout\tstderr\n' > "$RESULTS_FILE"

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
        harness_fail=$((harness_fail + 1))
        printf '%s\t%s\t%s\t%s\t%s\n' "$name" download-failed harness "$LOG_DIR/$name.out" "$LOG_DIR/$name.err" >> "$RESULTS_FILE"
        continue
    fi

    actual_sha="$(sha256sum "$asset" | awk '{print $1}')"
    if [ "$expected_sha" = "TO_FILL" ]; then
        echo "FAIL: external $name checksum is not pinned; actual sha256=$actual_sha"
        fail=$((fail + 1))
        harness_fail=$((harness_fail + 1))
        printf '%s\t%s\t%s\t%s\t%s\n' "$name" checksum-unpinned harness "$LOG_DIR/$name.out" "$LOG_DIR/$name.err" >> "$RESULTS_FILE"
        continue
    fi

    if [ "$actual_sha" != "$expected_sha" ]; then
        echo "FAIL: external $name checksum mismatch"
        echo "  expected: $expected_sha"
        echo "  actual:   $actual_sha"
        fail=$((fail + 1))
        harness_fail=$((harness_fail + 1))
        printf '%s\t%s\t%s\t%s\t%s\n' "$name" checksum-mismatch harness "$LOG_DIR/$name.out" "$LOG_DIR/$name.err" >> "$RESULTS_FILE"
        continue
    fi

    if ! extract_binary "$asset" "$binary_path" "$binary" "$extract_dir"; then
        echo "FAIL: external $name extraction failed"
        fail=$((fail + 1))
        harness_fail=$((harness_fail + 1))
        printf '%s\t%s\t%s\t%s\t%s\n' "$name" extraction-failed harness "$LOG_DIR/$name.out" "$LOG_DIR/$name.err" >> "$RESULTS_FILE"
        continue
    fi

    if run_with_diagnostics "$name" "$binary" "$args"; then
        pass=$((pass + 1))
        printf '%s\t0\tpass\t%s\t%s\n' "$name" "$LOG_DIR/$name.out" "$LOG_DIR/$name.err" >> "$RESULTS_FILE"
    else
        status=$?
        fail=$((fail + 1))
        if [ "$status" -ge 128 ] && [ "$status" -le 192 ]; then
            classification=crash
            crash=$((crash + 1))
        else
            classification=non-crash-failure
            non_crash_fail=$((non_crash_fail + 1))
        fi
        printf '%s\t%s\t%s\t%s\t%s\n' "$name" "$status" "$classification" "$LOG_DIR/$name.out" "$LOG_DIR/$name.err" >> "$RESULTS_FILE"
    fi
done < "$MANIFEST"

echo "---"
echo "$pass/$total external Mach-O CLI probes passed, $fail failed"
echo "$crash crashed, $non_crash_fail failed without a crash, $harness_fail harness failures"
echo "results: $RESULTS_FILE"
[ "$fail" -eq 0 ]
