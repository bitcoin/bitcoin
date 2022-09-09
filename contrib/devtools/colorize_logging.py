#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Colorize log output
Usage: ./src/bitcoind | ./contrib/devtools/colorize_logging.py
"""
import re

COLORS = {
    "cyan": "\033[36m",
    "red": "\033[31m",
    "blue": "\033[34m",
    "green": "\033[32m",
    "magenta": "\033[35m"
}


def colorize(text):
    # Colorize groups of brackets
    for i, match in enumerate(re.finditer(r"\[.*?\]", text)):
        text = text.replace(
            match.group(0),
            list(COLORS.values())[(i + 1) % len(COLORS)] + match.group(0) + "\033[0m",
        )

    # Colorize date
    date = re.search(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z", text)
    if date:
        text = text.replace(date.group(0), COLORS["cyan"] + date.group(0) + "\033[0m")

    return text


if __name__ == "__main__":
    import sys

    try:
        for line in sys.stdin:
            print(colorize(line), end="")
    except KeyboardInterrupt:
        print()
        sys.exit(0)
