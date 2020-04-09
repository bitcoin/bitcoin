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
import unittest
import os

from bitcoin.core import *
from bitcoin.core.scripteval import VerifyScript, SCRIPT_VERIFY_P2SH

from bitcoin.tests.test_scripteval import parse_script

def load_test_vectors(name):
    with open(os.path.dirname(__file__) + '/data/' + name, 'r') as fd:
        for test_case in json.load(fd):
            # Comments designated by single length strings
            if len(test_case) == 1:
                continue
            assert len(test_case) == 3

            prevouts = {}
            for json_prevout in test_case[0]:
                assert len(json_prevout) == 3
                n = json_prevout[1]
                if n == -1:
                    n = 0xffffffff
                prevout = COutPoint(lx(json_prevout[0]), n)
                prevouts[prevout] = parse_script(json_prevout[2])

            tx = CTransaction.deserialize(x(test_case[1]))
            enforceP2SH = test_case[2]

            yield (prevouts, tx, enforceP2SH)

class Test_COutPoint(unittest.TestCase):
    def test_is_null(self):
        self.assertTrue(COutPoint().is_null())
        self.assertTrue(COutPoint(hash=b'\x00'*32,n=0xffffffff).is_null())
        self.assertFalse(COutPoint(hash=b'\x00'*31 + b'\x01').is_null())
        self.assertFalse(COutPoint(n=1).is_null())

    def test_repr(self):
        def T(outpoint, expected):
            actual = repr(outpoint)
            self.assertEqual(actual, expected)
        T( COutPoint(),
          'COutPoint()')
        T( COutPoint(lx('4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b'), 0),
          "COutPoint(lx('4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b'), 0)")

    def test_str(self):
        def T(outpoint, expected):
            actual = str(outpoint)
            self.assertEqual(actual, expected)
        T(COutPoint(),
          '0000000000000000000000000000000000000000000000000000000000000000:4294967295')
        T(COutPoint(lx('4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b'), 0),
                       '4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b:0')
        T(COutPoint(lx('4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b'), 10),
                       '4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b:10')

class Test_CMutableOutPoint(unittest.TestCase):
    def test_GetHash(self):
        """CMutableOutPoint.GetHash() is not cached"""
        outpoint = CMutableOutPoint()

        h1 = outpoint.GetHash()
        outpoint.n = 1

        self.assertNotEqual(h1, outpoint.GetHash())


class Test_CTxIn(unittest.TestCase):
    def test_is_final(self):
        self.assertTrue(CTxIn().is_final())
        self.assertTrue(CTxIn(nSequence=0xffffffff).is_final())
        self.assertFalse(CTxIn(nSequence=0).is_final())

    def test_repr(self):
        def T(txin, expected):
            actual = repr(txin)
            self.assertEqual(actual, expected)
        T( CTxIn(),
          'CTxIn(COutPoint(), CScript([]), 0xffffffff)')

class Test_CMutableTxIn(unittest.TestCase):
    def test_GetHash(self):
        """CMutableTxIn.GetHash() is not cached"""
        txin = CMutableTxIn()

        h1 = txin.GetHash()
        txin.prevout.n = 1

        self.assertNotEqual(h1, txin.GetHash())

class Test_CTransaction(unittest.TestCase):
    def test_is_coinbase(self):
        tx = CMutableTransaction()
        self.assertFalse(tx.is_coinbase())

        tx.vin.append(CMutableTxIn())

        # IsCoinBase() in reference client doesn't check if vout is empty
        self.assertTrue(tx.is_coinbase())

        tx.vin[0].prevout.n = 0
        self.assertFalse(tx.is_coinbase())

        tx.vin[0] = CTxIn()
        tx.vin.append(CTxIn())
        self.assertFalse(tx.is_coinbase())

    def test_tx_valid(self):
        for prevouts, tx, enforceP2SH in load_test_vectors('tx_valid.json'):
            try:
                CheckTransaction(tx)
            except CheckTransactionError:
                self.fail('tx failed CheckTransaction(): ' \
                        + str((prevouts, b2x(tx.serialize()), enforceP2SH)))
                continue

            for i in range(len(tx.vin)):
                flags = set()
                if enforceP2SH:
                    flags.add(SCRIPT_VERIFY_P2SH)

                VerifyScript(tx.vin[i].scriptSig, prevouts[tx.vin[i].prevout], tx, i, flags=flags)


    def test_tx_invalid(self):
        for prevouts, tx, enforceP2SH in load_test_vectors('tx_invalid.json'):
            try:
                CheckTransaction(tx)
            except CheckTransactionError:
                continue

            with self.assertRaises(ValidationError):
                for i in range(len(tx.vin)):
                    flags = set()
                    if enforceP2SH:
                        flags.add(SCRIPT_VERIFY_P2SH)

                    VerifyScript(tx.vin[i].scriptSig, prevouts[tx.vin[i].prevout], tx, i, flags=flags)
