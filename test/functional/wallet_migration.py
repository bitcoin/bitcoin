#!/usr/bin/env python3
# Copyright (c) 2020-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Migrating a wallet from legacy to descriptor."""
import os.path
import random
import shutil
import struct
import time

from test_framework.address import (
    key_to_p2pkh,
    key_to_p2wpkh,
    script_to_p2sh,
    script_to_p2wsh,
)
from test_framework.descriptors import descsum_create
from test_framework.key import ECPubKey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import COIN, CTransaction, CTxOut, ser_string
from test_framework.script import hash160
from test_framework.script_util import key_to_p2pkh_script, key_to_p2pk_script, script_to_p2sh_script, script_to_p2wsh_script
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    find_vout_for_address,
    sha256sum_file,
)
from test_framework.wallet_util import (
    get_generate_key,
    generate_keypair,
)

BTREE_MAGIC = 0x053162


class WalletMigrationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.supports_cli = False
        self.extra_args = [[], ["-deprecatedrpc=create_bdb"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_previous_releases()

    def setup_nodes(self):
        self.add_nodes(
            self.num_nodes,
            extra_args=self.extra_args,
            versions=[
                None,
                280200,
            ],
        )
        self.start_nodes()
        self.init_wallet(node=0)

    def assert_is_sqlite(self, wallet_name):
        wallet_file_path = self.master_node.wallets_path / wallet_name / self.wallet_data_filename
        with open(wallet_file_path, 'rb') as f:
            file_magic = f.read(16)
            assert_equal(file_magic, b'SQLite format 3\x00')
        assert_equal(self.master_node.get_wallet_rpc(wallet_name).getwalletinfo()["format"], "sqlite")

    def assert_is_bdb(self, wallet_name):
        with open(self.master_node.wallets_path / wallet_name / self.wallet_data_filename, "rb") as f:
            data = f.read(16)
            _, _, magic = struct.unpack("QII", data)
            assert_equal(magic, BTREE_MAGIC)

    def create_legacy_wallet(self, wallet_name, **kwargs):
        self.old_node.createwallet(wallet_name=wallet_name, descriptors=False, **kwargs)
        wallet = self.old_node.get_wallet_rpc(wallet_name)
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["format"], "bdb")
        return wallet

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

    def migrate_and_get_rpc(self, wallet_name, **kwargs):
        # Since we may rescan on loading of a wallet, make sure that the best block
        # is written before beginning migration
        # Reload to force write that record
        self.old_node.unloadwallet(wallet_name)
        self.old_node.loadwallet(wallet_name)
        assert_equal(self.old_node.get_wallet_rpc(wallet_name).getwalletinfo()["descriptors"], False)
        # Now unload so we can copy it to the master node for the migration test
        self.old_node.unloadwallet(wallet_name)
        if wallet_name == "":
            shutil.copyfile(self.old_node.wallets_path / "wallet.dat", self.master_node.wallets_path / "wallet.dat")
        else:
            src = os.path.abspath(self.old_node.wallets_path / wallet_name)
            dst = os.path.abspath(self.master_node.wallets_path / wallet_name)
            if src != dst :
                shutil.copytree(self.old_node.wallets_path / wallet_name, self.master_node.wallets_path / wallet_name, dirs_exist_ok=True)
        # Check that the wallet shows up in listwalletdir with a warning about migration
        wallets = self.master_node.listwalletdir()
        for w in wallets["wallets"]:
            if w["name"] == wallet_name:
                assert_equal(w["warnings"], ["This wallet is a legacy wallet and will need to be migrated with migratewallet before it can be loaded"])

        # migratewallet uses current time in naming the backup file, set a mock time
        # to check that this works correctly.
        mocked_time = int(time.time())
        self.master_node.setmocktime(mocked_time)
        # Migrate, checking that rescan does not occur
        with self.master_node.assert_debug_log(expected_msgs=[], unexpected_msgs=["Rescanning"]):
            migrate_info = self.master_node.migratewallet(wallet_name=wallet_name, **kwargs)
        self.master_node.setmocktime(0)
        # Update wallet name in case the initial wallet was completely migrated to a watch-only wallet
        # (in which case the wallet name would be suffixed by the 'watchonly' term)
        migrated_wallet_name = migrate_info['wallet_name']
        wallet = self.master_node.get_wallet_rpc(migrated_wallet_name)
        wallet_info = wallet.getwalletinfo()
        assert_equal(wallet_info["descriptors"], True)
        self.assert_is_sqlite(migrated_wallet_name)
        # Always verify the backup path exist after migration
        assert os.path.exists(migrate_info['backup_path'])
        if wallet_name == "":
            backup_prefix = "default_wallet"
        else:
            backup_prefix = os.path.basename(os.path.realpath(self.old_node.wallets_path / wallet_name))

        backup_filename = f"{backup_prefix}_{mocked_time}.legacy.bak"
        expected_backup_path = self.master_node.wallets_path / backup_filename
        assert_equal(str(expected_backup_path), migrate_info['backup_path'])
        assert {"name": backup_filename} not in self.master_node.listwalletdir()["wallets"]

        # Open the wallet with sqlite and verify that the wallet has the last hardened cache flag
        # set and the last hardened cache entries
        def check_last_hardened(conn):
            flags_rec = conn.execute(f"SELECT value FROM main WHERE key = x'{ser_string(b'flags').hex()}'").fetchone()
            flags = int.from_bytes(flags_rec[0], byteorder="little")

            # All wallets should have the upgrade flag set
            assert_equal(bool(flags & (1 << 2)), True)

            # Fetch all records with the walletdescriptorlhcache prefix
            # if the wallet has private keys and is not blank
            if wallet_info["private_keys_enabled"] and not wallet_info["blank"]:
                lh_cache_recs = conn.execute(f"SELECT value FROM main where key >= x'{ser_string(b'walletdescriptorlhcache').hex()}' AND key < x'{ser_string(b'walletdescriptorlhcachf').hex()}'").fetchall()
                assert_greater_than(len(lh_cache_recs), 0)

        inspect_path = os.path.join(self.options.tmpdir, os.path.basename(f"{migrated_wallet_name}_inspect.dat"))
        wallet.backupwallet(inspect_path)
        self.inspect_sqlite_db(inspect_path, check_last_hardened)

        return migrate_info, wallet

    def test_basic(self):
        default = self.master_node.get_wallet_rpc(self.default_wallet_name)

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
        _, basic0 = self.migrate_and_get_rpc("basic0")

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

        self.generate(self.master_node, 1)

        for _ in range(0, 5):
            basic1.sendtoaddress(default.getnewaddress(), 0.5)

        self.generate(self.master_node, 1)
        bal = basic1.getbalance()
        txs = basic1.listtransactions()
        addr_gps = basic1.listaddressgroupings()

        basic1_migrate, basic1 = self.migrate_and_get_rpc("basic1")
        assert_equal(basic1.getbalance(), bal)
        self.assert_list_txs_equal(basic1.listtransactions(), txs)

        self.log.info("Test backup file can be successfully restored")
        self.old_node.restorewallet("basic1_restored", basic1_migrate['backup_path'])
        basic1_restored = self.old_node.get_wallet_rpc("basic1_restored")
        basic1_restored_wi = basic1_restored.getwalletinfo()
        assert_equal(basic1_restored_wi['balance'], bal)
        assert_equal(basic1_restored.listaddressgroupings(), addr_gps)
        self.assert_list_txs_equal(basic1_restored.listtransactions(), txs)

        # restart master node and verify that everything is still there
        self.restart_node(0)
        self.connect_nodes(0, 1)
        default = self.master_node.get_wallet_rpc(self.default_wallet_name)
        self.master_node.loadwallet("basic1")
        basic1 = self.master_node.get_wallet_rpc("basic1")
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
            self.generate(self.master_node, 1)
            assert_equal(basic2.getbalance(), basic2_balance)
        basic2_txs = basic2.listtransactions()

        # Now migrate and test that we still have the same balance/transactions
        _, basic2 = self.migrate_and_get_rpc("basic2")
        assert_equal(basic2.getbalance(), basic2_balance)
        self.assert_list_txs_equal(basic2.listtransactions(), basic2_txs)

        # Now test migration on a descriptor wallet
        self.log.info("Test \"nothing to migrate\" when the user tries to migrate a loaded wallet with no legacy data")
        assert_raises_rpc_error(-4, "Error: This wallet is already a descriptor wallet", basic2.migratewallet)

        self.log.info("Test \"nothing to migrate\" when the user tries to migrate an unloaded wallet with no legacy data")
        basic2.unloadwallet()
        assert_raises_rpc_error(-4, "Error: This wallet is already a descriptor wallet", self.master_node.migratewallet, "basic2")

    def test_multisig(self):
        default = self.master_node.get_wallet_rpc(self.default_wallet_name)

        # Contrived case where all the multisig keys are in a single wallet
        self.log.info("Test migration of a wallet with all keys for a multisig")
        multisig0 = self.create_legacy_wallet("multisig0")
        addr1 = multisig0.getnewaddress()
        addr2 = multisig0.getnewaddress()
        addr3 = multisig0.getnewaddress()

        ms_info = multisig0.addmultisigaddress(2, [addr1, addr2, addr3])

        _, multisig0 = self.migrate_and_get_rpc("multisig0")
        ms_addr_info = multisig0.getaddressinfo(ms_info["address"])
        assert_equal(ms_addr_info["ismine"], True)
        assert_equal(ms_addr_info["desc"], ms_info["descriptor"])
        assert_equal("multisig0_watchonly" in self.master_node.listwallets(), False)
        assert_equal("multisig0_solvables" in self.master_node.listwallets(), False)

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
        self.generate(self.master_node, 1)
        multisig1.gettransaction(txid)
        assert_equal(multisig1.getbalances()["watchonly"]["trusted"], 10)
        assert_equal(multisig1.getaddressinfo(addr2)["ismine"], False)
        assert_equal(multisig1.getaddressinfo(addr2)["iswatchonly"], False)
        assert_equal(multisig1.getaddressinfo(addr2)["solvable"], True)

        # Migrating multisig1 should see the multisig is no longer part of multisig1
        # A new wallet multisig1_watchonly is created which has the multisig address
        # Transaction to multisig is in multisig1_watchonly and not multisig1
        _, multisig1 = self.migrate_and_get_rpc("multisig1")
        assert_equal(multisig1.getaddressinfo(addr1)["ismine"], False)
        assert_equal(multisig1.getaddressinfo(addr1)["solvable"], False)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", multisig1.gettransaction, txid)
        assert_equal(multisig1.getbalance(), 0)
        assert_equal(multisig1.listtransactions(), [])

        assert_equal("multisig1_watchonly" in self.master_node.listwallets(), True)
        ms1_watchonly = self.master_node.get_wallet_rpc("multisig1_watchonly")
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
        assert_equal("multisig1_solvables" in self.master_node.listwallets(), True)
        ms1_solvable = self.master_node.get_wallet_rpc("multisig1_solvables")
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
        default = self.master_node.get_wallet_rpc(self.default_wallet_name)

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

        self.generate(self.master_node, 2)
        received_watchonly_tx_info = imports0.gettransaction(received_watchonly_txid, True)
        received_sent_watchonly_tx_info = imports0.gettransaction(received_sent_watchonly_utxo["txid"], True)

        balances = imports0.getbalances()
        spendable_bal = balances["mine"]["trusted"]
        watchonly_bal = balances["watchonly"]["trusted"]
        assert_equal(len(imports0.listtransactions(include_watchonly=True)), 6)

        # Mock time forward a bit so we can check that tx metadata is preserved
        self.master_node.setmocktime(int(time.time()) + 100)

        # Migrate
        _, imports0 = self.migrate_and_get_rpc("imports0")
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", imports0.gettransaction, received_watchonly_txid)
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", imports0.gettransaction, received_sent_watchonly_utxo['txid'])
        assert_raises_rpc_error(-5, "Invalid or non-wallet transaction id", imports0.gettransaction, sent_watchonly_txid)
        assert_equal(len(imports0.listtransactions()), 2)
        imports0.gettransaction(received_txid)
        imports0.gettransaction(watchonly_spendable_txid)
        assert_equal(imports0.getbalance(), spendable_bal)

        assert_equal("imports0_watchonly" in self.master_node.listwallets(), True)
        watchonly = self.master_node.get_wallet_rpc("imports0_watchonly")
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
        self.master_node.unloadwallet("imports0_watchonly")
        self.master_node.loadwallet("imports0_watchonly")
        labels = watchonly.listlabels()
        assert "external" in labels
        assert "imported" in labels

    def test_no_privkeys(self):
        default = self.master_node.get_wallet_rpc(self.default_wallet_name)

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
        self.generate(self.master_node, 1)

        _, watchonly0 = self.migrate_and_get_rpc("watchonly0")
        assert_equal("watchonly0_watchonly" in self.master_node.listwallets(), False)
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

        _, watchonly1 = self.migrate_and_get_rpc("watchonly1")
        info = watchonly1.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["private_keys_enabled"], False)
        self.assert_is_sqlite("watchonly1")
        # After migrating, the "keypool" is empty
        assert_raises_rpc_error(-4, "Error: This wallet has no available keys", watchonly1.getnewaddress)

        self.log.info("Test migration of a watch-only empty wallet")
        for idx, is_blank in enumerate([True, False], start=1):
            wallet_name = f"watchonly_empty{idx}"
            self.create_legacy_wallet(wallet_name, disable_private_keys=True, blank=is_blank)
            _, watchonly_empty = self.migrate_and_get_rpc(wallet_name)
            info = watchonly_empty.getwalletinfo()
            assert_equal(info["private_keys_enabled"], False)
            assert_equal(info["blank"], is_blank)

    def test_pk_coinbases(self):
        self.log.info("Test migration of a wallet using old pk() coinbases")
        wallet = self.create_legacy_wallet("pkcb")

        addr = wallet.getnewaddress()
        addr_info = wallet.getaddressinfo(addr)
        desc = descsum_create("pk(" + addr_info["pubkey"] + ")")

        self.generatetodescriptor(self.master_node, 1, desc)

        bals = wallet.getbalances()

        _, wallet = self.migrate_and_get_rpc("pkcb")

        assert_equal(bals, wallet.getbalances())

    def test_encrypted(self):
        self.log.info("Test migration of an encrypted wallet")
        wallet = self.create_legacy_wallet("encrypted")
        default = self.master_node.get_wallet_rpc(self.default_wallet_name)

        wallet.encryptwallet("pass")
        addr = wallet.getnewaddress()
        txid = default.sendtoaddress(addr, 1)
        self.generate(self.master_node, 1)
        bals = wallet.getbalances()

        # Use self.migrate_and_get_rpc to test this error to get everything copied over to the master node
        assert_raises_rpc_error(-4, "Error: Wallet decryption failed, the wallet passphrase was not provided or was incorrect", self.migrate_and_get_rpc, "encrypted")

        # Use the RPC directly on the master node for the rest of these checks
        self.master_node.bumpmocktime(1) # Prevents filename duplication on wallet backups which is a problem on Windows
        assert_raises_rpc_error(-4, "Error: Wallet decryption failed, the wallet passphrase was not provided or was incorrect", self.master_node.migratewallet, "encrypted", "badpass")

        self.master_node.bumpmocktime(1) # Prevents filename duplication on wallet backups which is a problem on Windows
        assert_raises_rpc_error(-4, "The passphrase contains a null character", self.master_node.migratewallet, "encrypted", "pass\0with\0null")

        # Verify we can properly migrate the encrypted wallet
        self.master_node.bumpmocktime(1) # Prevents filename duplication on wallet backups which is a problem on Windows
        self.master_node.migratewallet("encrypted", passphrase="pass")
        wallet = self.master_node.get_wallet_rpc("encrypted")

        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["format"], "sqlite")
        assert_equal(info["unlocked_until"], 0)
        wallet.gettransaction(txid)

        assert_equal(bals, wallet.getbalances())

    def test_nonexistent(self):
        self.log.info("Check migratewallet errors for nonexistent wallets")
        default = self.master_node.get_wallet_rpc(self.default_wallet_name)
        assert_raises_rpc_error(-8, "The RPC endpoint wallet and the wallet name parameter specify different wallets", default.migratewallet, "someotherwallet")
        assert_raises_rpc_error(-8, "Either the RPC endpoint wallet or the wallet name parameter must be provided", self.master_node.migratewallet)
        assert_raises_rpc_error(-4, "Error: Wallet does not exist", self.master_node.migratewallet, "notawallet")

    def test_unloaded_by_path(self):
        self.log.info("Test migration of a wallet that isn't loaded, specified by path")
        wallet = self.create_legacy_wallet("notloaded2")
        default = self.master_node.get_wallet_rpc(self.default_wallet_name)

        addr = wallet.getnewaddress()
        txid = default.sendtoaddress(addr, 1)
        self.generate(self.master_node, 1)
        bals = wallet.getbalances()

        wallet.unloadwallet()

        wallet_file_path = self.old_node.wallets_path / "notloaded2"
        self.master_node.migratewallet(wallet_file_path)

        # Because we gave the name by full path, the loaded wallet's name is that path too.
        wallet = self.master_node.get_wallet_rpc(str(wallet_file_path))

        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["format"], "sqlite")
        wallet.gettransaction(txid)

        assert_equal(bals, wallet.getbalances())

    def test_wallet_with_relative_path(self):
        self.log.info("Test migration of a wallet that isn't loaded, specified by a relative path")

        # Get the nearest common path of both nodes' wallet paths.
        common_parent = os.path.commonpath([self.master_node.wallets_path, self.old_node.wallets_path])

        # This test assumes that the relative path from each wallet directory to the common path is identical.
        assert_equal(os.path.relpath(common_parent, start=self.master_node.wallets_path), os.path.relpath(common_parent, start=self.old_node.wallets_path))

        wallet_name = "relative"
        absolute_path = os.path.abspath(os.path.join(common_parent, wallet_name))
        relative_name = os.path.relpath(absolute_path, start=self.master_node.wallets_path)

        wallet = self.create_legacy_wallet(relative_name)
        # listwalletdirs only returns wallets in the wallet directory
        assert {"name": relative_name} not in wallet.listwalletdir()["wallets"]
        assert relative_name in wallet.listwallets()

        default = self.master_node.get_wallet_rpc(self.default_wallet_name)
        addr = wallet.getnewaddress()
        txid = default.sendtoaddress(addr, 1)
        self.generate(self.master_node, 1)
        bals = wallet.getbalances()

        migrate_res, wallet = self.migrate_and_get_rpc(relative_name)

        # Check that the wallet was migrated, knows the right txid, and has the right balance.
        assert wallet.gettransaction(txid)
        assert_equal(bals, wallet.getbalances())

        # The migrated wallet should not be in the wallet dir, but should be in the list of wallets.
        info = wallet.getwalletinfo()

        walletdirlist = wallet.listwalletdir()
        assert {"name": info["walletname"]} not in walletdirlist["wallets"]

        walletlist = wallet.listwallets()
        assert info["walletname"] in walletlist

        # Check that old node can restore from the backup.
        self.old_node.restorewallet("relative_restored", migrate_res['backup_path'])
        wallet = self.old_node.get_wallet_rpc("relative_restored")
        assert wallet.gettransaction(txid)
        assert_equal(bals, wallet.getbalances())

        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["format"], "bdb")

    def test_wallet_with_path(self, wallet_path):
        self.log.info("Test migrating a wallet with the following path/name: %s", wallet_path)
        # the wallet data is actually inside of path/that/ends/
        wallet = self.create_legacy_wallet(wallet_path)
        default = self.master_node.get_wallet_rpc(self.default_wallet_name)

        addr = wallet.getnewaddress()
        txid = default.sendtoaddress(addr, 1)
        self.generate(self.master_node, 1)
        bals = wallet.getbalances()

        _, wallet = self.migrate_and_get_rpc(wallet_path)

        assert wallet.gettransaction(txid)

        assert_equal(bals, wallet.getbalances())

    def test_default_wallet(self):
        self.log.info("Test migration of the wallet named as the empty string")
        wallet = self.create_legacy_wallet("")

        res, wallet = self.migrate_and_get_rpc("")
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["format"], "sqlite")

        walletdir_list = wallet.listwalletdir()
        assert {"name": info["walletname"]} in [{"name": w["name"]} for w in walletdir_list["wallets"]]

        # Make sure the backup uses a non-empty filename
        # migrate_and_get_rpc already checks for backup file existence
        assert os.path.basename(res["backup_path"]).startswith("default_wallet")

        wallet.unloadwallet()

    def test_default_wallet_failure(self):
        self.log.info("Test failure during unnamed (default) wallet migration")
        master_wallet = self.master_node.get_wallet_rpc(self.default_wallet_name)
        wallet = self.create_legacy_wallet("", blank=True)
        wallet.importaddress(master_wallet.getnewaddress(address_type="legacy"))

        # Create wallet directory with the watch-only name and a wallet file.
        # Because the wallet dir exists, this will cause migration to fail.
        watch_only_dir = self.master_node.wallets_path / "_watchonly"
        os.mkdir(watch_only_dir)
        shutil.copyfile(self.old_node.wallets_path / "wallet.dat", watch_only_dir / "wallet.dat")

        mocked_time = int(time.time())
        self.master_node.setmocktime(mocked_time)
        assert_raises_rpc_error(-4, "Failed to create database", self.migrate_and_get_rpc, "")
        self.master_node.setmocktime(0)

        # Verify the /wallets/ path exists
        assert self.master_node.wallets_path.exists()
        # Check backup file exists. Because the wallet has no name, the backup is prefixed with 'default_wallet'
        assert (self.master_node.wallets_path / f"default_wallet_{mocked_time}.legacy.bak").exists()
        # Verify the original unnamed wallet was restored
        assert (self.master_node.wallets_path / "wallet.dat").exists()
        # And verify it is still a BDB wallet
        with open(self.master_node.wallets_path / "wallet.dat", "rb") as f:
            data = f.read(16)
            _, _, magic = struct.unpack("QII", data)
            assert_equal(magic, BTREE_MAGIC)

        # Test cleanup: Clear unnamed default wallet from old node
        os.remove(self.old_node.wallets_path / "wallet.dat")

    def test_direct_file(self):
        self.log.info("Test migration of a wallet that is not in a wallet directory")
        wallet = self.create_legacy_wallet("plainfile")
        wallet.unloadwallet()

        shutil.copyfile(
            self.old_node.wallets_path / "plainfile" / "wallet.dat" ,
            self.master_node.wallets_path / "plainfile"
        )
        assert (self.master_node.wallets_path / "plainfile").is_file()

        mocked_time = int(time.time())
        self.master_node.setmocktime(mocked_time)
        migrate_res = self.master_node.migratewallet("plainfile")
        assert_equal(f"plainfile_{mocked_time}.legacy.bak", os.path.basename(migrate_res["backup_path"]))
        wallet = self.master_node.get_wallet_rpc("plainfile")
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["format"], "sqlite")

        assert (self.master_node.wallets_path / "plainfile").is_dir()
        assert (self.master_node.wallets_path / "plainfile" / "wallet.dat").is_file()

    def test_addressbook(self):
        df_wallet = self.master_node.get_wallet_rpc(self.default_wallet_name)

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

        self.generate(self.master_node, 1)

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
        self.generate(self.master_node, 1)

        # Util wrapper func for 'addr_info'
        def check(info, node):
            self.check_address(node, info['addr'], info['is_mine'], info['is_change'], info["label"])

        # Pre-migration: set label and perform initial checks
        for addr_info in [addr_external, addr_external_with_label, addr_internal, addr_internal_with_label, change_address, watch_only_addr, ms_addr]:
            if not addr_info['is_change']:
                wallet.setlabel(addr_info['addr'], addr_info["label"])
            check(addr_info, wallet)

        # Migrate wallet
        info_migration, wallet = self.migrate_and_get_rpc("legacy_addrbook")
        wallet_wo = self.master_node.get_wallet_rpc(info_migration["watchonly_name"])
        wallet_solvables = self.master_node.get_wallet_rpc(info_migration["solvables_name"])

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
        self.master_node.unloadwallet("legacy_addrbook")
        self.master_node.loadwallet("legacy_addrbook")
        for addr_info in [addr_external, addr_external_with_label, addr_internal, addr_internal_with_label, change_address, ms_addr]:
            check(addr_info, wallet)

        # Watch-only wallet
        self.master_node.unloadwallet(info_migration["watchonly_name"])
        self.master_node.loadwallet(info_migration["watchonly_name"])
        self.check_address(wallet_wo, watch_only_addr['addr'], is_mine=True, is_change=watch_only_addr['is_change'], label=watch_only_addr["label"])
        for addr_info in [addr_external, addr_external_with_label, ms_addr]:
            check(addr_info, wallet_wo)

        # Solvables wallet
        self.master_node.unloadwallet(info_migration["solvables_name"])
        self.master_node.loadwallet(info_migration["solvables_name"])
        self.check_address(wallet_solvables, ms_addr['addr'], is_mine=True, is_change=ms_addr['is_change'], label=ms_addr["label"])
        for addr_info in [addr_external, addr_external_with_label]:
            check(addr_info, wallet_solvables)

    def test_migrate_raw_p2sh(self):
        self.log.info("Test migration of watch-only raw p2sh script")
        df_wallet = self.master_node.get_wallet_rpc(self.default_wallet_name)
        wallet = self.create_legacy_wallet("raw_p2sh")

        def send_to_script(script, amount):
            tx = CTransaction()
            tx.vout.append(CTxOut(nValue=amount*COIN, scriptPubKey=script))

            hex_tx = df_wallet.fundrawtransaction(tx.serialize().hex())['hex']
            signed_tx = df_wallet.signrawtransactionwithwallet(hex_tx)
            df_wallet.sendrawtransaction(signed_tx['hex'])
            self.generate(self.master_node, 1)

        # Craft sh(pkh(key)) script and send coins to it
        pubkey = df_wallet.getaddressinfo(df_wallet.getnewaddress())["pubkey"]
        script_pkh = key_to_p2pkh_script(pubkey)
        script_sh_pkh = script_to_p2sh_script(script_pkh)
        send_to_script(script=script_sh_pkh, amount=2)

        # Import script and check balance
        wallet.importaddress(address=script_pkh.hex(), label="raw_spk", rescan=True, p2sh=True)
        assert_equal(wallet.getbalances()['watchonly']['trusted'], 2)

        # Craft wsh(pkh(key)) and send coins to it
        pubkey = df_wallet.getaddressinfo(df_wallet.getnewaddress())["pubkey"]
        script_wsh_pkh = script_to_p2wsh_script(key_to_p2pkh_script(pubkey))
        send_to_script(script=script_wsh_pkh, amount=3)

        # Import script and check balance
        wallet.importaddress(address=script_wsh_pkh.hex(), label="raw_spk2", rescan=True, p2sh=False)
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
        wallet.importaddress(address=script_sh_pkh.hex(), label=label_sh_pkh, rescan=False, p2sh=True)

        # Migrate wallet and re-check balance
        info_migration, wallet = self.migrate_and_get_rpc("raw_p2sh")
        wallet_wo = self.master_node.get_wallet_rpc(info_migration["watchonly_name"])

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
        self.master_node.unloadwallet(info_migration["watchonly_name"])
        self.master_node.loadwallet(info_migration["watchonly_name"])
        assert_equal(wallet_wo.getbalances()['mine']['trusted'], 5)

    def test_conflict_txs(self):
        self.log.info("Test migration when wallet contains conflicting transactions")
        def_wallet = self.master_node.get_wallet_rpc(self.default_wallet_name)

        wallet = self.create_legacy_wallet("conflicts")
        def_wallet.sendtoaddress(wallet.getnewaddress(), 10)
        self.generate(self.master_node, 1)

        # parent tx
        parent_txid = wallet.sendtoaddress(wallet.getnewaddress(), 9)
        parent_txid_bytes = bytes.fromhex(parent_txid)[::-1]
        conflict_utxo = wallet.gettransaction(txid=parent_txid, verbose=True)["decoded"]["vin"][0]

        # The specific assertion in MarkConflicted being tested requires that the parent tx is already loaded
        # by the time the child tx is loaded. Since transactions end up being loaded in txid order due to how
        # sqlite stores things, we can just grind the child tx until it has a txid that is greater than the parent's.
        locktime = 500000000 # Use locktime as nonce, starting at unix timestamp minimum
        addr = wallet.getnewaddress()
        while True:
            child_send_res = wallet.send(outputs=[{addr: 8}], options={"add_to_wallet": False, "locktime": locktime})
            child_txid = child_send_res["txid"]
            child_txid_bytes = bytes.fromhex(child_txid)[::-1]
            if (child_txid_bytes > parent_txid_bytes):
                wallet.sendrawtransaction(child_send_res["hex"])
                break
            locktime += 1

        # conflict with parent
        conflict_unsigned = self.master_node.createrawtransaction(inputs=[conflict_utxo], outputs=[{wallet.getnewaddress(): 9.9999}])
        conflict_signed = wallet.signrawtransactionwithwallet(conflict_unsigned)["hex"]
        conflict_txid = self.master_node.sendrawtransaction(conflict_signed)
        self.generate(self.master_node, 1)
        assert_equal(wallet.gettransaction(txid=parent_txid)["confirmations"], -1)
        assert_equal(wallet.gettransaction(txid=child_txid)["confirmations"], -1)
        assert_equal(wallet.gettransaction(txid=conflict_txid)["confirmations"], 1)

        _, wallet = self.migrate_and_get_rpc("conflicts")
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

        migrate_info, wallet = self.migrate_and_get_rpc("hybrid_keys")

        # Both addresses should only appear in the watchonly wallet
        p2pkh_addr_info = wallet.getaddressinfo(p2pkh_addr)
        assert_equal(p2pkh_addr_info["ismine"], False)
        p2wpkh_addr_info = wallet.getaddressinfo(p2wpkh_addr)
        assert_equal(p2wpkh_addr_info["ismine"], False)

        watchonly_wallet = self.master_node.get_wallet_rpc(migrate_info["watchonly_name"])
        watchonly_p2pkh_addr_info = watchonly_wallet.getaddressinfo(p2pkh_addr)
        assert_equal(watchonly_p2pkh_addr_info["ismine"], True)
        watchonly_p2wpkh_addr_info = watchonly_wallet.getaddressinfo(p2wpkh_addr)
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
        solvables_path = self.master_node.wallets_path / "failed_solvables"
        shutil.copytree(self.old_node.wallets_path / "failed", solvables_path)
        original_shasum = sha256sum_file(solvables_path / "wallet.dat")

        self.old_node.loadwallet("failed")

        # Add a multisig so that a solvables wallet is created
        wallet.addmultisigaddress(2, [wallet.getnewaddress(), get_generate_key().pubkey])
        wallet.importaddress(get_generate_key().p2pkh_addr)

        self.old_node.unloadwallet("failed")
        shutil.copytree(self.old_node.wallets_path / "failed", self.master_node.wallets_path / "failed")
        assert_raises_rpc_error(-4, "Failed to create database", self.master_node.migratewallet, "failed")

        assert all(wallet not in self.master_node.listwallets() for wallet in ["failed", "failed_watchonly", "failed_solvables"])

        assert not (self.master_node.wallets_path / "failed_watchonly").exists()
        # Since the file in failed_solvables is one that we put there, migration shouldn't touch it
        assert solvables_path.exists()
        new_shasum = sha256sum_file(solvables_path / "wallet.dat")
        assert_equal(original_shasum, new_shasum)

        # Check the wallet we tried to migrate is still BDB
        self.assert_is_bdb("failed")

    def test_failed_migration_cleanup_relative_path(self):
        self.log.info("Test that a failed migration with a relative path is cleaned up")

        # Get the nearest common path of both nodes' wallet paths.
        common_parent = os.path.commonpath([self.master_node.wallets_path, self.old_node.wallets_path])

        # This test assumes that the relative path from each wallet directory to the common path is identical.
        assert_equal(os.path.relpath(common_parent, start=self.master_node.wallets_path), os.path.relpath(common_parent, start=self.old_node.wallets_path))

        wallet_name = "relativefailure"
        absolute_path = os.path.abspath(os.path.join(common_parent, wallet_name))
        relative_name = os.path.relpath(absolute_path, start=self.master_node.wallets_path)

        wallet = self.create_legacy_wallet(relative_name)

        # Make a copy of the wallet with the solvables wallet name so that we are unable
        # to create the solvables wallet when migrating, thus failing to migrate
        wallet.unloadwallet()
        solvables_path = os.path.join(common_parent, f"{wallet_name}_solvables")

        shutil.copytree(self.old_node.wallets_path / relative_name, solvables_path)
        original_shasum = sha256sum_file(os.path.join(solvables_path, "wallet.dat"))

        self.old_node.loadwallet(relative_name)

        # Add a multisig so that a solvables wallet is created
        wallet.addmultisigaddress(2, [wallet.getnewaddress(), get_generate_key().pubkey])
        wallet.importaddress(get_generate_key().p2pkh_addr)

        self.old_node.unloadwallet(relative_name)
        assert_raises_rpc_error(-4, "Failed to create database", self.master_node.migratewallet, relative_name)

        assert all(wallet not in self.master_node.listwallets() for wallet in [f"{wallet_name}", f"{wallet_name}_watchonly", f"{wallet_name}_solvables"])

        assert not (self.master_node.wallets_path / f"{wallet_name}_watchonly").exists()
        # Since the file in failed_solvables is one that we put there, migration shouldn't touch it
        assert os.path.exists(solvables_path)
        new_shasum = sha256sum_file(os.path.join(solvables_path , "wallet.dat"))
        assert_equal(original_shasum, new_shasum)

        # Check the wallet we tried to migrate is still BDB
        datfile = os.path.join(absolute_path, "wallet.dat")
        with open(datfile, "rb") as f:
            data = f.read(16)
            _, _, magic = struct.unpack("QII", data)
            assert_equal(magic, BTREE_MAGIC)

    def test_blank(self):
        self.log.info("Test that a blank wallet is migrated")
        wallet = self.create_legacy_wallet("blank", blank=True)
        assert_equal(wallet.getwalletinfo()["blank"], True)
        _, wallet = self.migrate_and_get_rpc("blank")
        assert_equal(wallet.getwalletinfo()["blank"], True)

    def test_avoidreuse(self):
        self.log.info("Test that avoidreuse persists after migration")
        def_wallet = self.master_node.get_wallet_rpc(self.default_wallet_name)

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

        self.generate(self.master_node, 1)

        # Send funds from the test wallet with both its own and the imported
        wallet.sendall([def_wallet.getnewaddress()])
        def_wallet.sendall(recipients=[def_wallet.getnewaddress()], inputs=imported_utxos)
        self.generate(self.master_node, 1)
        balances = wallet.getbalances()
        assert_equal(balances["mine"]["trusted"], 0)
        assert_equal(balances["watchonly"]["trusted"], 0)

        # Reuse the addresses
        def_wallet.sendtoaddress(reused_addr, 1)
        def_wallet.sendtoaddress(reused_imported_addr, 1)
        self.generate(self.master_node, 1)
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
        _, wallet = self.migrate_and_get_rpc("avoidreuse")
        watchonly_wallet = self.master_node.get_wallet_rpc("avoidreuse_watchonly")

        # One utxo in each wallet, marked used
        utxos = wallet.listunspent()
        assert_equal(len(utxos), 1)
        assert_equal(utxos[0]["reused"], True)
        watchonly_utxos = watchonly_wallet.listunspent()
        assert_equal(len(watchonly_utxos), 1)
        assert_equal(watchonly_utxos[0]["reused"], True)

    def test_preserve_tx_extra_info(self):
        self.log.info("Test that tx extra data is preserved after migration")
        def_wallet = self.master_node.get_wallet_rpc(self.default_wallet_name)

        # Create and fund wallet
        wallet = self.create_legacy_wallet("persist_comments")
        def_wallet.sendtoaddress(wallet.getnewaddress(), 2)

        self.generate(self.master_node, 6)

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
        _, wallet = self.migrate_and_get_rpc("persist_comments")
        # Post-migration verification
        check_comments()

        wallet.unloadwallet()

    def test_migrate_simple_watch_only(self):
        self.log.info("Test migrating a watch-only p2pk script")
        wallet = self.create_legacy_wallet("bare_p2pk", blank=True)
        _, pubkey = generate_keypair()
        p2pk_script = key_to_p2pk_script(pubkey)
        wallet.importaddress(address=p2pk_script.hex())
        # Migrate wallet in the latest node
        res, _ = self.migrate_and_get_rpc("bare_p2pk")
        wo_wallet = self.master_node.get_wallet_rpc(res['wallet_name'])
        assert_equal(wo_wallet.listdescriptors()['descriptors'][0]['desc'], descsum_create(f'pk({pubkey.hex()})'))
        assert_equal(wo_wallet.getwalletinfo()["private_keys_enabled"], False)

        # Ensure that migrating a wallet with watch-only scripts does not create a spendable wallet.
        assert_equal('bare_p2pk_watchonly', res['wallet_name'])
        assert "bare_p2pk" not in self.master_node.listwallets()
        assert "bare_p2pk" not in [w["name"] for w in self.master_node.listwalletdir()["wallets"]]

        wo_wallet.unloadwallet()

    def test_manual_keys_import(self):
        self.log.info("Test migrating standalone private keys")
        wallet = self.create_legacy_wallet("import_privkeys", blank=True)
        privkey, pubkey = generate_keypair(wif=True)
        wallet.importprivkey(privkey=privkey, label="hi", rescan=False)

        # Migrate and verify
        res, wallet = self.migrate_and_get_rpc("import_privkeys")

        # There should be descriptors containing the imported key for: pk(), pkh(), sh(wpkh()), wpkh()
        key_origin = hash160(pubkey)[:4].hex()
        pubkey_hex = pubkey.hex()
        combo_desc = descsum_create(f"combo([{key_origin}]{pubkey_hex})")

        # Verify all expected descriptors were migrated
        migrated_desc = [item['desc'] for item in wallet.listdescriptors()['descriptors'] if pubkey.hex() in item['desc']]
        assert_equal([combo_desc], migrated_desc)
        wallet.unloadwallet()

        ######################################################
        self.log.info("Test migrating standalone public keys")
        wallet = self.create_legacy_wallet("import_pubkeys", blank=True)
        wallet.importpubkey(pubkey=pubkey_hex, rescan=False)

        res, _ = self.migrate_and_get_rpc("import_pubkeys")

        # Same as before, there should be descriptors in the watch-only wallet for the imported pubkey
        wo_wallet = self.nodes[0].get_wallet_rpc(res['wallet_name'])
        # Assert this is a watch-only wallet
        assert_equal(wo_wallet.getwalletinfo()["private_keys_enabled"], False)
        # As we imported the pubkey only, there will be no key origin in the following descriptors
        pk_desc = descsum_create(f'pk({pubkey_hex})')
        pkh_desc = descsum_create(f'pkh({pubkey_hex})')
        sh_wpkh_desc = descsum_create(f'sh(wpkh({pubkey_hex}))')
        wpkh_desc = descsum_create(f'wpkh({pubkey_hex})')
        expected_descs = [pk_desc, pkh_desc, sh_wpkh_desc, wpkh_desc]

        # Verify all expected descriptors were migrated
        migrated_desc = [item['desc'] for item in wo_wallet.listdescriptors()['descriptors']]
        assert_equal(expected_descs, migrated_desc)
        # Ensure that migrating a wallet with watch-only scripts does not create a spendable wallet.
        assert_equal('import_pubkeys_watchonly', res['wallet_name'])
        assert "import_pubkeys" not in self.master_node.listwallets()
        assert "import_pubkeys" not in [w["name"] for w in self.master_node.listwalletdir()["wallets"]]
        wo_wallet.unloadwallet()

    def test_p2wsh(self):
        self.log.info("Test that non-multisig P2WSH output scripts are migrated")
        def_wallet = self.master_node.get_wallet_rpc(self.default_wallet_name)

        wallet = self.create_legacy_wallet("p2wsh")

        # Craft wsh(pkh(key))
        pubkey = wallet.getaddressinfo(wallet.getnewaddress())["pubkey"]
        pkh_script = key_to_p2pkh_script(pubkey).hex()
        wsh_pkh_script = script_to_p2wsh_script(pkh_script).hex()
        wsh_pkh_addr = script_to_p2wsh(pkh_script)

        # Legacy single key scripts (i.e. pkh(key) and pk(key)) are not inserted into mapScripts
        # automatically, they need to be imported directly if we want to receive to P2WSH (or P2SH)
        # wrappings of such scripts.
        wallet.importaddress(address=pkh_script, p2sh=False)
        wallet.importaddress(address=wsh_pkh_script, p2sh=False)

        def_wallet.sendtoaddress(wsh_pkh_addr, 5)
        self.generate(self.nodes[0], 6)
        assert_equal(wallet.getbalances()['mine']['trusted'], 5)

        _, wallet = self.migrate_and_get_rpc("p2wsh")

        assert_equal(wallet.getbalances()['mine']['trusted'], 5)
        addr_info = wallet.getaddressinfo(wsh_pkh_addr)
        assert_equal(addr_info["ismine"], True)
        assert_equal(addr_info["solvable"], True)

        wallet.unloadwallet()

    def test_disallowed_p2wsh(self):
        self.log.info("Test that P2WSH output scripts with invalid witnessScripts are not migrated and do not cause migration failure")
        def_wallet = self.master_node.get_wallet_rpc(self.default_wallet_name)

        wallet = self.create_legacy_wallet("invalid_p2wsh")

        invalid_addrs = []

        # For a P2WSH output script stored in the legacy wallet's mapScripts, both the native P2WSH
        # and the P2SH-P2WSH are detected by IsMine. We need to verify that descriptors for both
        # output scripts are added to the resulting descriptor wallet.
        # However, this cannot be done using a multisig as wallet migration treats multisigs specially.
        # Instead, this is tested by importing a wsh(pkh()) script. But importing this directly will
        # insert the wsh() into setWatchOnly which means that the setWatchOnly migration ends up handling
        # this case, which we do not want.
        # In order to get the wsh(pkh()) into only mapScripts and not setWatchOnly, we need to utilize
        # importmulti and wrap the wsh(pkh()) inside of a sh(). This will insert the sh(wsh(pkh())) into
        # setWatchOnly but not the wsh(pkh()).
        # Furthermore, migration should not migrate the wsh(pkh()) if the key is uncompressed.
        comp_wif, comp_pubkey = generate_keypair(compressed=True, wif=True)
        comp_pkh_script = key_to_p2pkh_script(comp_pubkey).hex()
        comp_wsh_pkh_script = script_to_p2wsh_script(comp_pkh_script).hex()
        comp_sh_wsh_pkh_script = script_to_p2sh_script(comp_wsh_pkh_script).hex()
        comp_wsh_pkh_addr = script_to_p2wsh(comp_pkh_script)

        uncomp_wif, uncomp_pubkey = generate_keypair(compressed=False, wif=True)
        uncomp_pkh_script = key_to_p2pkh_script(uncomp_pubkey).hex()
        uncomp_wsh_pkh_script = script_to_p2wsh_script(uncomp_pkh_script).hex()
        uncomp_sh_wsh_pkh_script = script_to_p2sh_script(uncomp_wsh_pkh_script).hex()
        uncomp_wsh_pkh_addr = script_to_p2wsh(uncomp_pkh_script)
        invalid_addrs.append(uncomp_wsh_pkh_addr)

        import_res = wallet.importmulti([
            {
                "scriptPubKey": comp_sh_wsh_pkh_script,
                "timestamp": "now",
                "redeemscript": comp_wsh_pkh_script,
                "witnessscript": comp_pkh_script,
                "keys": [
                    comp_wif,
                ],
            },
            {
                "scriptPubKey": uncomp_sh_wsh_pkh_script,
                "timestamp": "now",
                "redeemscript": uncomp_wsh_pkh_script,
                "witnessscript": uncomp_pkh_script,
                "keys": [
                    uncomp_wif,
                ],
            },
        ])
        assert_equal(import_res[0]["success"], True)
        assert_equal(import_res[1]["success"], True)

        # Create a wsh(sh(pkh())) - P2SH inside of P2WSH is invalid
        comp_sh_pkh_script = script_to_p2sh_script(comp_pkh_script).hex()
        wsh_sh_pkh_script = script_to_p2wsh_script(comp_sh_pkh_script).hex()
        wsh_sh_pkh_addr = script_to_p2wsh(comp_sh_pkh_script)
        invalid_addrs.append(wsh_sh_pkh_addr)

        # Import wsh(sh(pkh()))
        wallet.importaddress(address=comp_sh_pkh_script, p2sh=False)
        wallet.importaddress(address=wsh_sh_pkh_script, p2sh=False)

        # Create a wsh(wsh(pkh())) - P2WSH inside of P2WSH is invalid
        wsh_wsh_pkh_script = script_to_p2wsh_script(comp_wsh_pkh_script).hex()
        wsh_wsh_pkh_addr = script_to_p2wsh(comp_wsh_pkh_script)
        invalid_addrs.append(wsh_wsh_pkh_addr)

        # Import wsh(wsh(pkh()))
        wallet.importaddress(address=wsh_wsh_pkh_script, p2sh=False)

        # The wsh(pkh()) with a compressed key is always valid, so we should see that the wallet detects it as ismine, not
        # watchonly, and can provide us information about the witnessScript via "embedded"
        comp_wsh_pkh_addr_info = wallet.getaddressinfo(comp_wsh_pkh_addr)
        assert_equal(comp_wsh_pkh_addr_info["ismine"], True)
        assert_equal(comp_wsh_pkh_addr_info["iswatchonly"], False)
        assert "embedded" in comp_wsh_pkh_addr_info

        # The invalid addresses are invalid, so the legacy wallet should not detect them as ismine,
        # nor consider them watchonly. However, because the legacy wallet has the witnessScripts/redeemScripts,
        # we should see information about those in "embedded"
        for addr in invalid_addrs:
            addr_info = wallet.getaddressinfo(addr)
            assert_equal(addr_info["ismine"], False)
            assert_equal(addr_info["iswatchonly"], False)
            assert "embedded" in addr_info

        # Fund those output scripts, although the invalid addresses will not have any balance.
        # This behavior follows as the addresses are not ismine.
        def_wallet.send([{comp_wsh_pkh_addr: 1}] + [{k: i + 1} for i, k in enumerate(invalid_addrs)])
        self.generate(self.nodes[0], 6)
        bal = wallet.getbalances()
        assert_equal(bal["mine"]["trusted"], 1)
        assert_equal(bal["watchonly"]["trusted"], 0)

        res, wallet = self.migrate_and_get_rpc("invalid_p2wsh")
        assert "watchonly_name" not in res
        assert "solvables_name" not in res

        assert_equal(wallet.getbalances()["mine"]["trusted"], 1)

        # After migration, the wsh(pkh()) with a compressed key is still valid and the descriptor wallet will have
        # information about the witnessScript
        comp_wsh_pkh_addr_info = wallet.getaddressinfo(comp_wsh_pkh_addr)
        assert_equal(comp_wsh_pkh_addr_info["ismine"], True)
        assert "embedded" in comp_wsh_pkh_addr_info

        # After migration, the invalid addresses should still not be detected as ismine and not watchonly.
        # The descriptor wallet should not have migrated these at all, so there should additionally be no
        # information in "embedded" about the witnessScripts/redeemScripts.
        for addr in invalid_addrs:
            addr_info = wallet.getaddressinfo(addr)
            assert_equal(addr_info["ismine"], False)
            assert "embedded" not in addr_info

        wallet.unloadwallet()

    def test_miniscript(self):
        # It turns out that due to how signing logic works, legacy wallets that have valid miniscript witnessScripts
        # and the private keys for them can still sign and spend them, even though output scripts involving them
        # as a witnessScript would not be detected as ISMINE_SPENDABLE.
        self.log.info("Test migration of a legacy wallet containing miniscript")
        def_wallet = self.master_node.get_wallet_rpc(self.default_wallet_name)
        wallet = self.create_legacy_wallet("miniscript")

        privkey, _ = generate_keypair(compressed=True, wif=True)

        # Make a descriptor where we only have some of the keys. This will be migrated to the watchonly wallet.
        some_keys_priv_desc = descsum_create(f"wsh(or_b(pk({privkey}),s:pk(029ffbe722b147f3035c87cb1c60b9a5947dd49c774cc31e94773478711a929ac0)))")
        some_keys_addr = self.master_node.deriveaddresses(some_keys_priv_desc)[0]

        # Make a descriptor where we have all of the keys. This will stay in the migrated wallet
        all_keys_priv_desc = descsum_create(f"wsh(and_v(v:pk({privkey}),1))")
        all_keys_addr = self.master_node.deriveaddresses(all_keys_priv_desc)[0]

        imp = wallet.importmulti([
            {
                "desc": some_keys_priv_desc,
                "timestamp": "now",
            },
            {
                "desc": all_keys_priv_desc,
                "timestamp": "now",
            }
        ])
        assert_equal(imp[0]["success"], True)
        assert_equal(imp[1]["success"], True)

        def_wallet.sendtoaddress(some_keys_addr, 1)
        def_wallet.sendtoaddress(all_keys_addr, 1)
        self.generate(self.master_node, 6)
        # Check that the miniscript can be spent by the legacy wallet
        send_res = wallet.send(outputs=[{some_keys_addr: 1},{all_keys_addr: 0.75}], include_watching=True, change_address=def_wallet.getnewaddress())
        assert_equal(send_res["complete"], True)
        self.generate(self.old_node, 6)
        assert_equal(wallet.getbalances()["watchonly"]["trusted"], 1.75)

        _, wallet = self.migrate_and_get_rpc("miniscript")

        # The miniscript with all keys should be in the migrated wallet
        assert_equal(wallet.getbalances()["mine"], {"trusted": 0.75, "untrusted_pending": 0, "immature": 0})
        assert_equal(wallet.getaddressinfo(all_keys_addr)["ismine"], True)
        assert_equal(wallet.getaddressinfo(some_keys_addr)["ismine"], False)

        # The miniscript with some keys should be in the watchonly wallet
        assert "miniscript_watchonly" in self.master_node.listwallets()
        watchonly = self.master_node.get_wallet_rpc("miniscript_watchonly")
        assert_equal(watchonly.getbalances()["mine"], {"trusted": 1, "untrusted_pending": 0, "immature": 0})
        assert_equal(watchonly.getaddressinfo(some_keys_addr)["ismine"], True)
        assert_equal(watchonly.getaddressinfo(all_keys_addr)["ismine"], False)

    def test_taproot(self):
        # It turns out that due to how signing logic works, legacy wallets that have the private key for a Taproot
        # output key will be able to sign and spend those scripts, even though they would not be detected as ISMINE_SPENDABLE.
        self.log.info("Test migration of Taproot scripts")
        def_wallet = self.master_node.get_wallet_rpc(self.default_wallet_name)
        wallet = self.create_legacy_wallet("taproot")

        privkey, _ = generate_keypair(compressed=True, wif=True)

        rawtr_desc = descsum_create(f"rawtr({privkey})")
        rawtr_addr = self.master_node.deriveaddresses(rawtr_desc)[0]
        rawtr_spk = self.master_node.validateaddress(rawtr_addr)["scriptPubKey"]
        tr_desc = descsum_create(f"tr({privkey})")
        tr_addr = self.master_node.deriveaddresses(tr_desc)[0]
        tr_spk = self.master_node.validateaddress(tr_addr)["scriptPubKey"]
        tr_script_desc = descsum_create(f"tr(9ffbe722b147f3035c87cb1c60b9a5947dd49c774cc31e94773478711a929ac0,pk({privkey}))")
        tr_script_addr = self.master_node.deriveaddresses(tr_script_desc)[0]
        tr_script_spk = self.master_node.validateaddress(tr_script_addr)["scriptPubKey"]

        wallet.importaddress(rawtr_spk)
        wallet.importaddress(tr_spk)
        wallet.importaddress(tr_script_spk)
        wallet.importprivkey(privkey)

        txid = def_wallet.send([{rawtr_addr: 1},{tr_addr: 2}, {tr_script_addr: 3}])["txid"]
        rawtr_vout = find_vout_for_address(self.master_node, txid, rawtr_addr)
        tr_vout = find_vout_for_address(self.master_node, txid, tr_addr)
        tr_script_vout = find_vout_for_address(self.master_node, txid, tr_script_addr)
        self.generate(self.master_node, 6)
        assert_equal(wallet.getbalances()["watchonly"]["trusted"], 6)

        # Check that the rawtr can be spent by the legacy wallet
        send_res = wallet.send(outputs=[{rawtr_addr: 0.5}], include_watching=True, change_address=def_wallet.getnewaddress(), inputs=[{"txid": txid, "vout": rawtr_vout}])
        assert_equal(send_res["complete"], True)
        self.generate(self.old_node, 6)
        assert_equal(wallet.getbalances()["watchonly"]["trusted"], 5.5)
        assert_equal(wallet.getbalances()["mine"]["trusted"], 0)

        # Check that the tr() cannot be spent by the legacy wallet
        send_res = wallet.send(outputs=[{def_wallet.getnewaddress(): 4}], include_watching=True, inputs=[{"txid": txid, "vout": tr_vout}, {"txid": txid, "vout": tr_script_vout}])
        assert_equal(send_res["complete"], False)

        res, wallet = self.migrate_and_get_rpc("taproot")

        # The rawtr should be migrated
        assert_equal(wallet.getbalances()["mine"], {"trusted": 0.5, "untrusted_pending": 0, "immature": 0})
        assert_equal(wallet.getaddressinfo(rawtr_addr)["ismine"], True)
        assert_equal(wallet.getaddressinfo(tr_addr)["ismine"], False)
        assert_equal(wallet.getaddressinfo(tr_script_addr)["ismine"], False)

        # The tr() with some keys should be in the watchonly wallet
        assert "taproot_watchonly" in self.master_node.listwallets()
        watchonly = self.master_node.get_wallet_rpc("taproot_watchonly")
        assert_equal(watchonly.getbalances()["mine"], {"trusted": 5, "untrusted_pending": 0, "immature": 0})
        assert_equal(watchonly.getaddressinfo(rawtr_addr)["ismine"], False)
        assert_equal(watchonly.getaddressinfo(tr_addr)["ismine"], True)
        assert_equal(watchonly.getaddressinfo(tr_script_addr)["ismine"], True)

    def test_solvable_no_privs(self):
        self.log.info("Test migrating a multisig that we do not have any private keys for")
        wallet = self.create_legacy_wallet("multisig_noprivs")

        _, pubkey = generate_keypair(compressed=True, wif=True)

        add_ms_res = wallet.addmultisigaddress(nrequired=1, keys=[pubkey.hex()])
        addr = add_ms_res["address"]

        # The multisig address should be ISMINE_NO but we should have the script info
        addr_info = wallet.getaddressinfo(addr)
        assert_equal(addr_info["ismine"], False)
        assert "hex" in addr_info

        migrate_res, wallet = self.migrate_and_get_rpc("multisig_noprivs")
        assert_equal(migrate_res["solvables_name"], "multisig_noprivs_solvables")
        solvables = self.master_node.get_wallet_rpc(migrate_res["solvables_name"])

        # The multisig should not be in the spendable wallet
        addr_info = wallet.getaddressinfo(addr)
        assert_equal(addr_info["ismine"], False)
        assert "hex" not in addr_info

        # The multisig address should be in the solvables wallet
        addr_info = solvables.getaddressinfo(addr)
        assert_equal(addr_info["ismine"], True)
        assert_equal(addr_info["solvable"], True)
        assert "hex" in addr_info

    def test_loading_failure_after_migration(self):
        self.log.info("Test that a failed loading of the wallet at the end of migration restores the backup")
        self.stop_node(self.old_node.index)
        self.old_node.chain = "signet"
        self.old_node.replace_in_config([("regtest=", "signet="), ("[regtest]", "[signet]")])
        # Disable network sync and prevent disk space warning on small (tmp)fs
        self.start_node(self.old_node.index, extra_args=self.old_node.extra_args + ["-maxconnections=0", "-prune=550"])

        wallet_name = "failed_load_after_migrate"
        self.create_legacy_wallet(wallet_name)
        assert_raises_rpc_error(-4, "Wallet loading failed. Wallet files should not be reused across chains.", lambda: self.migrate_and_get_rpc(wallet_name))

        # Check the wallet we tried to migrate is still BDB
        self.assert_is_bdb(wallet_name)

        self.stop_node(self.old_node.index)
        self.old_node.chain = "regtest"
        self.old_node.replace_in_config([("signet=", "regtest="), ("[signet]", "[regtest]")])
        self.start_node(self.old_node.index)
        self.connect_nodes(1, 0)

    def run_test(self):
        self.master_node = self.nodes[0]
        self.old_node = self.nodes[1]

        self.generate(self.master_node, 101)

        # TODO: Test the actual records in the wallet for these tests too. The behavior may be correct, but the data written may not be what we actually want
        self.test_basic()
        self.test_multisig()
        self.test_other_watchonly()
        self.test_no_privkeys()
        self.test_pk_coinbases()
        self.test_encrypted()
        self.test_nonexistent()
        self.test_unloaded_by_path()
        self.test_wallet_with_relative_path()
        self.test_wallet_with_path("path/to/mywallet/")
        self.test_wallet_with_path("path/that/ends/in/..")
        self.test_default_wallet_failure()
        self.test_default_wallet()
        self.test_direct_file()
        self.test_addressbook()
        self.test_migrate_raw_p2sh()
        self.test_conflict_txs()
        self.test_hybrid_pubkey()
        self.test_failed_migration_cleanup()
        self.test_failed_migration_cleanup_relative_path()
        self.test_avoidreuse()
        self.test_preserve_tx_extra_info()
        self.test_blank()
        self.test_migrate_simple_watch_only()
        self.test_manual_keys_import()
        self.test_p2wsh()
        self.test_disallowed_p2wsh()
        self.test_miniscript()
        self.test_taproot()
        self.test_solvable_no_privs()
        self.test_loading_failure_after_migration()

if __name__ == '__main__':
    WalletMigrationTest(__file__).main()
