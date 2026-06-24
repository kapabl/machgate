#!/usr/bin/env python3
import argparse
import shutil
import subprocess
import sys
from pathlib import Path


SHIM_DYLIBS = {
    "libSystem",
    "libresolv",
    "CoreFoundation",
    "Security",
}

POSIX_RISK_SYMBOLS = {
    "accept",
    "access",
    "bind",
    "chdir",
    "chmod",
    "chown",
    "chroot",
    "clock_gettime",
    "close",
    "closedir",
    "connect",
    "dup",
    "dup2",
    "execve",
    "faccessat",
    "fchdir",
    "fchmod",
    "fchown",
    "fcntl",
    "fdopendir",
    "flock",
    "fork",
    "fstat",
    "fstatat",
    "fstatfs",
    "fsync",
    "ftruncate",
    "futimes",
    "getaddrinfo",
    "getcwd",
    "getgrgid_r",
    "getnameinfo",
    "getpeername",
    "getpwnam_r",
    "getpwuid_r",
    "getrlimit",
    "getsockname",
    "getsockopt",
    "gettimeofday",
    "getxattr",
    "ioctl",
    "kevent",
    "kill",
    "kqueue",
    "lchown",
    "link",
    "listen",
    "listxattr",
    "lseek",
    "lstat",
    "madvise",
    "mkdir",
    "mknod",
    "mlock",
    "mmap",
    "mprotect",
    "munmap",
    "open",
    "openat",
    "pathconf",
    "pipe",
    "pread",
    "ptrace",
    "pwrite",
    "raise",
    "read",
    "readdir_r",
    "readlink",
    "readlinkat",
    "recvfrom",
    "recvmsg",
    "rename",
    "rmdir",
    "sendfile",
    "sendmsg",
    "sendto",
    "setgid",
    "setgroups",
    "setpgid",
    "setrlimit",
    "setsid",
    "setsockopt",
    "setuid",
    "setxattr",
    "shutdown",
    "sigaction",
    "sigaltstack",
    "socket",
    "stat",
    "statfs",
    "symlink",
    "sysconf",
    "unlink",
    "unlinkat",
    "usleep",
    "utimensat",
    "utimes",
    "wait4",
    "write",
    "writev",
}


EXPLICIT_ERRNO_REVIEW_SYMBOLS = {
    "close",
    "dup",
    "dup2",
    "dup3",
    "fcntl",
    "fstat",
    "fstatat",
    "getfsstat",
    "ioctl",
    "kqueue",
    "lstat",
    "mmap",
    "mprotect",
    "munmap",
    "open",
    "openat",
    "pipe",
    "pipe2",
    "readlink",
    "readlinkat",
    "stat",
    "wait4",
}


def find_tool(name, fallbacks):
    candidates = [name] + fallbacks
    for candidate in candidates:
        path = shutil.which(candidate)
        if path:
            return candidate
    raise SystemExit(f"required tool not found: {name} ({', '.join(fallbacks)})")


def run(command, output_path, commands):
    commands.append(" ".join(command))
    result = subprocess.run(command, check=True, text=True, capture_output=True)
    output_path.write_text(result.stdout)


def macho_lookup_symbol(symbol):
    if symbol.startswith("_"):
        return symbol[1:]
    return symbol


def parse_bind_table(path):
    imports = []
    for line in path.read_text().splitlines():
        if not line.startswith("__"):
            continue
        parts = line.split()
        if len(parts) < 7:
            continue
        imports.append({
            "segment": parts[0],
            "section": parts[1],
            "address": parts[2],
            "dylib": parts[5],
            "symbol": parts[6],
            "lookup": macho_lookup_symbol(parts[6]),
        })
    return imports


def parse_shim_exports(path):
    exports = set()
    for line in path.read_text().splitlines():
        parts = line.split()
        if len(parts) < 3:
            continue
        symbol = parts[-1].split("@", 1)[0]
        if symbol:
            exports.add(symbol)
    return exports


def risk_label(dylib, lookup, explicit):
    if lookup not in POSIX_RISK_SYMBOLS:
        return "low"
    if not explicit and dylib in SHIM_DYLIBS:
        return "high: host dependency/fallback POSIX ABI or errno"
    if lookup in EXPLICIT_ERRNO_REVIEW_SYMBOLS:
        return "medium: explicit shim errno audit"
    return "medium: explicit shim POSIX semantics"


def write_ownership_table(path, imports, exports):
    with path.open("w") as table:
        table.write("dylib\tsymbol\tlookup\texplicit_shim_export\townership\trisk\n")
        for item in imports:
            explicit = item["lookup"] in exports
            if item["dylib"] in SHIM_DYLIBS and explicit:
                ownership = "libsystem_shim.so explicit export"
            elif item["dylib"] in SHIM_DYLIBS:
                ownership = "mapped shim handle dependency/fallback"
            else:
                ownership = "non-shim mapped dylib"
            table.write(
                f"{item['dylib']}\t{item['symbol']}\t{item['lookup']}\t"
                f"{int(explicit)}\t{ownership}\t"
                f"{risk_label(item['dylib'], item['lookup'], explicit)}\n"
            )


def main():
    parser = argparse.ArgumentParser(
        description="Generate Mach-O import ownership evidence for a mapped libSystem shim."
    )
    parser.add_argument("macho", type=Path)
    parser.add_argument("shim", type=Path)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--llvm-nm", default=None)
    parser.add_argument("--llvm-objdump", default=None)
    args = parser.parse_args()

    llvm_nm = args.llvm_nm or find_tool("llvm-nm-18", ["llvm-nm-21", "llvm-nm"])
    llvm_objdump = args.llvm_objdump or find_tool(
        "llvm-objdump-18", ["llvm-objdump-21", "llvm-objdump"]
    )

    args.out_dir.mkdir(parents=True, exist_ok=True)
    commands = []

    nm_output = args.out_dir / "macho.llvm-nm-m.txt"
    bind_output = args.out_dir / "macho.objdump-bind.txt"
    shim_exports_output = args.out_dir / "shim.exports.txt"

    run([llvm_nm, "-m", str(args.macho)], nm_output, commands)
    run([llvm_objdump, "--macho", "--bind", str(args.macho)], bind_output, commands)
    run(["nm", "-D", "--defined-only", str(args.shim)], shim_exports_output, commands)

    imports = parse_bind_table(bind_output)
    exports = parse_shim_exports(shim_exports_output)
    table_output = args.out_dir / "import-ownership.tsv"
    write_ownership_table(table_output, imports, exports)

    explicit_count = sum(1 for item in imports if item["lookup"] in exports)
    high_count = sum(
        1 for item in imports
        if risk_label(item["dylib"], item["lookup"], item["lookup"] in exports).startswith("high:")
    )

    (args.out_dir / "commands.txt").write_text("\n".join(commands) + "\n")
    (args.out_dir / "summary.txt").write_text(
        f"imports={len(imports)}\n"
        f"explicit_shim_exports={explicit_count}\n"
        f"not_explicit={len(imports) - explicit_count}\n"
        f"high_risk_dependency_or_fallback={high_count}\n"
    )

    print(f"wrote {table_output}")
    print(f"imports={len(imports)} explicit={explicit_count} high_risk={high_count}")


if __name__ == "__main__":
    sys.exit(main())
