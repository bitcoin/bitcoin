#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test indices in conjunction with prune."""
import concurrent.futures
import os
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)


class FeatureIndexPruneTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.extra_args = [
            ["-fastprune", "-prune=1", "-blockfilterindex=1"],
            ["-fastprune", "-prune=1", "-coinstatsindex=1"],
            ["-fastprune", "-prune=1", "-blockfilterindex=1", "-coinstatsindex=1"],
            [],
        ]

    def setup_network(self):
        self.setup_nodes()  # No P2P connection, so that linear_sync works

    def linear_sync(self, node_from, *, height_from=None):
        # Linear sync over RPC, because P2P sync may not be linear
        to_height = node_from.getblockcount()
        if height_from is None:
            height_from = min([n.getblockcount() for n in self.nodes]) + 1
        with concurrent.futures.ThreadPoolExecutor(max_workers=self.num_nodes) as rpc_threads:
            for i in range(height_from, to_height + 1):
                b = node_from.getblock(blockhash=node_from.getblockhash(i), verbosity=0)
                list(rpc_threads.map(lambda n: n.submitblock(b), self.nodes))

    def generate(self, node, num_blocks, sync_fun=None):
        return super().generate(node, num_blocks, sync_fun=sync_fun or (lambda: self.linear_sync(node)))

    def sync_index(self, height):
        expected_filter = {
            'basic block filter index': {'synced': True, 'best_block_height': height},
        }
        self.wait_until(lambda: self.nodes[0].getindexinfo() == expected_filter)

        expected_stats = {
            'coinstatsindex': {'synced': True, 'best_block_height': height}
        }
        self.wait_until(lambda: self.nodes[1].getindexinfo() == expected_stats, timeout=150)

        expected = {**expected_filter, **expected_stats}
        self.wait_until(lambda: self.nodes[2].getindexinfo() == expected)

    def restart_without_indices(self):
        for i in range(3):
            self.restart_node(i, extra_args=["-fastprune", "-prune=1"])

    def run_test(self):
        filter_nodes = [self.nodes[0], self.nodes[2]]
        stats_nodes = [self.nodes[1], self.nodes[2]]

        self.log.info("check if we can access blockfilters and coinstats when pruning is enabled but no blocks are actually pruned")
        self.sync_index(height=200)
        tip = self.nodes[0].getbestblockhash()
        for node in filter_nodes:
            assert_greater_than(len(node.getblockfilter(tip)['filter']), 0)
        for node in stats_nodes:
            assert node.gettxoutsetinfo(hash_type="muhash", hash_or_height=self.convert_to_json_for_cli(tip))['muhash']

        self.generate(self.nodes[0], 500)
        self.sync_index(height=700)

        self.log.info("prune some blocks")
        for node in self.nodes[:2]:
            with node.assert_debug_log(['limited pruning to height 689']):
                pruneheight_new = node.pruneblockchain(400)
                # the prune heights used here and below are magic numbers that are determined by the
                # thresholds at which block files wrap, so they depend on disk serialization and default block file size.
                assert_equal(pruneheight_new, 248)

        self.log.info("check if we can access the tips blockfilter and coinstats when we have pruned some blocks")
        tip = self.nodes[0].getbestblockhash()
        for node in filter_nodes:
            assert_greater_than(len(node.getblockfilter(tip)['filter']), 0)
        for node in stats_nodes:
            assert node.gettxoutsetinfo(hash_type="muhash", hash_or_height=self.convert_to_json_for_cli(tip))['muhash']

        self.log.info("check if we can access the blockfilter and coinstats of a pruned block")
        height_hash = self.nodes[0].getblockhash(2)
        for node in filter_nodes:
            assert_greater_than(len(node.getblockfilter(height_hash)['filter']), 0)
        for node in stats_nodes:
            assert node.gettxoutsetinfo(hash_type="muhash", hash_or_height=self.convert_to_json_for_cli(height_hash))['muhash']

        # mine and sync index up to a height that will later be the pruneheight
        self.generate(self.nodes[0], 51)
        self.sync_index(height=751)

        self.restart_without_indices()

        self.log.info("make sure trying to access the indices throws errors")
        for node in filter_nodes:
            msg = "Index is not enabled for filtertype basic"
            assert_raises_rpc_error(-1, msg, node.getblockfilter, height_hash)
        for node in stats_nodes:
            msg = "Querying specific block heights requires coinstatsindex"
            assert_raises_rpc_error(-8, msg, node.gettxoutsetinfo, "muhash", self.convert_to_json_for_cli(height_hash))

        self.generate(self.nodes[0], 749)

        self.log.info("prune exactly up to the indices best blocks while the indices are disabled")
        for i in range(3):
            pruneheight_2 = self.nodes[i].pruneblockchain(1000)
            assert_equal(pruneheight_2, 750)
            # Restart the nodes again with the indices activated
            self.restart_node(i, extra_args=self.extra_args[i])

        self.log.info("make sure that we can continue with the partially synced indices after having pruned up to the index height")
        self.sync_index(height=1500)

        self.log.info("prune further than the indices best blocks while the indices are disabled")
        self.restart_without_indices()
        self.generate(self.nodes[0], 1000)

        for i in range(3):
            pruneheight_3 = self.nodes[i].pruneblockchain(2000)
            assert_greater_than(pruneheight_3, pruneheight_2)
            self.stop_node(i)

        self.log.info("make sure we get an init error when starting the nodes again with the indices")
        filter_msg = "Error: basic block filter index best block of the index goes beyond pruned data. Please disable the index or reindex (which will download the whole blockchain again)"
        stats_msg = "Error: coinstatsindex best block of the index goes beyond pruned data. Please disable the index or reindex (which will download the whole blockchain again)"
        end_msg = f"{os.linesep}Error: A fatal internal error occurred, see debug.log for details: Failed to start indexes, shutting down…"
        for i, msg in enumerate([filter_msg, stats_msg, filter_msg]):
            self.nodes[i].assert_start_raises_init_error(extra_args=self.extra_args[i], expected_msg=msg+end_msg)

        self.log.info("make sure the nodes start again with the indices and an additional -reindex arg")
        for i in range(3):
            restart_args = self.extra_args[i] + ["-reindex"]
            self.restart_node(i, extra_args=restart_args)

        self.linear_sync(self.nodes[3])
        self.sync_index(height=2500)

        for node in self.nodes[:2]:
            with node.assert_debug_log(['limited pruning to height 2489']):
                pruneheight_new = node.pruneblockchain(2500)
                assert_equal(pruneheight_new, 2005)

        self.log.info("ensure that prune locks don't prevent indices from failing in a reorg scenario")
        with self.nodes[0].assert_debug_log(['basic block filter index prune lock moved back to 2480']):
            self.nodes[3].invalidateblock(self.nodes[0].getblockhash(2480))
            self.generate(self.nodes[3], 30, sync_fun=lambda: self.linear_sync(self.nodes[3], height_from=2480))


if __name__ == '__main__':
    FeatureIndexPruneTest(__file__).main()
