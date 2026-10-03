#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test pruned stale-fork blocks whose parent has no transactions.

A stale-fork block whose parent's transactions were never received cannot be
connected, so it is tracked in m_blocks_unlinked until they are. Pruning removes
its block data, but not the fact that its own transactions were received, so it
has to stay listed there. Otherwise the arrival of the parent no longer reaches
it, and neither it nor its descendants - which may well still have their own
block data - ever get their m_chain_tx_count, which trips CheckBlockIndex().
"""

from test_framework.blocktools import create_empty_fork
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error


class FeaturePruneStaleForkTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-prune=1", "-fastprune"]]

    def tip_status(self, node, block_hash):
        """Return the getchaintips status of block_hash, which must be a tip."""
        tips = [tip for tip in node.getchaintips() if tip["hash"] == block_hash]
        assert_equal(len(tips), 1)
        return tips[0]["status"]

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Create a 3-block stale fork: parent has no transactions, child has transactions")
        [side_parent, side_child, side_grandchild] = create_empty_fork(node, 3)

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

        # The child is now only listed in m_blocks_unlinked if the restart put it back there
        # without its block data, so what follows also covers reloading the block index.
        self.log.info("Submit the grandchild of the pruned child; it keeps its own block data")
        node.submitblock(side_grandchild.serialize().hex())
        node.getblock(side_grandchild.hash_hex)
        assert_equal(self.tip_status(node, side_grandchild.hash_hex), "headers-only")

        self.log.info("Submit the parent's block; the whole fork must be linked up")
        node.submitblock(side_parent.serialize().hex())
        assert_equal(node.getblockheader(side_parent.hash_hex)["nTx"], 1)
        # The child's block data is still gone, but its transactions were received at some
        # point, so the grandchild is no longer waiting for transactions the node has.
        assert_raises_rpc_error(-1, "Block not available (pruned data)", node.getblock, side_child.hash_hex)
        assert_equal(self.tip_status(node, side_grandchild.hash_hex), "valid-headers")

        self.log.info("Restart again; the reloaded node must agree with the running one")
        self.restart_node(0)
        assert_equal(self.tip_status(node, side_grandchild.hash_hex), "valid-headers")
        self.generate(node, 1)


if __name__ == '__main__':
    FeaturePruneStaleForkTest(__file__).main()
