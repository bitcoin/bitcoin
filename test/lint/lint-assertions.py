#!/usr/bin/env python3
#
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for assertions with obvious side effects.

import sys
import subprocess


def git_grep(params: [], error_msg: ""):
    try:
        output = subprocess.check_output(["git", "grep", *params], text=True, encoding="utf8")
        print(error_msg)
        print(output)
        return 1
    except subprocess.CalledProcessError as ex1:
        if ex1.returncode > 1:
            raise ex1
    return 0


def main():
    # PRE31-C (SEI CERT C Coding Standard):
    # "Assertions should not contain assignments, increment, or decrement operators."
    exit_code = git_grep([
        "-E",
        r"[^_]assert\(.*(\+\+|\-\-|[^=!<>]=[^=!<>]).*\);",
        "--",
        "*.cpp",
        "*.h",
    ], "Assertions should not have side effects:")

    # Aborting the whole process is undesirable for RPC code. So nonfatal
    # checks should be used over assert. See: src/util/check.h
    # src/rpc/server.cpp is excluded from this check since it's mostly meta-code.
    exit_code |= git_grep([
        "-nE",
        r"\<(A|a)ss(ume|ert) *\(.*\);",
        "--",
        "src/rpc/",
        "src/wallet/rpc*",
        ":(exclude)src/rpc/server.cpp",
    ], "CHECK_NONFATAL(condition) or NONFATAL_UNREACHABLE should be used instead of assert for RPC code.")

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
