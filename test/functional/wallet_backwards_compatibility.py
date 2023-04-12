#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Backwards compatibility functional test

Test various backwards compatibility scenarios. Requires previous releases binaries,
see test/README.md.

Due to RPC changes introduced in various versions the below tests
won't work for older versions without some patches or workarounds.

Use only the latest patch version of each release, unless a test specifically
needs an older patch version.
"""

import os
import shutil

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create

from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class BackwardsCompatibilityTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 11
        # Add new version after each release:
        self.extra_args = [
            ["-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # Pre-release: use to mine blocks. noban for immediate tx relay
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # Pre-release: use to receive coins, swap wallets, etc
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v24.0.1
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v23.0
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v22.0
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v0.21.0
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v0.20.1
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v0.19.1
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=127.0.0.1"], # v0.18.1
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=127.0.0.1"], # v0.17.2
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=127.0.0.1", "-wallet=wallet.dat"], # v0.16.3
        ]
        self.wallet_names = [self.default_wallet_name]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_previous_releases()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args, versions=[
            None,
            None,
            240001,
            230000,
            220000,
            210000,
            200100,
            190100,
            180100,
            170200,
            160300,
        ])

        self.start_nodes()
        self.import_deterministic_coinbase_privkeys()

    def nodes_wallet_dir(self, node):
        if node.version < 170000:
            return os.path.join(node.datadir, "regtest")
        return os.path.join(node.datadir, "regtest/wallets")

    def run_test(self):
        node_miner = self.nodes[0]
        node_master = self.nodes[1]
        node_v19 = self.nodes[self.num_nodes - 4]
        node_v18 = self.nodes[self.num_nodes - 3]
        node_v17 = self.nodes[self.num_nodes - 2]
        node_v16 = self.nodes[self.num_nodes - 1]

        legacy_nodes = self.nodes[2:]

        self.generatetoaddress(node_miner, COINBASE_MATURITY + 1, node_miner.getnewaddress())

        # Sanity check the test framework:
        res = node_v16.getblockchaininfo()
        assert_equal(res['blocks'], COINBASE_MATURITY + 1)

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
        node_miner.sendtoaddress(address, 10)
        self.sync_mempools()
        self.generate(node_miner, 1)
        # Create a conflicting transaction using RBF
        return_address = node_miner.getnewaddress()
        tx1_id = node_master.sendtoaddress(return_address, 1)
        tx2_id = node_master.bumpfee(tx1_id)["txid"]
        # Confirm the transaction
        self.sync_mempools()
        self.generate(node_miner, 1)
        # Create another conflicting transaction using RBF
        tx3_id = node_master.sendtoaddress(return_address, 1)
        tx4_id = node_master.bumpfee(tx3_id)["txid"]
        # Abandon transaction, but don't confirm
        node_master.abandontransaction(tx3_id)

        # w1_v19: regular wallet, created with v0.19
        node_v19.rpc.createwallet(wallet_name="w1_v19")
        wallet = node_v19.get_wallet_rpc("w1_v19")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] > 0
        # Use addmultisigaddress (see #18075)
        address_18075 = wallet.rpc.addmultisigaddress(1, ["0296b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52", "037211a824f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073"], "", "legacy")["address"]
        assert wallet.getaddressinfo(address_18075)["solvable"]
        node_v19.unloadwallet("w1_v19")

        # w1_v18: regular wallet, created with v0.18
        node_v18.rpc.createwallet(wallet_name="w1_v18")
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

        # w3: blank wallet, created on master: update this
        #     test when default blank wallets can no longer be opened by older versions.
        node_master.createwallet(wallet_name="w3", blank=True)
        wallet = node_master.get_wallet_rpc("w3")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] == 0

        # Unload wallets and copy to older nodes:
        node_master_wallets_dir = os.path.join(node_master.datadir, "regtest/wallets")
        node_v19_wallets_dir = os.path.join(node_v19.datadir, "regtest/wallets")
        node_v17_wallets_dir = os.path.join(node_v17.datadir, "regtest/wallets")
        node_v16_wallets_dir = os.path.join(node_v16.datadir, "regtest")
        node_master.unloadwallet("w1")
        node_master.unloadwallet("w2")
        node_master.unloadwallet("w3")

        for node in legacy_nodes:
            # Copy wallets to previous version
            for wallet in os.listdir(node_master_wallets_dir):
                shutil.copytree(
                    os.path.join(node_master_wallets_dir, wallet),
                    os.path.join(self.nodes_wallet_dir(node), wallet)
                )

        if not self.options.descriptors:
            # Descriptor wallets break compatibility, only run this test for legacy wallet
            # Load modern wallet with older nodes
            for node in legacy_nodes:
                for wallet_name in ["w1", "w2", "w3"]:
                    if node.version < 170000:
                        # loadwallet was introduced in v0.17.0
                        continue
                    if node.version < 180000 and wallet_name == "w3":
                        # Blank wallets were introduced in v0.18.0. We test the loading error below.
                        continue
                    node.loadwallet(wallet_name)
                    wallet = node.get_wallet_rpc(wallet_name)
                    info = wallet.getwalletinfo()
                    if wallet_name == "w1":
                        assert info['private_keys_enabled'] == True
                        assert info['keypoolsize'] > 0
                        txs = wallet.listtransactions()
                        assert_equal(len(txs), 5)
                        assert_equal(txs[1]["txid"], tx1_id)
                        assert_equal(txs[2]["walletconflicts"], [tx1_id])
                        assert_equal(txs[1]["replaced_by_txid"], tx2_id)
                        assert not txs[1]["abandoned"]
                        assert_equal(txs[1]["confirmations"], -1)
                        assert_equal(txs[2]["blockindex"], 1)
                        assert txs[3]["abandoned"]
                        assert_equal(txs[4]["walletconflicts"], [tx3_id])
                        assert_equal(txs[3]["replaced_by_txid"], tx4_id)
                        assert not hasattr(txs[3], "blockindex")
                    elif wallet_name == "w2":
                        assert info['private_keys_enabled'] == False
                        assert info['keypoolsize'] == 0
                    else:
                        assert info['private_keys_enabled'] == True
                        assert info['keypoolsize'] == 0
        else:
            for node in legacy_nodes:
                # Descriptor wallets appear to be corrupted wallets to old software
                # and loadwallet is introduced in v0.17.0
                if node.version >= 170000 and node.version < 210000:
                    for wallet_name in ["w1", "w2", "w3"]:
                        assert_raises_rpc_error(-4, "Wallet file verification failed: wallet.dat corrupt, salvage failed", node.loadwallet, wallet_name)

        # RPC loadwallet failure causes bitcoind to exit, in addition to the RPC
        # call failure, so the following test won't work:
        # assert_raises_rpc_error(-4, "Wallet loading failed.", node_v17.loadwallet, 'w3')

        # Instead, we stop node and try to launch it with the wallet:
        self.stop_node(node_v17.index)
        if self.options.descriptors:
            # Descriptor wallets appear to be corrupted wallets to old software
            node_v17.assert_start_raises_init_error(["-wallet=w1"], "Error: wallet.dat corrupt, salvage failed")
            node_v17.assert_start_raises_init_error(["-wallet=w2"], "Error: wallet.dat corrupt, salvage failed")
            node_v17.assert_start_raises_init_error(["-wallet=w3"], "Error: wallet.dat corrupt, salvage failed")
        else:
            node_v17.assert_start_raises_init_error(["-wallet=w3"], "Error: Error loading w3: Wallet requires newer version of Bitcoin Core")
        self.start_node(node_v17.index)

        if not self.options.descriptors:
            # Descriptor wallets break compatibility, only run this test for legacy wallets
            # Open most recent wallet in v0.16 (no loadwallet RPC)
            self.restart_node(node_v16.index, extra_args=["-wallet=w2"])
            wallet = node_v16.get_wallet_rpc("w2")
            info = wallet.getwalletinfo()
            assert info['keypoolsize'] == 1

        # Create upgrade wallet in v0.16
        self.restart_node(node_v16.index, extra_args=["-wallet=u1_v16"])
        wallet = node_v16.get_wallet_rpc("u1_v16")
        v16_addr = wallet.getnewaddress('', "bech32")
        v16_info = wallet.validateaddress(v16_addr)
        v16_pubkey = v16_info['pubkey']
        self.stop_node(node_v16.index)

        self.log.info("Test wallet upgrade path...")
        # u1: regular wallet, created with v0.17
        node_v17.rpc.createwallet(wallet_name="u1_v17")
        wallet = node_v17.get_wallet_rpc("u1_v17")
        address = wallet.getnewaddress("bech32")
        v17_info = wallet.getaddressinfo(address)
        hdkeypath = v17_info["hdkeypath"]
        pubkey = v17_info["pubkey"]

        if self.is_bdb_compiled():
            # Old wallets are BDB and will only work if BDB is compiled
            # Copy the 0.16 wallet to the last Bitcoin Core version and open it:
            shutil.copyfile(
                os.path.join(node_v16_wallets_dir, "wallets/u1_v16"),
                os.path.join(node_master_wallets_dir, "u1_v16")
            )
            load_res = node_master.loadwallet("u1_v16")
            # Make sure this wallet opens without warnings. See https://github.com/bitcoin/bitcoin/pull/19054
            if int(node_master.getnetworkinfo()["version"]) >= 249900:
                # loadwallet#warnings (added in v25) -- only present if there is a warning
                assert "warnings" not in load_res
            else:
                # loadwallet#warning (deprecated in v25) -- always present, but empty string if no warning
                assert_equal(load_res["warning"], '')
            wallet = node_master.get_wallet_rpc("u1_v16")
            info = wallet.getaddressinfo(v16_addr)
            descriptor = f"wpkh([{info['hdmasterfingerprint']}{hdkeypath[1:]}]{v16_pubkey})"
            assert_equal(info["desc"], descsum_create(descriptor))

            # Now copy that same wallet back to 0.16 to make sure no automatic upgrade breaks it
            node_master.unloadwallet("u1_v16")
            os.remove(os.path.join(node_v16_wallets_dir, "wallets/u1_v16"))
            shutil.copyfile(
                os.path.join(node_master_wallets_dir, "u1_v16"),
                os.path.join(node_v16_wallets_dir, "wallets/u1_v16")
            )
            self.start_node(node_v16.index, extra_args=["-wallet=u1_v16"])
            wallet = node_v16.get_wallet_rpc("u1_v16")
            info = wallet.validateaddress(v16_addr)
            assert_equal(info, v16_info)

            # Copy the 0.17 wallet to the last Bitcoin Core version and open it:
            node_v17.unloadwallet("u1_v17")
            shutil.copytree(
                os.path.join(node_v17_wallets_dir, "u1_v17"),
                os.path.join(node_master_wallets_dir, "u1_v17")
            )
            node_master.loadwallet("u1_v17")
            wallet = node_master.get_wallet_rpc("u1_v17")
            info = wallet.getaddressinfo(address)
            descriptor = f"wpkh([{info['hdmasterfingerprint']}{hdkeypath[1:]}]{pubkey})"
            assert_equal(info["desc"], descsum_create(descriptor))

            # Now copy that same wallet back to 0.17 to make sure no automatic upgrade breaks it
            node_master.unloadwallet("u1_v17")
            shutil.rmtree(os.path.join(node_v17_wallets_dir, "u1_v17"))
            shutil.copytree(
                os.path.join(node_master_wallets_dir, "u1_v17"),
                os.path.join(node_v17_wallets_dir, "u1_v17")
            )
            node_v17.loadwallet("u1_v17")
            wallet = node_v17.get_wallet_rpc("u1_v17")
            info = wallet.getaddressinfo(address)
            assert_equal(info, v17_info)

            # Copy the 0.19 wallet to the last Bitcoin Core version and open it:
            shutil.copytree(
                os.path.join(node_v19_wallets_dir, "w1_v19"),
                os.path.join(node_master_wallets_dir, "w1_v19")
            )
            node_master.loadwallet("w1_v19")
            wallet = node_master.get_wallet_rpc("w1_v19")
            assert wallet.getaddressinfo(address_18075)["solvable"]

            # Now copy that same wallet back to 0.19 to make sure no automatic upgrade breaks it
            node_master.unloadwallet("w1_v19")
            shutil.rmtree(os.path.join(node_v19_wallets_dir, "w1_v19"))
            shutil.copytree(
                os.path.join(node_master_wallets_dir, "w1_v19"),
                os.path.join(node_v19_wallets_dir, "w1_v19")
            )
            node_v19.loadwallet("w1_v19")
            wallet = node_v19.get_wallet_rpc("w1_v19")
            assert wallet.getaddressinfo(address_18075)["solvable"]

if __name__ == '__main__':
    BackwardsCompatibilityTest().main()
