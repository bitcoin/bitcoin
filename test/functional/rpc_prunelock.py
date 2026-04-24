#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listprunelocks and setprunelock RPCs, and the -prunelockheight startup option."""

import os

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

    def has_block_file(self, node, filenum):
        blockdir = os.path.join(node.blocks_path, "")
        filepath = os.path.join(blockdir, f"blk{filenum:05d}.dat")
        return os.path.isfile(filepath)

    def run_test(self):
        self.test_prune_lock_rpc()
        self.test_prune_lock_init()

    def test_prune_lock_rpc(self):
        node = self.nodes[0]

        self.log.info("Test empty listprunelocks")
        assert_equal(node.listprunelocks()["prune_locks"], [])

        self.log.info("Test adding prune locks")
        node.setprunelock("testlock", "add", 100)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "rpc:testlock"},
                            {"height": 100})

        self.log.info("Test updating prune lock")
        node.setprunelock("testlock", "add", 200)
        locks = node.listprunelocks()["prune_locks"]
        assert_equal(len(locks), 1)
        assert_array_result(locks, {"id": "rpc:testlock"}, {"height": 200})

        self.log.info("Test multiple prune locks")
        node.setprunelock("another", "add", 300)
        locks = node.listprunelocks()["prune_locks"]
        assert_equal(len(locks), 2)
        assert_array_result(locks, {"id": "rpc:testlock"}, {"height": 200})
        assert_array_result(locks, {"id": "rpc:another"}, {"height": 300})

        self.log.info("Test deleting prune locks")
        node.setprunelock("testlock", "remove")
        locks = node.listprunelocks()["prune_locks"]
        assert_equal(len(locks), 1)
        assert_array_result(locks, {"id": "rpc:another"}, {"height": 300})
        assert_array_result(locks, {"id": "rpc:testlock"}, {}, should_not_find=True)

        node.setprunelock("another", "remove")
        assert_equal(node.listprunelocks()["prune_locks"], [])

        self.log.info("Test removing a non-existent lock fails")
        assert_raises_rpc_error(-8, 'Prune lock "nonexistent" not found', node.setprunelock, "nonexistent", "remove")

        self.log.info("Test empty lock id fails")
        assert_raises_rpc_error(-8, "Prune lock id must not be empty", node.setprunelock, "", "add", 100)

        self.log.info("Test negative lock height fails")
        assert_raises_rpc_error(-8, "Block height must not be negative", node.setprunelock, "badlock", "add", -1)

        self.log.info("Test invalid setprunelock command")
        assert_raises_rpc_error(-8, 'Invalid command "badcmd"', node.setprunelock, "somelock", "badcmd")

        self.log.info("Test that a prune lock prevents pruning of protected blocks")
        self.generate(node, INITIAL_BLOCKS)
        assert_equal(node.getblockcount(), INITIAL_BLOCKS)
        assert self.has_block_file(node, 0), "blk00000.dat should exist before pruning"

        node.setprunelock("protect_early", "add", 2)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "rpc:protect_early"},
                            {"height": 2})

        prune_height_locked = node.pruneblockchain(500)
        assert self.has_block_file(node, 0), "blk00000.dat should still exist with prune lock active"

        self.log.info("Test that removing the prune lock allows further pruning")
        node.setprunelock("protect_early", "remove")
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "rpc:protect_early"}, {},
                            should_not_find=True)

        prune_height_unlocked = node.pruneblockchain(500)
        assert_greater_than(prune_height_unlocked, prune_height_locked)
        assert not self.has_block_file(node, 0), "blk00000.dat should be pruned after lock removal"

        self.log.info("Test that prune locks do not persist across restart")
        node.setprunelock("ephemeral_lock", "add", 100)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "rpc:ephemeral_lock"},
                            {"height": 100})
        self.restart_node(0)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "rpc:ephemeral_lock"}, {},
                            should_not_find=True)

        self.log.info("Test a prune lock at tip height")
        tip_height = node.getblockcount()
        node.setprunelock("at_tip", "add", tip_height)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "rpc:at_tip"},
                            {"height": tip_height})
        node.setprunelock("at_tip", "remove")

        self.log.info("Test a prune lock at height 0")
        node.setprunelock("genesis", "add", 0)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "rpc:genesis"},
                            {"height": 0})
        node.setprunelock("genesis", "remove")

        self.log.info("Test a prune lock with default height (0) to disable all pruning")
        node.setprunelock("default_height", "add")
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "rpc:default_height"},
                            {"height": 0})
        node.setprunelock("default_height", "remove")


    def test_prune_lock_init(self):
        node = self.nodes[0]

        self.log.info("Test -prunelockheight with explicit height")
        self.restart_node(0, extra_args=["-prune=1", "-prunelockheight=500"])
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "init"},
                            {"height": 500})

        self.log.info("Test -prunelockheight can coexist with RPC locks")
        node.setprunelock("mylock", "add", 100)
        locks = node.listprunelocks()["prune_locks"]
        assert_array_result(locks, {"id": "init"}, {"height": 500})
        assert_array_result(locks, {"id": "rpc:mylock"}, {"height": 100})
        node.setprunelock("mylock", "remove")

        self.log.info("Test restarting without -prunelockheight removes the startup lock")
        self.restart_node(0, extra_args=["-prune=1"])
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "init"}, {},
                            should_not_find=True)

        self.log.info("Test -prunelockheight=0 locks at tip")
        tip_height = node.getblockcount()
        self.restart_node(0, extra_args=["-prune=1", "-prunelockheight=0"])
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "init"},
                            {"height": tip_height})

        self.log.info("Test -prunelockheight without value locks at tip")
        self.restart_node(0, extra_args=["-prune=1", "-prunelockheight"])
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "init"},
                            {"height": tip_height})

        self.log.info("Test -prunelockheight without -prune fails")
        self.stop_node(0)
        node.assert_start_raises_init_error(
            extra_args=["-prunelockheight=100"],
            expected_msg="Error: -prunelockheight is only possible in prune mode. Set -prune to enable.",
        )

        self.log.info("Test -prunelockheight with negative value fails")
        node.assert_start_raises_init_error(
            extra_args=["-prune=1", "-prunelockheight=-5"],
            expected_msg='Error: -prunelockheight can not be negative',
        )


if __name__ == '__main__':
    PruneLockTest(__file__).main()
