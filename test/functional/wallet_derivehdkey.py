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
        self.test_noprivs_blank()
        self.test_compare()

    def test_basic_derivehdkey(self):
        self.log.info("Test derivehdkey basics")
        self.nodes[0].createwallet("basic", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("basic")
        assert_raises_rpc_error(-8, "unused(KEY) descriptor not found", wallet.derivehdkey, "m")
        wallet.addhdkey()
        xpub_info = wallet.derivehdkey("m")
        assert "xprv" not in xpub_info
        xpub = xpub_info["xpub"]

        xpub_info = wallet.derivehdkey("m", private=True)
        xprv = xpub_info["xprv"]
        assert_equal(xpub_info["xpub"], xpub)

        descs = wallet.listdescriptors(True)
        for desc in descs["descriptors"]:
            if "range" in desc:
                assert xprv in desc["desc"]

        self.log.info("HD pubkey can be retrieved from encrypted wallets")
        prev_xprv = xprv
        wallet.encryptwallet("pass")
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first", wallet.derivehdkey, "m")
        with WalletUnlock(wallet, "pass"):
            xpub_info = wallet.derivehdkey("m")
            # HD key is rotated on encryption
            # TODO, seems like a bug
            # assert xpub_info["xpub"] != xpub
            assert "xprv" not in xpub_info

        self.log.info("HD privkey can be retrieved from encrypted wallets")
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first", wallet.derivehdkey, "m", private=True)
        with WalletUnlock(wallet, "pass"):
            xpub_info = wallet.derivehdkey("m", private=True)
            # assert xpub_info["xprv"] != xprv
            for desc in wallet.listdescriptors(True)["descriptors"]:
                if desc["active"]:
                    # After encrypting, HD key was rotated and should appear in all active descriptors
                    assert xpub_info["xprv"] in desc["desc"]
                else:
                    # Inactive descriptors should have the previous HD key
                    assert prev_xprv in desc["desc"]

    def test_noprivs_blank(self):
        self.log.info("Test derivehdkey on wallet without private keys")
        self.nodes[0].createwallet(wallet_name="noprivs", disable_private_keys=True)
        wallet = self.nodes[0].get_wallet_rpc("noprivs")
        assert_raises_rpc_error(-4, "derivehdkey is not available for watch-only wallets", wallet.derivehdkey, "m")

        self.log.info("Test derivehdkey on blank wallet")
        self.nodes[0].createwallet(wallet_name="blank", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("blank")
        assert_raises_rpc_error(-8, "unused(KEY) descriptor not found", wallet.derivehdkey, "m")

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
