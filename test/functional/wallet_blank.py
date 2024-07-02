#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import BitcoinTestFramework
from test_framework.address import (
    ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR,
)
from test_framework.util import (
    assert_equal,
)


class WalletBlankTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_importdescriptors(self):
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

    def test_encrypt_descriptors(self):
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
        self.test_importdescriptors()
        self.test_encrypt_descriptors()


if __name__ == '__main__':
    WalletBlankTest().main()
