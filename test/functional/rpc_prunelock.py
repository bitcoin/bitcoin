#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listprunelocks and setprunelock RPCs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_array_result,
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)


# Blocks needed to generate at minimum to allow pruning to work on
# regtest (with fastprune).
INITIAL_BLOCKS = 1500


class PruneLockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-prune=1", "-fastprune"]]

    def run_test(self):
        node = self.nodes[0]

        self.log.info("Test setprunelock fails when the node is not in prune mode")
        self.restart_node(0, extra_args=[])
        assert_raises_rpc_error(-1, "Cannot set a prune lock because node is not in prune mode.", node.setprunelock, "testlock", "add", 100)
        assert_raises_rpc_error(-1, "Cannot set a prune lock because node is not in prune mode.", node.setprunelock, "testlock", "remove")

        self.log.info("Test listprunelocks distinguishes internal locks from user locks with the same id")
        self.restart_node(0, extra_args=["-prune=1", "-coinstatsindex"])
        node.setprunelock("coinstatsindex", "add", 100)
        node.setprunelock("a", "add", 50)
        assert_equal(node.listprunelocks()["prune_locks"], [
            {"id": "coinstatsindex", "type": "internal", "height": 0},
            {"id": "a", "type": "user", "height": 50},
            {"id": "coinstatsindex", "type": "user", "height": 100},
        ])
        self.restart_node(0)

        self.log.info("Test empty listprunelocks")
        assert_equal(node.listprunelocks()["prune_locks"], [])

        self.log.info("Test adding prune locks")
        node.setprunelock("testlock", "add", 100)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "testlock"},
                            {"height": 100})

        self.log.info("Test updating prune lock")
        node.setprunelock("testlock", "add", 200)
        locks = node.listprunelocks()["prune_locks"]
        assert_equal(len(locks), 1)
        assert_array_result(locks, {"id": "testlock"}, {"height": 200})

        self.log.info("Test multiple prune locks")
        node.setprunelock("another", "add", 300)
        locks = node.listprunelocks()["prune_locks"]
        assert_equal(len(locks), 2)
        assert_array_result(locks, {"id": "testlock"}, {"height": 200})
        assert_array_result(locks, {"id": "another"}, {"height": 300})

        self.log.info("Test deleting prune locks")
        node.setprunelock("testlock", "remove")
        locks = node.listprunelocks()["prune_locks"]
        assert_equal(len(locks), 1)
        assert_array_result(locks, {"id": "another"}, {"height": 300})
        assert_array_result(locks, {"id": "testlock"}, {}, should_not_find=True)

        node.setprunelock("another", "remove")
        assert_equal(node.listprunelocks()["prune_locks"], [])

        self.log.info("Test removing a non-existent lock fails")
        assert_raises_rpc_error(-8, 'Prune lock "nonexistent" not found', node.setprunelock, "nonexistent", "remove")

        self.log.info("Test invalid lock ids fail")
        for lock_id in ["", "app:100", "user:app", "two words", "a#b"]:
            assert_raises_rpc_error(-8, "Prune lock id must not be empty and may only contain", node.setprunelock, lock_id, "add", 100)
            assert_raises_rpc_error(-8, "Prune lock id must not be empty and may only contain", node.setprunelock, lock_id, "remove")

        self.log.info("Test negative lock height fails")
        assert_raises_rpc_error(-8, "Block height must not be negative", node.setprunelock, "badlock", "add", -1)

        self.log.info("Test invalid setprunelock command")
        assert_raises_rpc_error(-8, 'Invalid command "badcmd"', node.setprunelock, "somelock", "badcmd")

        self.log.info("Test that a prune lock prevents pruning of protected blocks")
        self.generate(node, INITIAL_BLOCKS)
        assert_equal(node.getblockcount(), INITIAL_BLOCKS)
        assert self.has_blockfile(node, "00000"), "blk00000.dat should exist before pruning"

        node.setprunelock("protect_early", "add", 2)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "protect_early"},
                            {"height": 2})

        # pruneblockchain returns -1 when nothing was pruned
        prune_height_locked = node.pruneblockchain(500)
        assert_equal(prune_height_locked, -1)
        assert self.has_blockfile(node, "00000"), "blk00000.dat should still exist with prune lock active"

        self.log.info("Test that removing the prune lock allows further pruning")
        node.setprunelock("protect_early", "remove")
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "protect_early"}, {},
                            should_not_find=True)

        prune_height_unlocked = node.pruneblockchain(500)
        assert_greater_than(prune_height_unlocked, 0)
        assert not self.has_blockfile(node, "00000"), "blk00000.dat should be pruned after lock removal"

        self.log.info("Test that prune locks do not persist across restart")
        node.setprunelock("ephemeral_lock", "add", 100)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "ephemeral_lock"},
                            {"height": 100})
        self.restart_node(0)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "ephemeral_lock"}, {},
                            should_not_find=True)

        self.log.info("Test a prune lock at tip height")
        tip_height = node.getblockcount()
        node.setprunelock("at_tip", "add", tip_height)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "at_tip"},
                            {"height": tip_height})
        node.setprunelock("at_tip", "remove")

        self.log.info("Test a prune lock at height 0")
        node.setprunelock("genesis", "add", 0)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "genesis"},
                            {"height": 0})
        node.setprunelock("genesis", "remove")

        self.log.info("Test a prune lock with default height (0) to disable all pruning")
        node.setprunelock("default_height", "add")
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "default_height"},
                            {"height": 0})
        node.setprunelock("default_height", "remove")


if __name__ == '__main__':
    PruneLockTest(__file__).main()
