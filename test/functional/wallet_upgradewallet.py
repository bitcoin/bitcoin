#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""upgradewallet RPC functional test

Requires previous releases binaries, see test/README.md.
Only v0.15.2 and v0.16.3 are required by this test. The others are used in feature_backwards_compatibility.py
"""

import os
import shutil

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_is_hex_string,
)


class UpgradeWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [
            [],                      # current wallet version
            ["-usehd=1"],            # v18.2.2 wallet
            ["-usehd=0"]             # v0.16.1.1 wallet
        ]
        self.wallet_names = [self.default_wallet_name, None, None]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()
        self.skip_if_no_previous_releases()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args, versions=[
            None,
            18020200, # that's last version before `default wallets` are created
            160101,   # that's oldest version that support `import_deterministic_coinbase_privkeys`
        ])
        self.start_nodes()
        self.import_deterministic_coinbase_privkeys()

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
        v18_2_node = self.nodes[1]
        to_height = node_from.getblockcount()
        height = self.nodes[1].getblockcount()
        for i in range(height, to_height+1):
            b = node_from.getblock(blockhash=node_from.getblockhash(i), verbose=0)
            v18_2_node.submitblock(b)
        assert_equal(v18_2_node.getblockcount(), to_height)

    def run_test(self):
        self.nodes[0].generatetoaddress(COINBASE_MATURITY + 1, self.nodes[0].getnewaddress())
        self.dumb_sync_blocks()
        # # Sanity check the test framework:
        res = self.nodes[0].getblockchaininfo()
        assert_equal(res['blocks'], COINBASE_MATURITY + 1)
        node_master = self.nodes[0]
        v18_2_node  = self.nodes[1]
        v16_1_node  = self.nodes[2]

        # Send coins to old wallets for later conversion checks.
        v18_2_wallet  = v18_2_node.get_wallet_rpc(self.default_wallet_name)
        v18_2_address = v18_2_wallet.getnewaddress()
        node_master.generatetoaddress(COINBASE_MATURITY + 1, v18_2_address)
        self.dumb_sync_blocks()
        v18_2_balance = v18_2_wallet.getbalance()

        self.log.info("Test upgradewallet RPC...")
        # Prepare for copying of the older wallet
        node_master_wallet_dir = os.path.join(node_master.datadir, "regtest/wallets")
        v18_2_wallet       = os.path.join(v18_2_node.datadir, "regtest/wallets/wallet.dat")
        v16_1_wallet       = os.path.join(v16_1_node.datadir, "regtest/wallets/wallet.dat")
        self.stop_nodes()

        # Copy the 0.16.3 wallet to the last Dash Core version and open it:
        shutil.rmtree(node_master_wallet_dir)
        os.mkdir(node_master_wallet_dir)
        shutil.copy(
            v18_2_wallet,
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
        assert_greater_than_or_equal(new_version, old_version)
        # wallet should still contain the same balance
        assert_equal(wallet.getbalance(), v18_2_balance)

        self.stop_node(0)
        # Copy the 19.3.0 wallet to the last Dash Core version and open it:
        shutil.rmtree(node_master_wallet_dir)
        os.mkdir(node_master_wallet_dir)
        shutil.copy(
            v16_1_wallet,
            node_master_wallet_dir
        )
        self.restart_node(0, ['-nowallet'])
        node_master.loadwallet('')

        wallet = node_master.get_wallet_rpc('')
        # should have no master key hash before conversion
        assert_equal('hdseedid' in wallet.getwalletinfo(), False)
        # calling upgradewallet with explicit version number
        # should return nothing if successful
        assert_equal(wallet.upgradewallet(120200), "")

        new_version = wallet.getwalletinfo()["walletversion"]
        # upgraded wallet would not have 120200 version until HD seed actually appeared
        assert_greater_than(120200, new_version)
        # after conversion master key hash should not be present yet
        assert 'hdchainid' not in wallet.getwalletinfo()
        assert_equal(wallet.upgradetohd(), True)
        new_version = wallet.getwalletinfo()["walletversion"]
        assert_equal(new_version, 120200)
        assert_is_hex_string(wallet.getwalletinfo()['hdchainid'])

if __name__ == '__main__':
    UpgradeWalletTest().main()
