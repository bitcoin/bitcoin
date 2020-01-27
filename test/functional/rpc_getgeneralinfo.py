#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getgeneralinfo RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, try_rpc

class GetGeneralInfoTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = False
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_cli()

    def run_test(self):
        """Test getgeneralinfo."""
        """Check if 'getgeneralinfo' is callable."""
        try_rpc(None, None, self.nodes[0].getgeneralinfo, None, None)
        """Check if 'getgeneralinfo' is idempotent (multiple requests return same data)."""
        json1 = self.nodes[0].getgeneralinfo()
        json2 = self.nodes[0].getgeneralinfo()
        assert_equal(json1, json2)

if __name__ == '__main__':
    GetGeneralInfoTest().main()
