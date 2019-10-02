#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listunspent API."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_array_result

class ListUnspentTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Generate one block to an address
        address = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(1, address)

        # Immature coinbase transactions should not be safe
        assert_array_result(self.nodes[0].listunspent(),
                    {"address": address},
                    {"safe": False})

        assert_array_result(self.nodes[0].listunspent(include_unsafe=False),
                    {"address": address},
                    {}, True)

        new_address = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(100, new_address)

        # Mature coinbase transactions should be safe
        assert_array_result(self.nodes[0].listunspent(),
                    {"address": address},
                    {"safe": True})

        assert_array_result(self.nodes[0].listunspent(include_unsafe=False),
                    {"address": address},
                    {"safe": True})


if __name__ == '__main__':
    ListUnspentTest().main()
