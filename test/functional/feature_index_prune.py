#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test indices in conjunction with prune."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)


class FeatureIndexPruneTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            ["-fastprune", "-prune=1", "-blockfilterindex=1"],
            ["-fastprune", "-prune=1", "-coinstatsindex=1"],
            ["-fastprune", "-prune=1", "-blockfilterindex=1", "-coinstatsindex=1"]
        ]

    def sync_index(self, height):
        expected_filter = {
            'basic block filter index': {'synced': True, 'best_block_height': height},
        }
        self.wait_until(lambda: self.nodes[0].getindexinfo() == expected_filter)

        expected_stats = {
            'coinstatsindex': {'synced': True, 'best_block_height': height}
        }
        self.wait_until(lambda: self.nodes[1].getindexinfo() == expected_stats)

        expected = {**expected_filter, **expected_stats}
        self.wait_until(lambda: self.nodes[2].getindexinfo() == expected)

    def reconnect_nodes(self):
        self.connect_nodes(0,1)
        self.connect_nodes(0,2)
        self.connect_nodes(1,2)
        self.connect_nodes(2,1)

    def mine_batches(self, n):
        for _ in range(n):
            self.nodes[0].generate(250)
            self.sync_blocks()

    def run_test(self):
        filter_nodes = [self.nodes[0], self.nodes[2]]
        stats_nodes = [self.nodes[1], self.nodes[2]]

        self.log.info("check if we can access blockfilters and coinstats when pruning is enabled but no blocks are actually pruned")
        self.sync_index(height=200)
        tip = self.nodes[0].getbestblockhash()
        for node in filter_nodes:
            assert_greater_than(len(node.getblockfilter(tip)['filter']), 0)
        for node in stats_nodes:
            assert(node.gettxoutsetinfo(hash_type="muhash", hash_or_height=tip)['muhash'])

        # Mine two batches of blocks to avoid hitting NODE_NETWORK_LIMITED_MIN_BLOCKS disconnection
        self.mine_batches(2)
        self.sync_index(height=700)

        self.log.info("prune some blocks")
        for node in self.nodes:
            pruneheight = node.pruneblockchain(400)
            assert_equal(pruneheight, 248)

        self.log.info("check if we can access the tips blockfilter and coinstats when we have pruned some blocks")
        tip = self.nodes[0].getbestblockhash()
        for node in filter_nodes:
            assert_greater_than(len(node.getblockfilter(tip)['filter']), 0)
        for node in stats_nodes:
            assert(node.gettxoutsetinfo(hash_type="muhash", hash_or_height=tip)['muhash'])

        self.log.info("check if we can access the blockfilter and coinstats of a pruned block")
        height_hash = self.nodes[0].getblockhash(2)
        for node in filter_nodes:
            assert_greater_than(len(node.getblockfilter(height_hash)['filter']), 0)
        for node in stats_nodes:
            assert(node.gettxoutsetinfo(hash_type="muhash", hash_or_height=height_hash)['muhash'])

        self.log.info("restart nodes without indices")
        for i in range(3):
            self.restart_node(i, extra_args=["-fastprune", "-prune=1"])
        self.reconnect_nodes()

        self.log.info("make sure trying to access the indices throws errors")
        for node in filter_nodes:
            msg = "Index is not enabled for filtertype basic"
            assert_raises_rpc_error(-1, msg, node.getblockfilter, height_hash)
        for node in stats_nodes:
            msg = "Querying specific block heights requires coinstatsindex"
            assert_raises_rpc_error(-8, msg, node.gettxoutsetinfo, "muhash", height_hash)

        self.mine_batches(4)

        self.log.info("prune further than the indices best blocks while the indices are disabled")
        for i in range(3):
            pruneheight_new = self.nodes[i].pruneblockchain(1000)
            assert_greater_than(pruneheight_new, pruneheight)
            self.stop_node(i)

        self.log.info("make sure we get an init error when starting the nodes again with the indices")
        filter_msg = "Error: basic block filter index best block of the index goes beyond pruned data. Please disable the index or reindex (which will download the whole blockchain again)"
        stats_msg = "Error: coinstatsindex best block of the index goes beyond pruned data. Please disable the index or reindex (which will download the whole blockchain again)"
        for i, msg in enumerate([filter_msg, stats_msg, filter_msg]):
            self.nodes[i].assert_start_raises_init_error(extra_args=self.extra_args[i], expected_msg=msg)

        self.log.info("make sure the nodes start again with the indices and an additional -reindex arg")
        for i in range(3):
            restart_args = self.extra_args[i]+["-reindex"]
            self.restart_node(i, extra_args=restart_args)


if __name__ == '__main__':
    FeatureIndexPruneTest().main()
