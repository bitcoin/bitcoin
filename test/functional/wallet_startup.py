#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet load on startup.

Verify that a bitcoind node can maintain list of wallets loading on startup
"""
import os
import shutil
import stat
import uuid


from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    is_dir_writable,
)


class WalletStartupTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes)
        self.start_nodes()

    def create_unnamed_wallet(self, **kwargs):
        """
        createwallet disallows empty wallet names, so create a temporary named wallet
        and move its wallet.dat to the unnamed wallet location
        """
        wallet_name = uuid.uuid4().hex
        self.nodes[0].createwallet(wallet_name=wallet_name, **kwargs)
        self.nodes[0].unloadwallet(wallet_name)
        shutil.move(self.nodes[0].wallets_path / wallet_name / "wallet.dat", self.nodes[0].wallets_path / "wallet.dat")
        shutil.rmtree(self.nodes[0].wallets_path / wallet_name)

    def test_load_unwritable_wallet(self, node):
        self.log.info("Test wallet load failure due to non-writable directory")
        wallet_name = "bad_permissions"

        node.createwallet(wallet_name)
        node.unloadwallet(wallet_name)

        dir_path = node.wallets_path / wallet_name
        original_dir_perms = dir_path.stat().st_mode
        os.chmod(dir_path, original_dir_perms & ~(stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH))

        if is_dir_writable(dir_path):
            self.log.warning("Skipping load non-writable directory test: unable to enforce read-only permissions")
        else:
            # Ensure we don't load a wallet located in a non-writable directory.
            # The node will crash later on if we cannot write to disk.
            assert_raises_rpc_error(-4, f"SQLiteDatabase: Failed to open database in directory '{str(dir_path)}': directory is not writable", node.loadwallet, wallet_name)

        # Reset directory permissions for cleanup
        dir_path.chmod(original_dir_perms)

    def run_test(self):
        self.log.info('Should start without any wallets')
        assert_equal(self.nodes[0].listwallets(), [])
        assert_equal(self.nodes[0].listwalletdir(), {'wallets': []})

        self.log.info('New default wallet should load by default when there are no other wallets')
        self.create_unnamed_wallet(load_on_startup=False)
        self.restart_node(0)
        assert_equal(self.nodes[0].listwallets(), [''])

        self.log.info('Test load on startup behavior')
        self.nodes[0].createwallet(wallet_name='w0', load_on_startup=True)
        self.nodes[0].createwallet(wallet_name='w1', load_on_startup=False)
        self.nodes[0].createwallet(wallet_name='w2', load_on_startup=True)
        self.nodes[0].createwallet(wallet_name='w3', load_on_startup=False)
        self.nodes[0].createwallet(wallet_name='w4', load_on_startup=False)
        self.nodes[0].unloadwallet(wallet_name='w0', load_on_startup=False)
        self.nodes[0].unloadwallet(wallet_name='w4', load_on_startup=False)
        self.nodes[0].loadwallet(filename='w4', load_on_startup=True)
        assert_equal(set(self.nodes[0].listwallets()), set(('', 'w1', 'w2', 'w3', 'w4')))
        self.restart_node(0)
        assert_equal(set(self.nodes[0].listwallets()), set(('', 'w2', 'w4')))
        self.nodes[0].unloadwallet(wallet_name='', load_on_startup=False)
        self.nodes[0].unloadwallet(wallet_name='w4', load_on_startup=False)
        self.nodes[0].loadwallet(filename='w3', load_on_startup=True)
        self.nodes[0].loadwallet(filename='')
        self.restart_node(0)
        assert_equal(set(self.nodes[0].listwallets()), set(('w2', 'w3')))

        self.test_load_unwritable_wallet(self.nodes[0])

if __name__ == '__main__':
    WalletStartupTest(__file__).main()
