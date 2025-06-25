#!/usr/bin/env python3
"""Merge two strings letter by letter.

Usage:
    letter_merge.py FIRST SECOND

Outputs the two strings interwoven so that letters from each
string appear alternately. If one string is longer, its trailing
characters are appended at the end.
"""
from __future__ import annotations

import argparse


def merge_strings(first: str, second: str) -> str:
    """Return a new string combining ``first`` and ``second`` letter by letter."""
    result = []
    max_length = max(len(first), len(second))
    for i in range(max_length):
        if i < len(first):
            result.append(first[i])
        if i < len(second):
            result.append(second[i])
    return "".join(result)


def main() -> None:
    parser = argparse.ArgumentParser(description="Merge two strings letter by letter")
    parser.add_argument("first", help="First input string")
    parser.add_argument("second", help="Second input string")
    args = parser.parse_args()

    merged = merge_strings(args.first, args.second)
    print(merged)


if __name__ == "__main__":
    main()
