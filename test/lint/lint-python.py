#!/usr/bin/env python3
#
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Check for type errors in python files using ty.
"""

import shutil
import subprocess

from importlib.metadata import metadata, PackageNotFoundError

DEPS = ['lief', 'pyzmq']


def check_dependencies():
    for dep in DEPS:
        try:
            metadata(dep)
        except PackageNotFoundError:
            print(f"Skipping Python linting since {dep} is not installed.")
            exit(0)

    if not shutil.which('ty'):
        print("Skipping Python linting since ty is not installed.")
        exit(0)


def main():
    check_dependencies()

    try:
        subprocess.check_call(['ty', 'check'])
    except subprocess.CalledProcessError:
        exit(1)


if __name__ == "__main__":
    main()
