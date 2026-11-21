#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listprunelocks and setprunelock RPCs, and the -prunelock startup option."""

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
# Blocks generated while a wallet is unloaded, enough to prune past its scan height.
BLOCKS_WHILE_UNLOADED = 600


class PruneLockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.uses_wallet = None
        self.extra_args = [["-prune=1", "-fastprune"]]

    def run_test(self):
        self.test_prune_lock_rpc()
        if self.is_wallet_compiled():
            self.test_prune_lock_wallet()
        self.test_prune_lock_init()

    def test_prune_lock_rpc(self):
        node = self.nodes[0]

        self.log.info("Test empty listprunelocks")
        assert_equal(node.listprunelocks()["prune_locks"], [])

        self.log.info("Test adding prune locks")
        node.setprunelock("testlock", "add", 100)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:testlock"},
                            {"height": 100})

        self.log.info("Test updating prune lock")
        node.setprunelock("testlock", "add", 200)
        locks = node.listprunelocks()["prune_locks"]
        assert_equal(len(locks), 1)
        assert_array_result(locks, {"id": "user:testlock"}, {"height": 200})

        self.log.info("Test multiple prune locks")
        node.setprunelock("another", "add", 300)
        locks = node.listprunelocks()["prune_locks"]
        assert_equal(len(locks), 2)
        assert_array_result(locks, {"id": "user:testlock"}, {"height": 200})
        assert_array_result(locks, {"id": "user:another"}, {"height": 300})

        self.log.info("Test deleting prune locks")
        node.setprunelock("testlock", "remove")
        locks = node.listprunelocks()["prune_locks"]
        assert_equal(len(locks), 1)
        assert_array_result(locks, {"id": "user:another"}, {"height": 300})
        assert_array_result(locks, {"id": "user:testlock"}, {}, should_not_find=True)

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
        assert self.has_blockfile(node, "00000"), "blk00000.dat should exist before pruning"

        node.setprunelock("protect_early", "add", 2)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:protect_early"},
                            {"height": 2})

        # pruneblockchain returns -1 when nothing was pruned
        prune_height_locked = node.pruneblockchain(500)
        assert_equal(prune_height_locked, -1)
        assert self.has_blockfile(node, "00000"), "blk00000.dat should still exist with prune lock active"

        self.log.info("Test that removing the prune lock allows further pruning")
        node.setprunelock("protect_early", "remove")
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:protect_early"}, {},
                            should_not_find=True)

        prune_height_unlocked = node.pruneblockchain(500)
        assert_greater_than(prune_height_unlocked, 0)
        assert not self.has_blockfile(node, "00000"), "blk00000.dat should be pruned after lock removal"

        self.log.info("Test that prune locks do not persist across restart")
        node.setprunelock("ephemeral_lock", "add", 100)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:ephemeral_lock"},
                            {"height": 100})
        self.restart_node(0)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:ephemeral_lock"}, {},
                            should_not_find=True)

        self.log.info("Test a prune lock at tip height")
        tip_height = node.getblockcount()
        node.setprunelock("at_tip", "add", tip_height)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:at_tip"},
                            {"height": tip_height})
        node.setprunelock("at_tip", "remove")

        self.log.info("Test a prune lock at height 0")
        node.setprunelock("genesis", "add", 0)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:genesis"},
                            {"height": 0})
        node.setprunelock("genesis", "remove")

        self.log.info("Test a prune lock with default height (0) to disable all pruning")
        node.setprunelock("default_height", "add")
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:default_height"},
                            {"height": 0})
        node.setprunelock("default_height", "remove")


    def test_prune_lock_wallet(self):
        # Simulates a wallet that stops following the chain and uses a prune lock
        # to keep the blocks it still needs to scan.
        node = self.nodes[0]
        lock_name = "wallet"

        self.log.info("Test that a prune lock keeps an unloaded wallet loadable")
        scan_height = node.getblockcount()
        for wallet_name in ["w1", "w2"]:
            node.createwallet(wallet_name)
            node.unloadwallet(wallet_name)
        node.setprunelock(lock_name, "add", scan_height)

        self.generate(node, BLOCKS_WHILE_UNLOADED)
        node.pruneblockchain(node.getblockcount())
        assert_greater_than(scan_height, node.getblockchaininfo()["pruneheight"])
        node.loadwallet("w1")
        node.unloadwallet("w1")

        self.log.info("Test that removing the lock leaves the wallet unloadable")
        node.setprunelock(lock_name, "remove")
        node.pruneblockchain(node.getblockcount())
        assert_greater_than(node.getblockchaininfo()["pruneheight"], scan_height)
        assert_raises_rpc_error(-4, "Prune: last wallet synchronisation goes beyond pruned data",
                                node.loadwallet, "w2")


    def test_prune_lock_init(self):
        node = self.nodes[0]

        self.log.info("Test -prunelock sets a lock at height 0")
        self.restart_node(0, extra_args=["-prune=1", "-prunelock=app"])
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:app"},
                            {"height": 0})

        self.log.info("Test the startup lock can be updated with setprunelock")
        node.setprunelock("app", "add", 500)
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:app"},
                            {"height": 500})

        self.log.info("Test restarting with -prunelock resets the lock to height 0")
        self.restart_node(0, extra_args=["-prune=1", "-prunelock=app"])
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:app"},
                            {"height": 0})

        self.log.info("Test the startup lock can be removed with setprunelock")
        node.setprunelock("app", "remove")
        assert_array_result(node.listprunelocks()["prune_locks"],
                            {"id": "user:app"}, {},
                            should_not_find=True)

        self.log.info("Test -prunelock can be specified multiple times")
        self.restart_node(0, extra_args=["-prune=1", "-prunelock=app1", "-prunelock=app2"])
        locks = node.listprunelocks()["prune_locks"]
        assert_array_result(locks, {"id": "user:app1"}, {"height": 0})
        assert_array_result(locks, {"id": "user:app2"}, {"height": 0})

        self.log.info("Test restarting without -prunelock removes the startup locks")
        self.restart_node(0, extra_args=["-prune=1"])
        assert_equal(node.listprunelocks()["prune_locks"], [])

        self.log.info("Test the -prunelock lock is set before the startup prune")
        log_size = node.debug_log_size(mode="rb")
        self.restart_node(0, extra_args=["-prune=1", "-prunelock=app"])
        with open(node.debug_log_path, "rb") as dl:
            dl.seek(log_size)
            log = dl.read()
        assert_greater_than(log.index(b"Pruning blockstore"), log.index(b'Initial prune lock "app" set at height 0'))

        self.log.info("Test -prunelock without -prune fails")
        self.stop_node(0)
        node.assert_start_raises_init_error(
            extra_args=["-prunelock=app"],
            expected_msg="Error: -prunelock is only possible in prune mode. Set -prune to enable.",
        )

        self.log.info("Test -prunelock with an invalid id fails")
        for lock_id in ["", "app:100"]:
            node.assert_start_raises_init_error(
                extra_args=["-prune=1", f"-prunelock={lock_id}"],
                expected_msg=f"Error: Invalid -prunelock id: '{lock_id}'",
            )


if __name__ == '__main__':
    PruneLockTest(__file__).main()
