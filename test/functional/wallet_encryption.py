#!/usr/bin/env python3
# Copyright (c) 2016-2019 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test Wallet encryption"""

import time

from test_framework.test_framework import WidecoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    assert_greater_than,
    assert_greater_than_or_equal,
)


class WalletEncryptionTest(WidecoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        passphrase = "WalletPassphrase"
        passphrase2 = "SecondWalletPassphrase"

        # Make sure the wallet isn't encrypted first
        msg = "test message"
        address = self.nodes[0].getnewaddress(address_type='legacy')
        sig = self.nodes[0].signmessage(address, msg)
        assert self.nodes[0].verifymessage(address, sig, msg)
        assert_raises_rpc_error(-15, "Error: running with an unencrypted wallet, but walletpassphrase was called", self.nodes[0].walletpassphrase, 'ff', 1)
        assert_raises_rpc_error(-15, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.", self.nodes[0].walletpassphrasechange, 'ff', 'ff')

        # Encrypt the wallet
        assert_raises_rpc_error(-8, "passphrase can not be empty", self.nodes[0].encryptwallet, '')
        self.nodes[0].encryptwallet(passphrase)

        # Test that the wallet is encrypted
        assert_raises_rpc_error(-13, "Please enter the wallet passphrase with walletpassphrase first", self.nodes[0].signmessage, address, msg)
        assert_raises_rpc_error(-15, "Error: running with an encrypted wallet, but encryptwallet was called.", self.nodes[0].encryptwallet, 'ff')
        assert_raises_rpc_error(-8, "passphrase can not be empty", self.nodes[0].walletpassphrase, '', 1)
        assert_raises_rpc_error(-8, "passphrase can not be empty", self.nodes[0].walletpassphrasechange, '', 'ff')

        # Check that walletpassphrase works
        self.nodes[0].walletpassphrase(passphrase, 2)
        sig = self.nodes[0].signmessage(address, msg)
        assert self.nodes[0].verifymessage(address, sig, msg)

        # Check that the timeout is right
        time.sleep(3)
        assert_raises_rpc_error(-13, "Please enter the wallet passphrase with walletpassphrase first", self.nodes[0].signmessage, address, msg)

        # Test wrong passphrase
        assert_raises_rpc_error(-14, "wallet passphrase entered was incorrect", self.nodes[0].walletpassphrase, passphrase + "wrong", 10)

        # Test walletlock
        self.nodes[0].walletpassphrase(passphrase, 84600)
        sig = self.nodes[0].signmessage(address, msg)
        assert self.nodes[0].verifymessage(address, sig, msg)
        self.nodes[0].walletlock()
        assert_raises_rpc_error(-13, "Please enter the wallet passphrase with walletpassphrase first", self.nodes[0].signmessage, address, msg)

        # Test passphrase changes
        self.nodes[0].walletpassphrasechange(passphrase, passphrase2)
        assert_raises_rpc_error(-14, "wallet passphrase entered was incorrect", self.nodes[0].walletpassphrase, passphrase, 10)
        self.nodes[0].walletpassphrase(passphrase2, 10)
        sig = self.nodes[0].signmessage(address, msg)
        assert self.nodes[0].verifymessage(address, sig, msg)
        self.nodes[0].walletlock()

        # Test timeout bounds
        assert_raises_rpc_error(-8, "Timeout cannot be negative.", self.nodes[0].walletpassphrase, passphrase2, -10)

        self.log.info('Check a timeout less than the limit')
        MAX_VALUE = 100000000
        expected_time = int(time.time()) + MAX_VALUE - 600
        self.nodes[0].walletpassphrase(passphrase2, MAX_VALUE - 600)
        # give buffer for walletpassphrase, since it iterates over all crypted keys
        expected_time_with_buffer = time.time() + MAX_VALUE - 600
        actual_time = self.nodes[0].getwalletinfo()['unlocked_until']
        assert_greater_than_or_equal(actual_time, expected_time)
        assert_greater_than(expected_time_with_buffer, actual_time)

        self.log.info('Check a timeout greater than the limit')
        expected_time = int(time.time()) + MAX_VALUE - 1
        self.nodes[0].walletpassphrase(passphrase2, MAX_VALUE + 1000)
        expected_time_with_buffer = time.time() + MAX_VALUE
        actual_time = self.nodes[0].getwalletinfo()['unlocked_until']
        assert_greater_than_or_equal(actual_time, expected_time)
        assert_greater_than(expected_time_with_buffer, actual_time)


if __name__ == '__main__':
    WalletEncryptionTest().main()
