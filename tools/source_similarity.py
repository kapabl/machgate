#!/usr/bin/env python3

import argparse
import hashlib
import json
import re
import subprocess
from collections import defaultdict
from pathlib import Path


SOURCE_SUFFIXES = {
    ".S",
    ".c",
    ".cc",
    ".cmake",
    ".cpp",
    ".h",
    ".py",
    ".s",
    ".sh",
    ".ver",
    ".yml",
}
SOURCE_NAMES = {"CMakeLists.txt"}
TOKEN_PATTERN = re.compile(
    r"[A-Za-z_][A-Za-z0-9_]*|0[xX][0-9A-Fa-f]+|[0-9]+|"
    r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|'
    r"==|!=|<=|>=|<<|>>|->|&&|\|\||\+\+|--|[^\s]"
)


def tracked_source_paths(root):
    result = subprocess.run(
        [
            "git",
            "-C",
            str(root),
            "ls-files",
            "--cached",
            "--others",
            "--exclude-standard",
            "-z",
        ],
        check=True,
        capture_output=True,
    )
    paths = []
    for raw_path in result.stdout.split(b"\0"):
        if not raw_path:
            continue
        relative_path = raw_path.decode("utf-8", errors="surrogateescape")
        path = Path(relative_path)
        if path.parts and path.parts[0] == "extern":
            continue
        if path.suffix in SOURCE_SUFFIXES or path.name in SOURCE_NAMES:
            paths.append(relative_path)
    return sorted(paths)


def read_sources(root):
    sources = {}
    for relative_path in tracked_source_paths(root):
        path = root / relative_path
        try:
            sources[relative_path] = path.read_text(
                encoding="utf-8", errors="surrogateescape"
            )
        except OSError:
            continue
    return sources


def normalized_lines(text):
    return [line.rstrip() for line in text.splitlines()]


def tokens(text):
    return TOKEN_PATTERN.findall(text)


def exact_files(current_sources, reference_sources):
    reference_hashes = defaultdict(list)
    for path, text in reference_sources.items():
        digest = hashlib.sha256(text.encode("utf-8", errors="surrogateescape")).hexdigest()
        reference_hashes[digest].append(path)

    matches = []
    for path, text in current_sources.items():
        digest = hashlib.sha256(text.encode("utf-8", errors="surrogateescape")).hexdigest()
        for reference_path in reference_hashes.get(digest, []):
            matches.append({"current": path, "reference": reference_path})
    return matches


def line_block_report(current_sources, reference_sources, minimum_lines):
    reference_lines = {
        path: normalized_lines(text) for path, text in reference_sources.items()
    }
    block_index = defaultdict(list)
    for path, lines in reference_lines.items():
        for start in range(len(lines) - minimum_lines + 1):
            block_index[tuple(lines[start : start + minimum_lines])].append((path, start))

    matches = []
    covered = defaultdict(set)
    seen = set()
    current_lines = {
        path: normalized_lines(text) for path, text in current_sources.items()
    }
    for current_path, lines in current_lines.items():
        for current_start in range(len(lines) - minimum_lines + 1):
            key = tuple(lines[current_start : current_start + minimum_lines])
            for reference_path, reference_start in block_index.get(key, []):
                other = reference_lines[reference_path]
                if (
                    current_start > 0
                    and reference_start > 0
                    and lines[current_start - 1] == other[reference_start - 1]
                ):
                    continue
                length = minimum_lines
                while (
                    current_start + length < len(lines)
                    and reference_start + length < len(other)
                    and lines[current_start + length] == other[reference_start + length]
                ):
                    length += 1
                identity = (
                    current_path,
                    current_start,
                    reference_path,
                    reference_start,
                    length,
                )
                if identity in seen:
                    continue
                seen.add(identity)
                matches.append(
                    {
                        "current": current_path,
                        "current_start": current_start + 1,
                        "current_end": current_start + length,
                        "reference": reference_path,
                        "reference_start": reference_start + 1,
                        "reference_end": reference_start + length,
                        "lines": length,
                    }
                )
                covered[current_path].update(
                    range(current_start, current_start + length)
                )

    per_file = []
    total_lines = 0
    total_covered = 0
    total_nonblank = 0
    total_nonblank_covered = 0
    for path, lines in current_lines.items():
        line_count = len(lines)
        covered_count = len(covered[path])
        nonblank = {index for index, line in enumerate(lines) if line.strip()}
        nonblank_covered = len(nonblank & covered[path])
        total_lines += line_count
        total_covered += covered_count
        total_nonblank += len(nonblank)
        total_nonblank_covered += nonblank_covered
        per_file.append(
            {
                "path": path,
                "lines": line_count,
                "covered_lines": covered_count,
                "coverage_percent": percentage(covered_count, line_count),
                "nonblank_lines": len(nonblank),
                "covered_nonblank_lines": nonblank_covered,
                "nonblank_coverage_percent": percentage(
                    nonblank_covered, len(nonblank)
                ),
            }
        )

    matches.sort(key=lambda match: (-match["lines"], match["current"], match["current_start"]))
    per_file.sort(key=lambda item: (-item["coverage_percent"], item["path"]))
    return {
        "minimum_lines": minimum_lines,
        "current_lines": total_lines,
        "covered_lines": total_covered,
        "coverage_percent": percentage(total_covered, total_lines),
        "current_nonblank_lines": total_nonblank,
        "covered_nonblank_lines": total_nonblank_covered,
        "nonblank_coverage_percent": percentage(
            total_nonblank_covered, total_nonblank
        ),
        "per_file": per_file,
        "matches": matches,
    }


def token_window_report(current_sources, reference_sources, window_size):
    reference_windows = set()
    for text in reference_sources.values():
        values = tokens(text)
        for start in range(len(values) - window_size + 1):
            reference_windows.add(tuple(values[start : start + window_size]))

    per_file = []
    total_tokens = 0
    total_covered = 0
    for path, text in current_sources.items():
        values = tokens(text)
        covered = set()
        for start in range(len(values) - window_size + 1):
            if tuple(values[start : start + window_size]) in reference_windows:
                covered.update(range(start, start + window_size))
        total_tokens += len(values)
        total_covered += len(covered)
        per_file.append(
            {
                "path": path,
                "tokens": len(values),
                "covered_tokens": len(covered),
                "coverage_percent": percentage(len(covered), len(values)),
            }
        )

    per_file.sort(key=lambda item: (-item["coverage_percent"], item["path"]))
    return {
        "window_size": window_size,
        "current_tokens": total_tokens,
        "covered_tokens": total_covered,
        "coverage_percent": percentage(total_covered, total_tokens),
        "per_file": per_file,
    }


def percentage(part, whole):
    if not whole:
        return 0.0
    return round(part * 100.0 / whole, 3)


def main():
    parser = argparse.ArgumentParser(
        description="Measure exact source overlap between two Git checkouts"
    )
    parser.add_argument("current", type=Path)
    parser.add_argument("reference", type=Path)
    parser.add_argument("--minimum-lines", type=int, default=8)
    parser.add_argument("--token-window", type=int, default=25)
    parser.add_argument("--output", type=Path)
    arguments = parser.parse_args()

    current_root = arguments.current.resolve()
    reference_root = arguments.reference.resolve()
    current_sources = read_sources(current_root)
    reference_sources = read_sources(reference_root)
    report = {
        "current_revision": subprocess.run(
            ["git", "-C", str(current_root), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip(),
        "reference_revision": subprocess.run(
            ["git", "-C", str(reference_root), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip(),
        "current_source_files": len(current_sources),
        "reference_source_files": len(reference_sources),
        "exact_files": exact_files(current_sources, reference_sources),
        "line_blocks": line_block_report(
            current_sources, reference_sources, arguments.minimum_lines
        ),
        "token_windows": token_window_report(
            current_sources, reference_sources, arguments.token_window
        ),
    }

    output = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if arguments.output:
        arguments.output.write_text(output, encoding="utf-8")
    else:
        print(output, end="")


if __name__ == "__main__":
    main()
