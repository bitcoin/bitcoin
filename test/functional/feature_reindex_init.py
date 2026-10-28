#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test reindex works on init after a db load failure"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
)
import os


class ReindexInitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        self.stop_nodes()

        self.log.info("Removing the block index leads to init error")
        self.cleanup_folder(node.blocks_path / "index")
        node.assert_start_raises_init_error(
            expected_msg=f"Error initializing block database.{os.linesep}"
            "Please restart with -reindex or -reindex-chainstate to recover.",
        )

        self.log.info("Allowing the reindex should work fine")
        self.start_node(0, extra_args=["-test=reindex_after_failure_noninteractive_yes"])
        assert_equal(node.getblockcount(), 200)

        self.log.info("A pruned node with an empty chainstate leads to init error")
        prune_args = ["-fastprune", "-prune=1"]
        self.restart_node(0, extra_args=prune_args)
        self.generate(node, 400)
        node.pruneblockchain(node.getblockcount())
        # The genesis block is gone, so the chainstate can't be rebuilt from disk
        assert_greater_than(node.getblockchaininfo()["pruneheight"], 0)
        self.stop_node(0)
        self.cleanup_folder(node.chain_path / "chainstate")
        node.assert_start_raises_init_error(
            extra_args=prune_args,
            expected_msg="The chainstate database is empty, but the blocks needed to rebuild it have been pruned."
            f"{os.linesep}Please restart with -reindex to recover.",
        )

        self.log.info("Allowing the reindex should work fine for the pruned node")
        self.start_node(0, extra_args=[*prune_args, "-test=reindex_after_failure_noninteractive_yes"])
        # Reindexing a pruned node starts over from the genesis block
        assert_equal(node.getblockcount(), 0)


if __name__ == "__main__":
    ReindexInitTest(__file__).main()
