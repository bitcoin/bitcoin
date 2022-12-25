#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the abandontransaction RPC.

 The abandontransaction RPC marks a transaction and all its in-wallet
 descendants as abandoned which allows their inputs to be respent. It can be
 used to replace "stuck" or evicted transactions. It only works on transactions
 which are not included in a block and are not currently in the mempool. It has
 no effect on transactions which are already abandoned.
"""
from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class AbandonConflictTest(SyscoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-minrelaytxfee=0.00001"], []]
        # whitelist peers to speed up tx relay / mempool sync
        for args in self.extra_args:
            args.append("-whitelist=noban@127.0.0.1")

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # create two wallets to tests conflicts from both sender's and receiver's sides
        alice = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet(wallet_name="bob")
        bob = self.nodes[0].get_wallet_rpc("bob")

        self.generate(self.nodes[1], COINBASE_MATURITY)
        balance = alice.getbalance()
        txA = alice.sendtoaddress(alice.getnewaddress(), Decimal("10"))
        txB = alice.sendtoaddress(alice.getnewaddress(), Decimal("10"))
        txC = alice.sendtoaddress(alice.getnewaddress(), Decimal("10"))
        self.sync_mempools()
        self.generate(self.nodes[1], 1)

        # Can not abandon non-wallet transaction
        assert_raises_rpc_error(-5, 'Invalid or non-wallet transaction id', lambda: alice.abandontransaction(txid='ff' * 32))
        # Can not abandon confirmed transaction
        assert_raises_rpc_error(-5, 'Transaction not eligible for abandonment', lambda: alice.abandontransaction(txid=txA))

        newbalance = alice.getbalance()
        assert balance - newbalance < Decimal("0.001")  #no more than fees lost
        balance = newbalance

        # Disconnect nodes so node0's transactions don't get into node1's mempool
        self.disconnect_nodes(0, 1)

        # Identify the 10sys outputs
        nA = next(tx_out["vout"] for tx_out in alice.gettransaction(txA)["details"] if tx_out["amount"] == Decimal("10"))
        nB = next(tx_out["vout"] for tx_out in alice.gettransaction(txB)["details"] if tx_out["amount"] == Decimal("10"))
        nC = next(tx_out["vout"] for tx_out in alice.gettransaction(txC)["details"] if tx_out["amount"] == Decimal("10"))

        inputs = []
        # spend 10sys outputs from txA and txB
        inputs.append({"txid": txA, "vout": nA})
        inputs.append({"txid": txB, "vout": nB})
        outputs = {}

        outputs[alice.getnewaddress()] = Decimal("14.99998")
        outputs[bob.getnewaddress()] = Decimal("5")
        signed = alice.signrawtransactionwithwallet(alice.createrawtransaction(inputs, outputs))
        txAB1 = self.nodes[0].sendrawtransaction(signed["hex"])

        # Identify the 14.99998sys output
        nAB = next(tx_out["vout"] for tx_out in alice.gettransaction(txAB1)["details"] if tx_out["amount"] == Decimal("14.99998"))

        #Create a child tx spending AB1 and C
        inputs = []
        inputs.append({"txid": txAB1, "vout": nAB})
        inputs.append({"txid": txC, "vout": nC})
        outputs = {}
        outputs[alice.getnewaddress()] = Decimal("24.9996")
        signed2 = alice.signrawtransactionwithwallet(alice.createrawtransaction(inputs, outputs))
        txABC2 = self.nodes[0].sendrawtransaction(signed2["hex"])

        # Create a child tx spending ABC2
        signed3_change = Decimal("24.999")
        inputs = [{"txid": txABC2, "vout": 0}]
        outputs = {alice.getnewaddress(): signed3_change}
        signed3 = alice.signrawtransactionwithwallet(alice.createrawtransaction(inputs, outputs))
        # note tx is never directly referenced, only abandoned as a child of the above
        self.nodes[0].sendrawtransaction(signed3["hex"])

        # In mempool txs from self should increase balance from change
        newbalance = alice.getbalance()
        assert_equal(newbalance, balance - Decimal("30") + signed3_change)
        balance = newbalance

        # Restart the node with a higher min relay fee so the parent tx is no longer in mempool
        # TODO: redo with eviction
        self.restart_node(0, extra_args=["-minrelaytxfee=0.0001"])
        alice = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        assert self.nodes[0].getmempoolinfo()['loaded']

        # Verify txs no longer in either node's mempool
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        assert_equal(len(self.nodes[1].getrawmempool()), 0)

        # Not in mempool txs from self should only reduce balance
        # inputs are still spent, but change not received
        newbalance = alice.getbalance()
        assert_equal(newbalance, balance - signed3_change)
        # Unconfirmed received funds that are not in mempool, also shouldn't show
        # up in unconfirmed balance
        balances = alice.getbalances()['mine']
        assert_equal(balances['untrusted_pending'] + balances['trusted'], newbalance)
        # Also shouldn't show up in listunspent
        assert not txABC2 in [utxo["txid"] for utxo in alice.listunspent(0)]
        balance = newbalance

        # Abandon original transaction and verify inputs are available again
        # including that the child tx was also abandoned
        alice.abandontransaction(txAB1)
        newbalance = alice.getbalance()
        assert_equal(newbalance, balance + Decimal("30"))
        balance = newbalance

        self.log.info("Check abandoned transactions in listsinceblock")
        listsinceblock = alice.listsinceblock()
        txAB1_listsinceblock = [d for d in listsinceblock['transactions'] if d['txid'] == txAB1 and d['category'] == 'send']
        for tx in txAB1_listsinceblock:
            assert_equal(tx['abandoned'], True)
            assert_equal(tx['confirmations'], 0)
            assert_equal(tx['trusted'], False)

        # Verify that even with a low min relay fee, the tx is not reaccepted from wallet on startup once abandoned
        self.restart_node(0, extra_args=["-minrelaytxfee=0.00001"])
        alice = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        assert self.nodes[0].getmempoolinfo()['loaded']

        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        assert_equal(alice.getbalance(), balance)

        # But if it is received again then it is unabandoned
        # And since now in mempool, the change is available
        # But its child tx remains abandoned
        self.nodes[0].sendrawtransaction(signed["hex"])
        newbalance = alice.getbalance()
        assert_equal(newbalance, balance - Decimal("20") + Decimal("14.99998"))
        balance = newbalance

        # Send child tx again so it is unabandoned
        self.nodes[0].sendrawtransaction(signed2["hex"])
        newbalance = alice.getbalance()
        assert_equal(newbalance, balance - Decimal("10") - Decimal("14.99998") + Decimal("24.9996"))
        balance = newbalance

        # Remove using high relay fee again
        self.restart_node(0, extra_args=["-minrelaytxfee=0.0001"])
        alice = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        assert self.nodes[0].getmempoolinfo()['loaded']
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        newbalance = alice.getbalance()
        assert_equal(newbalance, balance - Decimal("24.9996"))
        balance = newbalance

        self.log.info("Test transactions conflicted by a double spend")
        self.nodes[0].loadwallet("bob")
        bob = self.nodes[0].get_wallet_rpc("bob")

        # Create a double spend of AB1 by spending again from only A's 10 output
        # Mine double spend from node 1
        inputs = []
        inputs.append({"txid": txA, "vout": nA})
        outputs = {}
        outputs[self.nodes[1].getnewaddress()] = Decimal("3.9999")
        outputs[bob.getnewaddress()] = Decimal("5.9999")
        tx = alice.createrawtransaction(inputs, outputs)
        signed = alice.signrawtransactionwithwallet(tx)
        double_spend_txid = self.nodes[1].sendrawtransaction(signed["hex"])
        self.connect_nodes(0, 1)
        self.generate(self.nodes[1], 1)

        tx_list = alice.listtransactions()

        conflicted = [tx for tx in tx_list if tx["confirmations"] < 0]
        assert_equal(4, len(conflicted))

        wallet_conflicts = [tx for tx in conflicted if tx["walletconflicts"]]
        assert_equal(2, len(wallet_conflicts))

        double_spends = [tx for tx in tx_list if tx["walletconflicts"] and tx["confirmations"] > 0]
        assert_equal(2, len(double_spends))  # one for each output
        double_spend = double_spends[0]

        # Test the properties of the conflicted transactions, i.e. with confirmations < 0.
        for tx in conflicted:
            assert_equal(tx["abandoned"], False)
            assert_equal(tx["confirmations"], -1)
            assert_equal(tx["trusted"], False)

        # Test the properties of the double-spend transaction, i.e. having wallet conflicts and confirmations > 0.
        assert_equal(double_spend["abandoned"], False)
        assert_equal(double_spend["confirmations"], 1)
        assert "trusted" not in double_spend.keys()  # "trusted" only returned if tx has 0 or negative confirmations.

        # Test the walletconflicts field of each.
        for tx in wallet_conflicts:
            assert_equal(double_spend["walletconflicts"], [tx["txid"]])
            assert_equal(tx["walletconflicts"], [double_spend["txid"]])

        # Test walletconflicts on the receiver's side
        txinfo = bob.gettransaction(txAB1)
        assert_equal(txinfo['confirmations'], -1)
        assert_equal(txinfo['walletconflicts'], [double_spend['txid']])

        double_spends = [tx for tx in bob.listtransactions() if tx["walletconflicts"] and tx["confirmations"] > 0]
        assert_equal(1, len(double_spends))
        double_spend = double_spends[0]
        assert_equal(double_spend_txid, double_spend['txid'])
        assert_equal(double_spend["walletconflicts"], [txAB1])

        # Verify that B and C's 10 SYS outputs are available for spending again because AB1 is now conflicted
        newbalance = alice.getbalance()
        assert_equal(newbalance, balance + Decimal("20"))
        balance = newbalance

        # There is currently a minor bug around this and so this test doesn't work.  See Issue #7315
        # Invalidate the block with the double spend and B's 10 SYS output should no longer be available
        # Don't think C's should either
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        newbalance = alice.getbalance()
        #assert_equal(newbalance, balance - Decimal("10"))
        self.log.info("If balance has not declined after invalidateblock then out of mempool wallet tx which is no longer")
        self.log.info("conflicted has not resumed causing its inputs to be seen as spent.  See Issue #7315")
        assert_equal(balance, newbalance)


if __name__ == '__main__':
    AbandonConflictTest().main()
