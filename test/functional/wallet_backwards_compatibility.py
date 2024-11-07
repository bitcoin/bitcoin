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
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v25.0
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v24.0.1
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v23.0
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v22.0
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v0.21.0
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v0.20.1
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=noban@127.0.0.1"], # v0.19.1
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=127.0.0.1"], # v0.18.1
            ["-nowallet", "-walletrbf=1", "-addresstype=bech32", "-whitelist=127.0.0.1"], # v0.17.2
        ]
        self.wallet_names = [self.default_wallet_name]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_previous_releases()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args, versions=[
            None,
            None,
            250000,
            240001,
            230000,
            220000,
            210000,
            200100,
            190100,
            180100,
            170200,
        ])

        self.start_nodes()
        self.import_deterministic_coinbase_privkeys()

    def split_version(self, node):
        major = node.version // 10000
        minor = (node.version % 10000) // 100
        patch = (node.version % 100)
        return (major, minor, patch)

    def major_version_equals(self, node, major):
        node_major, _, _ = self.split_version(node)
        return node_major == major

    def major_version_less_than(self, node, major):
        node_major, _, _ = self.split_version(node)
        return node_major < major

    def major_version_at_least(self, node, major):
        node_major, _, _ = self.split_version(node)
        return node_major >= major

    def test_v19_addmultisigaddress(self):
        if not self.is_bdb_compiled():
            return
        # Specific test for addmultisigaddress using v19
        # See #18075
        self.log.info("Testing 0.19 addmultisigaddress case (#18075)")
        node_master = self.nodes[1]
        node_v19 = self.nodes[self.num_nodes - 4]
        node_v19.rpc.createwallet(wallet_name="w1_v19")
        wallet = node_v19.get_wallet_rpc("w1_v19")
        info = wallet.getwalletinfo()
        assert info['private_keys_enabled']
        assert info['keypoolsize'] > 0
        # Use addmultisigaddress (see #18075)
        address_18075 = wallet.rpc.addmultisigaddress(1, ["0296b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52", "037211a824f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073"], "", "legacy")["address"]
        assert wallet.getaddressinfo(address_18075)["solvable"]
        node_v19.unloadwallet("w1_v19")

        # Copy the 0.19 wallet to the last Bitcoin Core version and open it:
        shutil.copytree(
            os.path.join(node_v19.wallets_path, "w1_v19"),
            os.path.join(node_master.wallets_path, "w1_v19")
        )
        node_master.loadwallet("w1_v19")
        wallet = node_master.get_wallet_rpc("w1_v19")
        assert wallet.getaddressinfo(address_18075)["solvable"]

        # Now copy that same wallet back to 0.19 to make sure no automatic upgrade breaks it
        node_master.unloadwallet("w1_v19")
        shutil.rmtree(os.path.join(node_v19.wallets_path, "w1_v19"))
        shutil.copytree(
            os.path.join(node_master.wallets_path, "w1_v19"),
            os.path.join(node_v19.wallets_path, "w1_v19")
        )
        node_v19.loadwallet("w1_v19")
        wallet = node_v19.get_wallet_rpc("w1_v19")
        assert wallet.getaddressinfo(address_18075)["solvable"]

    def run_test(self):
        node_miner = self.nodes[0]
        node_master = self.nodes[1]
        node_v21 = self.nodes[self.num_nodes - 5]
        node_v17 = self.nodes[self.num_nodes - 1]

        legacy_nodes = self.nodes[2:] # Nodes that support legacy wallets
        legacy_only_nodes = self.nodes[-4:] # Nodes that only support legacy wallets
        descriptors_nodes = self.nodes[2:-4] # Nodes that support descriptor wallets

        self.generatetoaddress(node_miner, COINBASE_MATURITY + 1, node_miner.getnewaddress())

        # Sanity check the test framework:
        res = node_v17.getblockchaininfo()
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
        node_master_wallets_dir = node_master.wallets_path
        node_master.unloadwallet("w1")
        node_master.unloadwallet("w2")
        node_master.unloadwallet("w3")

        for node in legacy_nodes:
            # Copy wallets to previous version
            for wallet in os.listdir(node_master_wallets_dir):
                dest = node.wallets_path / wallet
                source = node_master_wallets_dir / wallet
                if self.major_version_equals(node, 16):
                    # 0.16 node expect the wallet to be in the wallet dir but as a plain file rather than in directories
                    shutil.copyfile(source / "wallet.dat", dest)
                else:
                    shutil.copytree(source, dest)

        self.test_v19_addmultisigaddress()

        self.log.info("Test that a wallet made on master can be opened on:")
        # In descriptors wallet mode, run this test on the nodes that support descriptor wallets
        # In legacy wallets mode, run this test on the nodes that support legacy wallets
        for node in descriptors_nodes if self.options.descriptors else legacy_nodes:
            self.log.info(f"- {node.version}")
            for wallet_name in ["w1", "w2", "w3"]:
                if self.major_version_less_than(node, 18) and wallet_name == "w3":
                    # Blank wallets were introduced in v0.18.0. We test the loading error below.
                    continue
                if self.major_version_less_than(node, 22) and wallet_name == "w1" and self.options.descriptors:
                    # Descriptor wallets created after 0.21 have taproot descriptors which 0.21 does not support, tested below
                    continue
                # Also try to reopen on master after opening on old
                for n in [node, node_master]:
                    n.loadwallet(wallet_name)
                    wallet = n.get_wallet_rpc(wallet_name)
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

                    # Copy back to master
                    wallet.unloadwallet()
                    if n == node:
                        shutil.rmtree(node_master.wallets_path / wallet_name)
                        shutil.copytree(
                            n.wallets_path / wallet_name,
                            node_master.wallets_path / wallet_name,
                        )

        # Check that descriptor wallets don't work on legacy only nodes
        if self.options.descriptors:
            self.log.info("Test descriptor wallet incompatibility on:")
            for node in legacy_only_nodes:
                # RPC loadwallet failure causes bitcoind to exit in <= 0.17, in addition to the RPC
                # call failure, so the following test won't work:
                # assert_raises_rpc_error(-4, "Wallet loading failed.", node_v17.loadwallet, 'w3')
                if self.major_version_less_than(node, 18):
                    continue
                self.log.info(f"- {node.version}")
                # Descriptor wallets appear to be corrupted wallets to old software
                assert self.major_version_at_least(node, 18) and self.major_version_less_than(node, 21)
                for wallet_name in ["w1", "w2", "w3"]:
                    assert_raises_rpc_error(-4, "Wallet file verification failed: wallet.dat corrupt, salvage failed", node.loadwallet, wallet_name)

        # Instead, we stop node and try to launch it with the wallet:
        self.stop_node(node_v17.index)
        if self.options.descriptors:
            self.log.info("Test descriptor wallet incompatibility with 0.17")
            # Descriptor wallets appear to be corrupted wallets to old software
            node_v17.assert_start_raises_init_error(["-wallet=w1"], "Error: wallet.dat corrupt, salvage failed")
            node_v17.assert_start_raises_init_error(["-wallet=w2"], "Error: wallet.dat corrupt, salvage failed")
            node_v17.assert_start_raises_init_error(["-wallet=w3"], "Error: wallet.dat corrupt, salvage failed")
        else:
            self.log.info("Test blank wallet incompatibility with v17")
            node_v17.assert_start_raises_init_error(["-wallet=w3"], "Error: Error loading w3: Wallet requires newer version of Bitcoin Core")
        self.start_node(node_v17.index)

        # When descriptors are enabled, w1 cannot be opened by 0.21 since it contains a taproot descriptor
        if self.options.descriptors:
            self.log.info("Test that 0.21 cannot open wallet containing tr() descriptors")
            assert_raises_rpc_error(-1, "map::at", node_v21.loadwallet, "w1")

        self.log.info("Test that a wallet can upgrade to and downgrade from master, from:")
        for node in descriptors_nodes if self.options.descriptors else legacy_nodes:
            self.log.info(f"- {node.version}")
            wallet_name = f"up_{node.version}"
            if self.major_version_less_than(node, 17):
                # createwallet is only available in 0.17+
                self.restart_node(node.index, extra_args=[f"-wallet={wallet_name}"])
                wallet_prev = node.get_wallet_rpc(wallet_name)
                address = wallet_prev.getnewaddress('', "bech32")
                addr_info = wallet_prev.validateaddress(address)
            else:
                if self.major_version_at_least(node, 21):
                    node.rpc.createwallet(wallet_name=wallet_name, descriptors=self.options.descriptors)
                else:
                    node.rpc.createwallet(wallet_name=wallet_name)
                wallet_prev = node.get_wallet_rpc(wallet_name)
                address = wallet_prev.getnewaddress('', "bech32")
                addr_info = wallet_prev.getaddressinfo(address)

            hdkeypath = addr_info["hdkeypath"].replace("'", "h")
            pubkey = addr_info["pubkey"]

            # Make a backup of the wallet file
            backup_path = os.path.join(self.options.tmpdir, f"{wallet_name}.dat")
            wallet_prev.backupwallet(backup_path)

            # Remove the wallet from old node
            if self.major_version_at_least(node, 17):
                wallet_prev.unloadwallet()
            else:
                self.stop_node(node.index)

            # Restore the wallet to master
            load_res = node_master.restorewallet(wallet_name, backup_path)

            # Make sure this wallet opens with only the migration warning. See https://github.com/bitcoin/bitcoin/pull/19054
            if not self.options.descriptors:
                # Legacy wallets will have only a deprecation warning
                assert_equal(load_res["warnings"], ["Wallet loaded successfully. The legacy wallet type is being deprecated and support for creating and opening legacy wallets will be removed in the future. Legacy wallets can be migrated to a descriptor wallet with migratewallet."])
            else:
                assert "warnings" not in load_res

            wallet = node_master.get_wallet_rpc(wallet_name)
            info = wallet.getaddressinfo(address)
            descriptor = f"wpkh([{info['hdmasterfingerprint']}{hdkeypath[1:]}]{pubkey})"
            assert_equal(info["desc"], descsum_create(descriptor))

            # Make backup so the wallet can be copied back to old node
            down_wallet_name = f"re_down_{node.version}"
            down_backup_path = os.path.join(self.options.tmpdir, f"{down_wallet_name}.dat")
            wallet.backupwallet(down_backup_path)

            # Check that taproot descriptors can be added to 0.21 wallets
            # This must be done after the backup is created so that 0.21 can still load
            # the backup
            if self.options.descriptors and self.major_version_equals(node, 21):
                assert_raises_rpc_error(-12, "No bech32m addresses available", wallet.getnewaddress, address_type="bech32m")
                xpubs = wallet.gethdkeys(active_only=True)
                assert_equal(len(xpubs), 1)
                assert_equal(len(xpubs[0]["descriptors"]), 6)
                wallet.createwalletdescriptor("bech32m")
                xpubs = wallet.gethdkeys(active_only=True)
                assert_equal(len(xpubs), 1)
                assert_equal(len(xpubs[0]["descriptors"]), 8)
                tr_descs = [desc["desc"] for desc in xpubs[0]["descriptors"] if desc["desc"].startswith("tr(")]
                assert_equal(len(tr_descs), 2)
                for desc in tr_descs:
                    assert info["hdmasterfingerprint"] in desc
                wallet.getnewaddress(address_type="bech32m")

            wallet.unloadwallet()

            # Check that no automatic upgrade broke the downgrading the wallet
            if self.major_version_less_than(node, 17):
                # loadwallet is only available in 0.17+
                shutil.copyfile(
                    down_backup_path,
                    node.wallets_path / down_wallet_name
                )
                self.start_node(node.index, extra_args=[f"-wallet={down_wallet_name}"])
                wallet_res = node.get_wallet_rpc(down_wallet_name)
                info = wallet_res.validateaddress(address)
                assert_equal(info, addr_info)
            else:
                target_dir = node.wallets_path / down_wallet_name
                os.makedirs(target_dir, exist_ok=True)
                shutil.copyfile(
                    down_backup_path,
                    target_dir / "wallet.dat"
                )
                node.loadwallet(down_wallet_name)
                wallet_res = node.get_wallet_rpc(down_wallet_name)
                info = wallet_res.getaddressinfo(address)
                assert_equal(info, addr_info)

if __name__ == '__main__':
    BackwardsCompatibilityTest(__file__).main()
