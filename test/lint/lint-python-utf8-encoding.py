#!/usr/bin/env python3
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Make sure we explicitly open all text files using UTF-8 (or ASCII) encoding to
# avoid potential issues on the BSDs where the locale is not always set.

import sys
import re

from subprocess import check_output, CalledProcessError

EXCLUDED_DIRS = ["src/crc32c/"]


def get_exclude_args():
    return [":(exclude)" + dir for dir in EXCLUDED_DIRS]


def check_fileopens():
    fileopens = list()

    try:
        fileopens = check_output(["git", "grep", r" open(", "--", "*.py"] + get_exclude_args(), text=True, encoding="utf8").splitlines()
    except CalledProcessError as e:
        if e.returncode > 1:
            raise e

    filtered_fileopens = [fileopen for fileopen in fileopens if not re.search(r"encoding=.(ascii|utf8|utf-8).|open\([^,]*, ['\"][^'\"]*b[^'\"]*['\"]", fileopen)]

    return filtered_fileopens


def check_checked_outputs():
    checked_outputs = list()

    try:
        checked_outputs = check_output(["git", "grep", "check_output(", "--", "*.py"] + get_exclude_args(), text=True, encoding="utf8").splitlines()
    except CalledProcessError as e:
        if e.returncode > 1:
            raise e

    filtered_checked_outputs = [checked_output for checked_output in checked_outputs if re.search(r"text=True", checked_output) and not re.search(r"encoding=.(ascii|utf8|utf-8).", checked_output)]

    return filtered_checked_outputs


def main():
    exit_code = 0

    nonexplicit_utf8_fileopens = check_fileopens()
    if nonexplicit_utf8_fileopens:
        print("Python's open(...) seems to be used to open text files without explicitly specifying encoding='utf8':\n")
        for fileopen in nonexplicit_utf8_fileopens:
            print(fileopen)
        exit_code = 1

    nonexplicit_utf8_checked_outputs = check_checked_outputs()
    if nonexplicit_utf8_checked_outputs:
        if nonexplicit_utf8_fileopens:
            print("\n")
        print("Python's check_output(...) seems to be used to get program outputs without explicitly specifying encoding='utf8':\n")
        for checked_output in nonexplicit_utf8_checked_outputs:
            print(checked_output)
        exit_code = 1

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
