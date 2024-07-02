#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet gethdkeys RPC."""

from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    assert_not_equal,
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
        self.test_basic_gethdkeys()
        self.test_ranged_imports()
        self.test_lone_key_imports()
        self.test_ranged_multisig()
        self.test_mixed_multisig()

    def test_basic_gethdkeys(self):
        self.log.info("Test gethdkeys basics")
        self.nodes[0].createwallet("basic")
        wallet = self.nodes[0].get_wallet_rpc("basic")
        xpub_info = wallet.gethdkeys()
        assert_equal(len(xpub_info), 1)
        assert_equal(xpub_info[0]["has_private"], True)

        assert "xprv" not in xpub_info[0]
        xpub = xpub_info[0]["xpub"]

        xpub_info = wallet.gethdkeys(private=True)
        xprv = xpub_info[0]["xprv"]
        assert_equal(xpub_info[0]["xpub"], xpub)
        assert_equal(xpub_info[0]["has_private"], True)

        descs = wallet.listdescriptors(True)
        for desc in descs["descriptors"]:
            assert xprv in desc["desc"]

        self.log.info("HD pubkey can be retrieved from encrypted wallets")
        prev_xprv = xprv
        wallet.encryptwallet("pass")
        # HD key is rotated on encryption, there should now be 2 HD keys
        assert_equal(len(wallet.gethdkeys()), 2)
        # New key is active, should be able to get only that one and its descriptors
        xpub_info = wallet.gethdkeys(active_only=True)
        assert_equal(len(xpub_info), 1)
        assert_not_equal(xpub_info[0]["xpub"], xpub)
        assert "xprv" not in xpub_info[0]
        assert_equal(xpub_info[0]["has_private"], True)

        self.log.info("HD privkey can be retrieved from encrypted wallets")
        assert_raises_rpc_error(-13, "Error: Please enter the wallet passphrase with walletpassphrase first", wallet.gethdkeys, private=True)
        with WalletUnlock(wallet, "pass"):
            xpub_info = wallet.gethdkeys(active_only=True, private=True)[0]
            assert_not_equal(xpub_info["xprv"], xprv)
            for desc in wallet.listdescriptors(True)["descriptors"]:
                if desc["active"]:
                    # After encrypting, HD key was rotated and should appear in all active descriptors
                    assert xpub_info["xprv"] in desc["desc"]
                else:
                    # Inactive descriptors should have the previous HD key
                    assert prev_xprv in desc["desc"]

    def test_ranged_imports(self):
        self.log.info("Keys of imported ranged descriptors appear in gethdkeys")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet("imports")
        wallet = self.nodes[0].get_wallet_rpc("imports")

        xpub_info = wallet.gethdkeys()
        assert_equal(len(xpub_info), 1)
        active_xpub = xpub_info[0]["xpub"]

        import_xpub = def_wallet.gethdkeys(active_only=True)[0]["xpub"]
        desc_import = def_wallet.listdescriptors(True)["descriptors"]
        for desc in desc_import:
            desc["active"] = False
        wallet.importdescriptors(desc_import)
        assert_equal(wallet.gethdkeys(active_only=True), xpub_info)

        xpub_info = wallet.gethdkeys()
        assert_equal(len(xpub_info), 2)
        for x in xpub_info:
            if x["xpub"] == active_xpub:
                for desc in x["descriptors"]:
                    assert_equal(desc["active"], True)
            elif x["xpub"] == import_xpub:
                for desc in x["descriptors"]:
                    assert_equal(desc["active"], False)
            else:
                assert False


    def test_lone_key_imports(self):
        self.log.info("Non-HD keys do not appear in gethdkeys")
        self.nodes[0].createwallet("lonekey", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("lonekey")

        assert_equal(wallet.gethdkeys(), [])
        wallet.importdescriptors([{"desc": descsum_create("wpkh(cTe1f5rdT8A8DFgVWTjyPwACsDPJM9ff4QngFxUixCSvvbg1x6sh)"), "timestamp": "now"}])
        assert_equal(wallet.gethdkeys(), [])

        self.log.info("HD keys of non-ranged descriptors should appear in gethdkeys")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        xpub_info = def_wallet.gethdkeys(private=True)
        xpub = xpub_info[0]["xpub"]
        xprv = xpub_info[0]["xprv"]
        prv_desc = descsum_create(f"wpkh({xprv})")
        pub_desc = descsum_create(f"wpkh({xpub})")
        assert_equal(wallet.importdescriptors([{"desc": prv_desc, "timestamp": "now"}])[0]["success"], True)
        xpub_info = wallet.gethdkeys()
        assert_equal(len(xpub_info), 1)
        assert_equal(xpub_info[0]["xpub"], xpub)
        assert_equal(len(xpub_info[0]["descriptors"]), 1)
        assert_equal(xpub_info[0]["descriptors"][0]["desc"], pub_desc)
        assert_equal(xpub_info[0]["descriptors"][0]["active"], False)

    def test_ranged_multisig(self):
        self.log.info("HD keys of a multisig appear in gethdkeys")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet("ranged_multisig")
        wallet = self.nodes[0].get_wallet_rpc("ranged_multisig")

        xpub1 = wallet.gethdkeys()[0]["xpub"]
        xprv1 = wallet.gethdkeys(private=True)[0]["xprv"]
        xpub2 = def_wallet.gethdkeys()[0]["xpub"]

        prv_multi_desc = descsum_create(f"wsh(multi(2,{xprv1}/*,{xpub2}/*))")
        pub_multi_desc = descsum_create(f"wsh(multi(2,{xpub1}/*,{xpub2}/*))")
        assert_equal(wallet.importdescriptors([{"desc": prv_multi_desc, "timestamp": "now"}])[0]["success"], True)

        xpub_info = wallet.gethdkeys()
        assert_equal(len(xpub_info), 2)
        for x in xpub_info:
            if x["xpub"] == xpub1:
                found_desc = next((d for d in xpub_info[0]["descriptors"] if d["desc"] == pub_multi_desc), None)
                assert found_desc is not None
                assert_equal(found_desc["active"], False)
            elif x["xpub"] == xpub2:
                assert_equal(len(x["descriptors"]), 1)
                assert_equal(x["descriptors"][0]["desc"], pub_multi_desc)
                assert_equal(x["descriptors"][0]["active"], False)
            else:
                assert False

    def test_mixed_multisig(self):
        self.log.info("Non-HD keys of a multisig do not appear in gethdkeys")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)
        self.nodes[0].createwallet("single_multisig")
        wallet = self.nodes[0].get_wallet_rpc("single_multisig")

        xpub = wallet.gethdkeys()[0]["xpub"]
        xprv = wallet.gethdkeys(private=True)[0]["xprv"]
        pub = def_wallet.getaddressinfo(def_wallet.getnewaddress())["pubkey"]

        prv_multi_desc = descsum_create(f"wsh(multi(2,{xprv},{pub}))")
        pub_multi_desc = descsum_create(f"wsh(multi(2,{xpub},{pub}))")
        import_res = wallet.importdescriptors([{"desc": prv_multi_desc, "timestamp": "now"}])
        assert_equal(import_res[0]["success"], True)

        xpub_info = wallet.gethdkeys()
        assert_equal(len(xpub_info), 1)
        assert_equal(xpub_info[0]["xpub"], xpub)
        found_desc = next((d for d in xpub_info[0]["descriptors"] if d["desc"] == pub_multi_desc), None)
        assert found_desc is not None
        assert_equal(found_desc["active"], False)


if __name__ == '__main__':
    WalletGetHDKeyTest().main()
