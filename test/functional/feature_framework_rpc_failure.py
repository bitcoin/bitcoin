#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Verify failure to connect to bitcoind's RPC interface only raises one exception.

Multiple exceptions being raised muddies the waters of what actually went wrong.
We should maintain this bar of only raising one exception as long as additional
maintenance and complexity is low.
"""

from test_framework.util import (
    assert_raises_message,
)
from test_framework.test_framework import BitcoinTestFramework

class FeatureFrameworkRPCFailure(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def setup_network(self):
        self.log.info("Setting RPC timeout to 0 to simulate an unresponsive bitcoind, expect one warning.")
        self.rpc_timeout = 0

        assert_raises_message(AssertionError, "[node 0] Unable to connect to bitcoind after 0s", BitcoinTestFramework.setup_network, self)

    def run_test(self):
        pass

if __name__ == '__main__':
    FeatureFrameworkRPCFailure(__file__).main()
