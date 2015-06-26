#!/usr/bin/env python3
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test replace-by-fee
#

import os
import sys

# Add python-bitcoinlib to module search path:
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "python-bitcoinlib"))

import unittest

import bitcoin
bitcoin.SelectParams('regtest')

import bitcoin.rpc

from bitcoin.core import *
from bitcoin.core.script import *
from bitcoin.wallet import *

class Test_ReplaceByFee(unittest.TestCase):
    proxy = None

    @classmethod
    def setUpClass(cls):
        if cls.proxy is None:
            cls.proxy = bitcoin.rpc.Proxy()

    @classmethod
    def mine_mempool(cls):
        """Mine until mempool is empty"""
        mempool_size = 1
        while mempool_size:
            cls.proxy.generate(1)
            new_mempool_size = len(cls.proxy.getrawmempool())

            # It's possible to get stuck in a loop here if the mempool has
            # transactions that can't be mined.
            assert(new_mempool_size != mempool_size)
            mempool_size = new_mempool_size

    @classmethod
    def tearDownClass(cls):
        # Make sure mining works
        cls.mine_mempool()

    def make_txouts(self, n, amount, confirmed=True, scriptPubKey=CScript([1])):
        """Create n txouts with a given amount and scriptPubKey

        Mines coins as needed.

        confirmed - txouts created will be confirmed in the blockchain;
                    unconfirmed otherwise.
        """
        if not n:
            return []

        fee = 1*COIN
        while self.proxy.getbalance() < n*amount + fee:
            self.proxy.generate(100)

        addr = P2SHBitcoinAddress.from_redeemScript(CScript([]))
        txid = self.proxy.sendtoaddress(addr, n*amount + fee)

        tx1 = self.proxy.getrawtransaction(txid)

        i = None
        for i, txout in enumerate(tx1.vout):
            if txout.scriptPubKey == addr.to_scriptPubKey():
                break
        assert i is not None

        tx2 = CTransaction([CTxIn(COutPoint(txid, i), CScript([1, CScript([])]))],
                           [CTxOut(amount, scriptPubKey)]*n)

        tx2_txid = self.proxy.sendrawtransaction(tx2, True)

        # If requested, ensure txouts are confirmed.
        if confirmed:
            self.mine_mempool()

        return [COutPoint(tx2_txid, i) for i in range(n)]

    def make_txout(self, amount, confirmed=True, scriptPubKey=CScript([1])):
        """Create a txout with a given amount and scriptPubKey

        Mines coins as needed.

        confirmed - txouts created will be confirmed in the blockchain;
                    unconfirmed otherwise.
        """
        return self.make_txouts(1, amount, confirmed, scriptPubKey)[0]

    def test_subset_prohibited(self):
        """Replacement prohibited if any recipient will receive less funds from replacement"""

        for n in range(1,4):
            utxo1 = self.make_txout(n*0.11*COIN)
            utxo2 = self.make_txout(1.0*COIN)

            tx1 = CTransaction([CTxIn(utxo1)],
                                [CTxOut(0.10*COIN, CScript([i])) for i in range(n)])
            tx1_txid = self.proxy.sendrawtransaction(tx1, True)

            # Double spend with too-few outputs
            for m in range(1,n):
                tx2 = CTransaction([CTxIn(utxo1), CTxIn(utxo2)],
                                   tx1.vout[0:m])

                try:
                    tx2_txid = self.proxy.sendrawtransaction(tx2, True)
                except bitcoin.rpc.JSONRPCException as exp:
                    self.assertEqual(exp.error['code'], -26)
                else:
                    self.fail()

            # Double spend with one of the outputs changed
            for m in range(0,n):
                vout = list(tx1.vout)
                vout[m] = CTxOut(0.10*COIN, CScript([b'a']))
                tx2 = CTransaction([CTxIn(utxo1), CTxIn(utxo2)], vout)

                try:
                    tx2_txid = self.proxy.sendrawtransaction(tx2, True)
                except bitcoin.rpc.JSONRPCException as exp:
                    self.assertEqual(exp.error['code'], -26)
                else:
                    self.fail()

            # Double spend with one of the outputs reduced in value by one satoshi
            for m in range(0,n):
                vout = list(tx1.vout)
                vout[m] = CTxOut(vout[m].nValue-1, vout[m].scriptPubKey)
                tx2 = CTransaction([CTxIn(utxo1), CTxIn(utxo2)], vout)

                try:
                    tx2_txid = self.proxy.sendrawtransaction(tx2, True)
                except bitcoin.rpc.JSONRPCException as exp:
                    self.assertEqual(exp.error['code'], -26)
                else:
                    self.fail()

            # With all the outputs the same, and an additional input, the
            # replacement will succeed.
            tx2 = CTransaction([CTxIn(utxo1), CTxIn(utxo2)], tx1.vout)
            tx2_txid = self.proxy.sendrawtransaction(tx2, True)

            with self.assertRaises(IndexError):
                self.proxy.getrawtransaction(tx1_txid)
            self.proxy.getrawtransaction(tx2_txid)

    def test_reorder_vout_prohibited(self):
        """Changing the order of vout is prohibited"""

        utxo1 = self.make_txout(2*1.1*COIN)
        utxo2 = self.make_txout(1.0*COIN)

        tx1 = CTransaction([CTxIn(utxo1)],
                            [CTxOut(1.0*COIN, CScript([1])),
                             CTxOut(1.0*COIN, CScript([2]))])
        tx1_txid = self.proxy.sendrawtransaction(tx1, True)

        tx2 = CTransaction([CTxIn(utxo1), CTxIn(utxo2)],
                           reversed(tx1.vout))

        try:
            tx2_txid = self.proxy.sendrawtransaction(tx2, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26)
        else:
            self.fail()

    def test_reorder_vin_allowed(self):
        """Changing the order of vin is allowed"""
        utxo1 = self.make_txout(1.1*COIN)
        utxo2 = self.make_txout(1.0*COIN)

        tx1 = CTransaction([CTxIn(utxo1)],
                            [CTxOut(1.0*COIN, CScript([1]))])
        tx1_txid = self.proxy.sendrawtransaction(tx1, True)

        tx2 = CTransaction([CTxIn(utxo2), CTxIn(utxo1)],
                           tx1.vout)

        tx2_txid = self.proxy.sendrawtransaction(tx2, True)
        with self.assertRaises(IndexError):
            self.proxy.getrawtransaction(tx1_txid)
        self.proxy.getrawtransaction(tx2_txid)

    def test_one_to_one(self):
        """Replacing multiple transactions at once is prohibited"""
        utxo1 = self.make_txout(1.1*COIN)
        utxo2 = self.make_txout(1.1*COIN)
        utxo3 = self.make_txout(1.0*COIN)

        tx1a = CTransaction([CTxIn(utxo1)],
                            [CTxOut(1.0*COIN, CScript([b'a']))])
        tx1a_txid = self.proxy.sendrawtransaction(tx1a, True)

        tx1b = CTransaction([CTxIn(utxo2)],
                            [CTxOut(1.0*COIN, CScript([b'b']))])
        tx1b_txid = self.proxy.sendrawtransaction(tx1b, True)

        tx2 = CTransaction([CTxIn(utxo1), CTxIn(utxo2), CTxIn(utxo3)],
                           tx1a.vout + tx1b.vout)

        try:
            tx2_txid = self.proxy.sendrawtransaction(tx2, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26)
        else:
            self.fail()

    def test_replaced_outputs_unspent(self):
        """Replaced transaction's outputs must be unspent"""

        for i in range(2):
            utxo1 = self.make_txout(1.2*COIN)
            utxo2 = self.make_txout(3.0*COIN)

            tx1a = CTransaction([CTxIn(utxo1)],
                                [CTxOut(0.5*COIN, CScript([b'a'])),
                                 CTxOut(0.5*COIN, CScript([b'b']))])
            tx1a_txid = self.proxy.sendrawtransaction(tx1a, True)

            tx1b = CTransaction([CTxIn(COutPoint(tx1a_txid, i))],
                                [CTxOut(0.4*COIN, CScript([b'b']))])
            tx1b_txid = self.proxy.sendrawtransaction(tx1b, True)

            tx2 = CTransaction([CTxIn(utxo1), CTxIn(utxo2)],
                               tx1a.vout + tx1b.vout)

            try:
                tx2_txid = self.proxy.sendrawtransaction(tx2, True)
            except bitcoin.rpc.JSONRPCException as exp:
                self.assertEqual(exp.error['code'], -26)
            else:
                self.fail()

    def test_additional_unconfirmed_inputs(self):
        """Replacement fails if additional unconfirmed inputs added"""
        confirmed_utxo = self.make_txout(1.1*COIN)
        unconfirmed_utxo = self.make_txout(0.1*COIN, False)

        tx1 = CTransaction([CTxIn(confirmed_utxo)],
                           [CTxOut(1.0*COIN, CScript([b'a']))])
        tx1_txid = self.proxy.sendrawtransaction(tx1, True)

        tx2 = CTransaction([CTxIn(confirmed_utxo), CTxIn(unconfirmed_utxo)],
                           tx1.vout)

        try:
            tx2_txid = self.proxy.sendrawtransaction(tx2, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26)
        else:
            self.fail()

    def test_spends_of_conflicting_outputs(self):
        """Replacements that spend conflicting tx outputs are rejected"""
        utxo1 = self.make_txout(1.2*COIN)
        utxo2 = self.make_txout(3.0*COIN)

        tx1a = CTransaction([CTxIn(utxo1)],
                            [CTxOut(1.1*COIN, CScript([b'a']))])
        tx1a_txid = self.proxy.sendrawtransaction(tx1a, True)

        # Direct spend an output of the transaction we're replacing.
        tx2 = CTransaction([CTxIn(utxo1), CTxIn(utxo2),
                            CTxIn(COutPoint(tx1a_txid, 0))],
                           tx1a.vout)

        try:
            tx2_txid = self.proxy.sendrawtransaction(tx2, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26)
        else:
            self.fail()

        # Spend tx1a's output to test the indirect case.
        tx1b = CTransaction([CTxIn(COutPoint(tx1a_txid, 0))],
                            [CTxOut(1.0*COIN, CScript([b'a']))])
        tx1b_txid = self.proxy.sendrawtransaction(tx1b, True)

        tx2 = CTransaction([CTxIn(utxo1), CTxIn(utxo2),
                            CTxIn(COutPoint(tx1b_txid, 0))],
                           tx1a.vout)

        try:
            tx2_txid = self.proxy.sendrawtransaction(tx2, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26)
        else:
            self.fail()


    def test_economics(self):
        """Replacement prohibited if uneconomical"""
        utxo1 = self.make_txout(110000)
        utxo2 = self.make_txout(1)
        utxo3 = self.make_txout(1000000)

        # By including utxo2 in tx1 but not tx2 lets us test the case where the
        # replacement pays less fees than the original while still respecting
        # the "no-decreases" rule.
        tx1 = CTransaction([CTxIn(utxo1), CTxIn(utxo2)],
                            [CTxOut(100000, CScript([b'a']))])
        tx1_txid = self.proxy.sendrawtransaction(tx1, True)

        # FIXME: these constants should be derived somehow, but right now
        # there's no way to get min fee info from the mempool over RPC
        for fee_delta in [-1,   # less fees than original
                          0, 1, # not enough to pay for bandwidth
                          114, 1068,  # fee/KB of replacement less than original
                         ]:
            tx2 = CTransaction([CTxIn(utxo1), CTxIn(utxo3)],
                               [tx1.vout[0],
                                CTxOut(1000000-1-fee_delta, CScript([b'b']))])

            try:
                tx2_txid = self.proxy.sendrawtransaction(tx2, True)
            except bitcoin.rpc.JSONRPCException as exp:
                self.assertEqual(exp.error['code'], -26)
            else:
                self.fail()


    def test_very_large_replacements(self):
        """Very large replacements"""
        n = 5000
        for n_vin, n_new_vin, n_vout, n_new_vout in [(n,0,2,0),
                                                     (n,n,2,0),
                                                     (n,n,n,0),
                                                     (n,n,n,n),
                                                     (2,n,n,n),
                                                     (2,0,n,n),
                                                     (2,0,2,n),
                                                     ]:
            utxos = self.make_txouts(n_vin, 1000)

            fee_utxo1 = self.make_txout(0.01*COIN)
            fee_utxo2 = self.make_txout(10*COIN)
            new_utxos = tuple(CTxIn(utxo) for utxo in self.make_txouts(n_new_vin, 1000))

            tx1 = CTransaction([CTxIn(utxo) for utxo in utxos] + [CTxIn(fee_utxo1)],
                               [CTxOut(1, CScript([b'a'])) for i in range(n_vout)])
            tx1_txid = self.proxy.sendrawtransaction(tx1, True)

            new_vout = tuple(CTxOut(1, CScript([b'b'])) for i in range(n_new_vout))

            # Prohibited replacement, last vout omitted
            tx2 = CTransaction(tx1.vin + (CTxIn(fee_utxo2),) + new_utxos,
                               tx1.vout[0:-1] + new_vout)
            try:
                tx2_txid = self.proxy.sendrawtransaction(tx2, True)
            except bitcoin.rpc.JSONRPCException as exp:
                self.assertEqual(exp.error['code'], -26)
            else:
                self.fail()

            # Prohibited replacement, last vout changed
            tx2 = CTransaction(tx1.vin + (CTxIn(fee_utxo2),) + new_utxos,
                               tx1.vout[0:-1] + (CTxOut(1, CScript([b'b'])),) + new_vout)
            try:
                tx2_txid = self.proxy.sendrawtransaction(tx2, True)
            except bitcoin.rpc.JSONRPCException as exp:
                self.assertEqual(exp.error['code'], -26)
            else:
                self.fail()

            # Succesful replacement
            tx2 = CTransaction(tx1.vin + (CTxIn(fee_utxo2),) + new_utxos,
                               tx1.vout + new_vout)

            tx2_txid = self.proxy.sendrawtransaction(tx2, True)
            with self.assertRaises(IndexError):
                self.proxy.getrawtransaction(tx1_txid)
            self.proxy.getrawtransaction(tx2_txid)

if __name__ == '__main__':
    unittest.main()
