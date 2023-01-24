#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet gethdkey RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import WalletUnlock


class WalletGetHDKeyTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, descriptors=True, legacy=False)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.test_basic_gethdkey()
        self.test_noprivs_blank()

    def test_basic_gethdkey(self):
        self.log.info("Test gethdkey basics")
        self.nodes[0].createwallet("basic")
        wallet = self.nodes[0].get_wallet_rpc("basic")
        xpub_info = wallet.gethdkey()
        assert "xprv" not in xpub_info
        xpub = xpub_info["xpub"]

        xpub_info = wallet.gethdkey(True)
        xprv = xpub_info["xprv"]
        assert_equal(xpub_info["xpub"], xpub)

        descs = wallet.listdescriptors(True)
        for desc in descs["descriptors"]:
            if "range" in desc:
                assert xprv in desc["desc"]

        self.log.info("HD pubkey can be retrieved from encrypted wallets")
        prev_xprv = xprv
        wallet.encryptwallet("pass")
        xpub_info = wallet.gethdkey()
        # HD key is rotated on encryption
        assert xpub_info["xpub"] != xpub
        assert "xprv" not in xpub_info

        self.log.info("HD privkey can be retrieved from encrypted wallets")
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first", wallet.gethdkey, True)
        with WalletUnlock(wallet, "pass"):
            xpub_info = wallet.gethdkey(True)
            assert xpub_info["xprv"] != xprv
            for desc in wallet.listdescriptors(True)["descriptors"]:
                if desc["active"]:
                    # After encrypting, HD key was rotated and should appear in all active descriptors
                    assert xpub_info["xprv"] in desc["desc"]
                else:
                    # Inactive descriptors should have the previous HD key
                    assert prev_xprv in desc["desc"]

    def test_noprivs_blank(self):
        self.log.info("Test gethdkey on wallet without private keys")
        self.nodes[0].createwallet(wallet_name="noprivs", disable_private_keys=True)
        wallet = self.nodes[0].get_wallet_rpc("noprivs")
        assert_raises_rpc_error(-4, "This wallet does not have an active HD key", wallet.gethdkey)

        self.log.info("Test gethdkey on blank wallet")
        self.nodes[0].createwallet(wallet_name="blank", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("blank")
        assert_raises_rpc_error(-4, "This wallet does not have an active HD key", wallet.gethdkey)


if __name__ == '__main__':
    WalletGetHDKeyTest().main()
