#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that a pruned node recovers from a crash during a pruning flush.

Pruning block files forces a flush of the chainstate. If the node crashes
before that flush completes, the chainstate on disk is still at the previous
flush, so the node needs the blocks after it to catch up again on restart.
"""
import http.client

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
)


class PruneCrashTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-fastprune", "-prune=1"]]

    def run_test(self):
        node = self.nodes[0]
        blk0 = node.blocks_path / "blk00000.dat"

        self.log.info("Mine blocks without writing them to the chainstate on disk")
        # Exit on the first partial write to the coins database, which is
        # forced by the pruning flush below.
        self.restart_node(0, extra_args=[*self.extra_args[0], "-dbcrashratio=1", "-dbbatchsize=1000"])
        self.generate(node, 1000)
        tip_height = node.getblockcount()

        self.log.info("Crash while flushing the chainstate for a manual prune")
        with node.assert_debug_log(["Simulating a crash. Goodbye."]):
            try:
                node.pruneblockchain(tip_height)
            except (ConnectionError, http.client.HTTPException):
                pass
        node.wait_until_stopped()
        # The chainstate on disk was not updated, so the node still needs these blocks
        assert blk0.exists()

        self.log.info("Restart and check that the node catches up from its own blocks")
        self.start_node(0)
        assert_equal(node.getblockcount(), tip_height)

        self.log.info("Check that the interrupted prune can be done again")
        node.pruneblockchain(tip_height)
        assert_greater_than(node.getblockchaininfo()["pruneheight"], 0)
        assert not blk0.exists()


if __name__ == '__main__':
    PruneCrashTest(__file__).main()
