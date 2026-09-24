#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the multisig setup wizard in contrib/multisig."""

import importlib
import os
import sys

from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create
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
        self.test_assemble()
        self.test_analyze_descriptor()
        self.test_verify()

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

    def setup_cosigners(self, wallet, count=3):
        """Build a setup whose cosigners each have a key from their own wallet."""
        setup = self.wizard.setup("Family Vault")
        for _ in range(count):
            self.cosigners += 1
            name = f"Cosigner {self.cosigners}"
            cosigner_wallet = self.node.get_wallet_rpc(self.wizard.create_wallet(self.node, name))
            self.wizard.add_cosigner(wallet, setup, name, self.wizard.generate_key(cosigner_wallet))
        return setup

    def test_assemble(self):
        wallet = self.node.get_wallet_rpc("Alice")
        setup = self.setup_cosigners(wallet)
        keys = [cosigner["key"] for cosigner in setup["cosigners"]]

        # Assembling WSH descriptor
        descriptor = self.wizard.assemble(wallet, setup, threshold=2)
        paths = ",".join(f"{key}/<0;1>/*" for key in keys)
        expected = f"wsh(sortedmulti(2,{paths}))"
        assert_equal(descriptor, f"{expected}#{wallet.getdescriptorinfo(expected)['checksum']}")
        assert_equal(setup["threshold"], 2)
        assert_equal(setup["type"], "bech32")

        info = wallet.getdescriptorinfo(descriptor)
        assert_equal(info["isrange"], True)
        assert_equal(info["hasprivatekeys"], False)
        assert_equal(len(info["multipath_expansion"]), 2)
        assert "/0/*" in info["multipath_expansion"][0]
        assert "/1/*" in info["multipath_expansion"][1]

        # Assembling TR descriptor
        taproot = self.wizard.assemble(wallet, setup, threshold=2, address_type="bech32m")
        assert taproot.startswith(f"tr(musig({','.join(keys)})/<0;1>/*,")
        assert f"sortedmulti_a(2,{paths})" in taproot
        assert_equal(len(wallet.getdescriptorinfo(taproot)["multipath_expansion"]), 2)

        for threshold in [1, len(keys)]:
            assert self.wizard.assemble(wallet, setup, threshold)

        # policy checks
        assert_raises_message(ValueError, "Unsupported address type", self.wizard.assemble, wallet, setup, threshold=2, address_type="p2sh-segwit")
        for threshold in [0, len(keys) + 1]:
            assert_raises_message(ValueError, "The threshold must be between", self.wizard.assemble, wallet, setup, threshold)
        too_few = self.setup_cosigners(wallet, count=1)
        assert_raises_message(ValueError, "at least 2 keys", self.wizard.assemble, wallet, too_few, threshold=1)

    def test_analyze_descriptor(self):
        wallet = self.node.get_wallet_rpc("Alice")
        setup = self.setup_cosigners(wallet)
        keys = [cosigner["key"] for cosigner in setup["cosigners"]]
        suffix = self.wizard.MULTIPATH_SUFFIX
        paths = ",".join(f"{key}{suffix}" for key in keys)
        taproot = self.wizard.assemble(wallet, setup, threshold=2, address_type="bech32m")
        nums = f"tr({self.wizard.NUMS_H},sortedmulti_a(2,{paths}))"

        assert_equal(self.wizard.analyze_descriptor(wallet, nums)["key_path"], self.wizard.KEY_PATH_NUMS)

        # Warns when keypath does not have a valid NUMS nor is a K of K musig
        outsider = self.node.get_wallet_rpc(self.wizard.create_wallet(self.node, "Outsider"))
        stranger = self.wizard.generate_key(outsider)
        for key_path in [f"{stranger}{suffix}",
                         f"musig({','.join(keys[:2])}){suffix}"]:
            assert_raises_message(ValueError, "may be spendable on its own", self.wizard.analyze_descriptor, wallet, f"tr({key_path},sortedmulti_a(2,{paths}))")

        # TR descriptors generate same address regardless of key ordering
        reordered = descsum_create(
            f"tr(musig({','.join(reversed(keys))}){suffix},sortedmulti_a(2,{paths}))")
        assert_equal(wallet.deriveaddresses(reordered, 0),
                     wallet.deriveaddresses(taproot, 0))
        assert_equal(self.wizard.analyze_descriptor(wallet, reordered)["key_path"],
                     self.wizard.KEY_PATH_MUSIG)

        # Invalid multisig descriptors
        for bad, expected in [
                (f"wpkh({keys[0]}{suffix})", "Not a wsh() or tr() descriptor"),
                (f"wsh(multi(2,{paths}))", "Expected sortedmulti(...)"),
                (f"tr(musig({','.join(keys)}){suffix},{{sortedmulti_a(2,{paths}),pk({keys[0]}{suffix})}})", "Expected sortedmulti_a(...)"),
                (f"wsh(sortedmulti(2,{paths.replace('<0;1>', '0')}))", "does not expand to a receive and a change path")]:
            assert_raises_message(ValueError, expected, self.wizard.analyze_descriptor, wallet, bad)

        assert_raises_message(ValueError, "no descriptor to verify",
                              self.wizard.verify, wallet, self.wizard.setup("Empty"))

    def test_verify(self):
        wallet = self.node.get_wallet_rpc("Alice")
        setup = self.setup_cosigners(wallet)
        descriptor = self.wizard.assemble(wallet, setup, threshold=2)
        keys = [cosigner["key"] for cosigner in setup["cosigners"]]

        summary = self.wizard.verify(wallet, setup)
        assert_equal(summary["threshold"], 2)
        assert_equal(summary["type"], "bech32")
        assert_equal(summary["key_path"], None)
        assert_equal([cosigner["fingerprint"] for cosigner in summary["cosigners"]],
                     [key[1:9] for key in keys])

        receive, change = wallet.deriveaddresses(descriptor, 0)
        assert_equal(summary["first_address"], receive[0])
        assert_not_equal(summary["first_address"], change[0])

        for cosigner in setup["cosigners"]:
            cosigner_summary = self.wizard.verify(self.node.get_wallet_rpc(cosigner["name"]), setup)
            yours = [c for c in cosigner_summary["cosigners"] if c["yours"]]
            assert_equal([c["name"] for c in yours], [cosigner["name"]])

        coordinator_summary = self.wizard.verify(self.node.get_wallet_rpc("Coordinator"), setup)
        assert_equal(coordinator_summary["holds_your_key"], False)
        assert not any(c["yours"] for c in coordinator_summary["cosigners"])

        received = self.wizard.setup("Family Vault")
        received["descriptor"] = descriptor
        bob = self.node.get_wallet_rpc(setup["cosigners"][1]["name"])
        bare = self.wizard.verify(bob, received)
        assert_equal(bare["threshold"], 2)
        assert_equal(bare["holds_your_key"], True)
        assert_equal(bare["cosigners"][1], {"name": None, "fingerprint": keys[1][1:9], "yours": True})

        taproot = dict(setup)
        self.wizard.assemble(wallet, taproot, threshold=2, address_type="bech32m")
        assert_equal(self.wizard.verify(wallet, taproot)["key_path"], self.wizard.KEY_PATH_MUSIG)


if __name__ == '__main__':
    MultisigWizardTest(__file__).main()
