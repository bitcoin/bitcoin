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
from test_framework.descriptors import descsum_create
from test_framework.wallet_util import WalletUnlock
import re


class WalletGetHDKeyTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, descriptors=True, legacy=False)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def run_test(self):
        self.test_basic_gethdkey()
        self.test_noprivs_blank()

    def test_basic_gethdkey(self):
        self.log.info("Test gethdkey basics")
        self.nodes[0].createwallet("basic")
        wallet = self.nodes[0].get_wallet_rpc("basic")
        xpub_info = wallet.gethdkey("m")
        assert "xprv" not in xpub_info
        xpub = xpub_info["xpub"]

        xpub_info = wallet.gethdkey("m", True)
        xprv = xpub_info["xprv"]
        assert_equal(xpub_info["xpub"], xpub)

        descs = wallet.listdescriptors(True)
        for desc in descs["descriptors"]:
            if "range" in desc:
                assert xprv in desc["desc"]

        self.log.info("HD pubkey can be retrieved from encrypted wallets")
        prev_xprv = xprv
        wallet.encryptwallet("pass")
        xpub_info = wallet.gethdkey("m")
        # HD key is rotated on encryption
        assert xpub_info["xpub"] != xpub
        assert "xprv" not in xpub_info

        self.log.info("HD privkey can be retrieved from encrypted wallets")
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first", wallet.gethdkey, "m", True)
        with WalletUnlock(wallet, "pass"):
            xpub_info = wallet.gethdkey("m", True)
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
        assert_raises_rpc_error(-4, "This wallet does not have an active HD key", wallet.gethdkey, "m")

        self.log.info("Test gethdkey on blank wallet")
        self.nodes[0].createwallet(wallet_name="blank", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("blank")
        assert_raises_rpc_error(-4, "This wallet does not have an active HD key", wallet.gethdkey, "m")

    def test_export(self):
        self.log.info(
            "Attempt to create a watch-only wallet clone by exporting an xpub")
        self.nodes[0].createwallet(wallet_name="w1", descriptors=True)
        w1 = self.nodes[0].get_wallet_rpc("w1")

        # Get the activate wpkh() receive descriptor
        desc = list(filter(lambda d:
                           d["active"] and not d["internal"] and d["desc"][0:4] == "wpkh",
                           w1.listdescriptors()["descriptors"])
                    )[0]["desc"]
        self.log.debug(desc)

        # Get master key fingerprint:
        fpr = re.search(r'\[(.*?)/', desc).group(1)

        self.nodes[1].createwallet(
            wallet_name="w2", descriptors=True, disable_private_keys=True)
        w2 = self.nodes[1].get_wallet_rpc("w2")

        self.log.info("Get xpub for BIP 84 native SegWit account 0")
        res = w1.gethdkey("m/84h/1h/0h")
        self.log.debug(res)
        assert res['xpub']
        assert_equal(res['fingerprint'], fpr)
        assert_equal(res['origin'], "[" + fpr + "/84h/1h/0h]")

        self.log.info("Import descriptor using the xpub and fingerprint")
        desc_receive = "wpkh([" + fpr + "/84/1h/0h]" + res['xpub'] + "/0/*)"
        desc_change = "wpkh([" + fpr + "/84/1h/0h]" + res['xpub'] + "/1/*)"
        res2 = w2.importdescriptors([{"desc": descsum_create(desc_receive),
                                      "timestamp": "now",
                                      "active": True,
                                      "internal": False,
                                      "range": [0, 10]},
                                    {"desc": descsum_create(desc_change),
                                        "timestamp": "now",
                                        "active": True,
                                        "internal": True,
                                        "range": [0, 10]}
                                     ])
        assert res2[0]['success'] and res2[1]['success']

        self.log.info("Sanity check imported xpub")
        w1_address_0 = w1.getnewaddress(address_type="bech32")
        w2_address_0 = w2.getnewaddress(address_type="bech32")
        assert_equal(w1_address_0, w2_address_0)


if __name__ == '__main__':
    WalletGetHDKeyTest().main()
