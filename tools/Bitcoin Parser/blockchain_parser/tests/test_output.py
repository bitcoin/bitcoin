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

from blockchain_parser.output import Output


class TestOutput(unittest.TestCase):
    def test_pubkeyhash_from_hex(self):
        raw_output = "01000000000000001976a91432ba382cf668657bae15ee0a97fa87" \
                     "f12e1bc89f88ac"
        output = Output.from_hex(a2b_hex(raw_output))
        self.assertTrue(output.is_pubkeyhash())
        self.assertEqual("pubkeyhash", output.type)
        self.assertEqual(1, output.value)
        self.assertEqual(1, len(output.addresses))

    def test_pubkey_from_hex(self):
        raw_output = "0100000000000000232102c0993f639534d348e1dca30566491e6c" \
                     "b11c14afa13ec244c05396a9839aeb17ac"
        output = Output.from_hex(a2b_hex(raw_output))
        self.assertTrue(output.is_pubkey())
        self.assertEqual("pubkey", output.type)
        self.assertEqual(1, len(output.addresses))

    def test_p2sh_from_hex(self):
        raw_output = "010000000000000017a91471c5c3727fac8dbace94bd38cf8ac16a" \
                     "034a794787"
        output = Output.from_hex(a2b_hex(raw_output))

        self.assertTrue(output.is_p2sh())
        self.assertEqual("p2sh", output.type)
        self.assertEqual(1, len(output.addresses))

    def test_return_from_hex(self):
        raw_output = "01000000000000002a6a2846610000000024958857cc0da391b" \
                     "7b2bf61bcba59bb9ee438873f902c25da4c079e53d0c55fe991"
        output = Output.from_hex(a2b_hex(raw_output))
        self.assertTrue(output.is_return())
        self.assertEqual("OP_RETURN", output.type)
        self.assertEqual(0, len(output.addresses))

    def test_multisig_from_hex(self):
        raw_output = "0100000000000000475121025cd452979d4d5e928d47c3581bb287" \
                     "41b2cf9c54185e7d563a663707b00d956d2102ff99d00aa9d195b9" \
                     "3732254def8bfe80a786a7973ef8e63afd8d2a65e97b6c3b52ae"
        output = Output.from_hex(a2b_hex(raw_output))
        self.assertTrue(output.is_multisig())
        self.assertEqual("multisig", output.type)
        self.assertEqual(2, len(output.addresses))

    def test_unknown_from_hex(self):
        raw_output = "01000000000000000151"
        output = Output.from_hex(a2b_hex(raw_output))
        self.assertEqual("unknown", output.type)
        self.assertEqual(0, len(output.addresses))
