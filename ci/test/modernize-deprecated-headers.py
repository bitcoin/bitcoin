#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
"""Modernize deprecated C includes that IWYU fixes might leave behind."""

from pathlib import Path
import sys

REPLACEMENTS = {
    "assert.h": "cassert",
    "ctype.h": "cctype",
    "errno.h": "cerrno",
    "float.h": "cfloat",
    "limits.h": "climits",
    "locale.h": "clocale",
    "math.h": "cmath",
    "setjmp.h": "csetjmp",
    "signal.h": "csignal",
    "stdarg.h": "cstdarg",
    "stddef.h": "cstddef",
    "stdio.h": "cstdio",
    "stdlib.h": "cstdlib",
    "string.h": "cstring",
    "time.h": "ctime",
    "wchar.h": "cwchar",
    "wctype.h": "cwctype",
    "uchar.h": "cuchar",
    "inttypes.h": "cinttypes",
    "stdint.h": "cstdint",
}


def _build_mapping() -> dict[str, str]:
    """Return mapping for literal include directives."""

    include_map: dict[str, str] = {}
    for legacy, modern in REPLACEMENTS.items():
        include_map[f"#include <{legacy}>"] = f"#include <{modern}>"
    return include_map


def main(argv: list[str]):
    include_map = _build_mapping()
    for filename in argv[1:]:
        path = Path(filename)
        if not path.is_file():
            continue
        print(f"Fixing up {path} ...")
        content = path.read_text()
        for legacy, modern in include_map.items():
            content = content.replace(legacy, modern)
        path.write_text(content)


if __name__ == "__main__":
    main(sys.argv)
