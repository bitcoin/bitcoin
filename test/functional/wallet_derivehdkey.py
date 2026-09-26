#!/usr/bin/env python3
# Copyright (c) 2025-Present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet derivehdkey RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import WalletUnlock


class WalletDeriveHDKeyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.test_basic_derivehdkey()
        self.test_multiple_unused_keys()
        self.test_active_descriptor()
        self.test_inactive_descriptor()
        self.test_noprivs_blank()
        self.test_compare()

    def test_basic_derivehdkey(self):
        self.log.info("Test derivehdkey basics")
        self.nodes[0].createwallet("basic", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("basic")
        assert_raises_rpc_error(
            -8,
            "Derivation path requires at least one hardened step",
            wallet.derivehdkey,
            "m",
        )
        assert_raises_rpc_error(
            -8,
            "Derivation path requires at least one hardened step",
            wallet.derivehdkey,
            "m/87/0/0",
        )
        # A BIP32 keypath element must be in 0..2^31-1. The
        # value 2^31 (2147483648 == 0x80000000) is out of range.
        for path in ["m/2147483648", "m/2147483648h", "m/2147483648'"]:
            assert_raises_rpc_error(
                -8,
                "Invalid BIP32 keypath",
                wallet.derivehdkey,
                path,
            )
        assert_raises_rpc_error(
            -5,
            "No active or unused(KEY) descriptor found",
            wallet.derivehdkey,
            "m/87h",
        )
        wallet.addhdkey()
        xpub_info = wallet.derivehdkey("m/87h")
        assert "xprv" not in xpub_info
        xpub = xpub_info["xpub"]
        root_fingerprint = wallet.derivehdkey("m/87h/0h")["origin"][1:9]
        assert_equal(xpub_info["origin"], f"[{root_fingerprint}/87h]")
        too_deep_path = "m/" + "/".join(["0h"] * 256)
        assert_raises_rpc_error(
            -8,
            "Unable to derive HD key at the requested path",
            wallet.derivehdkey,
            too_deep_path,
        )

        xpub_info = wallet.derivehdkey("m/87h/0h/0h/0")
        xpub_priv_info = wallet.derivehdkey("m/87h/0h/0h/0", private=True)
        xprv = xpub_priv_info["xprv"]
        assert_equal(xpub_priv_info["xpub"], xpub_info["xpub"])

        self.log.info("HD pubkey can be retrieved from encrypted wallets")
        prev_xprv = xprv
        wallet.encryptwallet("pass")
        assert_raises_rpc_error(
            -13,
            "Error: Please enter the wallet passphrase with walletpassphrase first",
            wallet.derivehdkey,
            "m/87h",
        )
        with WalletUnlock(wallet, "pass"):
            xpub_info = wallet.derivehdkey("m/87h")
            # Only automatically generated descriptors are rotated on
            # encryption, unused(KEY) is not.
            assert_equal(xpub_info["xpub"], xpub)
            assert "xprv" not in xpub_info

        self.log.info("HD privkey can be retrieved from encrypted wallets")
        assert_raises_rpc_error(
            -13,
            "Error: Please enter the wallet passphrase with walletpassphrase first",
            wallet.derivehdkey,
            "m/87h/0h/0h/0",
            private=True,
        )
        with WalletUnlock(wallet, "pass"):
            xpub_info = wallet.derivehdkey("m/87h/0h/0h/0", private=True)
            # Unused(KEY) is preferred over active descriptors and is not
            # rotated on encryption.
            assert_equal(xpub_info["xprv"], prev_xprv)

    def test_multiple_unused_keys(self):
        self.log.info("Test multiple unused(KEY) descriptors")
        self.nodes[0].createwallet("multiple_unused_keys", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("multiple_unused_keys")
        master_xpub_1 = wallet.addhdkey()['xpub']
        master_xpub_2 = wallet.addhdkey()['xpub']
        assert_raises_rpc_error(
            -5,
            "Unable to determine which HD key to use. Please specify with 'hdkey'",
            wallet.derivehdkey,
            "m/87h",
        )
        xpub_info_1 = wallet.derivehdkey("m/87h", hdkey=master_xpub_1)
        xpub_info_2 = wallet.derivehdkey("m/87h", hdkey=master_xpub_2)
        assert xpub_info_1["xpub"] != xpub_info_2["xpub"]

        assert_raises_rpc_error(
            -5,
            "Unable to parse HD key. Please provide a valid xpub",
            wallet.derivehdkey,
            "m/87h",
            hdkey="bad",
        )

    def test_active_descriptor(self):
        self.log.info("Test derivation in wallet with regular descriptors")
        self.nodes[0].createwallet("active_descriptor")
        wallet = self.nodes[0].get_wallet_rpc("active_descriptor")
        xpub_info = wallet.derivehdkey("m/44h/1h/0h")
        active_xpub = wallet.gethdkeys(active_only=True)[0]["xpub"]
        assert_equal(wallet.derivehdkey("m/44h/1h/0h", hdkey=active_xpub), xpub_info)

        # Get the activate wpkh() receive descriptor
        desc = list(filter(lambda d:
                           d["active"] and not d["internal"] and d["desc"][0:3] == "pkh",
                           wallet.listdescriptors()["descriptors"])
                    )[0]["desc"]
        self.log.debug(desc)

        assert(xpub_info["origin"] in desc)
        assert(xpub_info["xpub"] in desc)

        self.log.info("Test unused(KEY) descriptors are preferred over active descriptors")
        master_xpub = wallet.addhdkey()["xpub"]
        assert_equal(
            wallet.derivehdkey("m/87h", hdkey=master_xpub)["xpub"],
            wallet.derivehdkey("m/87h")["xpub"],
        )

    def test_inactive_descriptor(self):
        self.log.info("Test that the HD key of a used inactive descriptor is rejected")
        # Mint a spendable descriptor in a throwaway wallet, so we can import it
        # into the wallet under test and deactivate it there.
        self.nodes[0].createwallet(wallet_name="hdkey_source")
        source = self.nodes[0].get_wallet_rpc("hdkey_source")
        desc = next(d["desc"] for d in source.listdescriptors(private=True)["descriptors"]
                    if d["active"] and not d["internal"] and d["desc"].startswith("pkh("))

        self.nodes[0].createwallet(wallet_name="inactive_descriptor", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("inactive_descriptor")
        request = {"desc": desc, "range": [0, 9], "timestamp": "now", "internal": False}
        assert_equal(wallet.importdescriptors([{**request, "active": True}])[0]["success"], True)

        # While the descriptor is active its HD key can be used for derivation.
        xpub = wallet.gethdkeys(active_only=True)[0]["xpub"]
        wallet.derivehdkey("m/87h", hdkey=xpub)

        # Once deactivated the descriptor is neither active nor unused(KEY), so
        # its HD key is no longer a candidate.
        assert_equal(wallet.importdescriptors([{**request, "active": False}])[0]["success"], True)
        assert_raises_rpc_error(
            -5,
            "HD key is not used by an active or unused(KEY) descriptor",
            wallet.derivehdkey,
            "m/87h",
            hdkey=xpub,
        )

    def test_noprivs_blank(self):
        self.log.info("Test derivehdkey on wallet without private keys")
        self.nodes[0].createwallet(wallet_name="noprivs", disable_private_keys=True)
        wallet = self.nodes[0].get_wallet_rpc("noprivs")
        assert_raises_rpc_error(
            -4,
            "derivehdkey is not available for watch-only wallets",
            wallet.derivehdkey,
            "m/87h",
        )

        self.log.info("Test derivehdkey on blank wallet")
        self.nodes[0].createwallet(wallet_name="blank", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("blank")
        assert_raises_rpc_error(
            -5,
            "No active or unused(KEY) descriptor found",
            wallet.derivehdkey,
            "m/87h",
        )

    def test_compare(self):
        self.log.info(
            "Compare with result of createwalletdescriptor")
        self.nodes[0].createwallet(wallet_name="w1", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("w1")
        master_xpub = wallet.addhdkey()["xpub"]

        # Derive xpub for legacy descriptor
        xpub_info = wallet.derivehdkey("m/44h/1h/0h")

        # Generate legacy descriptor
        wallet.createwalletdescriptor(type="legacy", hdkey=master_xpub)

        # Get the activate wpkh() receive descriptor
        desc = list(filter(lambda d:
                           d["active"] and not d["internal"] and d["desc"][0:3] == "pkh",
                           wallet.listdescriptors()["descriptors"])
                    )[0]["desc"]
        self.log.debug(desc)

        assert(xpub_info["origin"] in desc)
        assert(xpub_info["xpub"] in desc)


if __name__ == '__main__':
    WalletDeriveHDKeyTest(__file__).main()
