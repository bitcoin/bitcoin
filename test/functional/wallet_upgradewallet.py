#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""upgradewallet RPC functional test

Test upgradewallet RPC. Download node binaries:

Requires previous releases binaries, see test/README.md.
Only v0.15.2 and v0.16.3 are required by this test.
"""

import os
import struct

from io import BytesIO

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.bdb import dump_bdb_kv
from test_framework.messages import deser_compact_size, deser_string
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_is_hex_string,
    sha256sum_file,
)
from test_framework.wallet_util import dump_sqlite_kv


UPGRADED_KEYMETA_VERSION = 12

def deser_keymeta(f):
    ver, create_time = struct.unpack('<Iq', f.read(12))
    kp_str = deser_string(f)
    seed_id = f.read(20)
    fpr = f.read(4)
    path_len = 0
    path = []
    has_key_orig = False
    if ver == UPGRADED_KEYMETA_VERSION:
        path_len = deser_compact_size(f)
        for i in range(0, path_len):
            path.append(struct.unpack('<I', f.read(4))[0])
        has_key_orig = bool(f.read(1))
    return ver, create_time, kp_str, seed_id, fpr, path_len, path, has_key_orig

class UpgradeWalletTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, descriptors=False)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 5
        self.extra_args = [
            ["-addresstype=bech32", "-keypool=2"], # current wallet version
            ["-usehd=1", "-keypool=2"],            # v0.16.3 wallet
            ["-usehd=0", "-keypool=2"],            # v0.15.2 wallet
            ["-keypool=2"],                        # v0.20.1 wallet
            ["-keypool=2"],                        # v23.0 wallet
        ]
        self.wallet_names = [self.default_wallet_name, None, None, None, self.default_wallet_name]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()
        self.skip_if_no_previous_releases()

    def setup_network(self):
        self.setup_nodes()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args, versions=[
            None,
            160300,
            150200,
            200100,
            230000,
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
        v16_3_node = self.nodes[1]
        to_height = node_from.getblockcount()
        height = self.nodes[1].getblockcount()
        for i in range(height, to_height+1):
            b = node_from.getblock(blockhash=node_from.getblockhash(i), verbose=0)
            v16_3_node.submitblock(b)
        assert_equal(v16_3_node.getblockcount(), to_height)

    def test_upgradewallet(self, wallet, previous_version, requested_version=None, expected_version=None):
        unchanged = expected_version == previous_version
        new_version = previous_version if unchanged else expected_version if expected_version else requested_version
        old_wallet_info = wallet.getwalletinfo()
        assert_equal(old_wallet_info["walletversion"], previous_version)
        assert_equal(wallet.upgradewallet(requested_version),
            {
                "wallet_name": old_wallet_info["walletname"],
                "previous_version": previous_version,
                "current_version": new_version,
                "result": "Already at latest version. Wallet version unchanged." if unchanged else "Wallet upgraded successfully from version {} to version {}.".format(previous_version, new_version),
            }
        )
        assert_equal(wallet.getwalletinfo()["walletversion"], new_version)

    def test_upgradewallet_error(self, wallet, previous_version, requested_version, msg):
        wallet_info = wallet.getwalletinfo()
        assert_equal(wallet_info["walletversion"], previous_version)
        assert_equal(wallet.upgradewallet(requested_version),
            {
                "wallet_name": wallet_info["walletname"],
                "previous_version": previous_version,
                "current_version": previous_version,
                "error": msg,
            }
        )
        assert_equal(wallet.getwalletinfo()["walletversion"], previous_version)

    def get_wallet_file_path(self, node, name):
        return os.path.join(node.datadir, "regtest/wallets", name, "wallet.dat")

    def test_encryption_key_sig_upgrade(self, node, version):
        self.log.info(f"Test that loading a {version} wallet upgrades encrypted keys")

        node.start()
        node.wait_for_rpc_connection()
        node.get_wallet_rpc(self.default_wallet_name).encryptwallet("passphrase")
        wallet_path = os.path.join(self.options.tmpdir, f"{version}_wallet")
        node.get_wallet_rpc(self.default_wallet_name).backupwallet(wallet_path)
        node.stop_node()
        node.wait_until_stopped()

        wallet_name = f"{version}_key_sig_upgrade"
        self.nodes[0].restorewallet(wallet_name, wallet_path)
        self.nodes[0].get_wallet_rpc(wallet_name).unloadwallet()
        restored_path = os.path.join(self.nodes[0].datadir, "regtest/wallets", wallet_name, "wallet.dat")

        if self.options.descriptors:
            kvs = dump_sqlite_kv(restored_path)
        else:
            kvs = dump_bdb_kv(restored_path)
        for key, value in kvs.items():
            if key.startswith(b"\x04ckey") or key.startswith(b"\x14walletdescriptorckey"):
                assert len(value) in [49, 81]

        load_res = self.nodes[0].loadwallet(wallet_name)
        assert_equal(load_res["warning"], "This wallet contains encrypted keys which lack proofs of correctness. Please unlock the wallet so that the encrypted keys can be checked and the proofs added to the wallet file.")

        self.nodes[0].get_wallet_rpc(wallet_name).walletpassphrase("passphrase", 10)
        self.nodes[0].unloadwallet(wallet_name)

        if self.options.descriptors:
            kvs = dump_sqlite_kv(restored_path)
        else:
            kvs = dump_bdb_kv(restored_path)
        for key, value in kvs.items():
            if key.startswith(b"\x04ckey") or key.startswith(b"\x14walletdescriptorckey"):
                assert len(value) in [114, 146]

        load_res = self.nodes[0].loadwallet(wallet_name)
        assert_equal(load_res["warning"], "")

    def run_test(self):
        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 1, self.nodes[0].getnewaddress(), sync_fun=lambda: self.dumb_sync_blocks())
        # # Sanity check the test framework:
        res = self.nodes[0].getblockchaininfo()
        assert_equal(res['blocks'], COINBASE_MATURITY + 1)
        node_master = self.nodes[0]
        v16_3_node  = self.nodes[1]
        v15_2_node  = self.nodes[2]
        v20_1_node  = self.nodes[3]
        v23_0_node  = self.nodes[4]

        # Send coins to old wallets for later conversion checks.
        v16_3_wallet  = v16_3_node.get_wallet_rpc('wallet.dat')
        v16_3_address = v16_3_wallet.getnewaddress()
        self.generatetoaddress(node_master, COINBASE_MATURITY + 1, v16_3_address, sync_fun=lambda: self.dumb_sync_blocks())
        v16_3_balance = v16_3_wallet.getbalance()

        self.log.info("Test upgradewallet RPC...")
        # Prepare for copying of the older wallet
        v16_3_wallet = os.path.join(self.options.tmpdir, "16_3_wallet.dat")
        v16_3_node.backupwallet(v16_3_wallet)
        v15_2_wallet = os.path.join(self.options.tmpdir, "15_2_wallet.dat")
        v15_2_node.backupwallet(v15_2_wallet)
        self.stop_nodes()

        # Make split hd wallet
        split_hd_wallet = os.path.join(self.options.tmpdir, "split_hd_wallet.dat")
        self.start_node(2, ['-usehd=1', '-keypool=2', '-wallet=splithd'])
        v15_2_node.backupwallet(split_hd_wallet)
        self.stop_node(2)

        self.restart_node(0)
        node_master.restorewallet("16_3_wallet", v16_3_wallet)
        wallet = node_master.get_wallet_rpc("16_3_wallet")
        self.log.info("Test upgradewallet without a version argument")
        self.test_upgradewallet(wallet, previous_version=159900, expected_version=169900)
        # wallet should still contain the same balance
        assert_equal(wallet.getbalance(), v16_3_balance)

        node_master.restorewallet("15_2_wallet", v15_2_wallet)
        wallet = node_master.get_wallet_rpc("15_2_wallet")
        # should have no master key hash before conversion
        assert_equal('hdseedid' in wallet.getwalletinfo(), False)
        self.log.info("Test upgradewallet with explicit version number")
        self.test_upgradewallet(wallet, previous_version=60000, requested_version=169900)
        # after conversion master key hash should be present
        assert_is_hex_string(wallet.getwalletinfo()['hdseedid'])

        self.log.info("Intermediary versions don't effect anything")
        node_master.restorewallet("15_2_wallet_upgrade", v15_2_wallet)
        wallet = node_master.get_wallet_rpc("15_2_wallet_upgrade")
        # Wallet starts with 60000
        assert_equal(60000, wallet.getwalletinfo()['walletversion'])
        wallet.unloadwallet()
        before_checksum = sha256sum_file(self.get_wallet_file_path(node_master, "15_2_wallet_upgrade"))
        node_master.loadwallet("15_2_wallet_upgrade")
        # Test an "upgrade" from 60000 to 129999 has no effect, as the next version is 130000
        self.test_upgradewallet(wallet, previous_version=60000, requested_version=129999, expected_version=60000)
        wallet.unloadwallet()
        assert_equal(before_checksum, sha256sum_file(self.get_wallet_file_path(node_master, "15_2_wallet_upgrade")))
        node_master.loadwallet("15_2_wallet_upgrade")

        self.log.info('Wallets cannot be downgraded')
        node_master.restorewallet("15_2_step_upgrade", v15_2_wallet)
        wallet = node_master.get_wallet_rpc("15_2_step_upgrade")
        self.test_upgradewallet_error(wallet, previous_version=60000, requested_version=40000,
            msg="Cannot downgrade wallet from version 60000 to version 40000. Wallet version unchanged.")
        wallet.unloadwallet()
        assert_equal(before_checksum, sha256sum_file(self.get_wallet_file_path(node_master, "15_2_step_upgrade")))
        node_master.loadwallet("15_2_step_upgrade")

        self.log.info('Can upgrade to HD')
        # Inspect the old wallet and make sure there is no hdchain
        orig_kvs = dump_bdb_kv(self.get_wallet_file_path(node_master, "15_2_step_upgrade"))
        assert b'\x07hdchain' not in orig_kvs
        # Upgrade to HD, no split
        self.test_upgradewallet(wallet, previous_version=60000, requested_version=130000)
        # Check that there is now a hd chain and it is version 1, no internal chain counter
        new_kvs = dump_bdb_kv(self.get_wallet_file_path(node_master, "15_2_step_upgrade"))
        assert b'\x07hdchain' in new_kvs
        hd_chain = new_kvs[b'\x07hdchain']
        assert_equal(28, len(hd_chain))
        hd_chain_version, external_counter, seed_id = struct.unpack('<iI20s', hd_chain)
        assert_equal(1, hd_chain_version)
        seed_id = bytearray(seed_id)
        seed_id.reverse()

        # New keys (including change) should be HD (the two old keys have been flushed)
        info = wallet.getaddressinfo(wallet.getnewaddress())
        assert_equal(seed_id.hex(), info['hdseedid'])
        assert_equal('m/0\'/0\'/0\'', info['hdkeypath'])
        prev_seed_id = info['hdseedid']
        # Change key should be HD and from the same keypool
        info = wallet.getaddressinfo(wallet.getrawchangeaddress())
        assert_equal(prev_seed_id, info['hdseedid'])
        assert_equal('m/0\'/0\'/1\'', info['hdkeypath'])

        self.log.info('Cannot upgrade to HD Split, needs Pre Split Keypool')
        for version in [139900, 159900, 169899]:
            self.test_upgradewallet_error(wallet, previous_version=130000, requested_version=version,
                msg="Cannot upgrade a non HD split wallet from version {} to version {} without upgrading to "
                    "support pre-split keypool. Please use version 169900 or no version specified.".format(130000, version))

        self.log.info('Upgrade HD to HD chain split')
        self.test_upgradewallet(wallet, previous_version=130000, requested_version=169900)
        # Check that the hdchain updated correctly
        new_kvs = dump_bdb_kv(self.get_wallet_file_path(node_master, "15_2_step_upgrade"))
        hd_chain = new_kvs[b'\x07hdchain']
        assert_equal(32, len(hd_chain))
        hd_chain_version, external_counter, seed_id, internal_counter = struct.unpack('<iI20sI', hd_chain)
        assert_equal(2, hd_chain_version)
        assert_equal(0, internal_counter)
        seed_id = bytearray(seed_id)
        seed_id.reverse()
        assert_equal(seed_id.hex(), prev_seed_id)
        # Next change address is the same keypool
        info = wallet.getaddressinfo(wallet.getrawchangeaddress())
        assert_equal(prev_seed_id, info['hdseedid'])
        assert_equal('m/0\'/0\'/2\'', info['hdkeypath'])
        # Next change address is the new keypool
        info = wallet.getaddressinfo(wallet.getrawchangeaddress())
        assert_equal(prev_seed_id, info['hdseedid'])
        assert_equal('m/0\'/1\'/0\'', info['hdkeypath'])
        # External addresses use the same keypool
        info = wallet.getaddressinfo(wallet.getnewaddress())
        assert_equal(prev_seed_id, info['hdseedid'])
        assert_equal('m/0\'/0\'/3\'', info['hdkeypath'])

        self.log.info('Upgrade non-HD to HD chain split')
        node_master.restorewallet("15_2_wallet_hd_upgrade", v15_2_wallet)
        wallet = node_master.get_wallet_rpc("15_2_wallet_hd_upgrade")
        self.test_upgradewallet(wallet, previous_version=60000, requested_version=169900)
        # Check that the hdchain updated correctly
        new_kvs = dump_bdb_kv(self.get_wallet_file_path(node_master, "15_2_wallet_hd_upgrade"))
        hd_chain = new_kvs[b'\x07hdchain']
        assert_equal(32, len(hd_chain))
        hd_chain_version, external_counter, seed_id, internal_counter = struct.unpack('<iI20sI', hd_chain)
        assert_equal(2, hd_chain_version)
        assert_equal(2, internal_counter)
        # The next addresses are HD and should be on different HD chains (the one remaining key in each pool should have been flushed)
        info = wallet.getaddressinfo(wallet.getnewaddress())
        ext_id = info['hdseedid']
        assert_equal('m/0\'/0\'/0\'', info['hdkeypath'])
        info = wallet.getaddressinfo(wallet.getrawchangeaddress())
        assert_equal(ext_id, info['hdseedid'])
        assert_equal('m/0\'/1\'/0\'', info['hdkeypath'])

        self.log.info('KeyMetadata should upgrade when loading into master')
        node_master.restorewallet("16_3_meta_upgrade", v16_3_wallet)
        wallet = node_master.get_wallet_rpc("16_3_meta_upgrade")
        old_kvs = dump_bdb_kv(v16_3_wallet)
        new_kvs = dump_bdb_kv(self.get_wallet_file_path(node_master, "16_3_meta_upgrade"))
        for k, old_v in old_kvs.items():
            if k.startswith(b'\x07keymeta'):
                new_ver, new_create_time, new_kp_str, new_seed_id, new_fpr, new_path_len, new_path, new_has_key_orig = deser_keymeta(BytesIO(new_kvs[k]))
                old_ver, old_create_time, old_kp_str, old_seed_id, old_fpr, old_path_len, old_path, old_has_key_orig = deser_keymeta(BytesIO(old_v))
                assert_equal(10, old_ver)
                if old_kp_str == b"": # imported things that don't have keymeta (i.e. imported coinbase privkeys) won't be upgraded
                    assert_equal(new_kvs[k], old_v)
                    continue
                assert_equal(12, new_ver)
                assert_equal(new_create_time, old_create_time)
                assert_equal(new_kp_str, old_kp_str)
                assert_equal(new_seed_id, old_seed_id)
                assert_equal(0, old_path_len)
                assert_equal(new_path_len, len(new_path))
                assert_equal([], old_path)
                assert_equal(False, old_has_key_orig)
                assert_equal(True, new_has_key_orig)

                # Check that the path is right
                built_path = []
                for s in new_kp_str.decode().split('/')[1:]:
                    h = 0
                    if s[-1] == '\'':
                        s = s[:-1]
                        h = 0x80000000
                    p = int(s) | h
                    built_path.append(p)
                assert_equal(new_path, built_path)

        self.log.info('Upgrading to NO_DEFAULT_KEY should not remove the defaultkey')
        node_master.restorewallet("split_hd_upgrade", split_hd_wallet)
        wallet = node_master.get_wallet_rpc("split_hd_upgrade")
        # Check the wallet has a default key initially
        old_kvs = dump_bdb_kv(self.get_wallet_file_path(node_master, "split_hd_upgrade"))
        defaultkey = old_kvs[b'\x0adefaultkey']
        self.log.info("Upgrade the wallet. Should still have the same default key.")
        self.test_upgradewallet(wallet, previous_version=139900, requested_version=159900)
        new_kvs = dump_bdb_kv(self.get_wallet_file_path(node_master, "split_hd_upgrade"))
        up_defaultkey = new_kvs[b'\x0adefaultkey']
        assert_equal(defaultkey, up_defaultkey)
        # 0.16.3 doesn't have a default key
        v16_3_kvs = dump_bdb_kv(v16_3_wallet)
        assert b'\x0adefaultkey' not in v16_3_kvs

        if self.is_sqlite_compiled():
            self.log.info("Checking that descriptor wallets do nothing, successfully")
            self.nodes[0].createwallet(wallet_name="desc_upgrade", descriptors=True)
            desc_wallet = self.nodes[0].get_wallet_rpc("desc_upgrade")
            self.test_upgradewallet(desc_wallet, previous_version=169900, expected_version=169900)

            self.log.info("Checking that descriptor wallets without privkeys do nothing, successfully")
            self.nodes[0].createwallet(wallet_name="desc_upgrade_nopriv", descriptors=True, disable_private_keys=True)
            desc_wallet = self.nodes[0].get_wallet_rpc("desc_upgrade_nopriv")
            self.test_upgradewallet(desc_wallet, previous_version=169900, expected_version=169900)

        if self.is_bdb_compiled():
            self.log.info("Upgrading a wallet with private keys disabled")
            self.nodes[0].createwallet(wallet_name="privkeys_disabled_upgrade", disable_private_keys=True, descriptors=False)
            disabled_wallet = self.nodes[0].get_wallet_rpc("privkeys_disabled_upgrade")
            self.test_upgradewallet(disabled_wallet, previous_version=169900, expected_version=169900)

        if not self.options.descriptors:
            self.test_encryption_key_sig_upgrade(v20_1_node, "v0.20.1")
        self.test_encryption_key_sig_upgrade(v23_0_node, "v23.0")

if __name__ == '__main__':
    UpgradeWalletTest().main()
