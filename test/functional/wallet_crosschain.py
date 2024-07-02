#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

class WalletCrossChain(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.add_nodes(self.num_nodes)

        # Switch node 1 to testnet before starting it.
        self.nodes[1].chain = 'testnet3'
        self.nodes[1].extra_args = ['-maxconnections=0', '-prune=550'] # disable testnet sync
        self.nodes[1].replace_in_config([('regtest=', 'testnet='), ('[regtest]', '[test]')])

        # Switch node 1 to testnet4 before starting it.
        self.nodes[2].chain = 'testnet4'
        self.nodes[2].extra_args = ['-maxconnections=0', '-prune=550'] # disable testnet4 sync
        self.nodes[2].replace_in_config([('regtest=', 'testnet4='), ('[regtest]', '[testnet4]')])

        self.start_nodes()

    def run_test(self):
        self.log.info("Creating wallets")

        node0_wallet = self.nodes[0].datadir_path / 'node0_wallet'
        node0_wallet_backup = self.nodes[0].datadir_path / 'node0_wallet.bak'
        self.nodes[0].createwallet(node0_wallet)
        self.nodes[0].backupwallet(node0_wallet_backup)
        self.nodes[0].unloadwallet(node0_wallet)
        node1_wallet = self.nodes[1].datadir_path / 'node1_wallet'
        node1_wallet_backup = self.nodes[0].datadir_path / 'node1_wallet.bak'
        self.nodes[1].createwallet(node1_wallet)
        self.nodes[1].backupwallet(node1_wallet_backup)
        self.nodes[1].unloadwallet(node1_wallet)
        node2_wallet = self.nodes[2].datadir_path / 'node2_wallet'
        node2_wallet_backup = self.nodes[0].datadir_path / 'node1_wallet.bak'
        self.nodes[2].createwallet(node2_wallet)
        self.nodes[2].backupwallet(node2_wallet_backup)
        self.nodes[2].unloadwallet(node2_wallet)

        self.log.info("Loading/restoring wallets into nodes with a different genesis block")

        if self.options.descriptors:
            assert_raises_rpc_error(-18, 'Wallet file verification failed.', self.nodes[0].loadwallet, node1_wallet)
            assert_raises_rpc_error(-18, 'Wallet file verification failed.', self.nodes[0].loadwallet, node2_wallet)
            assert_raises_rpc_error(-18, 'Wallet file verification failed.', self.nodes[1].loadwallet, node0_wallet)
            assert_raises_rpc_error(-18, 'Wallet file verification failed.', self.nodes[2].loadwallet, node0_wallet)
            assert_raises_rpc_error(-18, 'Wallet file verification failed.', self.nodes[0].restorewallet, 'w', node1_wallet_backup)
            assert_raises_rpc_error(-18, 'Wallet file verification failed.', self.nodes[0].restorewallet, 'w', node2_wallet_backup)
            assert_raises_rpc_error(-18, 'Wallet file verification failed.', self.nodes[1].restorewallet, 'w', node0_wallet_backup)
            assert_raises_rpc_error(-18, 'Wallet file verification failed.', self.nodes[2].restorewallet, 'w', node0_wallet_backup)
        else:
            assert_raises_rpc_error(-4, 'Wallet files should not be reused across chains.', self.nodes[0].loadwallet, node1_wallet)
            assert_raises_rpc_error(-4, 'Wallet files should not be reused across chains.', self.nodes[0].loadwallet, node2_wallet)
            assert_raises_rpc_error(-4, 'Wallet files should not be reused across chains.', self.nodes[1].loadwallet, node0_wallet)
            assert_raises_rpc_error(-4, 'Wallet files should not be reused across chains.', self.nodes[2].loadwallet, node0_wallet)
            assert_raises_rpc_error(-4, 'Wallet files should not be reused across chains.', self.nodes[0].restorewallet, 'w', node1_wallet_backup)
            assert_raises_rpc_error(-4, 'Wallet files should not be reused across chains.', self.nodes[0].restorewallet, 'w', node2_wallet_backup)
            assert_raises_rpc_error(-4, 'Wallet files should not be reused across chains.', self.nodes[1].restorewallet, 'w', node0_wallet_backup)
            assert_raises_rpc_error(-4, 'Wallet files should not be reused across chains.', self.nodes[2].restorewallet, 'w', node0_wallet_backup)

        if not self.options.descriptors:
            self.log.info("Override cross-chain wallet load protection")
            self.stop_nodes()
            self.start_nodes([['-walletcrosschain', '-prune=550']] * self.num_nodes)
            self.nodes[0].loadwallet(node1_wallet)
            self.nodes[1].loadwallet(node0_wallet)


if __name__ == '__main__':
    WalletCrossChain().main()
