#!/usr/bin/env python3
# Copyright (c) 2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Wallet encryption"""

import time

from test_framework.test_framework import BitcoinTestFramework, BITCOIND_PROC_WAIT_TIMEOUT
from test_framework.util import (
    assert_equal,
    assert_raises_jsonrpc,
)

class WalletEncryptionTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        passphrase = "WalletPassphrase"
        passphrase2 = "SecondWalletPassphrase"

        # Make sure the wallet isn't encrypted first
        address = self.nodes[0].getnewaddress()
        privkey = self.nodes[0].dumpprivkey(address)
        assert_equal(privkey[:1], "c")
        assert_equal(len(privkey), 52)

        # Encrypt the wallet
        self.nodes[0].encryptwallet(passphrase)
        self.bitcoind_processes[0].wait(timeout=BITCOIND_PROC_WAIT_TIMEOUT)
        self.nodes[0] = self.start_node(0, self.options.tmpdir)

        # Test that the wallet is encrypted
        assert_raises_jsonrpc(-13, "Please enter the wallet passphrase with walletpassphrase first", self.nodes[0].dumpprivkey, address)

        # Check that walletpassphrase works
        self.nodes[0].walletpassphrase(passphrase, 2)
        assert_equal(privkey, self.nodes[0].dumpprivkey(address))

        # Check that the timeout is right
        time.sleep(2)
        assert_raises_jsonrpc(-13, "Please enter the wallet passphrase with walletpassphrase first", self.nodes[0].dumpprivkey, address)

        # Test wrong passphrase
        assert_raises_jsonrpc(-14, "wallet passphrase entered was incorrect", self.nodes[0].walletpassphrase, passphrase + "wrong", 10)

        # Test walletlock
        self.nodes[0].walletpassphrase(passphrase, 84600)
        assert_equal(privkey, self.nodes[0].dumpprivkey(address))
        self.nodes[0].walletlock()
        assert_raises_jsonrpc(-13, "Please enter the wallet passphrase with walletpassphrase first", self.nodes[0].dumpprivkey, address)

        # Test passphrase changes
        self.nodes[0].walletpassphrasechange(passphrase, passphrase2)
        assert_raises_jsonrpc(-14, "wallet passphrase entered was incorrect", self.nodes[0].walletpassphrase, passphrase, 10)
        self.nodes[0].walletpassphrase(passphrase2, 10)
        assert_equal(privkey, self.nodes[0].dumpprivkey(address))

if __name__ == '__main__':
    WalletEncryptionTest().main()
