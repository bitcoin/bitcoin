#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2015-2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test -reindex with CheckBlockIndex
#
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class ReindexTest(BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 2)

    def setup_network(self):
        self.nodes = []
        self.is_network_split = False
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-checkblockindex=1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug", "-checkblockindex=1"]))
        interconnect_nodes(self.nodes)

    def run_test(self):

        # Generate enough blocks that we can spend some coinbase.
        nBlocks = 101
        self.nodes[0].generate(nBlocks)
        self.sync_all()

        # Generate transactions and mine them so there is a UTXO that is created.
        num_range = 15
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        self.nodes[0].generate(1)
        nBlocks += 1
        self.sync_all()

        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        self.nodes[0].generate(1)
        nBlocks += 1
        self.sync_all()

        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        self.nodes[0].generate(1)
        nBlocks += 1
        self.sync_all()

        stop_nodes(self.nodes)
        wait_bitcoinds()

        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-reindex", "-checkblockindex=1"]))
        i = 0
        while (i < 10):
            if (self.nodes[0].getblockcount() == nBlocks):
                break
            i += 1
            time.sleep(1)
        assert_equal(self.nodes[0].getblockcount(), nBlocks)

        print("Success")

if __name__ == '__main__':
    ReindexTest().main()
