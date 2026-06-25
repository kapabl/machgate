#!/usr/bin/env bash
set -euo pipefail

usage()
{
    cat >&2 <<'EOF'
Usage: scripts/run-macho-docker.sh /path/to/macos-arm64-binary [args...]

Runs an ARM64 macOS Mach-O CLI binary through the published MachGate release
inside an ARM64 Ubuntu Docker container. Works on ARM64 hosts and on Intel
hosts with Docker QEMU/binfmt support.

Optional environment:
  MACHGATE_VERSION    Release version to download, default: latest
                      Use a number like 0.3.0 to pin a release.
  MACHGATE_IMAGE      Docker image to use, default: ubuntu:24.04
EOF
}

if [ "$#" -lt 1 ]; then
    usage
    exit 2
fi

guest_binary="$1"
shift

if [ ! -f "$guest_binary" ]; then
    echo "Mach-O binary not found: $guest_binary" >&2
    exit 1
fi

guest_dir="$(cd "$(dirname "$guest_binary")" && pwd)"
guest_name="$(basename "$guest_binary")"
version="${MACHGATE_VERSION:-latest}"
image="${MACHGATE_IMAGE:-ubuntu:24.04}"

docker run --rm --platform linux/arm64 \
    -v "$guest_dir:/input:ro" \
    "$image" \
    bash -s -- "$version" "$guest_name" "$@" <<'EOF'
set -euo pipefail

requested_version="$1"
guest_name="$2"
shift 2

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends ca-certificates curl tar zlib1g

if [ "$requested_version" = "latest" ]; then
    latest_tag="$(curl -fsSL https://api.github.com/repos/kapabl/machgate/releases/latest |
        sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"v\([^"]*\)".*/\1/p' |
        head -1)"
    if [ -z "$latest_tag" ]; then
        echo "Could not resolve latest MachGate release" >&2
        exit 1
    fi
    release_url="https://github.com/kapabl/machgate/releases/download/v${latest_tag}/machgate-${latest_tag}-linux-arm64.tar.gz"
else
    version="${requested_version#v}"
    release_url="https://github.com/kapabl/machgate/releases/download/v${version}/machgate-${version}-linux-arm64.tar.gz"
fi

curl -L -o /tmp/machgate.tar.gz \
    "$release_url"
tar -xzf /tmp/machgate.tar.gz -C /opt

cat > /tmp/machismo.conf <<'CONFIG_EOF'
[general]
dylib_map = /tmp/dylib_map.conf
CONFIG_EOF

cat > /tmp/dylib_map.conf <<'MAP_EOF'
libSystem.B = /opt/machgate/lib/libsystem_shim.so
CoreFoundation = /opt/machgate/lib/libsystem_shim.so
CoreServices = /opt/machgate/lib/libsystem_shim.so
Security = /opt/machgate/lib/libsystem_shim.so
IOKit = /opt/machgate/lib/libsystem_shim.so
libresolv = /opt/machgate/lib/libsystem_shim.so
libicucore = /opt/machgate/lib/libsystem_shim.so
libz.1 = libz.so
libiconv = libc.so.6
libobjc = STUB
Foundation = SKIP
SystemConfiguration = SKIP
AppKit = SKIP
MAP_EOF

export LD_LIBRARY_PATH=/opt/machgate/lib
export MACHISMO_CONFIG=/tmp/machismo.conf
exec /opt/machgate/bin/machgate "/input/${guest_name}" "$@"
EOF
