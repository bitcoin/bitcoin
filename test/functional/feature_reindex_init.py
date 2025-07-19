#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test reindex works on init after a db load failure"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
import os
import shutil


class ReindexInitTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        self.stop_nodes()

        self.log.info("Removing the block index leads to init error")
        shutil.rmtree(node.blocks_path / "index")
        node.assert_start_raises_init_error(
            expected_msg=f": Error initializing block database.{os.linesep}"
            "Please restart with -reindex or -reindex-chainstate to recover.",
        )

        self.log.info("Allowing the reindex should work fine")
        self.start_node(0, extra_args=["-test=reindex_after_failure_noninteractive_yes"])
        assert_equal(node.getblockcount(), 200)


if __name__ == "__main__":
    ReindexInitTest(__file__).main()
