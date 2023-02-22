#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Migrating a wallet from legacy to descriptor."""

import os
import random
from test_framework.descriptors import descsum_create
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    find_vout_for_address,
)
from test_framework.wallet_util import (
    get_generate_key,
)


class WalletMigrationTest(SyscoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [[]]
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()
        self.skip_if_no_bdb()

    def assert_is_sqlite(self, wallet_name):
        wallet_file_path = os.path.join(self.nodes[0].datadir, "regtest/wallets", wallet_name, self.wallet_data_filename)
        with open(wallet_file_path, 'rb') as f:
            file_magic = f.read(16)
            assert_equal(file_magic, b'SQLite format 3\x00')
        assert_equal(self.nodes[0].get_wallet_rpc(wallet_name).getwalletinfo()["format"], "sqlite")

    def create_legacy_wallet(self, wallet_name, disable_private_keys=False):
        self.nodes[0].createwallet(wallet_name=wallet_name, descriptors=False, disable_private_keys=disable_private_keys)
        wallet = self.nodes[0].get_wallet_rpc(wallet_name)
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["format"], "bdb")
        assert_equal(info["private_keys_enabled"], not disable_private_keys)
        return wallet

    def assert_addr_info_equal(self, addr_info, addr_info_old):
        assert_equal(addr_info["address"], addr_info_old["address"])
        assert_equal(addr_info["scriptPubKey"], addr_info_old["scriptPubKey"])
        assert_equal(addr_info["ismine"], addr_info_old["ismine"])
        assert_equal(addr_info["hdkeypath"], addr_info_old["hdkeypath"])
        assert_equal(addr_info["solvable"], addr_info_old["solvable"])
        assert_equal(addr_info["ischange"], addr_info_old["ischange"])
        assert_equal(addr_info["hdmasterfingerprint"], addr_info_old["hdmasterfingerprint"])

    def assert_list_txs_equal(self, received_list_txs, expected_list_txs):
        for d in received_list_txs:
            if "parent_descs" in d:
                del d["parent_descs"]
        for d in expected_list_txs:
            if "parent_descs" in d:
                del d["parent_descs"]
        assert_equal(received_list_txs, expected_list_txs)

    def test_basic(self):
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        self.log.info("Test migration of a basic keys only wallet without balance")
        basic0 = self.create_legacy_wallet("basic0")

        addr = basic0.getnewaddress()
        change = basic0.getrawchangeaddress()

        old_addr_info = basic0.getaddressinfo(addr)
        old_change_addr_info = basic0.getaddressinfo(change)
        assert_equal(old_addr_info["ismine"], True)
        assert_equal(old_addr_info["hdkeypath"], "m/0'/0'/0'")
        assert_equal(old_change_addr_info["ismine"], True)
        assert_equal(old_change_addr_info["hdkeypath"], "m/0'/1'/0'")

        # Note: migration could take a while.
        basic0.migratewallet()

        # Verify created descriptors
        assert_equal(basic0.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("basic0")

        # The wallet should create the following descriptors:
        # * BIP32 descriptors in the form of "0'/0'/*" and "0'/1'/*" (2 descriptors)
        # * BIP44 descriptors in the form of "44'/1'/0'/0/*" and "44'/1'/0'/1/*" (2 descriptors)
        # * BIP49 descriptors, P2SH(P2WPKH), in the form of "86'/1'/0'/0/*" and "86'/1'/0'/1/*" (2 descriptors)
        # * BIP84 descriptors, P2WPKH, in the form of "84'/1'/0'/1/*" and "84'/1'/0'/1/*" (2 descriptors)
        # * BIP86 descriptors, P2TR, in the form of "86'/1'/0'/0/*" and "86'/1'/0'/1/*" (2 descriptors)
        # * A combo(PK) descriptor for the wallet master key.
        # So, should have a total of 11 descriptors on it.
        assert_equal(len(basic0.listdescriptors()["descriptors"]), 11)

        # Compare addresses info
        addr_info = basic0.getaddressinfo(addr)
        change_addr_info = basic0.getaddressinfo(change)
        self.assert_addr_info_equal(addr_info, old_addr_info)
        self.assert_addr_info_equal(change_addr_info, old_change_addr_info)

        addr_info = basic0.getaddressinfo(basic0.getnewaddress("", "bech32"))
        assert_equal(addr_info["hdkeypath"], "m/84'/1'/0'/0/0")

        self.log.info("Test migration of a basic keys only wallet with a balance")
        basic1 = self.create_legacy_wallet("basic1")

        for _ in range(0, 10):
            default.sendtoaddress(basic1.getnewaddress(), 1)

        self.generate(self.nodes[0], 1)

        for _ in range(0, 5):
            basic1.sendtoaddress(default.getnewaddress(), 0.5)

        self.generate(self.nodes[0], 1)
        bal = basic1.getbalance()
        txs = basic1.listtransactions()

        basic1.migratewallet()
        assert_equal(basic1.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("basic1")
        assert_equal(basic1.getbalance(), bal)
        self.assert_list_txs_equal(basic1.listtransactions(), txs)

        # restart node and verify that everything is still there
        self.restart_node(0)
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].loadwallet("basic1")
        basic1 = self.nodes[0].get_wallet_rpc("basic1")
        assert_equal(basic1.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("basic1")
        assert_equal(basic1.getbalance(), bal)
        self.assert_list_txs_equal(basic1.listtransactions(), txs)

        self.log.info("Test migration of a wallet with balance received on the seed")
        basic2 = self.create_legacy_wallet("basic2")
        basic2_seed = get_generate_key()
        basic2.sethdseed(True, basic2_seed.privkey)
        assert_equal(basic2.getbalance(), 0)

        # Receive coins on different output types for the same seed
        basic2_balance = 0
        for addr in [basic2_seed.p2pkh_addr, basic2_seed.p2wpkh_addr, basic2_seed.p2sh_p2wpkh_addr]:
            send_value = random.randint(1, 4)
            default.sendtoaddress(addr, send_value)
            basic2_balance += send_value
            self.generate(self.nodes[0], 1)
            assert_equal(basic2.getbalance(), basic2_balance)
        basic2_txs = basic2.listtransactions()

        # Now migrate and test that we still see have the same balance/transactions
        basic2.migratewallet()
        assert_equal(basic2.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("basic2")
        assert_equal(basic2.getbalance(), basic2_balance)
        self.assert_list_txs_equal(basic2.listtransactions(), basic2_txs)

        # Now test migration on a descriptor wallet
        self.log.info("Test \"nothing to migrate\" when the user tries to migrate a wallet with no legacy data")
        assert_raises_rpc_error(-4, "Error: This wallet is already a descriptor wallet", basic2.migratewallet)

    def test_multisig(self):
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        # Contrived case where all the multisig keys are in a single wallet
        self.log.info("Test migration of a wallet with all keys for a multisig")
        multisig0 = self.create_legacy_wallet("multisig0")
        addr1 = multisig0.getnewaddress()
        addr2 = multisig0.getnewaddress()
        addr3 = multisig0.getnewaddress()

        ms_info = multisig0.addmultisigaddress(2, [addr1, addr2, addr3])

        multisig0.migratewallet()
        assert_equal(multisig0.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("multisig0")
        ms_addr_info = multisig0.getaddressinfo(ms_info["address"])
        assert_equal(ms_addr_info["ismine"], True)
        assert_equal(ms_addr_info["desc"], ms_info["descriptor"])
        assert_equal("multisig0_watchonly" in self.nodes[0].listwallets(), False)
        assert_equal("multisig0_solvables" in self.nodes[0].listwallets(), False)

        pub1 = multisig0.getaddressinfo(addr1)["pubkey"]
        pub2 = multisig0.getaddressinfo(addr2)["pubkey"]

        # Some keys in multisig do not belong to this wallet
        self.log.info("Test migration of a wallet that has some keys in a multisig")
        multisig1 = self.create_legacy_wallet("multisig1")
        ms_info = multisig1.addmultisigaddress(2, [multisig1.getnewaddress(), pub1, pub2])
        ms_info2 = multisig1.addmultisigaddress(2, [multisig1.getnewaddress(), pub1, pub2])

        addr1 = ms_info["address"]
        addr2 = ms_info2["address"]
        txid = default.sendtoaddress(addr1, 10)
        multisig1.importaddress(addr1)
        assert_equal(multisig1.getaddressinfo(addr1)["ismine"], False)
        assert_equal(multisig1.getaddressinfo(addr1)["iswatchonly"], True)
        assert_equal(multisig1.getaddressinfo(addr1)["solvable"], True)
        self.generate(self.nodes[0], 1)
        multisig1.gettransaction(txid)
        assert_equal(multisig1.getbalances()["watchonly"]["trusted"], 10)
        assert_equal(multisig1.getaddressinfo(addr2)["ismine"], False)
        assert_equal(multisig1.getaddressinfo(addr2)["iswatchonly"], False)
        assert_equal(multisig1.getaddressinfo(addr2)["solvable"], True)

        # Migrating multisig1 should see the multisig is no longer part of multisig1
        # A new wallet multisig1_watchonly is created which has the multisig address
        # Transaction to multisig is in multisig1_watchonly and not multisig1
        multisig1.migratewallet()
        assert_equal(multisig1.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("multisig1")
        assert_equal(multisig1.getaddressinfo(addr1)["ismine"], False)
        assert_equal(multisig1.getaddressinfo(addr1)["iswatchonly"], False)
        assert_equal(multisig1.getaddressinfo(addr1)["solvable"], False)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", multisig1.gettransaction, txid)
        assert_equal(multisig1.getbalance(), 0)
        assert_equal(multisig1.listtransactions(), [])

        assert_equal("multisig1_watchonly" in self.nodes[0].listwallets(), True)
        ms1_watchonly = self.nodes[0].get_wallet_rpc("multisig1_watchonly")
        ms1_wallet_info = ms1_watchonly.getwalletinfo()
        assert_equal(ms1_wallet_info['descriptors'], True)
        assert_equal(ms1_wallet_info['private_keys_enabled'], False)
        self.assert_is_sqlite("multisig1_watchonly")
        assert_equal(ms1_watchonly.getaddressinfo(addr1)["ismine"], True)
        assert_equal(ms1_watchonly.getaddressinfo(addr1)["solvable"], True)
        # Because addr2 was not being watched, it isn't in multisig1_watchonly but rather multisig1_solvables
        assert_equal(ms1_watchonly.getaddressinfo(addr2)["ismine"], False)
        assert_equal(ms1_watchonly.getaddressinfo(addr2)["solvable"], False)
        ms1_watchonly.gettransaction(txid)
        assert_equal(ms1_watchonly.getbalance(), 10)

        # Migrating multisig1 should see the second multisig is no longer part of multisig1
        # A new wallet multisig1_solvables is created which has the second address
        # This should have no transactions
        assert_equal("multisig1_solvables" in self.nodes[0].listwallets(), True)
        ms1_solvable = self.nodes[0].get_wallet_rpc("multisig1_solvables")
        ms1_wallet_info = ms1_solvable.getwalletinfo()
        assert_equal(ms1_wallet_info['descriptors'], True)
        assert_equal(ms1_wallet_info['private_keys_enabled'], False)
        self.assert_is_sqlite("multisig1_solvables")
        assert_equal(ms1_solvable.getaddressinfo(addr1)["ismine"], False)
        assert_equal(ms1_solvable.getaddressinfo(addr1)["solvable"], False)
        assert_equal(ms1_solvable.getaddressinfo(addr2)["ismine"], True)
        assert_equal(ms1_solvable.getaddressinfo(addr2)["solvable"], True)
        assert_equal(ms1_solvable.getbalance(), 0)
        assert_equal(ms1_solvable.listtransactions(), [])


    def test_other_watchonly(self):
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        # Wallet with an imported address. Should be the same thing as the multisig test
        self.log.info("Test migration of a wallet with watchonly imports")
        imports0 = self.create_legacy_wallet("imports0")

        # External address label
        imports0.setlabel(default.getnewaddress(), "external")

        # Normal non-watchonly tx
        received_addr = imports0.getnewaddress()
        imports0.setlabel(received_addr, "Receiving")
        received_txid = default.sendtoaddress(received_addr, 10)

        # Watchonly tx
        import_addr = default.getnewaddress()
        imports0.importaddress(import_addr)
        imports0.setlabel(import_addr, "imported")
        received_watchonly_txid = default.sendtoaddress(import_addr, 10)

        # Received watchonly tx that is then spent
        import_sent_addr = default.getnewaddress()
        imports0.importaddress(import_sent_addr)
        received_sent_watchonly_txid = default.sendtoaddress(import_sent_addr, 10)
        received_sent_watchonly_vout = find_vout_for_address(self.nodes[0], received_sent_watchonly_txid, import_sent_addr)
        send = default.sendall(recipients=[default.getnewaddress()], options={"inputs": [{"txid": received_sent_watchonly_txid, "vout": received_sent_watchonly_vout}]})
        sent_watchonly_txid = send["txid"]

        self.generate(self.nodes[0], 1)

        balances = imports0.getbalances()
        spendable_bal = balances["mine"]["trusted"]
        watchonly_bal = balances["watchonly"]["trusted"]
        assert_equal(len(imports0.listtransactions(include_watchonly=True)), 4)

        # Migrate
        imports0.migratewallet()
        assert_equal(imports0.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("imports0")
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", imports0.gettransaction, received_watchonly_txid)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", imports0.gettransaction, received_sent_watchonly_txid)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", imports0.gettransaction, sent_watchonly_txid)
        assert_equal(len(imports0.listtransactions(include_watchonly=True)), 1)
        imports0.gettransaction(received_txid)
        assert_equal(imports0.getbalance(), spendable_bal)

        assert_equal("imports0_watchonly" in self.nodes[0].listwallets(), True)
        watchonly = self.nodes[0].get_wallet_rpc("imports0_watchonly")
        watchonly_info = watchonly.getwalletinfo()
        assert_equal(watchonly_info["descriptors"], True)
        self.assert_is_sqlite("imports0_watchonly")
        assert_equal(watchonly_info["private_keys_enabled"], False)
        watchonly.gettransaction(received_watchonly_txid)
        watchonly.gettransaction(received_sent_watchonly_txid)
        watchonly.gettransaction(sent_watchonly_txid)
        assert_equal(watchonly.getbalance(), watchonly_bal)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", watchonly.gettransaction, received_txid)
        assert_equal(len(watchonly.listtransactions(include_watchonly=True)), 3)

        # Check that labels were migrated and persisted to watchonly wallet
        self.nodes[0].unloadwallet("imports0_watchonly")
        self.nodes[0].loadwallet("imports0_watchonly")
        labels = watchonly.listlabels()
        assert "external" in labels
        assert "imported" in labels

    def test_no_privkeys(self):
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        # Migrating an actual watchonly wallet should not create a new watchonly wallet
        self.log.info("Test migration of a pure watchonly wallet")
        watchonly0 = self.create_legacy_wallet("watchonly0", disable_private_keys=True)

        addr = default.getnewaddress()
        desc = default.getaddressinfo(addr)["desc"]
        res = watchonly0.importmulti([
            {
                "desc": desc,
                "watchonly": True,
                "timestamp": "now",
            }])
        assert_equal(res[0]['success'], True)
        default.sendtoaddress(addr, 10)
        self.generate(self.nodes[0], 1)

        watchonly0.migratewallet()
        assert_equal("watchonly0_watchonly" in self.nodes[0].listwallets(), False)
        info = watchonly0.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["private_keys_enabled"], False)
        self.assert_is_sqlite("watchonly0")

        # Migrating a wallet with pubkeys added to the keypool
        self.log.info("Test migration of a pure watchonly wallet with pubkeys in keypool")
        watchonly1 = self.create_legacy_wallet("watchonly1", disable_private_keys=True)

        addr1 = default.getnewaddress(address_type="bech32")
        addr2 = default.getnewaddress(address_type="bech32")
        desc1 = default.getaddressinfo(addr1)["desc"]
        desc2 = default.getaddressinfo(addr2)["desc"]
        res = watchonly1.importmulti([
            {
                "desc": desc1,
                "keypool": True,
                "timestamp": "now",
            },
            {
                "desc": desc2,
                "keypool": True,
                "timestamp": "now",
            }
        ])
        assert_equal(res[0]["success"], True)
        assert_equal(res[1]["success"], True)
        # Before migrating, we can fetch addr1 from the keypool
        assert_equal(watchonly1.getnewaddress(address_type="bech32"), addr1)

        watchonly1.migratewallet()
        info = watchonly1.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["private_keys_enabled"], False)
        self.assert_is_sqlite("watchonly1")
        # After migrating, the "keypool" is empty
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", watchonly1.getnewaddress)

    def test_pk_coinbases(self):
        self.log.info("Test migration of a wallet using old pk() coinbases")
        wallet = self.create_legacy_wallet("pkcb")

        addr = wallet.getnewaddress()
        addr_info = wallet.getaddressinfo(addr)
        desc = descsum_create("pk(" + addr_info["pubkey"] + ")")

        self.nodes[0].generatetodescriptor(1, desc, invalid_call=False)

        bals = wallet.getbalances()

        wallet.migratewallet()

        assert_equal(bals, wallet.getbalances())

    def test_encrypted(self):
        self.log.info("Test migration of an encrypted wallet")
        wallet = self.create_legacy_wallet("encrypted")
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        wallet.encryptwallet("pass")
        addr = wallet.getnewaddress()
        txid = default.sendtoaddress(addr, 1)
        self.generate(self.nodes[0], 1)
        bals = wallet.getbalances()

        assert_raises_rpc_error(-4, "Error: Wallet decryption failed, the wallet passphrase was not provided or was incorrect", wallet.migratewallet)
        assert_raises_rpc_error(-4, "Error: Wallet decryption failed, the wallet passphrase was not provided or was incorrect", wallet.migratewallet, None, "badpass")
        assert_raises_rpc_error(-4, "The passphrase contains a null character", wallet.migratewallet, None, "pass\0with\0null")

        wallet.migratewallet(passphrase="pass")

        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["format"], "sqlite")
        assert_equal(info["unlocked_until"], 0)
        wallet.gettransaction(txid)

        assert_equal(bals, wallet.getbalances())

    def test_unloaded(self):
        self.log.info("Test migration of a wallet that isn't loaded")
        wallet = self.create_legacy_wallet("notloaded")
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        addr = wallet.getnewaddress()
        txid = default.sendtoaddress(addr, 1)
        self.generate(self.nodes[0], 1)
        bals = wallet.getbalances()

        wallet.unloadwallet()

        assert_raises_rpc_error(-8, "RPC endpoint wallet and wallet_name parameter specify different wallets", wallet.migratewallet, "someotherwallet")
        assert_raises_rpc_error(-8, "Either RPC endpoint wallet or wallet_name parameter must be provided", self.nodes[0].migratewallet)
        self.nodes[0].migratewallet("notloaded")

        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["format"], "sqlite")
        wallet.gettransaction(txid)

        assert_equal(bals, wallet.getbalances())

    def test_unloaded_by_path(self):
        self.log.info("Test migration of a wallet that isn't loaded, specified by path")
        wallet = self.create_legacy_wallet("notloaded2")
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        addr = wallet.getnewaddress()
        txid = default.sendtoaddress(addr, 1)
        self.generate(self.nodes[0], 1)
        bals = wallet.getbalances()

        wallet.unloadwallet()

        wallet_file_path = os.path.join(self.nodes[0].datadir, "regtest", "wallets", "notloaded2")
        self.nodes[0].migratewallet(wallet_file_path)

        # Because we gave the name by full path, the loaded wallet's name is that path too.
        wallet = self.nodes[0].get_wallet_rpc(wallet_file_path)

        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["format"], "sqlite")
        wallet.gettransaction(txid)

        assert_equal(bals, wallet.getbalances())

    def run_test(self):
        self.generate(self.nodes[0], 101)

        # TODO: Test the actual records in the wallet for these tests too. The behavior may be correct, but the data written may not be what we actually want
        self.test_basic()
        self.test_multisig()
        self.test_other_watchonly()
        self.test_no_privkeys()
        self.test_pk_coinbases()
        self.test_encrypted()
        self.test_unloaded()
        self.test_unloaded_by_path()

if __name__ == '__main__':
    WalletMigrationTest().main()
