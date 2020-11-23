#!/usr/bin/env python3
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Backwards compatibility functional test

Test various backwards compatibility scenarios. Download the previous node binaries:

contrib/devtools/previous_release.sh -b v0.19.0.1 v0.18.1 v0.17.1

Due to RPC changes introduced in various versions the below tests
won't work for older versions without some patches or workarounds.

Use only the latest patch version of each release, unless a test specifically
needs an older patch version.
"""

import os
import shutil

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.descriptors import descsum_create

from test_framework.util import (
    assert_equal,
    sync_blocks,
    sync_mempools
)

class BackwardsCompatibilityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 5
        # Add new version after each release:
        self.extra_args = [
            ["-addresstype=bech32"], # Pre-release: use to mine blocks
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32"], # Pre-release: use to receive coins, swap wallets, etc
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32"], # v0.19.0.1
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32"], # v0.18.1
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32"] # v0.17.1
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_nodes(self):
        if os.getenv("TEST_PREVIOUS_RELEASES") == "false":
            raise SkipTest("backwards compatibility tests")

        releases_path = os.getenv("PREVIOUS_RELEASES_DIR") or os.getcwd() + "/releases"
        if not os.path.isdir(releases_path):
            if os.getenv("TEST_PREVIOUS_RELEASES") == "true":
                raise AssertionError("TEST_PREVIOUS_RELEASES=1 but releases missing: " + releases_path)
            raise SkipTest("This test requires binaries for previous releases")

        self.add_nodes(self.num_nodes, extra_args=self.extra_args, versions=[
            None,
            None,
            190000,
            180100,
            170100
        ], binary=[
            self.options.bitcoind,
            self.options.bitcoind,
            releases_path + "/v0.19.0.1/bin/bitcoind",
            releases_path + "/v0.18.1/bin/bitcoind",
            releases_path + "/v0.17.1/bin/bitcoind"
        ], binary_cli=[
            self.options.bitcoincli,
            self.options.bitcoincli,
            releases_path + "/v0.19.0.1/bin/bitcoin-cli",
            releases_path + "/v0.18.1/bin/bitcoin-cli",
            releases_path + "/v0.17.1/bin/bitcoin-cli"
        ])

        self.start_nodes()

    def run_test(self):
        self.nodes[0].generatetoaddress(101, self.nodes[0].getnewaddress())

        sync_blocks(self.nodes)

        # Sanity check the test framework:
        res = self.nodes[self.num_nodes - 1].getblockchaininfo()
        assert_equal(res['blocks'], 101)

        node_master = self.nodes[self.num_nodes - 4]
        node_v19 = self.nodes[self.num_nodes - 3]
        node_v18 = self.nodes[self.num_nodes - 2]
        node_v17 = self.nodes[self.num_nodes - 1]

        self.log.info("Test wallet backwards compatibility...")
        # Create a number of wallets and open them in older versions:

        # w1: regular wallet, created on master: update this test when default
        #     wallets can no longer be opened by older versions.
        node_master.createwallet(wallet_name="w1")
        wallet = node_master.get_wallet_rpc("w1")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] > 0
        # Create a confirmed transaction, receiving coins
        address = wallet.getnewaddress()
        self.nodes[0].sendtoaddress(address, 10)
        sync_mempools(self.nodes)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        # Create a conflicting transaction using RBF
        return_address = self.nodes[0].getnewaddress()
        tx1_id = self.nodes[1].sendtoaddress(return_address, 1)
        tx2_id = self.nodes[1].bumpfee(tx1_id)["txid"]
        # Confirm the transaction
        sync_mempools(self.nodes)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)
        # Create another conflicting transaction using RBF
        tx3_id = self.nodes[1].sendtoaddress(return_address, 1)
        tx4_id = self.nodes[1].bumpfee(tx3_id)["txid"]
        # Abandon transaction, but don't confirm
        self.nodes[1].abandontransaction(tx3_id)

        # w1_v19: regular wallet, created with v0.19
        node_v19.createwallet(wallet_name="w1_v19")
        wallet = node_v19.get_wallet_rpc("w1_v19")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] > 0
        # Use addmultisigaddress (see #18075)
        address_18075 = wallet.addmultisigaddress(1, ["0296b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52", "037211a824f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073"], "", "legacy")["address"]
        assert wallet.getaddressinfo(address_18075)["solvable"]

        # w1_v18: regular wallet, created with v0.18
        node_v18.createwallet(wallet_name="w1_v18")
        wallet = node_v18.get_wallet_rpc("w1_v18")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] > 0

        # w2: wallet with private keys disabled, created on master: update this
        #     test when default wallets private keys disabled can no longer be
        #     opened by older versions.
        node_master.createwallet(wallet_name="w2", disable_private_keys=True)
        wallet = node_master.get_wallet_rpc("w2")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled'] == False
        assert info['keypoolsize'] == 0

        # w2_v19: wallet with private keys disabled, created with v0.19
        node_v19.createwallet(wallet_name="w2_v19", disable_private_keys=True)
        wallet = node_v19.get_wallet_rpc("w2_v19")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled'] == False
        assert info['keypoolsize'] == 0

        # w2_v18: wallet with private keys disabled, created with v0.18
        node_v18.createwallet(wallet_name="w2_v18", disable_private_keys=True)
        wallet = node_v18.get_wallet_rpc("w2_v18")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled'] == False
        assert info['keypoolsize'] == 0

        # w3: blank wallet, created on master: update this
        #     test when default blank wallets can no longer be opened by older versions.
        node_master.createwallet(wallet_name="w3", blank=True)
        wallet = node_master.get_wallet_rpc("w3")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] == 0

        # w3_v19: blank wallet, created with v0.19
        node_v19.createwallet(wallet_name="w3_v19", blank=True)
        wallet = node_v19.get_wallet_rpc("w3_v19")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] == 0

        # w3_v18: blank wallet, created with v0.18
        node_v18.createwallet(wallet_name="w3_v18", blank=True)
        wallet = node_v18.get_wallet_rpc("w3_v18")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] == 0

        # Copy the wallets to older nodes:
        node_master_wallets_dir = os.path.join(node_master.datadir, "regtest/wallets")
        node_v19_wallets_dir = os.path.join(node_v19.datadir, "regtest/wallets")
        node_v18_wallets_dir = os.path.join(node_v18.datadir, "regtest/wallets")
        node_v17_wallets_dir = os.path.join(node_v17.datadir, "regtest/wallets")
        node_master.unloadwallet("w1")
        node_master.unloadwallet("w2")
        node_v19.unloadwallet("w1_v19")
        node_v19.unloadwallet("w2_v19")
        node_v18.unloadwallet("w1_v18")
        node_v18.unloadwallet("w2_v18")

        # Copy wallets to v0.17
        for wallet in os.listdir(node_master_wallets_dir):
            shutil.copytree(
                os.path.join(node_master_wallets_dir, wallet),
                os.path.join(node_v17_wallets_dir, wallet)
            )
        for wallet in os.listdir(node_v18_wallets_dir):
            shutil.copytree(
                os.path.join(node_v18_wallets_dir, wallet),
                os.path.join(node_v17_wallets_dir, wallet)
            )

        # Copy wallets to v0.18
        for wallet in os.listdir(node_master_wallets_dir):
            shutil.copytree(
                os.path.join(node_master_wallets_dir, wallet),
                os.path.join(node_v18_wallets_dir, wallet)
            )

        # Copy wallets to v0.19
        for wallet in os.listdir(node_master_wallets_dir):
            shutil.copytree(
                os.path.join(node_master_wallets_dir, wallet),
                os.path.join(node_v19_wallets_dir, wallet)
            )

        # Open the wallets in v0.19
        node_v19.loadwallet("w1")
        wallet = node_v19.get_wallet_rpc("w1")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] > 0
        txs = wallet.listtransactions()
        assert_equal(len(txs), 5)
        assert_equal(txs[1]["txid"], tx1_id)
        assert_equal(txs[2]["walletconflicts"], [tx1_id])
        assert_equal(txs[1]["replaced_by_txid"], tx2_id)
        assert not(txs[1]["abandoned"])
        assert_equal(txs[1]["confirmations"], -1)
        assert_equal(txs[2]["blockindex"], 1)
        assert txs[3]["abandoned"]
        assert_equal(txs[4]["walletconflicts"], [tx3_id])
        assert_equal(txs[3]["replaced_by_txid"], tx4_id)
        assert not(hasattr(txs[3], "blockindex"))

        node_v19.loadwallet("w2")
        wallet = node_v19.get_wallet_rpc("w2")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled'] == False
        assert info['keypoolsize'] == 0

        node_v19.loadwallet("w3")
        wallet = node_v19.get_wallet_rpc("w3")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] == 0

        # Open the wallets in v0.18
        node_v18.loadwallet("w1")
        wallet = node_v18.get_wallet_rpc("w1")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] > 0
        txs = wallet.listtransactions()
        assert_equal(len(txs), 5)
        assert_equal(txs[1]["txid"], tx1_id)
        assert_equal(txs[2]["walletconflicts"], [tx1_id])
        assert_equal(txs[1]["replaced_by_txid"], tx2_id)
        assert not(txs[1]["abandoned"])
        assert_equal(txs[1]["confirmations"], -1)
        assert_equal(txs[2]["blockindex"], 1)
        assert txs[3]["abandoned"]
        assert_equal(txs[4]["walletconflicts"], [tx3_id])
        assert_equal(txs[3]["replaced_by_txid"], tx4_id)
        assert not(hasattr(txs[3], "blockindex"))

        node_v18.loadwallet("w2")
        wallet = node_v18.get_wallet_rpc("w2")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled'] == False
        assert info['keypoolsize'] == 0

        node_v18.loadwallet("w3")
        wallet = node_v18.get_wallet_rpc("w3")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] == 0

        # Open the wallets in v0.17
        node_v17.loadwallet("w1_v18")
        wallet = node_v17.get_wallet_rpc("w1_v18")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] > 0

        node_v17.loadwallet("w1")
        wallet = node_v17.get_wallet_rpc("w1")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] > 0

        node_v17.loadwallet("w2_v18")
        wallet = node_v17.get_wallet_rpc("w2_v18")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled'] == False
        assert info['keypoolsize'] == 0

        node_v17.loadwallet("w2")
        wallet = node_v17.get_wallet_rpc("w2")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled'] == False
        assert info['keypoolsize'] == 0

        # RPC loadwallet failure causes bitcoind to exit, in addition to the RPC
        # call failure, so the following test won't work:
        # assert_raises_rpc_error(-4, "Wallet loading failed.", node_v17.loadwallet, 'w3_v18')

        # Instead, we stop node and try to launch it with the wallet:
        self.stop_node(self.num_nodes - 1)
        node_v17.assert_start_raises_init_error(["-wallet=w3_v18"], "Error: Error loading w3_v18: Wallet requires newer version of Bitcoin Core")
        node_v17.assert_start_raises_init_error(["-wallet=w3"], "Error: Error loading w3: Wallet requires newer version of Bitcoin Core")
        self.start_node(self.num_nodes - 1)

        self.log.info("Test wallet upgrade path...")
        # u1: regular wallet, created with v0.17
        node_v17.createwallet(wallet_name="u1_v17")
        wallet = node_v17.get_wallet_rpc("u1_v17")
        address = wallet.getnewaddress("bech32")
        info = wallet.getaddressinfo(address)
        hdkeypath = info["hdkeypath"]
        pubkey = info["pubkey"]

        # Copy the 0.17 wallet to the last Bitcoin Core version and open it:
        node_v17.unloadwallet("u1_v17")
        shutil.copytree(
            os.path.join(node_v17_wallets_dir, "u1_v17"),
            os.path.join(node_master_wallets_dir, "u1_v17")
        )
        node_master.loadwallet("u1_v17")
        wallet = node_master.get_wallet_rpc("u1_v17")
        info = wallet.getaddressinfo(address)
        descriptor = "wpkh([" + info["hdmasterfingerprint"] + hdkeypath[1:] + "]" + pubkey + ")"
        assert_equal(info["desc"], descsum_create(descriptor))

        # Copy the 0.19 wallet to the last Bitcoin Core version and open it:
        shutil.copytree(
            os.path.join(node_v19_wallets_dir, "w1_v19"),
            os.path.join(node_master_wallets_dir, "w1_v19")
        )
        node_master.loadwallet("w1_v19")
        wallet = node_master.get_wallet_rpc("w1_v19")
        assert wallet.getaddressinfo(address_18075)["solvable"]

if __name__ == '__main__':
    BackwardsCompatibilityTest().main()
