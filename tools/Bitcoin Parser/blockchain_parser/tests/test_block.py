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
from binascii import a2b_hex
from datetime import datetime

from blockchain_parser.block import Block


class TestBlock(unittest.TestCase):
    def test_from_hex(self):
        block_str = "0100000000000000000000000000000000000000000000000000000" \
                    "000000000000000003ba3edfd7a7b12b27ac72c3e67768f617fc81b" \
                    "c3888a51323a9fb8aa4b1e5e4a29ab5f49ffff001d1dac2b7c01010" \
                    "0000001000000000000000000000000000000000000000000000000" \
                    "0000000000000000ffffffff4d04ffff001d0104455468652054696" \
                    "d65732030332f4a616e2f32303039204368616e63656c6c6f72206f" \
                    "6e206272696e6b206f66207365636f6e64206261696c6f757420666" \
                    "f722062616e6b73ffffffff0100f2052a01000000434104678afdb0" \
                    "fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb" \
                    "649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c70" \
                    "2b6bf11d5fac00000000"
        block_hex = a2b_hex(block_str)
        block = Block.from_hex(block_hex)
        self.assertEqual(1, block.n_transactions)
        block_hash = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1" \
                     "b60a8ce26f"
        self.assertEqual(block_hash, block.hash)
        self.assertEqual(486604799, block.header.bits)
        merkle_root = "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127" \
                      "b7afdeda33b"
        self.assertEqual(merkle_root, block.header.merkle_root)
        self.assertEqual(2083236893, block.header.nonce)
        self.assertEqual(1, block.header.version)
        self.assertEqual(1, block.header.difficulty)
        self.assertEqual(285, block.size)
        self.assertEqual(datetime.utcfromtimestamp(1231006505),
                         block.header.timestamp)
        self.assertEqual("0" * 64, block.header.previous_block_hash)

        for tx in block.transactions:
            self.assertEqual(1, tx.version)
            tx_hash = "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127" \
                      "b7afdeda33b"
            self.assertEqual(tx_hash, tx.hash)
            self.assertEqual(204, tx.size)
            self.assertEqual(0, tx.locktime)
            self.assertEqual(0xffffffff, tx.inputs[0].transaction_index)
            self.assertEqual(0xffffffff, tx.inputs[0].sequence_number)
            self.assertTrue("ffff001d" in tx.inputs[0].script.value)
            self.assertEqual("0" * 64, tx.inputs[0].transaction_hash)
            self.assertEqual(50 * 100000000, tx.outputs[0].value)
