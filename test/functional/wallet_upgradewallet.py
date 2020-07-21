#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""upgradewallet RPC functional test

Test upgradewallet RPC. Download node binaries:

contrib/devtools/previous_release.py -b v0.19.1 v0.18.1 v0.17.1 v0.16.3 v0.15.2

Only v0.15.2 and v0.16.3 are required by this test. The others are used in feature_backwards_compatibility.py
"""

import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_is_hex_string,
)


class UpgradeWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [
            ["-addresstype=bech32"], # current wallet version
            ["-usehd=1"],            # v0.16.3 wallet
            ["-usehd=0"]             # v0.15.2 wallet
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_previous_releases()

    def setup_network(self):
        self.setup_nodes()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args, versions=[
            None,
            160300,
            150200,
        ])
        self.start_nodes()

    def dumb_sync_blocks(self):
        """
        Little helper to sync older wallets.
        Notice that v0.15.2's regtest is hardforked, so there is
        no sync for it.
        v0.15.2 is only being used to test for version upgrade
        and master hash key presence.
        v0.16.3 is being used to test for version upgrade and balances.
        Further info: https://github.com/bitcoin/bitcoin/pull/18774#discussion_r416967844
        """
        node_from = self.nodes[0]
        v16_3_node = self.nodes[1]
        to_height = node_from.getblockcount()
        height = self.nodes[1].getblockcount()
        for i in range(height, to_height+1):
            b = node_from.getblock(blockhash=node_from.getblockhash(i), verbose=0)
            v16_3_node.submitblock(b)
        assert_equal(v16_3_node.getblockcount(), to_height)

    def run_test(self):
        self.nodes[0].generatetoaddress(101, self.nodes[0].getnewaddress())
        self.dumb_sync_blocks()
        # # Sanity check the test framework:
        res = self.nodes[0].getblockchaininfo()
        assert_equal(res['blocks'], 101)
        node_master = self.nodes[0]
        v16_3_node  = self.nodes[1]
        v15_2_node  = self.nodes[2]

        # Send coins to old wallets for later conversion checks.
        v16_3_wallet  = v16_3_node.get_wallet_rpc('wallet.dat')
        v16_3_address = v16_3_wallet.getnewaddress()
        node_master.generatetoaddress(101, v16_3_address)
        self.dumb_sync_blocks()
        v16_3_balance = v16_3_wallet.getbalance()

        self.log.info("Test upgradewallet RPC...")
        # Prepare for copying of the older wallet
        node_master_wallet_dir = os.path.join(node_master.datadir, "regtest/wallets")
        v16_3_wallet       = os.path.join(v16_3_node.datadir, "regtest/wallets/wallet.dat")
        v15_2_wallet       = os.path.join(v15_2_node.datadir, "regtest/wallet.dat")
        self.stop_nodes()

        # Copy the 0.16.3 wallet to the last Bitcoin Core version and open it:
        shutil.rmtree(node_master_wallet_dir)
        os.mkdir(node_master_wallet_dir)
        shutil.copy(
            v16_3_wallet,
            node_master_wallet_dir
        )
        self.restart_node(0, ['-nowallet'])
        node_master.loadwallet('')

        wallet = node_master.get_wallet_rpc('')
        old_version = wallet.getwalletinfo()["walletversion"]

        # calling upgradewallet without version arguments
        # should return nothing if successful
        assert_equal(wallet.upgradewallet(), "")
        new_version = wallet.getwalletinfo()["walletversion"]
        # upgraded wallet version should be greater than older one
        assert_greater_than(new_version, old_version)
        # wallet should still contain the same balance
        assert_equal(wallet.getbalance(), v16_3_balance)

        self.stop_node(0)
        # Copy the 0.15.2 wallet to the last Bitcoin Core version and open it:
        shutil.rmtree(node_master_wallet_dir)
        os.mkdir(node_master_wallet_dir)
        shutil.copy(
            v15_2_wallet,
            node_master_wallet_dir
        )
        self.restart_node(0, ['-nowallet'])
        node_master.loadwallet('')

        wallet = node_master.get_wallet_rpc('')
        # should have no master key hash before conversion
        assert_equal('hdseedid' in wallet.getwalletinfo(), False)
        # calling upgradewallet with explicit version number
        # should return nothing if successful
        assert_equal(wallet.upgradewallet(169900), "")
        new_version = wallet.getwalletinfo()["walletversion"]
        # upgraded wallet should have version 169900
        assert_equal(new_version, 169900)
        # after conversion master key hash should be present
        assert_is_hex_string(wallet.getwalletinfo()['hdseedid'])

if __name__ == '__main__':
    UpgradeWalletTest().main()
