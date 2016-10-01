#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *


class TxOutsByAddressTest(BitcoinTestFramework):
    """Tests -txoutindex"""

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [["-txoutindex"], [], []]

    def setup_network(self, split=False):
        print("Setup network...")
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, self.extra_args)
        connect_nodes(self.nodes[1], 0)
        connect_nodes(self.nodes[2], 0)
        self.is_network_split = False

    def run_test(self):
        print("Generating test blockchain...")

        # Check that there's no UTXO on any of the nodes
        for node in self.nodes:
            assert_equal(len(node.listunspent()), 0)

        # mining
        self.nodes[0].generate(101)
        self.sync_all()
        assert_equal(self.nodes[0].getbalance(), 50)

        # TX1: send from node0 to node1
        # - check if txout from tx1 is there
        address = self.nodes[1].getnewaddress()
        txid1 = self.nodes[0].sendtoaddress(address, 10)
        self.nodes[0].generate(101) # node will collect its own fee
        self.sync_all()
        assert_equal(self.nodes[0].getbalance(), 5090)
        assert_equal(self.nodes[1].getbalance(), 10)
        txouts = self.nodes[0].gettxoutsbyaddress(1, (address,))
        txid = txouts[0]["txid"]
        assert_is_hash_string(txid)
        assert_equal(txid, txid1)

        # stop node 2
        stop_node(self.nodes[2], 2)
        self.nodes.pop()

        # TX2: send from node1 to node0
        # - check if txout from tx1 is gone
        # - check if txout from tx2 is there
        address2 = self.nodes[0].getnewaddress()
        txid2 = self.nodes[1].sendtoaddress(address2, 5)
        self.nodes[1].generate(1)
        self.sync_all()
        assert_equal(self.nodes[0].getbalance(), 5145)
        txouts = self.nodes[0].gettxoutsbyaddress(1, (address,))
        assert_equal(txouts, [])
        txouts = self.nodes[0].gettxoutsbyaddress(1, (address2,))
        txid = txouts[0]["txid"]
        assert_is_hash_string(txid)
        assert_equal(txid, txid2)

        # start node 2
        self.nodes.append(start_node(2, self.options.tmpdir, self.extra_args[2]))

        # mine 10 blocks alone to have the longest chain
        self.nodes[2].generate(10)
        connect_nodes(self.nodes[0], 2)
        connect_nodes(self.nodes[1], 2)
        self.nodes[2].generate(1)
        sync_blocks(self.nodes)

        # TX2 must be reverted
        # - check if txout from tx1 is there again
        # - check if txout from tx2 is gone
        assert_equal(self.nodes[0].getbalance(), 5640)
        txouts = self.nodes[0].gettxoutsbyaddress(1, (address,))
        txid = txouts[0]["txid"]
        assert_is_hash_string(txid)
        assert_equal(txid, txid1)
        txouts = self.nodes[0].gettxoutsbyaddress(1, (address2,))
        assert_equal(txouts, [])

if __name__ == '__main__':
    TxOutsByAddressTest().main()

