#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test createwallet arguments.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class CreateWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1
        self.supports_cli = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        node.generate(1) # Leave IBD for sethdseed

        self.nodes[0].createwallet(wallet_name='w0')
        w0 = node.get_wallet_rpc('w0')
        address1 = w0.getnewaddress()

        self.log.info("Test disableprivatekeys creation.")
        self.nodes[0].createwallet(wallet_name='w1', disable_private_keys=True)
        w1 = node.get_wallet_rpc('w1')
        assert_raises_rpc_error(-4, "Error: Private keys are disabled for this wallet", w1.getnewaddress)
        assert_raises_rpc_error(-4, "Error: Private keys are disabled for this wallet", w1.getrawchangeaddress)
        w1.importpubkey(w0.getaddressinfo(address1)['pubkey'])

        self.log.info("Test empty creation with private keys disabled.")
        self.nodes[0].createwallet(wallet_name='w2', disable_private_keys=True, empty=True)
        w2 = node.get_wallet_rpc('w2')
        assert_raises_rpc_error(-4, "Error: Private keys are disabled for this wallet", w2.getnewaddress)
        assert_raises_rpc_error(-4, "Error: Private keys are disabled for this wallet", w2.getrawchangeaddress)
        w2.importpubkey(w0.getaddressinfo(address1)['pubkey'])

        self.log.info("Test empty creation with private keys enabled.")
        self.nodes[0].createwallet(wallet_name='w3', disable_private_keys=False, empty=True)
        w3 = node.get_wallet_rpc('w3')
        assert_equal(w3.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-12, "Error: Keypool ran out, please call keypoolrefill first", w3.getnewaddress)
        assert_raises_rpc_error(-12, "Error: Keypool ran out, please call keypoolrefill first", w3.getrawchangeaddress)
        # Import private key
        w3.importprivkey(w0.dumpprivkey(address1))
        # Imported private keys are currently ignored by the keypool
        assert_equal(w3.getwalletinfo()['keypoolsize'], 0)
        assert_raises_rpc_error(-12, "Error: Keypool ran out, please call keypoolrefill first", w3.getnewaddress)
        # Set the seed
        w3.sethdseed()
        assert_equal(w3.getwalletinfo()['keypoolsize'], 1)
        w3.getnewaddress()
        w3.getrawchangeaddress()

if __name__ == '__main__':
    CreateWalletTest().main()
