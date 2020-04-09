# Copyright (C) 2013-2015 The python-bitcoinlib developers
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

import unittest
import os

from bitcoin.core import b2x,x
from bitcoin.core.script import *

class Test_CScriptOp(unittest.TestCase):
    def test_pushdata(self):
        def T(data, expected):
            data = x(data)
            expected = x(expected)
            serialized_data = CScriptOp.encode_op_pushdata(data)
            self.assertEqual(serialized_data, expected)

        T('', '00')
        T('00', '0100')
        T('0011223344556677', '080011223344556677')
        T('ff'*0x4b, '4b' + 'ff'*0x4b)
        T('ff'*0x4c, '4c4c' + 'ff'*0x4c)
        T('ff'*0x4c, '4c4c' + 'ff'*0x4c)
        T('ff'*0xff, '4cff' + 'ff'*0xff)
        T('ff'*0x100, '4d0001' + 'ff'*0x100)
        T('ff'*0xffff, '4dffff' + 'ff'*0xffff)
        T('ff'*0x10000, '4e00000100' + 'ff'*0x10000)

    def test_is_singleton(self):
        self.assertTrue(OP_0 is CScriptOp(0x00))
        self.assertTrue(OP_1 is CScriptOp(0x51))
        self.assertTrue(OP_16 is CScriptOp(0x60))
        self.assertTrue(OP_CHECKSIG is CScriptOp(0xac))

        for i in range(0x0, 0x100):
            self.assertTrue(CScriptOp(i) is CScriptOp(i))

    def test_encode_decode_op_n(self):
        def t(n, op):
            actual = CScriptOp.encode_op_n(n)
            self.assertEqual(actual, op)
            self.assertTrue(isinstance(actual, CScriptOp))

            actual = op.decode_op_n()
            self.assertEqual(actual, n)
            self.assertTrue(isinstance(actual, int))

        t(0, OP_0)
        t(1, OP_1)
        t(2, OP_2)
        t(3, OP_3)
        t(4, OP_4)
        t(5, OP_5)
        t(6, OP_6)
        t(7, OP_7)
        t(8, OP_8)
        t(9, OP_9)
        t(9, OP_9)
        t(10, OP_10)
        t(11, OP_11)
        t(12, OP_12)
        t(13, OP_13)
        t(14, OP_14)
        t(15, OP_15)
        t(16, OP_16)

        with self.assertRaises(ValueError):
            OP_CHECKSIG.decode_op_n()

        with self.assertRaises(ValueError):
            CScriptOp(1).decode_op_n()

class Test_CScript(unittest.TestCase):
    def test_tokenize_roundtrip(self):
        def T(serialized_script, expected_tokens, test_roundtrip=True):
            serialized_script = x(serialized_script)
            script_obj = CScript(serialized_script)
            actual_tokens = list(script_obj)
            self.assertEqual(actual_tokens, expected_tokens)

            if test_roundtrip:
                recreated_script = CScript(actual_tokens)
                self.assertEqual(recreated_script, serialized_script)

        T('', [])

        # standard pushdata
        T('00', [OP_0])
        T('0100', [b'\x00'])
        T('4b' + 'ff'*0x4b, [b'\xff'*0x4b])

        # non-optimal pushdata
        T('4c00', [b''], False)
        T('4c04deadbeef', [x('deadbeef')], False)
        T('4d0000', [b''], False)
        T('4d0400deadbeef', [x('deadbeef')], False)
        T('4e00000000', [b''], False)
        T('4e04000000deadbeef', [x('deadbeef')], False)

        # numbers
        T('00', [0x0])
        T('4f', [OP_1NEGATE])
        T('51', [0x1])
        T('52', [0x2])
        T('53', [0x3])
        T('54', [0x4])
        T('55', [0x5])
        T('56', [0x6])
        T('57', [0x7])
        T('58', [0x8])
        T('59', [0x9])
        T('5a', [0xa])
        T('5b', [0xb])
        T('5c', [0xc])
        T('5d', [0xd])
        T('5e', [0xe])
        T('5f', [0xf])

        # some opcodes
        T('9b', [OP_BOOLOR])
        T('9a9b', [OP_BOOLAND, OP_BOOLOR])
        T('ff', [OP_INVALIDOPCODE])
        T('fafbfcfd', [CScriptOp(0xfa), CScriptOp(0xfb), CScriptOp(0xfc), CScriptOp(0xfd)])

        # all three types
        T('512103e2a0e6a91fa985ce4dda7f048fca5ec8264292aed9290594321aa53d37fdea32410478d430274f8c5ec1321338151e9f27f4c676a008bdf8638d07c0b6be9ab35c71a1518063243acd4dfe96b66e3f2ec8013c8e072cd09b3834a19f81f659cc345552ae',
          [1,
           x('03e2a0e6a91fa985ce4dda7f048fca5ec8264292aed9290594321aa53d37fdea32'),
           x('0478d430274f8c5ec1321338151e9f27f4c676a008bdf8638d07c0b6be9ab35c71a1518063243acd4dfe96b66e3f2ec8013c8e072cd09b3834a19f81f659cc3455'),
           2,
           OP_CHECKMULTISIG])

    def test_invalid_scripts(self):
        def T(serialized):
            with self.assertRaises(CScriptInvalidError):
                list(CScript(x(serialized)))

        T('01')
        T('02')
        T('0201')
        T('4b')
        T('4b' + 'ff'*0x4a)
        T('4c')
        T('4cff' + 'ff'*0xfe)
        T('4d')
        T('4dff')
        T('4dffff' + 'ff'*0xfffe)
        T('4e')
        T('4effffff')
        T('4effffffff' + 'ff'*0xfffe) # not going to test with 4GiB-1...

    def test_equality(self):
        # Equality is on the serialized script, not the logical meaning.
        # This is important for P2SH.
        def T(serialized1, serialized2, are_equal):
            script1 = CScript(x(serialized1))
            script2 = CScript(x(serialized2))
            if are_equal:
                self.assertEqual(script1, script2)
            else:
                self.assertNotEqual(script1, script2)

        T('', '', True)
        T('', '00', False)
        T('00', '00', True)
        T('00', '01', False)
        T('01ff', '01ff', True)
        T('fc01ff', '01ff', False)

        # testing equality on an invalid script is legal, and evaluates based
        # on the serialization
        T('4e', '4e', True)
        T('4e', '4e00', False)

    def test_add(self):
        script = CScript()
        script2 = script + 1

        # + operator must create a new instance
        self.assertIsNot(script, script2)

        script = script2
        self.assertEqual(script, b'\x51')

        script += 2
        # += should not be done in place
        self.assertIsNot(script, script2)
        self.assertEqual(script, b'\x51\x52')

        script += OP_CHECKSIG
        self.assertEqual(script, b'\x51\x52\xac')

        script += b'deadbeef'
        self.assertEqual(script, b'\x51\x52\xac\x08deadbeef')

        script = CScript() + 1 + 2 + OP_CHECKSIG + b'deadbeef'
        self.assertEqual(script, b'\x51\x52\xac\x08deadbeef')

        # big number
        script = CScript() + 2**64
        self.assertEqual(script, b'\x09\x00\x00\x00\x00\x00\x00\x00\x00\x01')

        # some stuff we can't add
        with self.assertRaises(TypeError):
            script += None
        self.assertEqual(script, b'\x09\x00\x00\x00\x00\x00\x00\x00\x00\x01')

        with self.assertRaises(TypeError):
            script += [1, 2, 3]
        self.assertEqual(script, b'\x09\x00\x00\x00\x00\x00\x00\x00\x00\x01')

        with self.assertRaises(TypeError):
            script = script + None
        self.assertEqual(script, b'\x09\x00\x00\x00\x00\x00\x00\x00\x00\x01')

    def test_repr(self):
        def T(script, expected_repr):
            actual_repr = repr(script)
            self.assertEqual(actual_repr, expected_repr)

        T( CScript([]),
          'CScript([])')

        T( CScript([1]),
          'CScript([1])')

        T( CScript([1, 2, 3]),
          'CScript([1, 2, 3])')

        T( CScript([1, x('7ac977d8373df875eceda362298e5d09d4b72b53'), OP_DROP]),
          "CScript([1, x('7ac977d8373df875eceda362298e5d09d4b72b53'), OP_DROP])")

        T(CScript(x('0001ff515261ff')),
          "CScript([0, x('ff'), 1, 2, OP_NOP, OP_INVALIDOPCODE])")

        # truncated scripts
        T(CScript(x('6101')),
          "CScript([OP_NOP, x('')...<ERROR: PUSHDATA(1): truncated data>])")

        T(CScript(x('614bff')),
          "CScript([OP_NOP, x('ff')...<ERROR: PUSHDATA(75): truncated data>])")

        T(CScript(x('614c')),
          "CScript([OP_NOP, <ERROR: PUSHDATA1: missing data length>])")

        T(CScript(x('614c0200')),
          "CScript([OP_NOP, x('00')...<ERROR: PUSHDATA1: truncated data>])")

    def test_is_p2sh(self):
        def T(serialized, b):
            script = CScript(x(serialized))
            self.assertEqual(script.is_p2sh(), b)

        # standard P2SH
        T('a9146567e91196c49e1dffd09d5759f6bbc0c6d4c2e587', True)

        # NOT a P2SH txout due to the non-optimal PUSHDATA encoding
        T('a94c146567e91196c49e1dffd09d5759f6bbc0c6d4c2e587', False)

    def test_is_push_only(self):
        def T(serialized, b):
            script = CScript(x(serialized))
            self.assertEqual(script.is_push_only(), b)

        T('', True)
        T('00', True)
        T('0101', True)
        T('4c00', True)
        T('4d0000', True)
        T('4e00000000', True)
        T('4f', True)

        # OP_RESERVED *is* considered to be a pushdata op by is_push_only!
        # Or specifically, the IsPushOnly() used in P2SH validation.
        T('50', True)

        T('51', True)
        T('52', True)
        T('53', True)
        T('54', True)
        T('55', True)
        T('56', True)
        T('57', True)
        T('58', True)
        T('59', True)
        T('5a', True)
        T('5b', True)
        T('5c', True)
        T('5d', True)
        T('5e', True)
        T('5f', True)
        T('60', True)

        T('61', False)

    def test_is_push_only_on_invalid_pushdata(self):
        def T(hex_script):
            invalid_script = CScript(x(hex_script))
            self.assertFalse(invalid_script.is_push_only())

        T('01')
        T('02ff')
        T('4b')
        T('4c01')
        T('4c02ff')
        T('4d')
        T('4d0100')
        T('4d0200ff')
        T('4e')
        T('4e01000000')
        T('4e02000000ff')

    def test_has_canonical_pushes(self):
        def T(hex_script, expected_result):
            script = CScript(x(hex_script))
            self.assertEqual(script.has_canonical_pushes(), expected_result)

        T('', True)
        T('00', True)
        T('FF', True)

        # could have used an OP_n code, rather than a 1-byte push
        T('0100', False)
        T('0101', False)
        T('0102', False)
        T('0103', False)
        T('0104', False)
        T('0105', False)
        T('0106', False)
        T('0107', False)
        T('0108', False)
        T('0109', False)
        T('010A', False)
        T('010B', False)
        T('010C', False)
        T('010D', False)
        T('010E', False)
        T('010F', False)
        T('0110', False)
        T('0111', True)

        # Could have used a normal n-byte push, rather than OP_PUSHDATA1
        T('4c00', False)
        T('4c0100', False)
        T('4c01FF', False)
        T('4b' + '00'*75, True)
        T('4c4b' + '00'*75, False)
        T('4c4c' + '00'*76, True)

        # Could have used a OP_PUSHDATA1.
        T('4d0000', False)
        T('4d0100FF', False)
        T('4dFF00' + 'FF'*0xFF, False)
        T('4d0001' + 'FF'*0x100, True)

        # Could have used a OP_PUSHDATA2.
        T('4e00000000', False)
        T('4e01000000FF', False)
        T('4eFFFF0000' + 'FF'*0xFFFF, False)
        T('4e00000100' + 'FF'*0x10000, True)

    def test_has_canonical_pushes_with_invalid_truncated_script(self):
        def T(hex_script):
            script = CScript(x(hex_script))
            self.assertEqual(script.has_canonical_pushes(), False)

        T('01')
        T('02ff')
        T('4b')
        T('4c01')
        T('4c02ff')
        T('4d')
        T('4d0100')
        T('4d0200ff')
        T('4e')
        T('4e01000000')
        T('4e02000000ff')

    def test_is_unspendable(self):
        def T(serialized, b):
            script = CScript(x(serialized))
            self.assertEqual(script.is_unspendable(), b)

        T('', False)
        T('00', False)
        T('006a', False)
        T('6a', True)
        T('6a6a', True)
        T('6a51', True)

    def test_is_valid(self):
        def T(serialized, b):
            script = CScript(x(serialized))
            self.assertEqual(script.is_valid(), b)

        T('', True)
        T('00', True)
        T('01', False)

        # invalid opcodes do not by themselves make a script invalid
        T('ff', True)

    def test_to_p2sh_scriptPubKey(self):
        def T(redeemScript, expected_hex_bytes):
            redeemScript = CScript(redeemScript)
            actual_script = redeemScript.to_p2sh_scriptPubKey()
            self.assertEqual(b2x(actual_script), expected_hex_bytes)

        T([],
          'a914b472a266d0bd89c13706a4132ccfb16f7c3b9fcb87')

        T([1,x('029b6d2c97b8b7c718c325d7be3ac30f7c9d67651bce0c929f55ee77ce58efcf84'),1,OP_CHECKMULTISIG],
          'a91419a7d869032368fd1f1e26e5e73a4ad0e474960e87')

        T([b'\xff'*517],
          'a9140da7fa40ebf248dfbca363c79921bdd665fed5ba87')

        with self.assertRaises(ValueError):
            CScript([b'a' * 518]).to_p2sh_scriptPubKey()

class Test_IsLowDERSignature(unittest.TestCase):
    def test_high_s_value(self):
        sig = x('3046022100820121109528efda8bb20ca28788639e5ba5b365e0a84f8bd85744321e7312c6022100a7c86a21446daa405306fe10d0a9906e37d1a2c6b6fdfaaf6700053058029bbe')
        self.assertFalse(IsLowDERSignature(sig))
    def test_low_s_value(self):
        sig = x('3045022100b135074e08cc93904a1712b2600d3cb01899a5b1cc7498caa4b8585bcf5f27e7022074ab544045285baef0a63f0fb4c95e577dcbf5c969c0bf47c7da8e478909d669')
        self.assertTrue(IsLowDERSignature(sig))
