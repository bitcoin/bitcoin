#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Quick tests for pruning nodes.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, connect_nodes, sync_blocks, wait_until

class FastPruneTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

        # Create node 0 to mine.
        # Create nodes 1-2 to test pruning.
        self.extra_args = [
            [],
            ["-prune=550"],
            [],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[2], 0)
        sync_blocks(self.nodes[0:2])

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, self.extra_args)
        self.start_nodes()

    def prepare_chain(self):
        connect_nodes(self.nodes[2], 1)
        self.nodes[0].generate(200)
        connect_nodes(self.nodes[1], 0)
        sync_blocks(self.nodes[0:2])
        assert_equal(self.nodes[0].getblockcount(), 200)
        assert_equal(self.nodes[1].getblockcount(), 200)
        assert_equal(self.nodes[2].getblockcount(), 200)
        # Shut down last node before making long chain
        self.stop_node(2)
        self.nodes[0].generate(1000)
        sync_blocks(self.nodes[0:1])

    def slacking_test(self):
        # test that a node that is offline for a while will not be disconnected
        # from a pruned node despite being beyond limits
        # node connects only to pruned node at first, and ensures it does not
        # get itself disconnected despite requiring more than
        #       NODE_NETWORK_LIMITED_MIN_BLOCKS + 2 = 288 + 2 = 290
        # to sync up. The node should NOT ask the limited peer for blocks that
        # would cause a disconnect-slap.
        self.log.info("- connecting to pruning node should not result in invalid requests")
        self.start_node(2)
        n2 = self.nodes[2]
        n1 = self.nodes[1]
        assert_equal(n2.getblockcount(),  200)
        assert_equal(n2.getconnectioncount(), 0)
        # connect to pruned node with so many blocks we cannot sync from them
        assert(n1.getconnectioncount() > 0)
        assert(n1.getblockcount() > n2.getblockcount() + 290)
        connect_nodes(n2, 1)
        self.nodes[0].generate(100)
        # We should NOT receive any blocks, because our tip (#200) is way below
        # the limit for pruned nodes (1200 - 290 = 910)
        assert_equal(n2.getblockcount(), 200)
        # However, we should NOT be disconnected from the pruned peer, since
        # we will eventually catch up
        assert_equal(n2.getconnectioncount(), 1)

        self.log.info("- connecting to full node should properly sync, without losing connection to pruned node")
        connect_nodes(n2, 0)
        self.nodes[0].generate(100)
        self.sync_all()
        assert_equal(n2.getblockcount(), n1.getblockcount())
        # node 2 should be connected to node 0 and node 1
        assert_equal(n2.getconnectioncount(), 2)

        self.log.info("- syncing up with pruned node within limits should work")
        # shut down again
        self.stop_node(2)
        # make blocks but within limit (288+2)
        self.nodes[0].generate(288)
        wait_until(lambda: self.nodes[1].getblockcount() == 1688, timeout=30)
        sync_blocks(self.nodes[0:1])
        assert_equal(self.nodes[0].getblockcount(), 1688)
        assert_equal(self.nodes[1].getblockcount(), 1688)
        # get back up and connect to pruned node which should now give us blocks
        self.start_node(2)
        connect_nodes(n2, 1)
        self.sync_all()
        assert_equal(self.nodes[2].getblockcount(), 1688)

    def run_test(self):
        self.prepare_chain()

        self.log.info("Test slacking node")
        self.slacking_test()

        self.log.info("Done")

if __name__ == '__main__':
    FastPruneTest().main()
