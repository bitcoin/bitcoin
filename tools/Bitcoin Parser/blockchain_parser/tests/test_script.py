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
from blockchain_parser.script import Script, is_public_key


class TestScript(unittest.TestCase):
    def test_from_hex(self):
        case1 = "6a"
        script = Script.from_hex(a2b_hex(case1))
        self.assertEqual("OP_RETURN", script.value)

    def test_invalid_script(self):
        case = "40"
        script = Script.from_hex(a2b_hex(case))
        self.assertEqual("INVALID_SCRIPT", script.value)

    def test_is_public_key(self):
        case1 = "010000000000000017a91471c5c3727fac8dbace94bd38cf8ac16a034a7" \
                "94787"
        self.assertFalse(is_public_key(a2b_hex(case1)))
        self.assertFalse(is_public_key(None))
        case3 = "02c0993f639534d348e1dca30566491e6cb11c14afa13ec244c05396a98" \
                "39aeb17"
        self.assertTrue(is_public_key(a2b_hex(case3)))
