#!/usr/bin/env python3
# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Routines for compressing transaction output amounts and scripts."""
import unittest

from .messages import COIN


def compress_amount(n):
    if n == 0:
        return 0
    e = 0
    while ((n % 10) == 0) and (e < 9):
        n //= 10
        e += 1
    if e < 9:
        d = n % 10
        assert (d >= 1 and d <= 9)
        n //= 10
        return 1 + (n*9 + d - 1)*10 + e
    else:
        return 1 + (n - 1)*10 + 9


def decompress_amount(x):
    if x == 0:
        return 0
    x -= 1
    e = x % 10
    x //= 10
    n = 0
    if e < 9:
        d = (x % 9) + 1
        x //= 9
        n = x * 10 + d
    else:
        n = x + 1
    while e > 0:
        n *= 10
        e -= 1
    return n


class TestFrameworkCompressor(unittest.TestCase):
    def test_amount_compress_decompress(self):
        def check_amount(amount, expected_compressed):
            self.assertEqual(compress_amount(amount), expected_compressed)
            self.assertEqual(decompress_amount(expected_compressed), amount)

        # test cases from compress_tests.cpp:compress_amounts
        check_amount(0, 0x0)
        check_amount(1, 0x1)
        check_amount(1000000, 0x7)
        check_amount(COIN, 0x9)
        check_amount(50*COIN, 0x32)
        check_amount(21000000*COIN, 0x1406f40)
