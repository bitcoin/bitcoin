#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test error messages for 'getaddressinfo' and 'validateaddress' RPC commands."""

from test_framework.test_framework import BitcoinTestFramework

from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

BECH32_VALID = 'bcrt1qtmp74ayg7p24uslctssvjm06q5phz4yrxucgnv'
BECH32_INVALID_SIZE = 'bcrt1sqqpl9r5c'
BECH32_INVALID_PREFIX = 'bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4'

BASE58_VALID = 'mipcBbFg9gMiCh81Kj8tqqdgoZub1ZJRfn'
BASE58_INVALID_PREFIX = '17VZNX1SN5NtKa8UQFxwQbFeFc3iqRYhem'

INVALID_ADDRESS = 'asfah14i8fajz0123f'

class InvalidAddressErrorMessageTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_validateaddress(self):
        node = self.nodes[0]

        # Bech32
        info = node.validateaddress(BECH32_INVALID_SIZE)
        assert not info['isvalid']
        assert_equal(info['error'], 'Invalid Bech32 address data size')

        info = node.validateaddress(BECH32_INVALID_PREFIX)
        assert not info['isvalid']
        assert_equal(info['error'], 'Invalid prefix for Bech32 address')

        info = node.validateaddress(BECH32_VALID)
        assert info['isvalid']
        assert 'error' not in info

        # Base58
        info = node.validateaddress(BASE58_INVALID_PREFIX)
        assert not info['isvalid']
        assert_equal(info['error'], 'Invalid prefix for Base58-encoded address')

        info = node.validateaddress(BASE58_VALID)
        assert info['isvalid']
        assert 'error' not in info

        # Invalid address format
        info = node.validateaddress(INVALID_ADDRESS)
        assert not info['isvalid']
        assert_equal(info['error'], 'Invalid address format')

    def test_getaddressinfo(self):
        node = self.nodes[0]

        assert_raises_rpc_error(-5, "Invalid Bech32 address data size", node.getaddressinfo, BECH32_INVALID_SIZE)

        assert_raises_rpc_error(-5, "Invalid prefix for Bech32 address", node.getaddressinfo, BECH32_INVALID_PREFIX)

        assert_raises_rpc_error(-5, "Invalid prefix for Base58-encoded address", node.getaddressinfo, BASE58_INVALID_PREFIX)

        assert_raises_rpc_error(-5, "Invalid address format", node.getaddressinfo, INVALID_ADDRESS)

    def run_test(self):
        self.test_validateaddress()
        self.test_getaddressinfo()


if __name__ == '__main__':
    InvalidAddressErrorMessageTest().main()
