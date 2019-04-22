#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test descriptor wallet function."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error
)


class WalletDescriptorTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-keypool=100']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Make a descriptor wallet
        self.log.info("Making a descriptor wallet")
        self.nodes[0].createwallet("desc1", False, False, True)
        self.nodes[0].unloadwallet("")

        # A descriptor wallet should have 100 addresses * 3 types = 300 keys
        self.log.info("Checking wallet info")
        wallet_info = self.nodes[0].getwalletinfo()
        assert_equal(wallet_info['keypoolsize'], 300)
        assert_equal(wallet_info['keypoolsize_hd_internal'], 300)
        assert wallet_info['descriptors']

        # Check that getnewaddress works
        self.log.info("Test that getnewaddress and getrawchangeaddress work")
        addr = self.nodes[0].getnewaddress("", "legacy")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert_equal(addr_info['hdkeypath'], 'm/44\'/0\'/0\'')

        addr = self.nodes[0].getnewaddress("", "p2sh-segwit")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert_equal(addr_info['hdkeypath'], 'm/49\'/0\'/0\'')

        addr = self.nodes[0].getnewaddress("", "bech32")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert_equal(addr_info['hdkeypath'], 'm/84\'/0\'/0\'')

        # Check that getrawchangeaddress works
        addr = self.nodes[0].getrawchangeaddress("legacy")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert_equal(addr_info['hdkeypath'], 'm/44\'/1\'/0\'')

        addr = self.nodes[0].getrawchangeaddress("p2sh-segwit")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert_equal(addr_info['hdkeypath'], 'm/49\'/1\'/0\'')

        addr = self.nodes[0].getrawchangeaddress("bech32")
        addr_info = self.nodes[0].getaddressinfo(addr)
        assert_equal(addr_info['hdkeypath'], 'm/84\'/1\'/0\'')

        # Make a wallet to receive coins at
        self.nodes[0].createwallet("desc2", False, False, True)
        recv_wrpc = self.nodes[0].get_wallet_rpc("desc2")
        assert recv_wrpc.getwalletinfo()['descriptors']
        send_wrpc = self.nodes[0].get_wallet_rpc("desc1")

        # Generate some coins
        send_wrpc.generatetoaddress(101, send_wrpc.getnewaddress())

        # Make transactions
        self.log.info("Test sending and receiving")
        addr = recv_wrpc.getnewaddress()
        send_wrpc.sendtoaddress(addr, 10)

        # Make sure things are disabled
        self.log.info("Test disabled RPCs")
        assert_raises_rpc_error(-4, "importprivkey is not available for descriptor wallets", recv_wrpc.importprivkey, "cVpF924EspNh8KjYsfhgY96mmxvT6DgdWiTYMtMjuM74hJaU5psW")
        assert_raises_rpc_error(-4, "addmultisigaddress is not available for descriptor wallets", recv_wrpc.addmultisigaddress, 1, [recv_wrpc.getnewaddress()])
        assert_raises_rpc_error(-4, "importaddress is not available for descriptor wallets", recv_wrpc.importaddress, recv_wrpc.getnewaddress())
        assert_raises_rpc_error(-4, "dumpprivkey is not available for descriptor wallets", recv_wrpc.dumpprivkey, recv_wrpc.getnewaddress())
        assert_raises_rpc_error(-4, "importpubkey is not available for descriptor wallets", recv_wrpc.importpubkey, send_wrpc.getaddressinfo(send_wrpc.getnewaddress()))
        assert_raises_rpc_error(-4, "importmulti is not available for descriptor wallets", recv_wrpc.importmulti, [])

if __name__ == '__main__':
    WalletDescriptorTest().main ()
