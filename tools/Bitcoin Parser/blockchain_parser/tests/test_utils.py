# Copyright (C) 2015-2016 The bitcoin-blockchain-parser developers
#
# This file is part of bitcoin-blockchain-parser.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of bitcoin-blockchain-parser, including this file, may be copied,
# modified, propagated, or distributed except according to the terms contained
# in the LICENSE file.

import unittest
from binascii import b2a_hex, a2b_hex

from blockchain_parser import utils


class TestUtils(unittest.TestCase):
    def test_btc_ripemd160(self):
        data = a2b_hex("02218AD6CDC632E7AE7D04472374311CEBBBBF0AB540D2D08C3400BB844C654231")
        ripe = utils.btc_ripemd160(data)
        expected = b"5238c71458e464d9ff90299abca4a1d7b9cb76ab"
        self.assertEqual(b2a_hex(ripe), expected)

    def test_format_hash(self):
        data = a2b_hex("deadbeef")
        self.assertEqual(utils.format_hash(data), "efbeadde")

    def test_decode_uint32(self):
        uint32_dict = {
            "01000000": 1,
            "000000ff": 4278190080
        }

        for uint32, value in uint32_dict.items():
            self.assertEqual(utils.decode_uint32(a2b_hex(uint32)), value)

    def test_decode_uint64(self):
        uint64_dict = {
            "0100000000000000": 1,
            "00000000000000ff": 18374686479671623680
        }

        for uint64, value in uint64_dict.items():
            self.assertEqual(utils.decode_uint64(a2b_hex(uint64)), value)

    def test_decode_varint(self):
        case1 = a2b_hex("fa")
        self.assertEqual(utils.decode_varint(case1), (250, 1))
        case2 = a2b_hex("fd0100")
        self.assertEqual(utils.decode_varint(case2), (1, 3))
        case3 = a2b_hex("fe01000000")
        self.assertEqual(utils.decode_varint(case3), (1, 5))
        case4 = a2b_hex("ff0100000000000000")
        self.assertEqual(utils.decode_varint(case4), (1, 9))
