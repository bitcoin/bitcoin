#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test blockfilterindex in conjunction with prune."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)


class FeatureBlockfilterindexPruneTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-fastprune", "-prune=1", "-blockfilterindex=1"]]

    def sync_index(self, height):
        expected = {'basic block filter index': {'synced': True, 'best_block_height': height}}
        self.wait_until(lambda: self.nodes[0].getindexinfo() == expected)

    def run_test(self):
        self.log.info("check if we can access a blockfilter when pruning is enabled but no blocks are actually pruned")
        self.sync_index(height=200)
        assert_greater_than(len(self.nodes[0].getblockfilter(self.nodes[0].getbestblockhash())['filter']), 0)
        # Mine two batches of blocks to avoid hitting NODE_NETWORK_LIMITED_MIN_BLOCKS disconnection
        self.nodes[0].generate(250)
        self.sync_all()
        self.nodes[0].generate(250)
        self.sync_all()
        self.sync_index(height=700)

        self.log.info("prune some blocks")
        pruneheight = self.nodes[0].pruneblockchain(400)
        assert_equal(pruneheight, 250)

        self.log.info("check if we can access the tips blockfilter when we have pruned some blocks")
        assert_greater_than(len(self.nodes[0].getblockfilter(self.nodes[0].getbestblockhash())['filter']), 0)

        self.log.info("check if we can access the blockfilter of a pruned block")
        assert_greater_than(len(self.nodes[0].getblockfilter(self.nodes[0].getblockhash(2))['filter']), 0)

        self.log.info("start node without blockfilterindex")
        self.restart_node(0, extra_args=["-fastprune", "-prune=1"])

        self.log.info("make sure accessing the blockfilters throws an error")
        assert_raises_rpc_error(-1, "Index is not enabled for filtertype basic", self.nodes[0].getblockfilter, self.nodes[0].getblockhash(2))
        self.nodes[0].generate(1000)

        self.log.info("prune below the blockfilterindexes best block while blockfilters are disabled")
        pruneheight_new = self.nodes[0].pruneblockchain(1000)
        assert_greater_than(pruneheight_new, pruneheight)
        self.stop_node(0)

        self.log.info("make sure we get an init error when starting the node again with block filters")
        with self.nodes[0].assert_debug_log(["basic block filter index best block of the index goes beyond pruned data. Please disable the index or reindex (which will download the whole blockchain again)"]):
            self.nodes[0].assert_start_raises_init_error(extra_args=["-fastprune", "-prune=1", "-blockfilterindex=1"])

        self.log.info("make sure the node starts again with the -reindex arg")
        self.start_node(0, extra_args = ["-fastprune", "-prune=1", "-blockfilterindex", "-reindex"])


if __name__ == '__main__':
    FeatureBlockfilterindexPruneTest().main()
