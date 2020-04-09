# Copyright (C) 2013-2014 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

from __future__ import absolute_import, division, print_function, unicode_literals

import json
import os
import unittest
import array
import sys
_bchr = chr
_bord = ord
_tobytes = lambda x: array.array('B', x).tostring()
if sys.version > '3':
    long = int
    _bchr = lambda x: bytes([x])
    _bord = lambda x: x
    _tobytes = bytes

from binascii import unhexlify

from bitcoin.core.script import CScript, OP_0, OP_1, OP_16
from bitcoin.bech32 import *
from bitcoin.segwit_addr import encode, decode


def load_test_vectors(name):
    with open(os.path.dirname(__file__) + '/data/' + name, 'r') as fd:
        for testcase in json.load(fd):
            yield testcase

def to_scriptPubKey(witver, witprog):
    """Decoded bech32 address to script"""
    return CScript([witver]) + CScript(_tobytes(witprog))

class Test_bech32(unittest.TestCase):

    def op_decode(self, witver):
        """OP encoding to int"""
        if witver == OP_0:
            return 0
        if OP_1 <= witver <= OP_16:
            return witver - OP_1 + 1
        self.fail('Wrong witver: %d' % witver)

    def test_encode_decode(self):
        for exp_bin, exp_bech32 in load_test_vectors('bech32_encode_decode.json'):
            exp_bin = [_bord(y) for y in unhexlify(exp_bin.encode('utf8'))]
            witver = self.op_decode(exp_bin[0])
            hrp = exp_bech32[:exp_bech32.rindex('1')].lower()
            self.assertEqual(exp_bin[1], len(exp_bin[2:]))
            act_bech32 = encode(hrp, witver, exp_bin[2:])
            act_bin = decode(hrp, exp_bech32)

            self.assertEqual(act_bech32.lower(), exp_bech32.lower())
            self.assertEqual(to_scriptPubKey(*act_bin), _tobytes(exp_bin))

class Test_CBech32Data(unittest.TestCase):
    def test_from_data(self):
        b = CBech32Data.from_bytes(0, unhexlify('751e76e8199196d454941c45d1b3a323f1433bd6'))
        self.assertEqual(b.witver, 0)
        self.assertEqual(str(b).upper(), 'BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4')

    def test_invalid_bech32_exception(self):

        for invalid, _ in load_test_vectors("bech32_invalid.json"):
            msg = '%r should have raised Bech32Error but did not' % invalid
            with self.assertRaises(Bech32Error, msg=msg):
                CBech32Data(invalid)
