#!/usr/bin/env python3
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

"""Report coverage for changed lines in a git diff using llvm-cov export JSON."""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

COMMENT_MARKER = "<!-- diff-coverage -->"
SNIPPET_CONTEXT_LINES = 3


def run(cmd: list[str]) -> str:
    return subprocess.check_output(cmd, text=True)


def repo_root() -> Path:
    return Path(run(["git", "rev-parse", "--show-toplevel"]).strip())


def tracked_files() -> set[str]:
    return set(run(["git", "ls-files"]).splitlines())


def remap_path(path_str: str, path_prefix_from: Path | None, path_prefix_to: Path | None) -> str:
    if path_prefix_from is not None and path_prefix_to is not None:
        path = Path(path_str)
        return os.fspath(path_prefix_to / path.relative_to(path_prefix_from))
    return path_str


def normalize_path(path_str: str, root: Path, exact: set[str]) -> str | None:
    path = Path(path_str)
    candidate = os.fspath(path.relative_to(root))
    if candidate in exact:
        return candidate
    return None


def changed_lines(base: str) -> dict[str, set[int]]:
    diff = run(["git", "diff", "--unified=0", f"{base}...HEAD", "--"])
    changed: dict[str, set[int]] = {}
    current_file: str | None = None
    hunk_re = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")

    for line in diff.splitlines():
        if line.startswith("+++ b/"):
            current_file = line[6:]
            if current_file == "/dev/null":
                current_file = None
            continue
        if current_file is None:
            continue
        match = hunk_re.match(line)
        if not match:
            continue
        start = int(match.group(1))
        count = int(match.group(2) or "1")
        if count > 0:
            changed.setdefault(current_file, set()).update(range(start, start + count))

    return changed


def file_lines(path: Path) -> list[str]:
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def file_line_count(path: Path) -> int:
    return len(file_lines(path))


def grouped_lines(lines: list[int]) -> list[tuple[int, int]]:
    if not lines:
        return []

    groups: list[tuple[int, int]] = []
    start = end = lines[0]

    for lineno in lines[1:]:
        if lineno == end + 1:
            end = lineno
            continue
        groups.append((start, end))
        start = end = lineno

    groups.append((start, end))
    return groups


def format_line_range(start: int, end: int) -> str:
    if start == end:
        return str(start)
    return f"{start}-{end}"


def snippet_markdown(filename: str, uncovered: list[int], root: Path) -> str:
    source_lines = file_lines(root / filename)
    sections: list[str] = []
    uncovered_set = set(uncovered)

    for start, end in grouped_lines(uncovered):
        snippet_start = max(1, start - SNIPPET_CONTEXT_LINES)
        snippet_end = min(len(source_lines), end + SNIPPET_CONTEXT_LINES)
        snippet_lines = []

        for lineno in range(snippet_start, snippet_end + 1):
            marker = "-" if lineno in uncovered_set else " "
            snippet_lines.append(f"{marker} {lineno:>4} | {source_lines[lineno - 1]}")

        if not snippet_lines:
            continue
        sections.extend(
            [
                "```diff",
                *snippet_lines,
                "```",
                ""
            ]
        )

    return "\n".join(sections)


# https://github.com/llvm/llvm-project/blob/e3574d46e0de8d2c96beb0d092812a3cab153db7/llvm/tools/llvm-cov/CoverageExporterJson.cpp
def coverage_by_file(
    json_path: Path,
    root: Path,
    exact: set[str],
    path_prefix_from: Path | None,
    path_prefix_to: Path | None,
) -> tuple[dict[str, dict[int, bool]], int]:
    exported = json.loads(json_path.read_text())
    result: dict[str, dict[int, bool]] = {}
    skipped_paths: set[str] = set()

    for entry in exported.get("data", []):
        for file_entry in entry.get("files", []):
            remapped_filename = remap_path(file_entry["filename"], path_prefix_from, path_prefix_to)
            filename = normalize_path(remapped_filename, root, exact)
            if filename is None:
                if remapped_filename not in skipped_paths:
                    skipped_paths.add(remapped_filename)
                    print(
                        f"warning: skipping coverage for unmatched file: {remapped_filename}",
                        file=sys.stderr,
                    )
                continue

            source_path = root / filename
            last_line = file_line_count(source_path) if source_path.exists() else 0
            if last_line == 0:
                continue

            line_coverage: dict[int, bool] = {}
            segments = file_entry.get("segments", [])

            for index, segment in enumerate(segments):
                line, _col, count, has_count, _is_region_entry, is_gap_region = segment
                if not has_count or is_gap_region:
                    continue
                start = int(line)
                if index + 1 < len(segments):
                    stop = max(start, int(segments[index + 1][0]) - 1)
                else:
                    stop = last_line
                covered = int(count) > 0
                for lineno in range(start, stop + 1):
                    line_coverage[lineno] = line_coverage.get(lineno, False) or covered

            result[filename] = line_coverage

    return result, len(skipped_paths)


def summarize(changed: dict[str, set[int]], covered: dict[str, dict[int, bool]], root: Path) -> tuple[str, list[str]]:
    details: list[str] = []
    total_relevant = 0
    total_covered = 0

    for filename in sorted(changed):
        file_coverage = covered.get(filename, {})
        relevant = sorted(lineno for lineno in changed[filename] if lineno in file_coverage)
        if not relevant:
            continue

        uncovered = [lineno for lineno in relevant if not file_coverage[lineno]]
        covered_count = len(relevant) - len(uncovered)
        total_relevant += len(relevant)
        total_covered += covered_count

        summary = f"- `{filename}`: {covered_count}/{len(relevant)} changed executable lines covered"
        if uncovered:
            summary = "\n".join([summary, "", snippet_markdown(filename, uncovered, root)])
        details.append(summary)

    if total_relevant == 0:
        return "No changed executable lines found in the diff.", details
    percent_covered = (total_covered * 100) // total_relevant
    return f"Changed executable lines covered: {total_covered}/{total_relevant} ({percent_covered}%)", details


def markdown(summary: str, details: list[str], skipped_files: int) -> str:
    body = [COMMENT_MARKER, "## Diff Coverage", "", summary]
    if details:
        body.extend(["", "Legend: `-` uncovered line, unprefixed lines are context.", "", *details])
    if skipped_files:
        body.extend(["", f"Note: {skipped_files} unmatched coverage file(s) were skipped, check the logs for details."])
    body.append("")
    return "\n".join(body)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--coverage-json", required=True, type=Path)
    parser.add_argument("--base", required=True, help="Diff base ref, e.g. origin/master")
    parser.add_argument("--comment-file", type=Path, help="Optional markdown output for a PR comment")
    parser.add_argument("--path-prefix-from", type=Path, help="Optional coverage path prefix to replace")
    parser.add_argument("--path-prefix-to", type=Path, help="Optional replacement prefix for coverage paths")
    args = parser.parse_args()

    print(
        "info: diff_coverage.py options: "
        f"coverage_json={args.coverage_json} "
        f"base={args.base} "
        f"comment_file={args.comment_file} "
        f"path_prefix_from={args.path_prefix_from} "
        f"path_prefix_to={args.path_prefix_to}",
        file=sys.stderr,
    )

    root = repo_root()
    exact = tracked_files()
    covered, skipped_files = coverage_by_file(
        args.coverage_json.resolve(),
        root,
        exact,
        args.path_prefix_from,
        args.path_prefix_to,
    )
    summary, details = summarize(changed_lines(args.base), covered, root)

    print(summary)
    for line in details:
        print(line)

    output = markdown(summary, details, skipped_files) + "\n"

    if os.getenv("GITHUB_STEP_SUMMARY"):
        with open(os.environ["GITHUB_STEP_SUMMARY"], "a", encoding="utf-8") as file:
            file.write(output)

    if args.comment_file:
        args.comment_file.write_text(output, encoding="utf-8")

    return 0


if __name__ == "__main__":
    sys.exit(main())
