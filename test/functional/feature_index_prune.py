#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test indices in conjunction with prune."""
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)


class FeatureIndexPruneTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.rpc_timeout = 240
        self.extra_args = [
            ["-fastprune", "-prune=1", "-blockfilterindex=1","-dip3params=9000:9000"],
            ["-fastprune", "-prune=1", "-coinstatsindex=1","-dip3params=9000:9000"],
            ["-fastprune", "-prune=1", "-blockfilterindex=1", "-coinstatsindex=1","-dip3params=9000:9000"],
            ["-dip3params=9000:9000"]
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
        self.connect_nodes(0,3)

    def mine_batches(self, blocks):
        n = blocks // 250
        for _ in range(n):
            self.generate(self.nodes[0], 250)
        self.generate(self.nodes[0], blocks % 250)
        self.sync_blocks()

    def restart_without_indices(self):
        for i in range(3):
            self.restart_node(i, extra_args=["-fastprune", "-prune=1","-dip3params=9000:9000"])
        self.reconnect_nodes()

    def run_test(self):
        filter_nodes = [self.nodes[0], self.nodes[2]]
        stats_nodes = [self.nodes[1], self.nodes[2]]

        self.log.info("check if we can access blockfilters and coinstats when pruning is enabled but no blocks are actually pruned")
        self.sync_index(height=200)
        tip = self.nodes[0].getbestblockhash()
        for node in filter_nodes:
            assert_greater_than(len(node.getblockfilter(tip)['filter']), 0)
        for node in stats_nodes:
            assert node.gettxoutsetinfo(hash_type="muhash", hash_or_height=tip)['muhash']

        self.mine_batches(500)
        self.sync_index(height=700)

        self.log.info("prune some blocks")
        for node in self.nodes[:2]:
            with node.assert_debug_log(['limited pruning to height 689']):
                pruneheight_new = node.pruneblockchain(400)
                # SYSCOIN the prune heights used here and below are magic numbers that are determined by the
                # thresholds at which block files wrap, so they depend on disk serialization and default block file size.
                assert_equal(pruneheight_new, 247)

        self.log.info("check if we can access the tips blockfilter and coinstats when we have pruned some blocks")
        tip = self.nodes[0].getbestblockhash()
        for node in filter_nodes:
            assert_greater_than(len(node.getblockfilter(tip)['filter']), 0)
        for node in stats_nodes:
            assert node.gettxoutsetinfo(hash_type="muhash", hash_or_height=tip)['muhash']

        self.log.info("check if we can access the blockfilter and coinstats of a pruned block")
        height_hash = self.nodes[0].getblockhash(2)
        for node in filter_nodes:
            assert_greater_than(len(node.getblockfilter(height_hash)['filter']), 0)
        for node in stats_nodes:
            assert node.gettxoutsetinfo(hash_type="muhash", hash_or_height=height_hash)['muhash']

        # mine and sync index up to a height that will later be the pruneheight
        self.generate(self.nodes[0], 298)
        self.sync_index(height=998)

        self.restart_without_indices()

        self.log.info("make sure trying to access the indices throws errors")
        for node in filter_nodes:
            msg = "Index is not enabled for filtertype basic"
            assert_raises_rpc_error(-1, msg, node.getblockfilter, height_hash)
        for node in stats_nodes:
            msg = "Querying specific block heights requires coinstatsindex"
            assert_raises_rpc_error(-8, msg, node.gettxoutsetinfo, "muhash", height_hash)

        self.mine_batches(502)

        self.log.info("prune exactly up to the indices best blocks while the indices are disabled")
        for i in range(3):
            pruneheight_2 = self.nodes[i].pruneblockchain(1000)
            # SYSCOIN
            assert_equal(pruneheight_2, 997)
            # Restart the nodes again with the indices activated
            self.restart_node(i, extra_args=self.extra_args[i])

        self.log.info("make sure that we can continue with the partially synced indices after having pruned up to the index height")
        self.sync_index(height=1500)

        self.log.info("prune further than the indices best blocks while the indices are disabled")
        self.restart_without_indices()
        self.mine_batches(1000)

        for i in range(3):
            pruneheight_3 = self.nodes[i].pruneblockchain(2000)
            assert_greater_than(pruneheight_3, pruneheight_2)
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
            # The nodes need to be reconnected to the non-pruning node upon restart, otherwise they will be stuck
            self.connect_nodes(i, 3)

        self.sync_blocks(timeout=300)
        self.sync_index(height=2500)

        for node in self.nodes[:2]:
            with node.assert_debug_log(['limited pruning to height 2489']):
                pruneheight_new = node.pruneblockchain(2500)
                assert_equal(pruneheight_new, 1997)

        self.log.info("ensure that prune locks don't prevent indices from failing in a reorg scenario")
        with self.nodes[0].assert_debug_log(['basic block filter index prune lock moved back to 2480']):
            self.nodes[3].invalidateblock(self.nodes[0].getblockhash(2480))
            self.generate(self.nodes[3], 30)
            self.sync_blocks()


if __name__ == '__main__':
    FeatureIndexPruneTest().main()
