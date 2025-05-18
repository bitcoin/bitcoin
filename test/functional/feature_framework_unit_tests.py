#!/usr/bin/env python3
# Copyright (c) 2017-2024 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Framework unit tests

Unit tests for the test framework.
"""

import sys
import unittest

from test_framework.test_framework import TEST_EXIT_PASSED, TEST_EXIT_FAILED

# List of framework modules containing unit tests. Should be kept in sync with
# the output of `git grep unittest.TestCase ./test/functional/test_framework`
TEST_FRAMEWORK_MODULES = [
    "address",
    "crypto.bip324_cipher",
    "blocktools",
    "crypto.chacha20",
    "crypto.ellswift",
    "key",
    "messages",
    "crypto.muhash",
    "crypto.poly1305",
    "crypto.ripemd160",
    "crypto.secp256k1",
    "script",
    "script_util",
    "segwit_addr",
    "wallet_util",
]


def run_unit_tests():
    test_framework_tests = unittest.TestSuite()
    for module in TEST_FRAMEWORK_MODULES:
        test_framework_tests.addTest(
            unittest.TestLoader().loadTestsFromName(f"test_framework.{module}")
        )
    result = unittest.TextTestRunner(stream=sys.stdout, verbosity=1, failfast=True).run(
        test_framework_tests
    )
    if not result.wasSuccessful():
        sys.exit(TEST_EXIT_FAILED)
    sys.exit(TEST_EXIT_PASSED)


if __name__ == "__main__":
    run_unit_tests()

