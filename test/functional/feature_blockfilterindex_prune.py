#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test blockfilterindex in conjunction with prune."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    assert_greater_than,
)


class FeatureBlockfilterindexPruneTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-fastprune", "-prune=1"], ["-fastprune", "-prune=1", "-blockfilterindex=1"]]

    def run_test(self):
        # test basic pruning compatibility & filter access of pruned blocks
        self.log.info("check if we can access a blockfilter when pruning is enabled but no blocks are actually pruned")
        assert len(self.nodes[1].getblockfilter(self.nodes[1].getbestblockhash())['filter']) > 0
        # Mine two batches of blocks to avoid hitting NODE_NETWORK_LIMITED_MIN_BLOCKS disconnection
        self.nodes[1].generate(250)
        self.sync_all()
        self.nodes[1].generate(250)
        self.sync_all()
        self.log.info("prune some blocks")
        pruneheight = self.nodes[1].pruneblockchain(400)
        assert pruneheight != 0
        self.log.info("check if we can access the tips blockfilter when we have pruned some blocks")
        assert len(self.nodes[1].getblockfilter(self.nodes[1].getbestblockhash())['filter']) > 0
        self.log.info("check if we can access the blockfilter of a pruned block")
        assert len(self.nodes[1].getblockfilter(self.nodes[1].getblockhash(2))['filter']) > 0
        self.log.info("start node without blockfilterindex")
        self.stop_node(1)
        self.start_node(1, extra_args=self.extra_args[0])
        self.log.info("make sure accessing the blockfilters throws an error")
        assert_raises_rpc_error(-1, "Index is not enabled for filtertype basic", self.nodes[1].getblockfilter, self.nodes[1].getblockhash(2))
        self.nodes[1].generate(1000)
        self.log.info("prune below the blockfilterindexes best block while blockfilters are disabled")
        pruneheight_new = self.nodes[1].pruneblockchain(1000)
        assert_greater_than(pruneheight_new, pruneheight)
        self.stop_node(1)
        self.log.info("make sure we get an init error when starting the node again with block filters")
        with self.nodes[1].assert_debug_log(["basic block filter index best block of the index goes beyond pruned data. Please disable the index or reindex (which will download the whole blockchain again)"]):
            self.nodes[1].assert_start_raises_init_error(extra_args=self.extra_args[1])
        self.log.info("make sure the node starts again with the -reindex arg")
        reindex_args = self.extra_args[1]
        reindex_args.append("-reindex")
        self.start_node(1, extra_args=reindex_args)


if __name__ == '__main__':
    FeatureBlockfilterindexPruneTest().main()
