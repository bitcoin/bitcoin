#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of RPC calls."""
from test_framework.test_framework import BitcoinTestFramework

class DeprecatedRpcTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[]]

    def run_test(self):
        # This test should be used to verify the errors of the currently
        # deprecated RPC methods (without the -deprecatedrpc flag) until
        # such RPCs are fully removed.
        #
        # self.log.info("Test generate RPC")
        # assert_raises_rpc_error(-32, 'The wallet generate rpc method is deprecated', self.nodes[0].rpc.generate, 1)
        #
        # Please ensure that the -deprecatedrpc flag for this RPC method is
        # used in another test where the RPC functionality is actually tested.

        self.log.info("Currently no tests for deprecated RPC methods")

if __name__ == '__main__':
    DeprecatedRpcTest(__file__).main()
