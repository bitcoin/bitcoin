#!/usr/bin/env python3
"""Check that a libsecp256k1 shared library exports only expected symbols.

Usage examples:
  - When building with Autotools:
    ./tools/symbol-check.py .libs/libsecp256k1.so
    ./tools/symbol-check.py .libs/libsecp256k1-<V>.dll
    ./tools/symbol-check.py .libs/libsecp256k1.dylib

  - When building with CMake:
    ./tools/symbol-check.py build/lib/libsecp256k1.so
    ./tools/symbol-check.py build/bin/libsecp256k1-<V>.dll
    ./tools/symbol-check.py build/lib/libsecp256k1.dylib"""

import re
import sys
import subprocess

import lief


class UnexpectedExport(RuntimeError):
    pass


def get_exported_exports(library) -> list[str]:
    """Adapter function to get exported symbols based on the library format."""
    if library.format == lief.Binary.FORMATS.ELF:
        return [symbol.name for symbol in library.exported_symbols]
    elif library.format == lief.Binary.FORMATS.PE:
        return [entry.name for entry in library.get_export().entries]
    elif library.format == lief.Binary.FORMATS.MACHO:
        return [symbol.name[1:] for symbol in library.exported_symbols]
    raise NotImplementedError(f"Unsupported format: {library.format}")


def grep_expected_symbols() -> list[str]:
    """Guess the list of expected exported symbols from the source code."""
    grep_output = subprocess.check_output(
        ["git", "grep", r"^\s*SECP256K1_API", "--", "include"],
        universal_newlines=True,
        encoding="utf-8"
    )
    lines = grep_output.split("\n")
    pattern = re.compile(r'\bsecp256k1_\w+')
    exported: list[str] = [pattern.findall(line)[-1] for line in lines if line.strip()]
    return exported


def check_symbols(library, expected_exports) -> None:
    """Check that the library exports only the expected symbols."""
    actual_exports = get_exported_exports(library)
    unexpected_exports = set(actual_exports) - set(expected_exports)
    if unexpected_exports != set():
        raise UnexpectedExport(f"Unexpected exported symbols: {unexpected_exports}")

def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 1
    library = lief.parse(sys.argv[1])
    expected_exports = grep_expected_symbols()
    try:
        check_symbols(library, expected_exports)
    except UnexpectedExport as e:
        print(f"{sys.argv[0]}: In {sys.argv[1]}: {e}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
