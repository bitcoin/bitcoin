#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Check the test suite naming conventions
"""

import re
import sys
from subprocess import check_output
from collections import Counter
from typing import NoReturn

CMD_GET_TEST_SUITES = r"git grep -E '^BOOST_FIXTURE_TEST_SUITE\(' -- src/test/**.cpp src/wallet/test/**.cpp"  


def main() -> NoReturn:
    entries = check_output(CMD_GET_TEST_SUITES, shell=True).decode('utf-8').splitlines()
    
    check_regex = re.compile(r"/(.*?)\.cpp:BOOST_FIXTURE_TEST_SUITE\(\1, .*\)$")
    inconsistent_test_suits = []
    test_suit_names = []
    for line in entries:
        res = check_regex.search(line)
        if res: 
            test_suit_names.append(res.group(1))
        else:
            inconsistent_test_suits.append(line)

    exit_code = 0

    if len(inconsistent_test_suits):
        exit_code = 1
        print("The test suite in file src/test/foo_tests.cpp should be named")
        print("\"foo_tests\". Please make sure the following test suites follow")
        print("that convention:")
        print()
        for line in inconsistent_test_suits:
            print(line)

    test_suit_names_count = Counter(test_suit_names)
    repeated_test_suit_names = [name for name in test_suit_names_count if test_suit_names_count[name] > 1]

    if len(repeated_test_suit_names):
        exit_code = 1
        print("Test suite names must be unique. The following test suite names")
        print("appear to be used more than once:")
        print()
        for line in repeated_test_suit_names:
            print(line)
        
    sys.exit(exit_code)

if __name__ == "__main__":
    main()