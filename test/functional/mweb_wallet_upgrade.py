#!/usr/bin/env python3
# Copyright (c) 2021 The Litecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests that non-HD wallets that are upgraded are able to receive via MWEB"""

import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.ltc_util import create_non_hd_wallet, setup_mweb_chain
from test_framework.util import assert_equal

class MWEBWalletUpgradeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-whitelist=noban@127.0.0.1'],[]]  # immediate tx relay

    def skip_test_if_missing_module(self):
        self.skip_if_no_previous_releases()
        self.skip_if_no_wallet()

    def run_test(self):
        node0 = self.nodes[0]
        node1 = self.nodes[1]

        #
        # Mine until MWEB is activated
        #
        self.log.info("Setting up MWEB chain")
        setup_mweb_chain(node0)
        self.sync_all()
        
        #
        # Create a non-HD wallet using an older litecoin core version
        #
        self.log.info("Creating non-hd wallet")
        nonhd_wallet_dat = create_non_hd_wallet(self.chain, self.options)

        #
        # Replace node1's wallet with the non-HD wallet.dat
        #
        self.log.info("Replacing wallet with non-hd wallet.dat")
        node1.get_wallet_rpc(self.default_wallet_name).unloadwallet()
        upgrade_wallet_dir = os.path.join(node1.datadir, "regtest", "wallets", self.default_wallet_name)
        shutil.rmtree(upgrade_wallet_dir)
        os.mkdir(upgrade_wallet_dir)
        shutil.copy(nonhd_wallet_dat, upgrade_wallet_dir)
        node1.loadwallet(self.default_wallet_name)

        #
        # Upgrade node1's non-HD wallet to the latest version
        #
        self.log.info("Upgrading wallet")
        node1.upgradewallet()
        
        #
        # Send to MWEB address of upgraded wallet (node1)
        #
        self.log.info("Send to upgraded wallet's mweb address")
        mweb_addr = node1.getnewaddress(address_type='mweb')
        tx1_id = node0.sendtoaddress(mweb_addr, 25)
        self.sync_mempools()

        #
        # Verify transaction is received by upgraded wallet (node1)
        #
        self.log.info("Verify upgraded wallet lists the transaction")
        tx1 = node1.gettransaction(txid=tx1_id)
        assert_equal(tx1['confirmations'], 0)
        assert_equal(tx1['amount'], 25)
        assert_equal(tx1['details'][0]['address'], mweb_addr)

        node0.generate(1)
        self.sync_all()

        #
        # Verify that MWEB coins can be spent by upgraded wallet (node1)
        #
        self.log.info("Spending MWEB coins")
        mining_mweb_addr = node0.getnewaddress(address_type='mweb')
        tx2_id = node1.sendtoaddress(mining_mweb_addr, 10)
        self.sync_mempools()

        #
        # Mine 1 block and verify transaction confirms
        #
        self.log.info("Mining block to verify it confirms")
        node0.generate(1)
        tx2 = node0.gettransaction(txid=tx2_id)
        assert_equal(tx2['confirmations'], 1)
        assert_equal(tx2['amount'], 10)
        assert_equal(tx2['details'][0]['address'], mining_mweb_addr)

if __name__ == '__main__':
    MWEBWalletUpgradeTest().main()
