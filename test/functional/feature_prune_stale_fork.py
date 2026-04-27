#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node restart with a pruned stale-fork block whose parent has no transactions."""

from test_framework.blocktools import create_empty_fork
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


class FeaturePruneStaleForkTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-prune=1", "-fastprune"]]

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Create a 2-block stale fork: parent has no transactions, child has transactions")
        [side_parent, side_child] = create_empty_fork(node, 2)

        node.submitheader(side_parent.serialize().hex())
        node.submitblock(side_child.serialize().hex())
        assert_equal(node.getblockheader(side_parent.hash_hex)["nTx"], 0)
        assert_equal(node.getblockheader(side_child.hash_hex)["nTx"], 1)

        self.log.info("Advance and prune so the stale-fork child's block data is removed from disk")
        self.generate(node, 500)
        node.pruneblockchain(node.getblockcount() - 100)
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, side_child.hash_hex)

        self.log.info("Restart and mine; node must reload cleanly after the stale-fork child was pruned")
        self.restart_node(0)
        self.generate(node, 1)


if __name__ == '__main__':
    FeaturePruneStaleForkTest(__file__).main()
