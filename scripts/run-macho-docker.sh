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
  MACHGATE_LOCAL_DIR  Local Linux ARM64 MachGate directory to mount instead
                      of downloading a release. Supports either release layout
                      with bin/machgate and lib/libsystem_shim.so, or build
                      layout with machgate and libsystem_shim.so.
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
local_dir="${MACHGATE_LOCAL_DIR:-}"

docker_args=(
    docker run --rm --platform linux/arm64
    -v "$guest_dir:/input:ro"
)

if [ -n "$local_dir" ]; then
    if [ ! -d "$local_dir" ]; then
        echo "MACHGATE_LOCAL_DIR is not a directory: $local_dir" >&2
        exit 1
    fi
    local_dir="$(cd "$local_dir" && pwd)"
    docker_args+=(-v "$local_dir:/opt/machgate-local:ro")
fi

if ! docker run --rm --platform linux/arm64 --entrypoint /bin/sh "$image" -c 'test "$(uname -m)" = aarch64' >/dev/null 2>&1; then
    cat >&2 <<EOF
Docker cannot execute linux/arm64 containers on this host.

On an Intel Linux host, install/register ARM64 binfmt support once:

  docker run --privileged --rm tonistiigi/binfmt --install arm64

Then rerun:

  $0 "$guest_binary" $*
EOF
    exit 1
fi

"${docker_args[@]}" \
    "$image" \
    bash -s -- "$version" "$guest_name" "$local_dir" "$@" <<'EOF'
set -euo pipefail

requested_version="$1"
guest_name="$2"
local_dir="$3"
shift 3

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends ca-certificates curl tar zlib1g

if [ -n "$local_dir" ]; then
    if [ -x /opt/machgate-local/bin/machgate ]; then
        machgate_root=/opt/machgate-local
        machgate_bin=/opt/machgate-local/bin/machgate
        shim_path=/opt/machgate-local/lib/libsystem_shim.so
    elif [ -x /opt/machgate-local/machgate ]; then
        machgate_root=/opt/machgate-local
        machgate_bin=/opt/machgate-local/machgate
        shim_path=/opt/machgate-local/libsystem_shim.so
    else
        echo "MACHGATE_LOCAL_DIR must contain bin/machgate or machgate" >&2
        exit 1
    fi
    if [ ! -f "$shim_path" ]; then
        echo "MACHGATE_LOCAL_DIR is missing libsystem_shim.so next to the selected layout" >&2
        exit 1
    fi
else
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
    machgate_root=/opt/machgate
    machgate_bin=/opt/machgate/bin/machgate
    shim_path=/opt/machgate/lib/libsystem_shim.so
fi

cat > /tmp/machismo.conf <<'CONFIG_EOF'
[general]
dylib_map = /tmp/dylib_map.conf
CONFIG_EOF

cat > /tmp/dylib_map.conf <<'MAP_EOF'
libSystem.B = __SHIM_PATH__
CoreFoundation = __SHIM_PATH__
CoreServices = __SHIM_PATH__
Security = __SHIM_PATH__
IOKit = __SHIM_PATH__
libresolv = __SHIM_PATH__
libicucore = __SHIM_PATH__
libz.1 = libz.so
libiconv = libc.so.6
libobjc = STUB
Foundation = SKIP
SystemConfiguration = SKIP
AppKit = SKIP
MAP_EOF
sed -i "s#__SHIM_PATH__#${shim_path}#g" /tmp/dylib_map.conf

export LD_LIBRARY_PATH="${machgate_root}/lib:${machgate_root}"
export MACHISMO_CONFIG=/tmp/machismo.conf
exec "$machgate_bin" "/input/${guest_name}" "$@"
EOF
