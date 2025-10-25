#!/usr/bin/env python3
#
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Find dead Python code.
"""

from subprocess import check_output, STDOUT, CalledProcessError

FILES_ARGS = ['git', 'ls-files', '--', '*.py']


def check_vulture_install():
    try:
        check_output(["vulture", "--version"])
    except FileNotFoundError:
        print("Skipping Python dead code linting since vulture is not installed. Install by running \"pip3 install vulture\"")
        exit(0)


def main():
    check_vulture_install()

    files = check_output(FILES_ARGS, text=True).splitlines()
    # --min-confidence 100 will only report code that is guaranteed to be unused within the analyzed files.
    # Any value below 100 introduces the risk of false positives, which would create an unacceptable maintenance burden.
    vulture_args = ['vulture', '--min-confidence=100'] + files

    try:
        check_output(vulture_args, stderr=STDOUT, text=True)
    except CalledProcessError as e:
        print(e.output, end="")
        print("Python dead code detection found some issues")
        exit(1)


if __name__ == "__main__":
    main()
