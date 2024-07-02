#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import BitcoinTestFramework
from test_framework.address import (
    ADDRESS_BCRT1_UNSPENDABLE,
    ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
)
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet_util import generate_keypair


class WalletBlankTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def add_options(self, options):
        self.add_wallet_options(options)

    def test_importaddress(self):
        if self.options.descriptors:
            return
        self.log.info("Test that importaddress unsets the blank flag")
        self.nodes[0].createwallet(wallet_name="iaddr", disable_private_keys=True, blank=True)
        wallet = self.nodes[0].get_wallet_rpc("iaddr")
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["blank"], True)
        wallet.importaddress(ADDRESS_BCRT1_UNSPENDABLE)
        assert_equal(wallet.getwalletinfo()["blank"], False)

    def test_importpubkey(self):
        if self.options.descriptors:
            return
        self.log.info("Test that importpubkey unsets the blank flag")
        for i, comp in enumerate([True, False]):
            self.nodes[0].createwallet(wallet_name=f"ipub{i}", disable_private_keys=True, blank=True)
            wallet = self.nodes[0].get_wallet_rpc(f"ipub{i}")
            info = wallet.getwalletinfo()
            assert_equal(info["descriptors"], False)
            assert_equal(info["blank"], True)

            _, pubkey = generate_keypair(compressed=comp)
            wallet.importpubkey(pubkey.hex())
            assert_equal(wallet.getwalletinfo()["blank"], False)

    def test_importprivkey(self):
        if self.options.descriptors:
            return
        self.log.info("Test that importprivkey unsets the blank flag")
        for i, comp in enumerate([True, False]):
            self.nodes[0].createwallet(wallet_name=f"ipriv{i}", blank=True)
            wallet = self.nodes[0].get_wallet_rpc(f"ipriv{i}")
            info = wallet.getwalletinfo()
            assert_equal(info["descriptors"], False)
            assert_equal(info["blank"], True)

            wif, _ = generate_keypair(compressed=comp, wif=True)
            wallet.importprivkey(wif)
            assert_equal(wallet.getwalletinfo()["blank"], False)

    def test_importmulti(self):
        if self.options.descriptors:
            return
        self.log.info("Test that importmulti unsets the blank flag")
        self.nodes[0].createwallet(wallet_name="imulti", disable_private_keys=True, blank=True)
        wallet = self.nodes[0].get_wallet_rpc("imulti")
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["blank"], True)
        wallet.importmulti([{
            "desc": ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
            "timestamp": "now",
        }])
        assert_equal(wallet.getwalletinfo()["blank"], False)

    def test_importdescriptors(self):
        if not self.options.descriptors:
            return
        self.log.info("Test that importdescriptors preserves the blank flag")
        self.nodes[0].createwallet(wallet_name="idesc", disable_private_keys=True, blank=True)
        wallet = self.nodes[0].get_wallet_rpc("idesc")
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["blank"], True)
        wallet.importdescriptors([{
            "desc": ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
            "timestamp": "now",
        }])
        assert_equal(wallet.getwalletinfo()["blank"], True)

    def test_importwallet(self):
        if self.options.descriptors:
            return
        self.log.info("Test that importwallet unsets the blank flag")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        self.nodes[0].createwallet(wallet_name="iwallet", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("iwallet")
        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["blank"], True)

        wallet_dump_path = self.nodes[0].datadir_path / "wallet.dump"
        def_wallet.dumpwallet(wallet_dump_path)

        wallet.importwallet(wallet_dump_path)
        assert_equal(wallet.getwalletinfo()["blank"], False)

    def test_encrypt_legacy(self):
        if self.options.descriptors:
            return
        self.log.info("Test that encrypting a blank legacy wallet preserves the blank flag and does not generate a seed")
        self.nodes[0].createwallet(wallet_name="encblanklegacy", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("encblanklegacy")

        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], False)
        assert_equal(info["blank"], True)
        assert "hdseedid" not in info

        wallet.encryptwallet("pass")
        info = wallet.getwalletinfo()
        assert_equal(info["blank"], True)
        assert "hdseedid" not in info

    def test_encrypt_descriptors(self):
        if not self.options.descriptors:
            return
        self.log.info("Test that encrypting a blank descriptor wallet preserves the blank flag and descriptors remain the same")
        self.nodes[0].createwallet(wallet_name="encblankdesc", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("encblankdesc")

        info = wallet.getwalletinfo()
        assert_equal(info["descriptors"], True)
        assert_equal(info["blank"], True)
        descs = wallet.listdescriptors()

        wallet.encryptwallet("pass")
        assert_equal(wallet.getwalletinfo()["blank"], True)
        assert_equal(descs, wallet.listdescriptors())

    def run_test(self):
        self.test_importaddress()
        self.test_importpubkey()
        self.test_importprivkey()
        self.test_importmulti()
        self.test_importdescriptors()
        self.test_importwallet()
        self.test_encrypt_legacy()
        self.test_encrypt_descriptors()


if __name__ == '__main__':
    WalletBlankTest(__file__).main()
