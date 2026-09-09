#!/usr/bin/env python3
#
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check library dependencies against doc/design/libraries.md

import argparse
import os
import re
import shutil
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent.parent

HAVE_CXXFILT = shutil.which("c++filt") is not None

# nm -o -g output: "lib.a:obj.o:<addr> <type> <sym>" (Linux, addr glued)
#                  "lib.a:obj.o: <addr> <type> <sym>" (macOS, addr separate token)
NM_LINE_RE = re.compile(r'^[^:]*:([^:]*):.*\s([UTBDWRV])\s+(\S+)')

# Known violations to suppress: (consumer_obj, provider_obj, symbol) tuples.
# Symbols are matched as prefixes to allow for ABI-tagged variants.
SUPPRESS = {
    ("muhash.cpp.o", "check.cpp.o", "_Z14assertion_failRKSt15source_locationSt17basic_string_viewIcSt11char_traitsIcEE"),
      # muhash.cpp uses Assume from util/check which calls a function in Debug builds
}

def parse_library_deps(md_path):
    """Parse mermaid graph in libraries.md to extract dependency edges."""
    text = md_path.read_text()
    m = re.search(r"```mermaid\s*(.*?)```", text, re.DOTALL)
    if not m:
        sys.exit(f"Error: could not find mermaid graph in {md_path}")

    deps = defaultdict(set)
    for line in m.group(1).splitlines():
        edge = re.match(r"\s*(libbitcoin\w+)\s*-->\s*(libbitcoin\w+)\s*;", line)
        if edge:
            consumer = edge.group(1).removeprefix("lib")
            provider = edge.group(2).removeprefix("lib")
            deps[consumer].add(provider)
            deps.setdefault(provider, set())
    return dict(deps)


def transitive_deps(lib, declared_deps):
    """Compute the transitively reachable deps from lib."""
    trans = set()
    stack = list(declared_deps.get(lib, set()))
    while stack:
        d = stack.pop()
        if d not in trans:
            trans.add(d)
            stack.extend(declared_deps.get(d, set()))
    return trans


def demangle(symbols):
    """Demangle C++ symbols via c++filt if available."""
    if not HAVE_CXXFILT or not symbols:
        return {s: s for s in symbols}
    proc = subprocess.run(
        ["c++filt"], input="\n".join(symbols),
        capture_output=True, text=True
    )
    return dict(zip(symbols, proc.stdout.splitlines()))


def get_symbols_per_obj(libpath):
    """Extract defined and undefined global symbols per .o file from an archive."""
    result = subprocess.run(
        ["nm", "-o", "-g", str(libpath)],
        capture_output=True, text=True, check=True
    )
    defined = defaultdict(set)   # obj -> {symbols}
    undefined = defaultdict(set) # obj -> {symbols}
    for line in result.stdout.splitlines():
        m = NM_LINE_RE.match(line)
        if not m:
            continue
        obj, sym_type, symbol = m.groups()
        if sym_type == "U":
            undefined[obj].add(symbol)
        else:
            defined[obj].add(symbol)
    return defined, undefined


def get_lib_symbols(libpath):
    """Get aggregate defined and undefined symbols for a library."""
    defined_per_obj, undefined_per_obj = get_symbols_per_obj(libpath)
    all_defined = set()
    all_undefined = set()
    for syms in defined_per_obj.values():
        all_defined.update(syms)
    for syms in undefined_per_obj.values():
        all_undefined.update(syms)
    return all_defined, all_undefined, defined_per_obj, undefined_per_obj


def is_suppressed(src_obj, dst_obj, symbol, used_suppressions):
    """Check if a violation is in the SUPPRESS list."""
    for sup in SUPPRESS:
        s_src, s_dst, s_sym = sup
        if src_obj == s_src and dst_obj == s_dst and symbol.startswith(s_sym):
            used_suppressions.add(sup)
            return True
    return False


def main():
    parser = argparse.ArgumentParser(description="Check library dependencies against doc/design/libraries.md")
    parser.add_argument("build_dir", nargs="?", default=str(REPO_ROOT / "build"),
                        help="Path to build directory (default: %(default)s)")
    parser.add_argument("--no-build", action="store_true",
                        help="Skip building libraries before checking")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Show full dependency matrix")
    parser.add_argument("--show-suppressions", action="store_true",
                        help="Show suppress directives for each error")
    args = parser.parse_args()

    build_dir = Path(args.build_dir)
    lib_dir = build_dir / "lib"

    md_path = REPO_ROOT / "doc" / "design" / "libraries.md"
    if not md_path.exists():
        sys.exit(f"Error: {md_path} not found")

    declared_deps = parse_library_deps(md_path)

    # Derive expected library filenames from libraries.md
    lib_paths = {}
    for name in declared_deps:
        lib_paths[name] = lib_dir / f"lib{name}.a"

    if not args.no_build:
        if not build_dir.exists():
            sys.exit(f"Error: build directory {build_dir} not found")
        not_buildable = []
        for target in declared_deps:
            result = subprocess.run(
                ["cmake", "--build", str(build_dir), "-j", str(os.cpu_count()), "-t", target],
                capture_output=True, text=True
            )
            if result.returncode != 0:
                not_buildable.append(target)
        if not_buildable:
            print(f"Warning: targets not buildable: {', '.join(not_buildable)}", file=sys.stderr)

    # Load symbols for all libraries
    libs = {}
    missing = []
    for name, path in sorted(lib_paths.items()):
        if not path.exists():
            missing.append(name)
            continue
        all_def, all_undef, def_per_obj, undef_per_obj = get_lib_symbols(path)
        libs[name] = {
            "defined": all_def,
            "undefined": all_undef,
            "def_per_obj": def_per_obj,
            "undef_per_obj": undef_per_obj,
        }

    if missing:
        # If we tried to build, any missing lib that was buildable is an error
        if not args.no_build:
            build_failures = [m for m in missing if m not in not_buildable]
            if build_failures:
                sys.exit(f"Error: built successfully but library not found: {', '.join(build_failures)}")
            # The rest are just not-buildable, already warned about
        else:
            print(f"Warning: libraries not found: {', '.join(missing)}", file=sys.stderr)

    # Compute actual dependencies (with self-symbol subtraction)
    actual_deps = defaultdict(dict)
    for consumer, cdata in libs.items():
        for provider, pdata in libs.items():
            if consumer == provider:
                continue
            shared = (cdata["undefined"] - cdata["defined"]) & pdata["defined"]
            if shared:
                actual_deps[consumer][provider] = shared

    # Check for unexpected dependencies
    errors = []
    used_suppressions = set()
    all_error_syms = set()

    for consumer in sorted(libs):
        allowed = transitive_deps(consumer, declared_deps)

        # Collect all symbols available from allowed dependencies
        allowed_syms = set()
        for dep in allowed:
            if dep in libs:
                allowed_syms.update(libs[dep]["defined"])

        for provider, syms in sorted(actual_deps.get(consumer, {}).items()):
            if provider in allowed:
                continue

            # Only flag symbols not already available from an allowed dep
            syms = syms - allowed_syms
            if not syms:
                continue

            cdata = libs[consumer]
            pdata = libs[provider]

            for symbol in sorted(syms):
                # Find which .o files are involved
                src_objs = [obj for obj, s in cdata["undef_per_obj"].items() if symbol in s]
                dst_objs = [obj for obj, s in pdata["def_per_obj"].items() if symbol in s]

                for src_obj in src_objs:
                    for dst_obj in dst_objs:
                        if not is_suppressed(src_obj, dst_obj, symbol, used_suppressions):
                            errors.append((consumer, provider, src_obj, dst_obj, symbol))
                            all_error_syms.add(symbol)

    # Demangle error symbols
    demangled = demangle(all_error_syms)

    # Print errors grouped by library pair
    if errors:
        grouped = defaultdict(list)
        for consumer, provider, src_obj, dst_obj, symbol in errors:
            grouped[(consumer, provider)].append((src_obj, dst_obj, symbol))
        for (consumer, provider), items in sorted(grouped.items()):
            print(f"Error: {consumer} -> {provider} ({len(items)} symbols)")
            for src_obj, dst_obj, symbol in items:
                print(f"  {src_obj} -> {dst_obj}: {demangled.get(symbol, symbol)}")
                if args.show_suppressions:
                    print(f"    suppress with: (\"{src_obj}\", \"{dst_obj}\", \"{symbol}\"),")
            print()
        if not args.show_suppressions:
            print("(use --show-suppressions to see suppress directives)")

    # Check for stale suppressions
    if args.verbose:
        stale = SUPPRESS - used_suppressions
        for sup in sorted(stale):
            print(f"Warning: suppression {sup} was not triggered, consider removing", file=sys.stderr)

    # Declared but unused dependencies
    if args.verbose:
        missing = []
        for consumer in sorted(libs):
            direct = declared_deps.get(consumer, set())
            for dep in sorted(direct):
                if dep in libs and dep not in actual_deps.get(consumer, {}):
                    missing.append((consumer, dep))
        if missing:
            print()
            print("Declared dependencies with no symbol references:")
            for consumer, dep in missing:
                print(f"  {consumer} -> {dep}")

    # Verbose: full matrix
    if args.verbose:
        print()
        print("Full dependency matrix:")
        for consumer in sorted(libs):
            deps = actual_deps.get(consumer, {})
            if deps:
                allowed = transitive_deps(consumer, declared_deps)
                parts = []
                for p, syms in sorted(deps.items(), key=lambda x: -len(x[1])):
                    marker = "" if p in allowed else " (!)"
                    parts.append(f"{p}({len(syms)}{marker})")
                print(f"  {consumer}: {', '.join(parts)}")

    if errors:
        print(f"\n{len(errors)} unexpected dependencies found.", file=sys.stderr)
        return 1
    else:
        print("Success! No unexpected dependencies detected.")
        return 0


if __name__ == "__main__":
    sys.exit(main())
