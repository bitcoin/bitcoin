#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test long chained transactions in the wallet code."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (connect_nodes_bi,
                                 assert_equal,
                                )

class WalletTestLongchain(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4

    def setup_network(self):
        self.add_nodes(3)
        self.start_node(0)
        self.start_node(1)
        self.start_node(2)
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        self.sync_all([self.nodes[0:3]])

    def run_test(self):

        # Check that there's no UTXO on any of the nodes
        assert_equal(len(self.nodes[0].listunspent()), 0)
        assert_equal(len(self.nodes[1].listunspent()), 0)

        self.log.info("Mining blocks...")

        self.nodes[0].generate(101)

        walletinfo = self.nodes[0].getwalletinfo()
        assert_equal(walletinfo['immature_balance'], 5000)
        assert_equal(walletinfo['balance'], 50)

        self.sync_all()

        assert_equal(self.nodes[0].getbalance(), 50)
        assert_equal(self.nodes[1].getbalance(), 0)

        # Check that only first node has UTXOs
        utxos = self.nodes[0].listunspent()
        assert_equal(len(utxos), 1)
        assert_equal(len(self.nodes[1].listunspent()), 0)

        # Send 0.1 BTC 100 times (=10 BTC) to node 2; we will eventually hit the mempool chain limit
        for i in range(100):
            try:
                self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.1)
            except:
                break
        # After failure, we should have at least ~39 BTC left, as we would have at most sent 10*0.1
        # (plus 100 tx fees)
        assert(self.nodes[0].getbalance() > 25)
        self.sync_all()
        self.nodes[1].generate(1)
        self.sync_all()
        # After generating, we should be able to send again
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 39)

if __name__ == '__main__':
    WalletTestLongchain().main()
