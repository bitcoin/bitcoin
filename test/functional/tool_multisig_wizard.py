#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the multisig setup wizard in contrib/multisig."""

import importlib
import os
import sys

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    assert_raises_message,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import WalletUnlock


class MultisigWizardTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        wizard_dir = os.path.join(self.config["environment"]["SRCDIR"], "contrib", "multisig")
        sys.path.insert(0, wizard_dir)
        self.wizard = importlib.import_module("wizard")
        self.node = self.nodes[0]
        self.cosigners = 0

        self.test_create_participant()
        self.test_create_coordinator()
        self.test_create_encrypted()
        self.test_generate_key()
        self.test_generate_key_encrypted()
        self.test_validate_key()
        self.test_add_cosigner()

    def test_create_participant(self):
        self.log.info("Creating blank participant wallet with private keys")
        setup = self.wizard.setup("Family Vault")
        assert_equal(setup["cosigners"], [])
        name = self.wizard.create_wallet(self.node, setup["name"])
        assert_equal(name, "Family Vault")

        wallet = self.node.get_wallet_rpc(name)
        info = wallet.getwalletinfo()
        assert_equal(info["private_keys_enabled"], True)
        assert_equal(info["blank"], True)
        assert "unlocked_until" not in info
        assert_equal(wallet.listdescriptors()["descriptors"], [])

    def test_create_coordinator(self):
        self.log.info("creating watch-only wallet for coordinator")
        name = self.wizard.create_wallet(self.node, "Coordinator", holds_key=False)
        info = self.node.get_wallet_rpc(name).getwalletinfo()
        assert_equal(info["private_keys_enabled"], False)
        assert_equal(info["blank"], True)
        assert_raises_message(ValueError, "only applies to a wallet that holds a key",
                              self.wizard.create_wallet, self.node, "Coordinator 2",
                              holds_key=False, passphrase="secret")

    def test_create_encrypted(self):
        self.log.info("Encrypting wallet")
        name = self.wizard.create_wallet(self.node, "Encrypted Vault", passphrase="secret")
        info = self.node.get_wallet_rpc(name).getwalletinfo()
        assert_equal(info["unlocked_until"], 0)

    def test_generate_key(self):
        keys = []
        for name in ["Alice", "Bob"]:
            wallet = self.node.get_wallet_rpc(self.wizard.create_wallet(self.node, name))
            key = self.wizard.generate_key(wallet)
            keys.append(key)

            master_xpub = wallet.gethdkeys()[0]["xpub"]
            expected = wallet.derivehdkey("m/87h/1h/0h", hdkey=master_xpub)
            assert_equal(key, f"{expected['origin']}{expected['xpub']}")
            assert key.startswith("[")
            assert "/87h/1h/0h]" in key

            descriptors = wallet.listdescriptors()["descriptors"]
            assert_equal(len(descriptors), 1)
            assert descriptors[0]["desc"].startswith(f"unused({master_xpub})")
            assert_equal(descriptors[0]["active"], False)

        assert_not_equal(keys[0], keys[1])

        self.keys = keys

    def test_generate_key_encrypted(self):
        wallet = self.node.get_wallet_rpc(
            self.wizard.create_wallet(self.node, "Carol", passphrase="secret"))
        assert_raises_rpc_error(-13, "Please enter the wallet passphrase with walletpassphrase first", self.wizard.generate_key, wallet)
        with WalletUnlock(wallet, "secret"):
            key = self.wizard.generate_key(wallet)
        assert "/87h/1h/0h]" in key

    def test_validate_key(self):
        wallet = self.node.get_wallet_rpc("Alice")
        key = self.keys[0]
        assert_equal(self.wizard.validate_key(wallet, key), key)

        origin, xpub = key.split("]")
        assert_equal(self.wizard.validate_key(wallet, f"{origin.replace('h', chr(39))}]{xpub}"), key)

        for bad in [key.replace("tpub", "tpuB"), key.replace("[", "").replace("]", "")]:
            assert_raises_message(ValueError, "Invalid key", self.wizard.validate_key, wallet, bad)

        assert_raises_message(ValueError, "must include key origin information", self.wizard.validate_key, wallet, key[key.index("]") + 1:])
        assert_raises_message(ValueError, "must not specify a derivation path", self.wizard.validate_key, wallet, f"{key}/0")
        assert_raises_message(ValueError, "must not specify a multipath derivation", self.wizard.validate_key, wallet, f"{key}/<0;1>/*")

        master_xprv = wallet.gethdkeys(private=True)[0]["xprv"]
        origin = key[:key.index("]") + 1]
        assert_raises_message(ValueError, "contains private key", self.wizard.validate_key, wallet, f"{origin}{master_xprv}")

        mainnet_xpub = "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8"
        assert_raises_message(ValueError, "Invalid key", self.wizard.validate_key, wallet, f"{origin}{mainnet_xpub}")

        master_xpub = wallet.gethdkeys()[0]["xpub"]
        other = wallet.derivehdkey("m/48h/1h/0h/2h", hdkey=master_xpub)
        assert_raises_message(ValueError, "the wizard uses the BIP 87 path", self.wizard.validate_key, wallet, f"{other['origin']}{other['xpub']}")

    def test_add_cosigner(self):
        wallet = self.node.get_wallet_rpc("Alice")
        setup = self.wizard.setup("Family Vault")

        assert_equal(self.wizard.add_cosigner(wallet, setup, "Alice", self.keys[0]), [])
        assert_equal(self.wizard.add_cosigner(wallet, setup, "Bob", self.keys[1]), [])
        assert_equal(setup["cosigners"], [{"name": "Alice", "key": self.keys[0]}, {"name": "Bob", "key": self.keys[1]}])

        # Raise if duplicate keys or cosigner names exist
        assert_raises_message(ValueError, "Duplicate key", self.wizard.add_cosigner, wallet, setup, "Carol", self.keys[0])
        origin, xpub = self.keys[0].split("]")
        assert_raises_message(ValueError, "Duplicate key", self.wizard.add_cosigner, wallet, setup, "Carol", f"{origin.replace('h', chr(39))}]{xpub}")
        assert_raises_message(ValueError, "already a cosigner called Bob", self.wizard.add_cosigner, wallet, setup, "Bob", self.keys[1])

        # Warn if multiple keys have same fingerprint.
        master_xpub = wallet.gethdkeys()[0]["xpub"]
        second = wallet.derivehdkey("m/87h/1h/1h", hdkey=master_xpub)
        warnings = self.wizard.add_cosigner(wallet, setup, "Alice spare", f"{second['origin']}{second['xpub']}", account=1)
        assert_equal(len(warnings), 1)
        assert "same fingerprint as Alice" in warnings[0]
        assert_equal(len(setup["cosigners"]), 3)


if __name__ == '__main__':
    MultisigWizardTest(__file__).main()
