#!/usr/bin/env python3
# Copyright (c) 2020-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test deprecation of the RPC getaddressinfo `label` field. It has been
superseded by the `labels` field.

"""
from test_framework.test_framework import BitcoinTestFramework

class GetAddressInfoLabelDeprecationTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        # Start node[0] with -deprecatedrpc=label, and node[1] without.
        self.extra_args = [["-deprecatedrpc=label"], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def test_label_with_deprecatedrpc_flag(self):
        self.log.info("Test getaddressinfo label with -deprecatedrpc flag")
        node = self.nodes[0]
        address = node.getnewaddress()
        info = node.getaddressinfo(address)
        assert "label" in info

    def test_label_without_deprecatedrpc_flag(self):
        self.log.info("Test getaddressinfo label without -deprecatedrpc flag")
        node = self.nodes[1]
        address = node.getnewaddress()
        info = node.getaddressinfo(address)
        assert "label" not in info

    def run_test(self):
        """Test getaddressinfo label with and without -deprecatedrpc flag."""
        self.test_label_with_deprecatedrpc_flag()
        self.test_label_without_deprecatedrpc_flag()


if __name__ == '__main__':
    GetAddressInfoLabelDeprecationTest().main()
