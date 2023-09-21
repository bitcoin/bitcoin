#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet load on startup.

Verify that a bitcoind node can initialise wallet directory correctly on startup
"""

import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)


class WalletInitTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 4

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes)
        self.start_nodes()

    def run_test(self):
        self.log.info('Test node0 is initialized with a wallets/ dir')
        node0_data_dir = self.nodes[0].chain_path
        assert_equal(os.path.isdir(os.path.join(node0_data_dir, "wallets")), True)
        self.nodes[0].createwallet("w0")
        assert_equal(self.nodes[0].listwallets(), ['w0'])
        assert_equal(os.path.isdir(os.path.join(node0_data_dir, "wallets", "w0")), True)

        self.log.info('Test node2 initialized with invalid -walletsdir')
        node2_data_dir = self.nodes[2].chain_path
        self.stop_node(2)
        self.nodes[2].assert_start_raises_init_error(['-walletdir=wallets'], 'Error: Specified -walletdir "wallets" does not exist')
        self.nodes[2].assert_start_raises_init_error(['-walletdir=wallets'], 'Error: Specified -walletdir "wallets" is a relative path', cwd=node2_data_dir)
        self.nodes[2].assert_start_raises_init_error(['-walletdir=debug.log'], 'Error: Specified -walletdir "debug.log" is not a directory', cwd=node2_data_dir)

        self.log.info('Test node3 is initialized with a valid -walletsdir')
        node3_data_dir = self.nodes[3].chain_path
        new_walletdir = os.path.join(node3_data_dir, "my_wallets")
        self.stop_node(3)
        shutil.rmtree(os.path.join(node3_data_dir, "wallets"))
        os.makedirs(new_walletdir)
        self.restart_node(3, ['-walletdir=' + str(new_walletdir)])
        self.nodes[3].createwallet("w3")
        assert_equal(self.nodes[3].listwallets(), ["w3"])
        assert_equal(os.path.isdir(os.path.join(new_walletdir, "w3")), True)

if __name__ == '__main__':
    WalletInitTest().main()
