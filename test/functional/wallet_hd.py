#!/usr/bin/env python3
# Copyright (c) 2016-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Hierarchical Deterministic wallet function."""

import shutil

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    wallet_importprivkey,
    assert_raises_rpc_error,
)

BIP39_MNEMONIC = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
BIP39_XPRV = "tprv8ZgxMBicQKsPe5YMU9gHen4Ez3ApihUfykaqUorj9t6FDqy3nP6eoXiAo2ssvpAjoLroQxHqr3R5nE3a5dU3DHTjTgJDd7zrbniJr6nrCzd"
BIP39_XPUB = "tpubD6NzVbkrYhZ4XYa9MoLt4BiMZ4gkt2faZ4BcmKu2a9te4LDpQmvEz2L2yDERivHxFPnxXXhqDRkUNnQCpZggCyEZLBktV7VaSmwayqMJy1s"
BIP39_FINGERPRINT = "73c5da0a"
BIP39_BIP84_ACCOUNT_XPUB = "tpubDC8msFGeGuwnKG9Upg7DM2b4DaRqg3CUZa5g8v2SRQ6K4NSkxUgd7HsL2XVWbVm39yBA4LAxysQAm397zwQSQoQgewGiYZqrA9DsP4zbQ1M"
BIP39_FIRST_BECH32_ADDRESS = "bcrt1q6rz28mcfaxtmd6v789l9rrlrusdprr9pz3cppk"
BIP39_PASSPHRASE = "TREZOR"
BIP39_PASSPHRASE_XPRV = "tprv8ZgxMBicQKsPeWHBt7a68nPnvgTnuDhUgDWC8wZCgA8GahrQ3f3uWpq7wE7Uc1dLBnCe1hhCZ886K6ND37memRDWqsA9HgSKDXtwh2Qxo6J"
BIP39_PASSPHRASE_XPUB = "tpubD6NzVbkrYhZ4XyJymmEgYC3uVhyj4YtPFX6yRTbW6RvfRC7Ag3sVhKSz7MNzFWW5MJ7aVBKXCAX7En296EYdpo43M4a4LaeaHuhhgHToSJF"


class WalletHDTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [[], ['-keypool=0']]
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_addhdkey(self):
        self.log.info("Test addhdkey")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet("hdkey")
        wallet = self.nodes[0].get_wallet_rpc("hdkey")

        assert_equal(len(wallet.gethdkeys()), 1)

        wallet.addhdkey()
        xpub_info = wallet.gethdkeys()
        assert_equal(len(xpub_info), 2)
        for x in xpub_info:
            if len(x["descriptors"]) == 1 and x["descriptors"][0]["desc"].startswith("unused("):
                break
        else:
            assert False, "Did not find HD key with no descriptors"

        imp_xpub_info = def_wallet.gethdkeys(private=True)[0]
        imp_xpub = imp_xpub_info["xpub"]
        imp_xprv = imp_xpub_info["xprv"]

        assert_raises_rpc_error(-5, "Extended public key (xpub) provided, but extended private key (xprv) is required", wallet.addhdkey, imp_xpub)
        add_res = wallet.addhdkey(imp_xprv)
        expected_unused_desc = descsum_create(f"unused({imp_xpub})")
        assert_equal(add_res["xpub"], imp_xpub)
        xpub_info = wallet.gethdkeys()
        assert_equal(len(xpub_info), 3)
        for x in xpub_info:
            if x["xpub"] == imp_xpub:
                assert_equal(len(x["descriptors"]), 1)
                assert_equal(x["descriptors"][0]["desc"], expected_unused_desc)
                break
        else:
            assert False, "Added HD key was not found in wallet"

        for d in wallet.listdescriptors()["descriptors"]:
            if d["desc"] == expected_unused_desc:
                assert_equal(d["active"], False)
                break
        else:
            assert False, "Added HD key's descriptor was not found in wallet"

        assert_raises_rpc_error(-4, "HD key already exists", wallet.addhdkey, imp_xprv)

        self.log.info("Test addhdkey BIP 39 mnemonic and passphrase arguments")
        assert_raises_rpc_error(
            -8,
            'Cannot specify both "hdkey" and "mnemonic"',
            wallet.addhdkey,
            hdkey=imp_xprv,
            mnemonic=BIP39_MNEMONIC,
        )
        assert_raises_rpc_error(
            -8,
            '"bip39_passphrase" may only be used with "mnemonic"',
            wallet.addhdkey,
            bip39_passphrase=BIP39_PASSPHRASE,
        )
        assert_raises_rpc_error(
            -8,
            "word count must be 12, 15, 18, 21, or 24",
            wallet.addhdkey,
            mnemonic="abandon abandon",
        )
        assert_raises_rpc_error(
            -8,
            "word not in the English wordlist",
            wallet.addhdkey,
            mnemonic="abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon notaword",
        )
        assert_raises_rpc_error(
            -8,
            "Invalid BIP 39 mnemonic checksum",
            wallet.addhdkey,
            mnemonic="abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon",
        )
        assert_raises_rpc_error(
            -8,
            "BIP 39 passphrase must contain only ASCII characters",
            wallet.addhdkey,
            mnemonic=BIP39_MNEMONIC,
            bip39_passphrase="non-ascii-\N{LATIN SMALL LETTER E WITH OGONEK}",
        )

        add_res = wallet.addhdkey(mnemonic=BIP39_MNEMONIC)
        assert_equal(add_res, {"xpub": BIP39_XPUB})
        assert_raises_rpc_error(
            -4,
            "HD key already exists",
            wallet.addhdkey,
            mnemonic=BIP39_MNEMONIC,
        )

        # An explicitly empty passphrase is equivalent to omitting it: the
        # derived key already exists from the addhdkey(mnemonic=...) call above.
        assert_raises_rpc_error(
            -4,
            "HD key already exists",
            wallet.addhdkey,
            mnemonic=BIP39_MNEMONIC,
            bip39_passphrase="",
        )

        passphrase_res = wallet.addhdkey(
            mnemonic=BIP39_MNEMONIC,
            bip39_passphrase=BIP39_PASSPHRASE,
        )
        assert_equal(passphrase_res, {"xpub": BIP39_PASSPHRASE_XPUB})

        public_descriptors = wallet.listdescriptors()
        descriptor_text = str(public_descriptors)
        for secret in (
            BIP39_MNEMONIC,
            BIP39_XPRV,
            BIP39_PASSPHRASE,
            BIP39_PASSPHRASE_XPRV,
        ):
            assert secret not in str(add_res)
            assert secret not in str(passphrase_res)
            assert secret not in descriptor_text

        expected_mnemonic_descs = {
            descsum_create(f"unused({BIP39_XPUB})"),
            descsum_create(f"unused({BIP39_PASSPHRASE_XPUB})"),
        }
        wallet_descs = {
            desc["desc"]: desc for desc in public_descriptors["descriptors"]
        }
        assert expected_mnemonic_descs <= wallet_descs.keys()
        for desc in expected_mnemonic_descs:
            assert_equal(wallet_descs[desc]["active"], False)

        self.log.info("Test addhdkey BIP 39 mnemonic with an encrypted wallet")
        self.nodes[0].createwallet("hdkey_encrypted")
        encrypted_wallet = self.nodes[0].get_wallet_rpc("hdkey_encrypted")
        encrypted_wallet.encryptwallet("wallet-passphrase")
        assert_raises_rpc_error(
            -13,
            "Please enter the wallet passphrase with walletpassphrase first",
            encrypted_wallet.addhdkey,
            mnemonic=BIP39_MNEMONIC,
        )
        encrypted_wallet.walletpassphrase("wallet-passphrase", 60)
        assert_equal(
            encrypted_wallet.addhdkey(mnemonic=BIP39_MNEMONIC),
            {"xpub": BIP39_XPUB},
        )
        encrypted_wallet.walletlock()

        self.log.info("Test BIP 39 mnemonic recovery into active descriptors")
        recovery_txid = def_wallet.sendtoaddress(BIP39_FIRST_BECH32_ADDRESS, 1)
        self.generate(self.nodes[0], 1)
        recovery_height = self.nodes[0].getblockcount()

        self.nodes[0].createwallet("bip39_recovery", blank=True)
        recovery_wallet = self.nodes[0].get_wallet_rpc("bip39_recovery")
        assert_equal(
            recovery_wallet.addhdkey(mnemonic=BIP39_MNEMONIC),
            {"xpub": BIP39_XPUB},
        )
        recovery_descs = recovery_wallet.createwalletdescriptor(
            type="bech32",
            hdkey=BIP39_XPUB,
        )["descs"]
        assert_equal(
            set(recovery_descs),
            {
                descsum_create(f"wpkh([{BIP39_FINGERPRINT}/84h/1h/0h]{BIP39_BIP84_ACCOUNT_XPUB}/0/*)"),
                descsum_create(f"wpkh([{BIP39_FINGERPRINT}/84h/1h/0h]{BIP39_BIP84_ACCOUNT_XPUB}/1/*)"),
            },
        )
        recovery_address = recovery_wallet.getnewaddress()
        assert_equal(recovery_address, BIP39_FIRST_BECH32_ADDRESS)
        assert_equal(
            recovery_wallet.getaddressinfo(recovery_address)["hdkeypath"],
            "m/84h/1h/0h/0/0",
        )
        assert_equal(recovery_wallet.getbalance(), 0)
        scan = recovery_wallet.rescanblockchain(recovery_height)
        assert_equal(scan["start_height"], recovery_height)
        assert_equal(scan["stop_height"], recovery_height)
        assert_equal(recovery_wallet.getbalance(), 1)
        assert_equal(
            recovery_wallet.gettransaction(recovery_txid)["confirmations"],
            1,
        )

    def test_addhdkey_noprivs(self):
        self.log.info("Test addhdkey is not available for wallets without privkeys")
        self.nodes[0].createwallet("hdkey_noprivs", disable_private_keys=True)
        wallet = self.nodes[0].get_wallet_rpc("hdkey_noprivs")
        assert_raises_rpc_error(-4, "addhdkey is not available for wallets without private keys", wallet.addhdkey)
        assert_raises_rpc_error(
            -4,
            "addhdkey is not available for wallets without private keys",
            wallet.addhdkey,
            mnemonic=BIP39_MNEMONIC,
        )

    def run_test(self):
        # Make sure we use hd, keep masterkeyid
        hd_fingerprint = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress())['hdmasterfingerprint']
        assert_equal(len(hd_fingerprint), 8)

        # create an internal key
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV = self.nodes[1].getaddressinfo(change_addr)
        assert_equal(change_addrV["hdkeypath"], "m/84h/1h/0h/1/0")

        # Import a non-HD private key in the HD wallet
        non_hd_add = 'bcrt1qmevj8zfx0wdvp05cqwkmr6mxkfx60yezwjksmt'
        non_hd_key = 'cS9umN9w6cDMuRVYdbkfE4c7YUFLJRoXMfhQ569uY4odiQbVN8Rt'
        wallet_importprivkey(self.nodes[1], non_hd_key, "now")

        # This should be enough to keep the master key and the non-HD key
        self.nodes[1].backupwallet(self.nodes[1].datadir_path / "hd.bak")

        # Derive some HD addresses and remember the last
        # Also send funds to each add
        self.generate(self.nodes[0], COINBASE_MATURITY + 1)
        hd_add = None
        NUM_HD_ADDS = 10
        for i in range(1, NUM_HD_ADDS + 1):
            hd_add = self.nodes[1].getnewaddress()
            hd_info = self.nodes[1].getaddressinfo(hd_add)
            assert_equal(hd_info["hdkeypath"], "m/84h/1h/0h/0/" + str(i))
            assert_equal(hd_info["hdmasterfingerprint"], hd_fingerprint)
            self.nodes[0].sendtoaddress(hd_add, 1)
            self.generate(self.nodes[0], 1)
        self.nodes[0].sendtoaddress(non_hd_add, 1)
        self.generate(self.nodes[0], 1)

        # create an internal key (again)
        change_addr = self.nodes[1].getrawchangeaddress()
        change_addrV = self.nodes[1].getaddressinfo(change_addr)
        assert_equal(change_addrV["hdkeypath"], "m/84h/1h/0h/1/1")

        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        self.log.info("Restore backup ...")
        self.stop_node(1)
        # we need to delete the complete chain directory
        # otherwise node1 would auto-recover all funds in flag the keypool keys as used
        self.cleanup_folder(self.nodes[1].blocks_path)
        self.cleanup_folder(self.nodes[1].chain_path / "chainstate")
        shutil.copyfile(
            self.nodes[1].datadir_path / "hd.bak",
            self.nodes[1].wallets_path / self.default_wallet_name / self.wallet_data_filename
        )
        self.start_node(1)

        # Assert that derivation is deterministic
        hd_add_2 = None
        for i in range(1, NUM_HD_ADDS + 1):
            hd_add_2 = self.nodes[1].getnewaddress()
            hd_info_2 = self.nodes[1].getaddressinfo(hd_add_2)
            assert_equal(hd_info_2["hdkeypath"], "m/84h/1h/0h/0/" + str(i))
            assert_equal(hd_info_2["hdmasterfingerprint"], hd_fingerprint)
        assert_equal(hd_add, hd_add_2)
        self.connect_nodes(0, 1)
        self.sync_all()

        # Needs rescan
        self.nodes[1].rescanblockchain()
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        # Try a RPC based rescan
        self.stop_node(1)
        self.cleanup_folder(self.nodes[1].blocks_path)
        self.cleanup_folder(self.nodes[1].chain_path / "chainstate")
        shutil.copyfile(
            self.nodes[1].datadir_path / "hd.bak",
            self.nodes[1].wallets_path / self.default_wallet_name / self.wallet_data_filename
        )
        self.start_node(1, extra_args=self.extra_args[1])
        self.connect_nodes(0, 1)
        self.sync_all()
        # Wallet automatically scans blocks older than key on startup
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)
        out = self.nodes[1].rescanblockchain(0, 1)
        assert_equal(out['start_height'], 0)
        assert_equal(out['stop_height'], 1)
        out = self.nodes[1].rescanblockchain()
        assert_equal(out['start_height'], 0)
        assert_equal(out['stop_height'], self.nodes[1].getblockcount())
        assert_equal(self.nodes[1].getbalance(), NUM_HD_ADDS + 1)

        # send a tx and make sure its using the internal chain for the changeoutput
        txid = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        outs = self.nodes[1].gettransaction(txid=txid, verbose=True)['decoded']['vout']
        keypath = ""
        for out in outs:
            if out['value'] != 1:
                keypath = self.nodes[1].getaddressinfo(out['scriptPubKey']['address'])['hdkeypath']

        assert_equal(keypath[0:14], "m/84h/1h/0h/1/")

        self.test_addhdkey()
        self.test_addhdkey_noprivs()

if __name__ == '__main__':
    WalletHDTest(__file__).main()
