#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
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
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class AbandonConflictTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-minrelaytxfee=0.00001"], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.generate(self.nodes[1], COINBASE_MATURITY)
        self.sync_blocks()
        balance = self.nodes[0].getbalance()
        txA = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10"))
        txB = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10"))
        txC = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), Decimal("10"))
        self.sync_mempools()
        self.generate(self.nodes[1], 1)

        # Can not abandon non-wallet transaction
        assert_raises_rpc_error(-5, 'Invalid or non-wallet transaction id', lambda: self.nodes[0].abandontransaction(txid='ff' * 32))
        # Can not abandon confirmed transaction
        assert_raises_rpc_error(-5, 'Transaction not eligible for abandonment', lambda: self.nodes[0].abandontransaction(txid=txA))

        self.sync_blocks()
        newbalance = self.nodes[0].getbalance()
        assert balance - newbalance < Decimal("0.001")  #no more than fees lost
        balance = newbalance

        # Disconnect nodes so node0's transactions don't get into node1's mempool
        self.disconnect_nodes(0, 1)

        # Identify the 10btc outputs
        nA = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(txA)["details"] if tx_out["amount"] == Decimal("10"))
        nB = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(txB)["details"] if tx_out["amount"] == Decimal("10"))
        nC = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(txC)["details"] if tx_out["amount"] == Decimal("10"))

        inputs = []
        # spend 10btc outputs from txA and txB
        inputs.append({"txid": txA, "vout": nA})
        inputs.append({"txid": txB, "vout": nB})
        outputs = {}

        outputs[self.nodes[0].getnewaddress()] = Decimal("14.99998")
        outputs[self.nodes[1].getnewaddress()] = Decimal("5")
        signed = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs, outputs))
        txAB1 = self.nodes[0].sendrawtransaction(signed["hex"])

        # Identify the 14.99998btc output
        nAB = next(tx_out["vout"] for tx_out in self.nodes[0].gettransaction(txAB1)["details"] if tx_out["amount"] == Decimal("14.99998"))

        #Create a child tx spending AB1 and C
        inputs = []
        inputs.append({"txid": txAB1, "vout": nAB})
        inputs.append({"txid": txC, "vout": nC})
        outputs = {}
        outputs[self.nodes[0].getnewaddress()] = Decimal("24.9996")
        signed2 = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs, outputs))
        txABC2 = self.nodes[0].sendrawtransaction(signed2["hex"])

        # Create a child tx spending ABC2
        signed3_change = Decimal("24.999")
        inputs = [{"txid": txABC2, "vout": 0}]
        outputs = {self.nodes[0].getnewaddress(): signed3_change}
        signed3 = self.nodes[0].signrawtransactionwithwallet(self.nodes[0].createrawtransaction(inputs, outputs))
        # note tx is never directly referenced, only abandoned as a child of the above
        self.nodes[0].sendrawtransaction(signed3["hex"])

        # In mempool txs from self should increase balance from change
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance - Decimal("30") + signed3_change)
        balance = newbalance

        # Restart the node with a higher min relay fee so the parent tx is no longer in mempool
        # TODO: redo with eviction
        self.restart_node(0, extra_args=["-minrelaytxfee=0.0001"])
        assert self.nodes[0].getmempoolinfo()['loaded']

        # Verify txs no longer in either node's mempool
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        assert_equal(len(self.nodes[1].getrawmempool()), 0)

        # Not in mempool txs from self should only reduce balance
        # inputs are still spent, but change not received
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance - signed3_change)
        # Unconfirmed received funds that are not in mempool, also shouldn't show
        # up in unconfirmed balance
        balances = self.nodes[0].getbalances()['mine']
        assert_equal(balances['untrusted_pending'] + balances['trusted'], newbalance)
        # Also shouldn't show up in listunspent
        assert not txABC2 in [utxo["txid"] for utxo in self.nodes[0].listunspent(0)]
        balance = newbalance

        # Abandon original transaction and verify inputs are available again
        # including that the child tx was also abandoned
        self.nodes[0].abandontransaction(txAB1)
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance + Decimal("30"))
        balance = newbalance

        # Verify that even with a low min relay fee, the tx is not reaccepted from wallet on startup once abandoned
        self.restart_node(0, extra_args=["-minrelaytxfee=0.00001"])
        assert self.nodes[0].getmempoolinfo()['loaded']

        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        assert_equal(self.nodes[0].getbalance(), balance)

        # But if it is received again then it is unabandoned
        # And since now in mempool, the change is available
        # But its child tx remains abandoned
        self.nodes[0].sendrawtransaction(signed["hex"])
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance - Decimal("20") + Decimal("14.99998"))
        balance = newbalance

        # Send child tx again so it is unabandoned
        self.nodes[0].sendrawtransaction(signed2["hex"])
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance - Decimal("10") - Decimal("14.99998") + Decimal("24.9996"))
        balance = newbalance

        # Remove using high relay fee again
        self.restart_node(0, extra_args=["-minrelaytxfee=0.0001"])
        assert self.nodes[0].getmempoolinfo()['loaded']
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance - Decimal("24.9996"))
        balance = newbalance

        self.log.info("Test transactions conflicted by a double spend")
        # Create a double spend of AB1 by spending again from only A's 10 output
        # Mine double spend from node 1
        inputs = []
        inputs.append({"txid": txA, "vout": nA})
        outputs = {}
        outputs[self.nodes[1].getnewaddress()] = Decimal("9.9999")
        tx = self.nodes[0].createrawtransaction(inputs, outputs)
        signed = self.nodes[0].signrawtransactionwithwallet(tx)
        self.nodes[1].sendrawtransaction(signed["hex"])
        self.generate(self.nodes[1], 1)

        self.connect_nodes(0, 1)
        self.sync_blocks()

        tx_list = self.nodes[0].listtransactions()

        conflicted = [tx for tx in tx_list if tx["confirmations"] < 0]
        assert_equal(4, len(conflicted))

        wallet_conflicts = [tx for tx in conflicted if tx["walletconflicts"]]
        assert_equal(2, len(wallet_conflicts))

        double_spends = [tx for tx in tx_list if tx["walletconflicts"] and tx["confirmations"] > 0]
        assert_equal(1, len(double_spends))
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

        # Verify that B and C's 10 BTC outputs are available for spending again because AB1 is now conflicted
        newbalance = self.nodes[0].getbalance()
        assert_equal(newbalance, balance + Decimal("20"))
        balance = newbalance

        # There is currently a minor bug around this and so this test doesn't work.  See Issue #7315
        # Invalidate the block with the double spend and B's 10 BTC output should no longer be available
        # Don't think C's should either
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        newbalance = self.nodes[0].getbalance()
        #assert_equal(newbalance, balance - Decimal("10"))
        self.log.info("If balance has not declined after invalidateblock then out of mempool wallet tx which is no longer")
        self.log.info("conflicted has not resumed causing its inputs to be seen as spent.  See Issue #7315")
        self.log.info(str(balance) + " -> " + str(newbalance) + " ?")


if __name__ == '__main__':
    AbandonConflictTest().main()
