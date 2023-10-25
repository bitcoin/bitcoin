#!/usr/bin/env python3
#
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.
#
# Check for leading whitespaces in the translatable strings.

import subprocess
import sys


def main():
    tr_strings = subprocess.run(['git', 'grep', '-e', 'tr("[[:space:]]', '--', 'src/qt'], stdout=subprocess.PIPE, text=True).stdout

    if tr_strings.strip():
        print("Avoid leading whitespaces in:")
        print(tr_strings)
        sys.exit(1)


if __name__ == "__main__":
    main()
