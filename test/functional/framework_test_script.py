#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests for test_framework.script."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.script import bn2vch
from test_framework.util import assert_equal

def test_bn2vch():
    assert_equal(bn2vch(0), bytes([]))
    assert_equal(bn2vch(1), bytes([0x01]))
    assert_equal(bn2vch(-1), bytes([0x81]))
    assert_equal(bn2vch(0x7F), bytes([0x7F]))
    assert_equal(bn2vch(-0x7F), bytes([0xFF]))
    assert_equal(bn2vch(0x80), bytes([0x80, 0x00]))
    assert_equal(bn2vch(-0x80), bytes([0x80, 0x80]))
    assert_equal(bn2vch(0xFF), bytes([0xFF, 0x00]))
    assert_equal(bn2vch(-0xFF), bytes([0xFF, 0x80]))
    assert_equal(bn2vch(0x100), bytes([0x00, 0x01]))
    assert_equal(bn2vch(-0x100), bytes([0x00, 0x81]))
    assert_equal(bn2vch(0x7FFF), bytes([0xFF, 0x7F]))
    assert_equal(bn2vch(-0x8000), bytes([0x00, 0x80, 0x80]))
    assert_equal(bn2vch(-0x7FFFFF), bytes([0xFF, 0xFF, 0xFF]))
    assert_equal(bn2vch(0x80000000), bytes([0x00, 0x00, 0x00, 0x80, 0x00]))
    assert_equal(bn2vch(-0x80000000), bytes([0x00, 0x00, 0x00, 0x80, 0x80]))
    assert_equal(bn2vch(0xFFFFFFFF), bytes([0xFF, 0xFF, 0xFF, 0xFF, 0x00]))

    assert_equal(bn2vch(123456789), bytes([0x15, 0xCD, 0x5B, 0x07]))
    assert_equal(bn2vch(-54321), bytes([0x31, 0xD4, 0x80]))

class FrameworkTestScript(BitcoinTestFramework):
    def setup_network(self):
        pass

    def set_test_params(self):
        self.num_nodes = 0

    def run_test(self):
        test_bn2vch()

if __name__ == '__main__':
    FrameworkTestScript().main()
