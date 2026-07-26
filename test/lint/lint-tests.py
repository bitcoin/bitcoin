#!/usr/bin/env python3
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Check the test suite naming conventions
"""

import re
import subprocess
import sys


def grep_boost_test_suites():
    command = [
        "git",
        "grep",
        "-E",
        r"^(BOOST_FIXTURE_TEST_SUITE|BOOST_AUTO_TEST_SUITE)\(",
        "--",
        "src/ipc/test/**.cpp",
        "src/test/**.cpp",
        "src/wallet/test/**.cpp",
    ]
    return subprocess.check_output(command, text=True)


def main():
    test_suite_list = grep_boost_test_suites().splitlines()
    not_matching = [
        x
        for x in test_suite_list
        if re.search(r"/(.*?)\.cpp:(?:BOOST_FIXTURE_TEST_SUITE|BOOST_AUTO_TEST_SUITE)\(\1(_[a-z0-9]+)?[,)]", x) is None
    ]
    if len(not_matching) > 0:
        not_matching = "\n".join(not_matching)
        error_msg = (
            "The test suite in file src/test/foo_tests.cpp should be named\n"
            '`foo_tests`, or if there are multiple test suites, `foo_tests_bar`.\n'
            'Please make sure the following test suites follow that convention:\n\n'
            f"{not_matching}\n"
        )
        print(error_msg)
        sys.exit(1)


if __name__ == "__main__":
    main()
