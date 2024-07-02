#!/usr/bin/env python3
# Copyright (c) 2021-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test Inactive HD Chains.
"""
import shutil
import time

from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet_util import (
    get_generate_key,
)


class InactiveHDChainsTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, descriptors=False)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-keypool=10"], ["-nowallet", "-keypool=10"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()
        self.skip_if_no_previous_releases()

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args, versions=[
            None,
            170200, # 0.17.2 Does not have the key metadata upgrade
        ])

        self.start_nodes()
        self.init_wallet(node=0)

    def prepare_wallets(self, wallet_basename, encrypt=False):
        self.nodes[0].createwallet(wallet_name=f"{wallet_basename}_base", descriptors=False, blank=True)
        self.nodes[0].createwallet(wallet_name=f"{wallet_basename}_test", descriptors=False, blank=True)
        base_wallet = self.nodes[0].get_wallet_rpc(f"{wallet_basename}_base")
        test_wallet = self.nodes[0].get_wallet_rpc(f"{wallet_basename}_test")

        # Setup both wallets with the same HD seed
        seed = get_generate_key()
        base_wallet.sethdseed(True, seed.privkey)
        test_wallet.sethdseed(True, seed.privkey)

        if encrypt:
            # Encrypting will generate a new HD seed and flush the keypool
            test_wallet.encryptwallet("pass")
        else:
            # Generate a new HD seed on the test wallet
            test_wallet.sethdseed()

        return base_wallet, test_wallet

    def do_inactive_test(self, base_wallet, test_wallet, encrypt=False):
        default = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        # The first address should be known by both wallets.
        addr1 = base_wallet.getnewaddress()
        assert test_wallet.getaddressinfo(addr1)["ismine"]
        # The address at index 9 is the first address that the test wallet will not know initially
        for _ in range(0, 9):
            base_wallet.getnewaddress()
        addr2 = base_wallet.getnewaddress()
        assert not test_wallet.getaddressinfo(addr2)["ismine"]

        # Send to first address on the old seed
        txid = default.sendtoaddress(addr1, 10)
        self.generate(self.nodes[0], 1)

        # Wait for the test wallet to see the transaction
        while True:
            try:
                test_wallet.gettransaction(txid)
                break
            except JSONRPCException:
                time.sleep(0.1)

        if encrypt:
            # The test wallet will not be able to generate the topped up keypool
            # until it is unlocked. So it still should not know about the second address
            assert not test_wallet.getaddressinfo(addr2)["ismine"]
            test_wallet.walletpassphrase("pass", 1)

        # The test wallet should now know about the second address as it
        # should have generated it in the inactive chain's keypool
        assert test_wallet.getaddressinfo(addr2)["ismine"]

        # Send to second address on the old seed
        txid = default.sendtoaddress(addr2, 10)
        self.generate(self.nodes[0], 1)
        test_wallet.gettransaction(txid)

    def test_basic(self):
        self.log.info("Test basic case for inactive HD chains")
        self.do_inactive_test(*self.prepare_wallets("basic"))

    def test_encrypted_wallet(self):
        self.log.info("Test inactive HD chains when wallet is encrypted")
        self.do_inactive_test(*self.prepare_wallets("enc", encrypt=True), encrypt=True)

    def test_without_upgraded_keymeta(self):
        # Test that it is possible to top up inactive hd chains even if there is no key origin
        # in CKeyMetadata. This tests for the segfault reported in
        # https://github.com/bitcoin/bitcoin/issues/21605
        self.log.info("Test that topping up inactive HD chains does not need upgraded key origin")

        self.nodes[0].createwallet(wallet_name="keymeta_base", descriptors=False, blank=True)
        # Createwallet is overridden in the test framework so that the descriptor option can be filled
        # depending on the test's cli args. However we don't want to do that when using old nodes that
        # do not support descriptors. So we use the createwallet_passthrough function.
        self.nodes[1].createwallet_passthrough(wallet_name="keymeta_test")
        base_wallet = self.nodes[0].get_wallet_rpc("keymeta_base")
        test_wallet = self.nodes[1].get_wallet_rpc("keymeta_test")

        # Setup both wallets with the same HD seed
        seed = get_generate_key()
        base_wallet.sethdseed(True, seed.privkey)
        test_wallet.sethdseed(True, seed.privkey)

        # Encrypting will generate a new HD seed and flush the keypool
        test_wallet.encryptwallet("pass")

        # Copy test wallet to node 0
        test_wallet.unloadwallet()
        test_wallet_dir = self.nodes[1].wallets_path / "keymeta_test"
        new_test_wallet_dir = self.nodes[0].wallets_path / "keymeta_test"
        shutil.copytree(test_wallet_dir, new_test_wallet_dir)
        self.nodes[0].loadwallet("keymeta_test")
        test_wallet = self.nodes[0].get_wallet_rpc("keymeta_test")

        self.do_inactive_test(base_wallet, test_wallet, encrypt=True)

    def run_test(self):
        self.generate(self.nodes[0], 101)

        self.test_basic()
        self.test_encrypted_wallet()
        self.test_without_upgraded_keymeta()


if __name__ == '__main__':
    InactiveHDChainsTest(__file__).main()
