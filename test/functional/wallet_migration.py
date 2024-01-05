#!/usr/bin/env python3
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Migrating a wallet from legacy to descriptor."""

import random
import shutil
import struct
import time

from test_framework.address import (
    script_to_p2sh,
    key_to_p2pkh,
    key_to_p2wpkh,
)
from test_framework.bdb import BTREE_MAGIC
from test_framework.descriptors import descsum_create
from test_framework.key import ECPubKey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import COIN, CTransaction, CTxOut
from test_framework.script_util import key_to_p2pkh_script, script_to_p2sh_script, script_to_p2wsh_script
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    sha256sum_file,
)
from test_framework.wallet_util import (
    get_generate_key,
)


class WalletMigrationTest(BitcoinTestFramework):
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
        wallet_file_path = self.nodes[0].wallets_path / wallet_name / self.wallet_data_filename
        with open(wallet_file_path, 'rb') as f:
            file_magic = f.read(16)
            assert_equal(file_magic, b'SQLite format 3\x00')
        assert_equal(self.nodes[0].get_wallet_rpc(wallet_name).getwalletinfo()["format"], "sqlite")

    def create_legacy_wallet(self, wallet_name, **kwargs):
        self.nodes[0].createwallet(wallet_name=wallet_name, descriptors=False, **kwargs)
        wallet = self.nodes[0].get_wallet_rpc(wallet_name)
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["format"], "bdb")
        return wallet

    def migrate_wallet(self, wallet_rpc, *args, **kwargs):
        # Helper to ensure that only migration happens
        # Since we may rescan on loading of a wallet, make sure that the best block
        # is written before beginning migration
        # Reload to force write that record
        wallet_name = wallet_rpc.getwalletinfo()["walletname"]
        wallet_rpc.unloadwallet()
        self.nodes[0].loadwallet(wallet_name)
        # Migrate, checking that rescan does not occur
        with self.nodes[0].assert_debug_log(expected_msgs=[], unexpected_msgs=["Rescanning"]):
            return wallet_rpc.migratewallet(*args, **kwargs)

    def assert_addr_info_equal(self, addr_info, addr_info_old):
        assert_equal(addr_info["address"], addr_info_old["address"])
        assert_equal(addr_info["scriptPubKey"], addr_info_old["scriptPubKey"])
        assert_equal(addr_info["ismine"], addr_info_old["ismine"])
        assert_equal(addr_info["hdkeypath"], addr_info_old["hdkeypath"].replace("'","h"))
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

    def check_address(self, wallet, addr, is_mine, is_change, label):
        addr_info = wallet.getaddressinfo(addr)
        assert_equal(addr_info['ismine'], is_mine)
        assert_equal(addr_info['ischange'], is_change)
        if label is not None:
            assert_equal(addr_info['labels'], [label]),
        else:
            assert_equal(addr_info['labels'], []),

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
        self.migrate_wallet(basic0)

        # Verify created descriptors
        assert_equal(basic0.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("basic0")

        # The wallet should create the following descriptors:
        # * BIP32 descriptors in the form of "0h/0h/*" and "0h/1h/*" (2 descriptors)
        # * BIP44 descriptors in the form of "44h/1h/0h/0/*" and "44h/1h/0h/1/*" (2 descriptors)
        # * BIP49 descriptors, P2SH(P2WPKH), in the form of "86h/1h/0h/0/*" and "86h/1h/0h/1/*" (2 descriptors)
        # * BIP84 descriptors, P2WPKH, in the form of "84h/1h/0h/1/*" and "84h/1h/0h/1/*" (2 descriptors)
        # * BIP86 descriptors, P2TR, in the form of "86h/1h/0h/0/*" and "86h/1h/0h/1/*" (2 descriptors)
        # * A combo(PK) descriptor for the wallet master key.
        # So, should have a total of 11 descriptors on it.
        assert_equal(len(basic0.listdescriptors()["descriptors"]), 11)

        # Compare addresses info
        addr_info = basic0.getaddressinfo(addr)
        change_addr_info = basic0.getaddressinfo(change)
        self.assert_addr_info_equal(addr_info, old_addr_info)
        self.assert_addr_info_equal(change_addr_info, old_change_addr_info)

        addr_info = basic0.getaddressinfo(basic0.getnewaddress("", "bech32"))
        assert_equal(addr_info["hdkeypath"], "m/84h/1h/0h/0/0")

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
        addr_gps = basic1.listaddressgroupings()

        basic1_migrate = self.migrate_wallet(basic1)
        assert_equal(basic1.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("basic1")
        assert_equal(basic1.getbalance(), bal)
        self.assert_list_txs_equal(basic1.listtransactions(), txs)

        self.log.info("Test backup file can be successfully restored")
        self.nodes[0].restorewallet("basic1_restored", basic1_migrate['backup_path'])
        basic1_restored = self.nodes[0].get_wallet_rpc("basic1_restored")
        basic1_restored_wi = basic1_restored.getwalletinfo()
        assert_equal(basic1_restored_wi['balance'], bal)
        assert_equal(basic1_restored.listaddressgroupings(), addr_gps)
        self.assert_list_txs_equal(basic1_restored.listtransactions(), txs)

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
        self.migrate_wallet(basic2)
        assert_equal(basic2.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("basic2")
        assert_equal(basic2.getbalance(), basic2_balance)
        self.assert_list_txs_equal(basic2.listtransactions(), basic2_txs)

        # Now test migration on a descriptor wallet
        self.log.info("Test \"nothing to migrate\" when the user tries to migrate a loaded wallet with no legacy data")
        assert_raises_rpc_error(-4, "Error: This wallet is already a descriptor wallet", basic2.migratewallet)

        self.log.info("Test \"nothing to migrate\" when the user tries to migrate an unloaded wallet with no legacy data")
        basic2.unloadwallet()
        assert_raises_rpc_error(-4, "Error: This wallet is already a descriptor wallet", self.nodes[0].migratewallet, "basic2")

    def test_multisig(self):
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        # Contrived case where all the multisig keys are in a single wallet
        self.log.info("Test migration of a wallet with all keys for a multisig")
        multisig0 = self.create_legacy_wallet("multisig0")
        addr1 = multisig0.getnewaddress()
        addr2 = multisig0.getnewaddress()
        addr3 = multisig0.getnewaddress()

        ms_info = multisig0.addmultisigaddress(2, [addr1, addr2, addr3])

        self.migrate_wallet(multisig0)
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
        self.migrate_wallet(multisig1)
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
        received_sent_watchonly_utxo = self.create_outpoints(node=default, outputs=[{import_sent_addr: 10}])[0]

        send = default.sendall(recipients=[default.getnewaddress()], inputs=[received_sent_watchonly_utxo])
        sent_watchonly_txid = send["txid"]

        # Tx that has both a watchonly and spendable output
        watchonly_spendable_txid = default.send(outputs=[{received_addr: 1}, {import_addr:1}])["txid"]

        self.generate(self.nodes[0], 2)
        received_watchonly_tx_info = imports0.gettransaction(received_watchonly_txid, True)
        received_sent_watchonly_tx_info = imports0.gettransaction(received_sent_watchonly_utxo["txid"], True)

        balances = imports0.getbalances()
        spendable_bal = balances["mine"]["trusted"]
        watchonly_bal = balances["watchonly"]["trusted"]
        assert_equal(len(imports0.listtransactions(include_watchonly=True)), 6)

        # Mock time forward a bit so we can check that tx metadata is preserved
        self.nodes[0].setmocktime(int(time.time()) + 100)

        # Migrate
        self.migrate_wallet(imports0)
        assert_equal(imports0.getwalletinfo()["descriptors"], True)
        self.assert_is_sqlite("imports0")
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", imports0.gettransaction, received_watchonly_txid)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", imports0.gettransaction, received_sent_watchonly_utxo['txid'])
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", imports0.gettransaction, sent_watchonly_txid)
        assert_equal(len(imports0.listtransactions(include_watchonly=True)), 2)
        imports0.gettransaction(received_txid)
        imports0.gettransaction(watchonly_spendable_txid)
        assert_equal(imports0.getbalance(), spendable_bal)

        assert_equal("imports0_watchonly" in self.nodes[0].listwallets(), True)
        watchonly = self.nodes[0].get_wallet_rpc("imports0_watchonly")
        watchonly_info = watchonly.getwalletinfo()
        assert_equal(watchonly_info["descriptors"], True)
        self.assert_is_sqlite("imports0_watchonly")
        assert_equal(watchonly_info["private_keys_enabled"], False)
        received_migrated_watchonly_tx_info = watchonly.gettransaction(received_watchonly_txid)
        assert_equal(received_watchonly_tx_info["time"], received_migrated_watchonly_tx_info["time"])
        assert_equal(received_watchonly_tx_info["timereceived"], received_migrated_watchonly_tx_info["timereceived"])
        received_sent_migrated_watchonly_tx_info = watchonly.gettransaction(received_sent_watchonly_utxo["txid"])
        assert_equal(received_sent_watchonly_tx_info["time"], received_sent_migrated_watchonly_tx_info["time"])
        assert_equal(received_sent_watchonly_tx_info["timereceived"], received_sent_migrated_watchonly_tx_info["timereceived"])
        watchonly.gettransaction(sent_watchonly_txid)
        watchonly.gettransaction(watchonly_spendable_txid)
        assert_equal(watchonly.getbalance(), watchonly_bal)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", watchonly.gettransaction, received_txid)
        assert_equal(len(watchonly.listtransactions(include_watchonly=True)), 4)

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

        self.migrate_wallet(watchonly0)
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

        self.migrate_wallet(watchonly1)
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

        self.migrate_wallet(wallet)

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

        self.migrate_wallet(wallet, passphrase="pass")

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

        wallet_file_path = self.nodes[0].wallets_path / "notloaded2"
        self.nodes[0].migratewallet(wallet_file_path)

        # Because we gave the name by full path, the loaded wallet's name is that path too.
        wallet = self.nodes[0].get_wallet_rpc(str(wallet_file_path))

        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["format"], "sqlite")
        wallet.gettransaction(txid)

        assert_equal(bals, wallet.getbalances())

    def test_default_wallet(self):
        self.log.info("Test migration of the wallet named as the empty string")
        wallet = self.create_legacy_wallet("")

        # Set time to verify backup existence later
        curr_time = int(time.time())
        wallet.setmocktime(curr_time)

        res = self.migrate_wallet(wallet)
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["format"], "sqlite")

        # Check backup existence and its non-empty wallet filename
        backup_path = self.nodes[0].wallets_path / f'default_wallet_{curr_time}.legacy.bak'
        assert backup_path.exists()
        assert_equal(str(backup_path), res['backup_path'])

    def test_direct_file(self):
        self.log.info("Test migration of a wallet that is not in a wallet directory")
        wallet = self.create_legacy_wallet("plainfile")
        wallet.unloadwallet()

        wallets_dir = self.nodes[0].wallets_path
        wallet_path = wallets_dir / "plainfile"
        wallet_dat_path = wallet_path / "wallet.dat"
        shutil.copyfile(wallet_dat_path, wallets_dir / "plainfile.bak")
        shutil.rmtree(wallet_path)
        shutil.move(wallets_dir / "plainfile.bak", wallet_path)

        self.nodes[0].loadwallet("plainfile")
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["format"], "bdb")

        self.migrate_wallet(wallet)
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["format"], "sqlite")

        assert wallet_path.is_dir()
        assert wallet_dat_path.is_file()

    def test_addressbook(self):
        df_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        self.log.info("Test migration of address book data")
        wallet = self.create_legacy_wallet("legacy_addrbook")
        df_wallet.sendtoaddress(wallet.getnewaddress(), 3)

        # Import watch-only script to create a watch-only wallet after migration
        watch_addr = df_wallet.getnewaddress()
        wallet.importaddress(watch_addr)
        df_wallet.sendtoaddress(watch_addr, 2)

        # Import solvable script
        multi_addr1 = wallet.getnewaddress()
        multi_addr2 = wallet.getnewaddress()
        multi_addr3 = df_wallet.getnewaddress()
        wallet.importpubkey(df_wallet.getaddressinfo(multi_addr3)["pubkey"])
        ms_addr_info = wallet.addmultisigaddress(2, [multi_addr1, multi_addr2, multi_addr3])

        self.generate(self.nodes[0], 1)

        # Test vectors
        addr_external = {
            "addr": df_wallet.getnewaddress(),
            "is_mine": False,
            "is_change": False,
            "label": ""
        }
        addr_external_with_label = {
            "addr": df_wallet.getnewaddress(),
            "is_mine": False,
            "is_change": False,
            "label": "external"
        }
        addr_internal = {
            "addr": wallet.getnewaddress(),
            "is_mine": True,
            "is_change": False,
            "label": ""
        }
        addr_internal_with_label = {
            "addr": wallet.getnewaddress(),
            "is_mine": True,
            "is_change": False,
            "label": "internal"
        }
        change_address = {
            "addr": wallet.getrawchangeaddress(),
            "is_mine": True,
            "is_change": True,
            "label": None
        }
        watch_only_addr = {
            "addr": watch_addr,
            "is_mine": False,
            "is_change": False,
            "label": "imported"
        }
        ms_addr = {
            "addr": ms_addr_info['address'],
            "is_mine": False,
            "is_change": False,
            "label": "multisig"
        }

        # To store the change address in the addressbook need to send coins to it
        wallet.send(outputs=[{wallet.getnewaddress(): 2}], options={"change_address": change_address['addr']})
        self.generate(self.nodes[0], 1)

        # Util wrapper func for 'addr_info'
        def check(info, node):
            self.check_address(node, info['addr'], info['is_mine'], info['is_change'], info["label"])

        # Pre-migration: set label and perform initial checks
        for addr_info in [addr_external, addr_external_with_label, addr_internal, addr_internal_with_label, change_address, watch_only_addr, ms_addr]:
            if not addr_info['is_change']:
                wallet.setlabel(addr_info['addr'], addr_info["label"])
            check(addr_info, wallet)

        # Migrate wallet
        info_migration = self.migrate_wallet(wallet)
        wallet_wo = self.nodes[0].get_wallet_rpc(info_migration["watchonly_name"])
        wallet_solvables = self.nodes[0].get_wallet_rpc(info_migration["solvables_name"])

        #########################
        # Post migration checks #
        #########################

        # First check the main wallet
        for addr_info in [addr_external, addr_external_with_label, addr_internal, addr_internal_with_label, change_address, ms_addr]:
            check(addr_info, wallet)

        # Watch-only wallet will contain the watch-only entry (with 'is_mine=True') and all external addresses ('send')
        self.check_address(wallet_wo, watch_only_addr['addr'], is_mine=True, is_change=watch_only_addr['is_change'], label=watch_only_addr["label"])
        for addr_info in [addr_external, addr_external_with_label, ms_addr]:
            check(addr_info, wallet_wo)

        # Solvables wallet will contain the multisig entry (with 'is_mine=True') and all external addresses ('send')
        self.check_address(wallet_solvables, ms_addr['addr'], is_mine=True, is_change=ms_addr['is_change'], label=ms_addr["label"])
        for addr_info in [addr_external, addr_external_with_label]:
            check(addr_info, wallet_solvables)

        ########################################################################################
        # Now restart migrated wallets and verify that the addressbook entries are still there #
        ########################################################################################

        # First the main wallet
        self.nodes[0].unloadwallet("legacy_addrbook")
        self.nodes[0].loadwallet("legacy_addrbook")
        for addr_info in [addr_external, addr_external_with_label, addr_internal, addr_internal_with_label, change_address, ms_addr]:
            check(addr_info, wallet)

        # Watch-only wallet
        self.nodes[0].unloadwallet(info_migration["watchonly_name"])
        self.nodes[0].loadwallet(info_migration["watchonly_name"])
        self.check_address(wallet_wo, watch_only_addr['addr'], is_mine=True, is_change=watch_only_addr['is_change'], label=watch_only_addr["label"])
        for addr_info in [addr_external, addr_external_with_label, ms_addr]:
            check(addr_info, wallet_wo)

        # Solvables wallet
        self.nodes[0].unloadwallet(info_migration["solvables_name"])
        self.nodes[0].loadwallet(info_migration["solvables_name"])
        self.check_address(wallet_solvables, ms_addr['addr'], is_mine=True, is_change=ms_addr['is_change'], label=ms_addr["label"])
        for addr_info in [addr_external, addr_external_with_label]:
            check(addr_info, wallet_solvables)

    def test_migrate_raw_p2sh(self):
        self.log.info("Test migration of watch-only raw p2sh script")
        df_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        wallet = self.create_legacy_wallet("raw_p2sh")

        def send_to_script(script, amount):
            tx = CTransaction()
            tx.vout.append(CTxOut(nValue=amount*COIN, scriptPubKey=script))

            hex_tx = df_wallet.fundrawtransaction(tx.serialize().hex())['hex']
            signed_tx = df_wallet.signrawtransactionwithwallet(hex_tx)
            df_wallet.sendrawtransaction(signed_tx['hex'])
            self.generate(self.nodes[0], 1)

        # Craft sh(pkh(key)) script and send coins to it
        pubkey = df_wallet.getaddressinfo(df_wallet.getnewaddress())["pubkey"]
        script_pkh = key_to_p2pkh_script(pubkey)
        script_sh_pkh = script_to_p2sh_script(script_pkh)
        send_to_script(script=script_sh_pkh, amount=2)

        # Import script and check balance
        wallet.rpc.importaddress(address=script_pkh.hex(), label="raw_spk", rescan=True, p2sh=True)
        assert_equal(wallet.getbalances()['watchonly']['trusted'], 2)

        # Craft wsh(pkh(key)) and send coins to it
        pubkey = df_wallet.getaddressinfo(df_wallet.getnewaddress())["pubkey"]
        script_wsh_pkh = script_to_p2wsh_script(key_to_p2pkh_script(pubkey))
        send_to_script(script=script_wsh_pkh, amount=3)

        # Import script and check balance
        wallet.rpc.importaddress(address=script_wsh_pkh.hex(), label="raw_spk2", rescan=True, p2sh=False)
        assert_equal(wallet.getbalances()['watchonly']['trusted'], 5)

        # Import sh(pkh()) script, by using importaddress(), with the p2sh flag enabled.
        # This will wrap the script under another sh level, which is invalid!, and store it inside the wallet.
        # The migration process must skip the invalid scripts and the addressbook records linked to them.
        # They are not being watched by the current wallet, nor should be watched by the migrated one.
        label_sh_pkh = "raw_sh_pkh"
        script_pkh = key_to_p2pkh_script(df_wallet.getaddressinfo(df_wallet.getnewaddress())["pubkey"])
        script_sh_pkh = script_to_p2sh_script(script_pkh)
        addy_script_sh_pkh = script_to_p2sh(script_pkh)  # valid script address
        addy_script_double_sh_pkh = script_to_p2sh(script_sh_pkh)  # invalid script address

        # Note: 'importaddress()' will add two scripts, a valid one sh(pkh()) and an invalid one 'sh(sh(pkh()))'.
        #       Both of them will be stored with the same addressbook label. And only the latter one should
        #       be discarded during migration. The first one must be migrated.
        wallet.rpc.importaddress(address=script_sh_pkh.hex(), label=label_sh_pkh, rescan=False, p2sh=True)

        # Migrate wallet and re-check balance
        info_migration = self.migrate_wallet(wallet)
        wallet_wo = self.nodes[0].get_wallet_rpc(info_migration["watchonly_name"])

        # Watch-only balance is under "mine".
        assert_equal(wallet_wo.getbalances()['mine']['trusted'], 5)
        # The watch-only scripts are no longer part of the main wallet
        assert_equal(wallet.getbalances()['mine']['trusted'], 0)

        # The invalid sh(sh(pk())) script label must not be part of the main wallet anymore
        assert label_sh_pkh not in wallet.listlabels()
        # But, the standard sh(pkh()) script should be part of the watch-only wallet.
        addrs_by_label = wallet_wo.getaddressesbylabel(label_sh_pkh)
        assert addy_script_sh_pkh in addrs_by_label
        assert addy_script_double_sh_pkh not in addrs_by_label

        # Also, the watch-only wallet should have the descriptor for the standard sh(pkh())
        desc = descsum_create(f"addr({addy_script_sh_pkh})")
        assert next(it['desc'] for it in wallet_wo.listdescriptors()['descriptors'] if it['desc'] == desc)
        # And doesn't have a descriptor for the invalid one
        desc_invalid = descsum_create(f"addr({addy_script_double_sh_pkh})")
        assert_equal(next((it['desc'] for it in wallet_wo.listdescriptors()['descriptors'] if it['desc'] == desc_invalid), None), None)

        # Just in case, also verify wallet restart
        self.nodes[0].unloadwallet(info_migration["watchonly_name"])
        self.nodes[0].loadwallet(info_migration["watchonly_name"])
        assert_equal(wallet_wo.getbalances()['mine']['trusted'], 5)

    def test_conflict_txs(self):
        self.log.info("Test migration when wallet contains conflicting transactions")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        wallet = self.create_legacy_wallet("conflicts")
        def_wallet.sendtoaddress(wallet.getnewaddress(), 10)
        self.generate(self.nodes[0], 1)

        # parent tx
        parent_txid = wallet.sendtoaddress(wallet.getnewaddress(), 9)
        parent_txid_bytes = bytes.fromhex(parent_txid)[::-1]
        conflict_utxo = wallet.gettransaction(txid=parent_txid, verbose=True)["decoded"]["vin"][0]

        # The specific assertion in MarkConflicted being tested requires that the parent tx is already loaded
        # by the time the child tx is loaded. Since transactions end up being loaded in txid order due to how both
        # and sqlite store things, we can just grind the child tx until it has a txid that is greater than the parent's.
        locktime = 500000000 # Use locktime as nonce, starting at unix timestamp minimum
        addr = wallet.getnewaddress()
        while True:
            child_send_res = wallet.send(outputs=[{addr: 8}], add_to_wallet=False, locktime=locktime)
            child_txid = child_send_res["txid"]
            child_txid_bytes = bytes.fromhex(child_txid)[::-1]
            if (child_txid_bytes > parent_txid_bytes):
                wallet.sendrawtransaction(child_send_res["hex"])
                break
            locktime += 1

        # conflict with parent
        conflict_unsigned = self.nodes[0].createrawtransaction(inputs=[conflict_utxo], outputs=[{wallet.getnewaddress(): 9.9999}])
        conflict_signed = wallet.signrawtransactionwithwallet(conflict_unsigned)["hex"]
        conflict_txid = self.nodes[0].sendrawtransaction(conflict_signed)
        self.generate(self.nodes[0], 1)
        assert_equal(wallet.gettransaction(txid=parent_txid)["confirmations"], -1)
        assert_equal(wallet.gettransaction(txid=child_txid)["confirmations"], -1)
        assert_equal(wallet.gettransaction(txid=conflict_txid)["confirmations"], 1)

        self.migrate_wallet(wallet)
        assert_equal(wallet.gettransaction(txid=parent_txid)["confirmations"], -1)
        assert_equal(wallet.gettransaction(txid=child_txid)["confirmations"], -1)
        assert_equal(wallet.gettransaction(txid=conflict_txid)["confirmations"], 1)

        wallet.unloadwallet()

    def test_hybrid_pubkey(self):
        self.log.info("Test migration when wallet contains a hybrid pubkey")

        wallet = self.create_legacy_wallet("hybrid_keys")

        # Get the hybrid pubkey for one of the keys in the wallet
        normal_pubkey = wallet.getaddressinfo(wallet.getnewaddress())["pubkey"]
        first_byte = bytes.fromhex(normal_pubkey)[0] + 4 # Get the hybrid pubkey first byte
        parsed_pubkey = ECPubKey()
        parsed_pubkey.set(bytes.fromhex(normal_pubkey))
        parsed_pubkey.compressed = False
        hybrid_pubkey_bytes = bytearray(parsed_pubkey.get_bytes())
        hybrid_pubkey_bytes[0] = first_byte # Make it hybrid
        hybrid_pubkey = hybrid_pubkey_bytes.hex()

        # Import the hybrid pubkey
        wallet.importpubkey(hybrid_pubkey)
        p2pkh_addr = key_to_p2pkh(hybrid_pubkey)
        p2pkh_addr_info = wallet.getaddressinfo(p2pkh_addr)
        assert_equal(p2pkh_addr_info["iswatchonly"], True)
        assert_equal(p2pkh_addr_info["ismine"], False) # Things involving hybrid pubkeys are not spendable

        # Also import the p2wpkh for the pubkey to make sure we don't migrate it
        p2wpkh_addr = key_to_p2wpkh(hybrid_pubkey)
        wallet.importaddress(p2wpkh_addr)

        migrate_info = self.migrate_wallet(wallet)

        # Both addresses should only appear in the watchonly wallet
        p2pkh_addr_info = wallet.getaddressinfo(p2pkh_addr)
        assert_equal(p2pkh_addr_info["iswatchonly"], False)
        assert_equal(p2pkh_addr_info["ismine"], False)
        p2wpkh_addr_info = wallet.getaddressinfo(p2wpkh_addr)
        assert_equal(p2wpkh_addr_info["iswatchonly"], False)
        assert_equal(p2wpkh_addr_info["ismine"], False)

        watchonly_wallet = self.nodes[0].get_wallet_rpc(migrate_info["watchonly_name"])
        watchonly_p2pkh_addr_info = watchonly_wallet.getaddressinfo(p2pkh_addr)
        assert_equal(watchonly_p2pkh_addr_info["iswatchonly"], False)
        assert_equal(watchonly_p2pkh_addr_info["ismine"], True)
        watchonly_p2wpkh_addr_info = watchonly_wallet.getaddressinfo(p2wpkh_addr)
        assert_equal(watchonly_p2wpkh_addr_info["iswatchonly"], False)
        assert_equal(watchonly_p2wpkh_addr_info["ismine"], True)

        # There should only be raw or addr descriptors
        for desc in watchonly_wallet.listdescriptors()["descriptors"]:
            if desc["desc"].startswith("raw(") or desc["desc"].startswith("addr("):
                continue
            assert False, "Hybrid pubkey watchonly wallet has more than just raw() and addr()"

        wallet.unloadwallet()

    def test_failed_migration_cleanup(self):
        self.log.info("Test that a failed migration is cleaned up")
        wallet = self.create_legacy_wallet("failed")

        # Make a copy of the wallet with the solvables wallet name so that we are unable
        # to create the solvables wallet when migrating, thus failing to migrate
        wallet.unloadwallet()
        solvables_path = self.nodes[0].wallets_path / "failed_solvables"
        shutil.copytree(self.nodes[0].wallets_path / "failed", solvables_path)
        original_shasum = sha256sum_file(solvables_path / "wallet.dat")

        self.nodes[0].loadwallet("failed")

        # Add a multisig so that a solvables wallet is created
        wallet.addmultisigaddress(2, [wallet.getnewaddress(), get_generate_key().pubkey])
        wallet.importaddress(get_generate_key().p2pkh_addr)

        assert_raises_rpc_error(-4, "Failed to create database", wallet.migratewallet)

        assert "failed" in self.nodes[0].listwallets()
        assert "failed_watchonly" not in self.nodes[0].listwallets()
        assert "failed_solvables" not in self.nodes[0].listwallets()

        assert not (self.nodes[0].wallets_path / "failed_watchonly").exists()
        # Since the file in failed_solvables is one that we put there, migration shouldn't touch it
        assert solvables_path.exists()
        new_shasum = sha256sum_file(solvables_path / "wallet.dat")
        assert_equal(original_shasum, new_shasum)

        wallet.unloadwallet()
        # Check the wallet we tried to migrate is still BDB
        with open(self.nodes[0].wallets_path / "failed" / "wallet.dat", "rb") as f:
            data = f.read(16)
            _, _, magic = struct.unpack("QII", data)
            assert_equal(magic, BTREE_MAGIC)

    def test_blank(self):
        self.log.info("Test that a blank wallet is migrated")
        wallet = self.create_legacy_wallet("blank", blank=True)
        assert_equal(wallet.getwalletinfo()["blank"], True)
        wallet.migratewallet()
        assert_equal(wallet.getwalletinfo()["blank"], True)
        assert_equal(wallet.getwalletinfo()["descriptors"], True)

    def test_avoidreuse(self):
        self.log.info("Test that avoidreuse persists after migration")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        wallet = self.create_legacy_wallet("avoidreuse")
        wallet.setwalletflag("avoid_reuse", True)

        # Import a pubkey to the test wallet and send some funds to it
        reused_imported_addr = def_wallet.getnewaddress()
        wallet.importpubkey(def_wallet.getaddressinfo(reused_imported_addr)["pubkey"])
        imported_utxos = self.create_outpoints(def_wallet, outputs=[{reused_imported_addr: 2}])
        def_wallet.lockunspent(False, imported_utxos)

        # Send funds to the test wallet
        reused_addr = wallet.getnewaddress()
        def_wallet.sendtoaddress(reused_addr, 2)

        self.generate(self.nodes[0], 1)

        # Send funds from the test wallet with both its own and the imported
        wallet.sendall([def_wallet.getnewaddress()])
        def_wallet.sendall(recipients=[def_wallet.getnewaddress()], inputs=imported_utxos)
        self.generate(self.nodes[0], 1)
        balances = wallet.getbalances()
        assert_equal(balances["mine"]["trusted"], 0)
        assert_equal(balances["watchonly"]["trusted"], 0)

        # Reuse the addresses
        def_wallet.sendtoaddress(reused_addr, 1)
        def_wallet.sendtoaddress(reused_imported_addr, 1)
        self.generate(self.nodes[0], 1)
        balances = wallet.getbalances()
        assert_equal(balances["mine"]["used"], 1)
        # Reused watchonly will not show up in balances
        assert_equal(balances["watchonly"]["trusted"], 0)
        assert_equal(balances["watchonly"]["untrusted_pending"], 0)
        assert_equal(balances["watchonly"]["immature"], 0)

        utxos = wallet.listunspent()
        assert_equal(len(utxos), 2)
        for utxo in utxos:
            assert_equal(utxo["reused"], True)

        # Migrate
        migrate_res = wallet.migratewallet()
        watchonly_wallet = self.nodes[0].get_wallet_rpc(migrate_res["watchonly_name"])

        # One utxo in each wallet, marked used
        utxos = wallet.listunspent()
        assert_equal(len(utxos), 1)
        assert_equal(utxos[0]["reused"], True)
        watchonly_utxos = watchonly_wallet.listunspent()
        assert_equal(len(watchonly_utxos), 1)
        assert_equal(watchonly_utxos[0]["reused"], True)

    def test_preserve_tx_extra_info(self):
        self.log.info("Test that tx extra data is preserved after migration")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        # Create and fund wallet
        wallet = self.create_legacy_wallet("persist_comments")
        def_wallet.sendtoaddress(wallet.getnewaddress(), 2)

        self.generate(self.nodes[0], 6)

        # Create tx and bump it to store 'replaced_by_txid' and 'replaces_txid' data within the transactions.
        # Additionally, store an extra comment within the original tx.
        extra_comment = "don't discard me"
        original_tx_id = wallet.sendtoaddress(address=wallet.getnewaddress(), amount=1, comment=extra_comment)
        bumped_tx = wallet.bumpfee(txid=original_tx_id)

        def check_comments():
            for record in wallet.listtransactions():
                if record["txid"] == original_tx_id:
                    assert_equal(record["replaced_by_txid"], bumped_tx["txid"])
                    assert_equal(record['comment'], extra_comment)
                elif record["txid"] == bumped_tx["txid"]:
                    assert_equal(record["replaces_txid"], original_tx_id)

        # Pre-migration verification
        check_comments()
        # Migrate
        wallet.migratewallet()
        # Post-migration verification
        check_comments()

        wallet.unloadwallet()


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
        self.test_default_wallet()
        self.test_direct_file()
        self.test_addressbook()
        self.test_migrate_raw_p2sh()
        self.test_conflict_txs()
        self.test_hybrid_pubkey()
        self.test_failed_migration_cleanup()
        self.test_avoidreuse()
        self.test_preserve_tx_extra_info()
        self.test_blank()

if __name__ == '__main__':
    WalletMigrationTest().main()
