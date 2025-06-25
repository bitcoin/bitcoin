#!/usr/bin/env python3
"""Merge multiple strings letter by letter.

Usage:
    letter_merge.py STRING [STRING ...]

Outputs the input strings interwoven so that letters from each
string appear alternately. If strings differ in length, trailing
characters from the longer strings are appended at the end.
"""

import argparse
from itertools import zip_longest


def merge_strings(*strings: str) -> str:
    """Return a new string combining ``strings`` letter by letter."""
    return "".join(
        "".join(ch for ch in group if ch is not None)
        for group in zip_longest(*strings)
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="Merge strings letter by letter")
    parser.add_argument(
        "strings",
        nargs="+",
        help="Input strings to merge in sequence",
    )
    args = parser.parse_args()

    merged = merge_strings(*args.strings)
    print(merged)


if __name__ == "__main__":
    main()
