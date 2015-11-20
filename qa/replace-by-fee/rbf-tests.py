#!/usr/bin/env python3
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test replace-by-fee
#

import os
import sys

# Add python-bitcoinlib to module search path, prior to any system-wide
# python-bitcoinlib.
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "python-bitcoinlib"))

import unittest

import bitcoin
bitcoin.SelectParams('regtest')

import bitcoin.rpc

from bitcoin.core import *
from bitcoin.core.script import *
from bitcoin.wallet import *

MAX_REPLACEMENT_LIMIT = 100

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
            cls.proxy.call('generate', 1)
            new_mempool_size = len(cls.proxy.getrawmempool())

            # It's possible to get stuck in a loop here if the mempool has
            # transactions that can't be mined.
            assert(new_mempool_size != mempool_size)
            mempool_size = new_mempool_size

    @classmethod
    def tearDownClass(cls):
        # Make sure mining works
        cls.mine_mempool()

    def make_txout(self, amount, confirmed=True, scriptPubKey=CScript([1])):
        """Create a txout with a given amount and scriptPubKey

        Mines coins as needed.

        confirmed - txouts created will be confirmed in the blockchain;
                    unconfirmed otherwise.
        """
        fee = 1*COIN
        while self.proxy.getbalance() < amount + fee:
            self.proxy.call('generate', 100)

        addr = P2SHBitcoinAddress.from_redeemScript(CScript([]))
        txid = self.proxy.sendtoaddress(addr, amount + fee)

        tx1 = self.proxy.getrawtransaction(txid)

        i = None
        for i, txout in enumerate(tx1.vout):
            if txout.scriptPubKey == addr.to_scriptPubKey():
                break
        assert i is not None

        tx2 = CTransaction([CTxIn(COutPoint(txid, i), CScript([1, CScript([])]), nSequence=0)],
                           [CTxOut(amount, scriptPubKey)])

        tx2_txid = self.proxy.sendrawtransaction(tx2, True)

        # If requested, ensure txouts are confirmed.
        if confirmed:
            self.mine_mempool()

        return COutPoint(tx2_txid, 0)

    def test_simple_doublespend(self):
        """Simple doublespend"""
        tx0_outpoint = self.make_txout(1.1*COIN)

        tx1a = CTransaction([CTxIn(tx0_outpoint, nSequence=0)],
                            [CTxOut(1*COIN, CScript([b'a']))])
        tx1a_txid = self.proxy.sendrawtransaction(tx1a, True)

        # Should fail because we haven't changed the fee
        tx1b = CTransaction([CTxIn(tx0_outpoint, nSequence=0)],
                            [CTxOut(1*COIN, CScript([b'b']))])

        try:
            tx1b_txid = self.proxy.sendrawtransaction(tx1b, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26) # insufficient fee
        else:
            self.fail()

        # Extra 0.1 BTC fee
        tx1b = CTransaction([CTxIn(tx0_outpoint, nSequence=0)],
                            [CTxOut(0.9*COIN, CScript([b'b']))])
        tx1b_txid = self.proxy.sendrawtransaction(tx1b, True)

        # tx1a is in fact replaced
        with self.assertRaises(IndexError):
            self.proxy.getrawtransaction(tx1a_txid)

        self.assertEqual(tx1b, self.proxy.getrawtransaction(tx1b_txid))

    def test_doublespend_chain(self):
        """Doublespend of a long chain"""

        initial_nValue = 50*COIN
        tx0_outpoint = self.make_txout(initial_nValue)

        prevout = tx0_outpoint
        remaining_value = initial_nValue
        chain_txids = []
        while remaining_value > 10*COIN:
            remaining_value -= 1*COIN
            tx = CTransaction([CTxIn(prevout, nSequence=0)],
                              [CTxOut(remaining_value, CScript([1]))])
            txid = self.proxy.sendrawtransaction(tx, True)
            chain_txids.append(txid)
            prevout = COutPoint(txid, 0)

        # Whether the double-spend is allowed is evaluated by including all
        # child fees - 40 BTC - so this attempt is rejected.
        dbl_tx = CTransaction([CTxIn(tx0_outpoint, nSequence=0)],
                              [CTxOut(initial_nValue - 30*COIN, CScript([1]))])

        try:
            self.proxy.sendrawtransaction(dbl_tx, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26) # insufficient fee
        else:
            self.fail()

        # Accepted with sufficient fee
        dbl_tx = CTransaction([CTxIn(tx0_outpoint, nSequence=0)],
                              [CTxOut(1*COIN, CScript([1]))])
        self.proxy.sendrawtransaction(dbl_tx, True)

        for doublespent_txid in chain_txids:
            with self.assertRaises(IndexError):
                self.proxy.getrawtransaction(doublespent_txid)

    def test_doublespend_tree(self):
        """Doublespend of a big tree of transactions"""

        initial_nValue = 50*COIN
        tx0_outpoint = self.make_txout(initial_nValue)

        def branch(prevout, initial_value, max_txs, *, tree_width=5, fee=0.0001*COIN, _total_txs=None):
            if _total_txs is None:
                _total_txs = [0]
            if _total_txs[0] >= max_txs:
                return

            txout_value = (initial_value - fee) // tree_width
            if txout_value < fee:
                return

            vout = [CTxOut(txout_value, CScript([i+1]))
                    for i in range(tree_width)]
            tx = CTransaction([CTxIn(prevout, nSequence=0)],
                              vout)

            self.assertTrue(len(tx.serialize()) < 100000)
            txid = self.proxy.sendrawtransaction(tx, True)
            yield tx
            _total_txs[0] += 1

            for i, txout in enumerate(tx.vout):
                yield from branch(COutPoint(txid, i), txout_value,
                                  max_txs,
                                  tree_width=tree_width, fee=fee,
                                  _total_txs=_total_txs)

        fee = 0.0001*COIN
        n = MAX_REPLACEMENT_LIMIT
        tree_txs = list(branch(tx0_outpoint, initial_nValue, n, fee=fee))
        self.assertEqual(len(tree_txs), n)

        # Attempt double-spend, will fail because too little fee paid
        dbl_tx = CTransaction([CTxIn(tx0_outpoint, nSequence=0)],
                              [CTxOut(initial_nValue - fee*n, CScript([1]))])
        try:
            self.proxy.sendrawtransaction(dbl_tx, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26) # insufficient fee
        else:
            self.fail()

        # 1 BTC fee is enough
        dbl_tx = CTransaction([CTxIn(tx0_outpoint, nSequence=0)],
                              [CTxOut(initial_nValue - fee*n - 1*COIN, CScript([1]))])
        self.proxy.sendrawtransaction(dbl_tx, True)

        for tx in tree_txs:
            with self.assertRaises(IndexError):
                self.proxy.getrawtransaction(tx.GetHash())

        # Try again, but with more total transactions than the "max txs
        # double-spent at once" anti-DoS limit.
        for n in (MAX_REPLACEMENT_LIMIT, MAX_REPLACEMENT_LIMIT*2):
            fee = 0.0001*COIN
            tx0_outpoint = self.make_txout(initial_nValue)
            tree_txs = list(branch(tx0_outpoint, initial_nValue, n, fee=fee))
            self.assertEqual(len(tree_txs), n)

            dbl_tx = CTransaction([CTxIn(tx0_outpoint, nSequence=0)],
                                  [CTxOut(initial_nValue - fee*n, CScript([1]))])
            try:
                self.proxy.sendrawtransaction(dbl_tx, True)
            except bitcoin.rpc.JSONRPCException as exp:
                self.assertEqual(exp.error['code'], -26)
            else:
                self.fail()

            for tx in tree_txs:
                self.proxy.getrawtransaction(tx.GetHash())

    def test_replacement_feeperkb(self):
        """Replacement requires fee-per-KB to be higher"""
        tx0_outpoint = self.make_txout(1.1*COIN)

        tx1a = CTransaction([CTxIn(tx0_outpoint, nSequence=0)],
                            [CTxOut(1*COIN, CScript([b'a']))])
        tx1a_txid = self.proxy.sendrawtransaction(tx1a, True)

        # Higher fee, but the fee per KB is much lower, so the replacement is
        # rejected.
        tx1b = CTransaction([CTxIn(tx0_outpoint, nSequence=0)],
                            [CTxOut(0.001*COIN,
                                    CScript([b'a'*999000]))])

        try:
            tx1b_txid = self.proxy.sendrawtransaction(tx1b, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26) # insufficient fee
        else:
            self.fail()

    def test_spends_of_conflicting_outputs(self):
        """Replacements that spend conflicting tx outputs are rejected"""
        utxo1 = self.make_txout(1.2*COIN)
        utxo2 = self.make_txout(3.0*COIN)

        tx1a = CTransaction([CTxIn(utxo1, nSequence=0)],
                            [CTxOut(1.1*COIN, CScript([b'a']))])
        tx1a_txid = self.proxy.sendrawtransaction(tx1a, True)

        # Direct spend an output of the transaction we're replacing.
        tx2 = CTransaction([CTxIn(utxo1, nSequence=0), CTxIn(utxo2, nSequence=0),
                            CTxIn(COutPoint(tx1a_txid, 0), nSequence=0)],
                           tx1a.vout)

        try:
            tx2_txid = self.proxy.sendrawtransaction(tx2, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26)
        else:
            self.fail()

        # Spend tx1a's output to test the indirect case.
        tx1b = CTransaction([CTxIn(COutPoint(tx1a_txid, 0), nSequence=0)],
                            [CTxOut(1.0*COIN, CScript([b'a']))])
        tx1b_txid = self.proxy.sendrawtransaction(tx1b, True)

        tx2 = CTransaction([CTxIn(utxo1, nSequence=0), CTxIn(utxo2, nSequence=0),
                            CTxIn(COutPoint(tx1b_txid, 0))],
                           tx1a.vout)

        try:
            tx2_txid = self.proxy.sendrawtransaction(tx2, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26)
        else:
            self.fail()

    def test_new_unconfirmed_inputs(self):
        """Replacements that add new unconfirmed inputs are rejected"""
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

    def test_too_many_replacements(self):
        """Replacements that evict too many transactions are rejected"""
        # Try directly replacing more than MAX_REPLACEMENT_LIMIT
        # transactions

        # Start by creating a single transaction with many outputs
        initial_nValue = 10*COIN
        utxo = self.make_txout(initial_nValue)
        fee = 0.0001*COIN
        split_value = int((initial_nValue-fee)/(MAX_REPLACEMENT_LIMIT+1))
        actual_fee = initial_nValue - split_value*(MAX_REPLACEMENT_LIMIT+1)

        outputs = []
        for i in range(MAX_REPLACEMENT_LIMIT+1):
            outputs.append(CTxOut(split_value, CScript([1])))

        splitting_tx = CTransaction([CTxIn(utxo, nSequence=0)], outputs)
        txid = self.proxy.sendrawtransaction(splitting_tx, True)

        # Now spend each of those outputs individually
        for i in range(MAX_REPLACEMENT_LIMIT+1):
            tx_i = CTransaction([CTxIn(COutPoint(txid, i), nSequence=0)],
                                [CTxOut(split_value-fee, CScript([b'a']))])
            self.proxy.sendrawtransaction(tx_i, True)

        # Now create doublespend of the whole lot, should fail
        # Need a big enough fee to cover all spending transactions and have
        # a higher fee rate
        double_spend_value = (split_value-100*fee)*(MAX_REPLACEMENT_LIMIT+1)
        inputs = []
        for i in range(MAX_REPLACEMENT_LIMIT+1):
            inputs.append(CTxIn(COutPoint(txid, i), nSequence=0))
        double_tx = CTransaction(inputs, [CTxOut(double_spend_value, CScript([b'a']))])

        try:
            self.proxy.sendrawtransaction(double_tx, True)
        except bitcoin.rpc.JSONRPCException as exp:
            self.assertEqual(exp.error['code'], -26)
            self.assertEqual("too many potential replacements" in exp.error['message'], True)
        else:
            self.fail()

        # If we remove an input, it should pass
        double_tx = CTransaction(inputs[0:-1],
                                 [CTxOut(double_spend_value, CScript([b'a']))])

        self.proxy.sendrawtransaction(double_tx, True)

if __name__ == '__main__':
    unittest.main()
