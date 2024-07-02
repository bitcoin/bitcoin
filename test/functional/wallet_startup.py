#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet load on startup.

Verify that a bitcoind node can maintain list of wallets loading on startup
"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)


class WalletStartupTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.supports_cli = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes)
        self.start_nodes()

    def run_test(self):
        self.log.info('Should start without any wallets')
        assert_equal(self.nodes[0].listwallets(), [])
        assert_equal(self.nodes[0].listwalletdir(), {'wallets': []})

        self.log.info('New default wallet should load by default when there are no other wallets')
        self.nodes[0].createwallet(wallet_name='', load_on_startup=False)
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

if __name__ == '__main__':
    WalletStartupTest().main()
