#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Litecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the -mempoolreplacement (RBF) flag."""

from test_framework.messages import COIN, COutPoint, CTransaction, CTxIn, CTxOut
from test_framework.script import CScript, OP_DROP
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error, satoshi_round
from test_framework.ltc_util import make_utxo
from test_framework.script_util import DUMMY_P2WPKH_SCRIPT


class LtcReplaceByFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [
            ["-acceptnonstdtxn=1"],
            ["-acceptnonstdtxn=1", "-mempoolreplacement=1"]
        ]
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Leave IBD
        self.nodes[0].generate(1)

        self.test_rbf_flag()

    def test_rbf_flag(self):
        non_rbf_node = self.nodes[0]
        rbf_node = self.nodes[1]

        #
        # Create a non-opting in transaction
        #
        tx0_outpoint = make_utxo(self.nodes[0], int(1.1*COIN))

        tx1a = CTransaction()
        tx1a.vin = [CTxIn(tx0_outpoint, nSequence=0xffffffff)]
        tx1a.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        tx1a_hex = tx1a.serialize().hex()
        tx1a_txid = self.nodes[0].sendrawtransaction(tx1a_hex, 0)
        
        assert_equal(self.nodes[0].getmempoolentry(tx1a_txid)['bip125-replaceable'], False)
        
        #
        # Create a 2nd non-opting in transaction
        #
        tx1_outpoint = make_utxo(self.nodes[0], int(1.1*COIN))

        tx2a = CTransaction()
        tx2a.vin = [CTxIn(tx1_outpoint, nSequence=0xfffffffe)]
        tx2a.vout = [CTxOut(1 * COIN, DUMMY_P2WPKH_SCRIPT)]
        tx2a_hex = tx2a.serialize().hex()
        tx2a_txid = self.nodes[0].sendrawtransaction(tx2a_hex, 0)

        assert_equal(self.nodes[0].getmempoolentry(tx2a_txid)['bip125-replaceable'], False)

        # Now create a new transaction that spends from tx1a and tx2a
        # opt-in on one of the inputs
        tx1a_txid = int(tx1a_txid, 16)
        tx2a_txid = int(tx2a_txid, 16)

        tx3a = CTransaction()
        tx3a.vin = [CTxIn(COutPoint(tx1a_txid, 0), nSequence=0xffffffff),
                    CTxIn(COutPoint(tx2a_txid, 0), nSequence=0xfffffffd)]
        tx3a.vout = [CTxOut(int(0.9*COIN), CScript([b'c'])), CTxOut(int(0.9*COIN), CScript([b'd']))]
        tx3a_hex = tx3a.serialize().hex()

        tx3a_txid = self.nodes[0].sendrawtransaction(tx3a_hex, 0)
        
        self.sync_all()

        # This transaction is shown as replaceable
        assert_equal(self.nodes[0].getmempoolentry(tx3a_txid)['bip125-replaceable'], True)

        tx3b = CTransaction()
        tx3b.vin = [CTxIn(COutPoint(tx1a_txid, 0), nSequence=0)]
        tx3b.vout = [CTxOut(int(0.5 * COIN), DUMMY_P2WPKH_SCRIPT)]
        tx3b_hex = tx3b.serialize().hex()
        
        # Transaction should not be replaceable on non-RBF node
        assert_raises_rpc_error(-26, "txn-mempool-conflict", non_rbf_node.sendrawtransaction, tx3b_hex, 0)
        
        # Transaction should be replaceable on either input on RBF node
        rbf_node.sendrawtransaction(tx3b_hex, 0)

        tx3c = CTransaction()
        tx3c.vin = [CTxIn(COutPoint(tx2a_txid, 0), nSequence=0)]
        tx3c.vout = [CTxOut(int(0.5 * COIN), DUMMY_P2WPKH_SCRIPT)]
        tx3c_hex = tx3c.serialize().hex()

        # If tx3b was accepted, tx3c won't look like a replacement,
        # but make sure it is accepted anyway
        rbf_node.sendrawtransaction(tx3c_hex, 0)

if __name__ == '__main__':
    LtcReplaceByFeeTest().main()