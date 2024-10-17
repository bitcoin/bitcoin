#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Verify failure to connect to bitcoind's RPC interface only raises one exception.

Multiple exceptions being raised muddies the waters of what actually went wrong.
We should maintain this bar of only raising one exception as long as additional
maintenance and complexity is low.
"""

from pathlib import Path

from test_framework.util import (
    assert_equal,
)
from test_framework.test_framework import BitcoinTestFramework

class FeatureFrameworkRPCFailure(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def setup_network(self):
        test_name = Path(__file__).name
        self.log.info(f"{test_name}: Setting RPC timeout to 0 to simulate an unresponsive bitcoind")
        self.rpc_timeout = 0

        try:
            BitcoinTestFramework.setup_network(self)
        except AssertionError as e:
            assert_equal(str(e)[:47], "[node 0] Unable to connect to bitcoind after 0s")
            self.log.info(f"{test_name}: Caught AssertionError with expected RPC connection failure message")
        else:
            raise AssertionError("Didn't raise expected error")

    def run_test(self):
        pass

if __name__ == '__main__':
    FeatureFrameworkRPCFailure(__file__).main()
