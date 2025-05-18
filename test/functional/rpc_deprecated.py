#!/usr/bin/env python3
# Copyright (c) 2017-2021 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of RPC calls."""
from test_framework.test_framework import TortoisecoinTestFramework

class DeprecatedRpcTest(TortoisecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[], ['-deprecatedrpc=bumpfee']]

    def run_test(self):
        # This test should be used to verify correct behaviour of deprecated
        # RPC methods with and without the -deprecatedrpc flags. For example:
        #
        # In set_test_params:
        # self.extra_args = [[], ["-deprecatedrpc=generate"]]
        #
        # In run_test:
        # self.log.info("Test generate RPC")
        # assert_raises_rpc_error(-32, 'The wallet generate rpc method is deprecated', self.nodes[0].rpc.generate, 1)
        # self.generate(self.nodes[1], 1)

        self.log.info("No tested deprecated RPC methods")

if __name__ == '__main__':
    DeprecatedRpcTest(__file__).main()
