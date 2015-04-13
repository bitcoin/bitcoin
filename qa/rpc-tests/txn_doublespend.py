#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test proper accounting with malleable transactions
#

from test_framework import BitcoinTestFramework
from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from decimal import Decimal
from util import *
import os
import shutil

class TxnMallTest(BitcoinTestFramework):

    def add_options(self, parser):
        parser.add_option("--mineblock", dest="mine_block", default=False, action="store_true",
                          help="Test double-spend of 1-confirmed transaction")

    def setup_network(self):
        # Start with split network:
        return super(TxnMallTest, self).setup_network(True)

    def run_test(self):
        # All nodes should start with 1,250 BTC:
        starting_balance = 1250
        for i in range(4):
            assert_equal(self.nodes[i].getbalance(), starting_balance)
            self.nodes[i].getnewaddress("")  # bug workaround, coins generated assigned to first getnewaddress!
        
        # Assign coins to foo and bar accounts:
        self.nodes[0].move("", "foo", 1220)
        self.nodes[0].move("", "bar", 30)
        assert_equal(self.nodes[0].getbalance(""), 0)

        # Coins are sent to node1_address
        node1_address = self.nodes[1].getnewaddress("from0")

        # First: use raw transaction API to send 1210 BTC to node1_address,
        # but don't broadcast:
        (total_in, inputs) = gather_inputs(self.nodes[0], 1210)
        change_address = self.nodes[0].getnewaddress("foo")
        outputs = {}
        outputs[change_address] = 40
        outputs[node1_address] = 1210
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        doublespend = self.nodes[0].signrawtransaction(rawtx)
        assert_equal(doublespend["complete"], True)

        # Create two transaction from node[0] to node[1]; the
        # second must spend change from the first because the first
        # spends all mature inputs:
        txid1 = self.nodes[0].sendfrom("foo", node1_address, 1210, 0)
        txid2 = self.nodes[0].sendfrom("bar", node1_address, 20, 0)
        
        # Have node0 mine a block:
        if (self.options.mine_block):
            self.nodes[0].setgenerate(True, 1)
            sync_blocks(self.nodes[0:2])

        tx1 = self.nodes[0].gettransaction(txid1)
        tx2 = self.nodes[0].gettransaction(txid2)

        # Node0's balance should be starting balance, plus 50BTC for another
        # matured block, minus 1210, minus 20, and minus transaction fees:
        expected = starting_balance
        if self.options.mine_block: expected += 50
        expected += tx1["amount"] + tx1["fee"]
        expected += tx2["amount"] + tx2["fee"]
        assert_equal(self.nodes[0].getbalance(), expected)

        # foo and bar accounts should be debited:
        assert_equal(self.nodes[0].getbalance("foo"), 1220+tx1["amount"]+tx1["fee"])
        assert_equal(self.nodes[0].getbalance("bar"), 30+tx2["amount"]+tx2["fee"])

        if self.options.mine_block:
            assert_equal(tx1["confirmations"], 1)
            assert_equal(tx2["confirmations"], 1)
            # Node1's "from0" balance should be both transaction amounts:
            assert_equal(self.nodes[1].getbalance("from0"), -(tx1["amount"]+tx2["amount"]))
        else:
            assert_equal(tx1["confirmations"], 0)
            assert_equal(tx2["confirmations"], 0)
        
        # Now give doublespend to miner:
        mutated_txid = self.nodes[2].sendrawtransaction(doublespend["hex"])
        # ... mine a block...
        self.nodes[2].setgenerate(True, 1)

        # Reconnect the split network, and sync chain:
        connect_nodes(self.nodes[1], 2)
        self.nodes[2].setgenerate(True, 1)  # Mine another block to make sure we sync
        sync_blocks(self.nodes)

        # Re-fetch transaction info:
        tx1 = self.nodes[0].gettransaction(txid1)
        tx2 = self.nodes[0].gettransaction(txid2)
        
        # Both transactions should be conflicted
        assert_equal(tx1["confirmations"], -1)
        assert_equal(tx2["confirmations"], -1)

        # Node0's total balance should be starting balance, plus 100BTC for 
        # two more matured blocks, minus 1210 for the double-spend:
        expected = starting_balance + 100 - 1210
        assert_equal(self.nodes[0].getbalance(), expected)
        assert_equal(self.nodes[0].getbalance("*"), expected)

        # foo account should be debited, but bar account should not:
        assert_equal(self.nodes[0].getbalance("foo"), 1220-1210)
        assert_equal(self.nodes[0].getbalance("bar"), 30)

        # Node1's "from" account balance should be just the mutated send:
        assert_equal(self.nodes[1].getbalance("from0"), 1210)

if __name__ == '__main__':
    TxnMallTest().main()
