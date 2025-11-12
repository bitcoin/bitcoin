#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the estimatefee RPCs.

Test the following RPCs:
   - estimatesmartfee
   - estimaterawfee
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

class EstimateFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        # missing required params
        assert_raises_rpc_error(-1, "estimatesmartfee", self.nodes[0].estimatesmartfee)
        assert_raises_rpc_error(-1, "estimaterawfee", self.nodes[0].estimaterawfee)

        # cli handles wrong types differently
        if not self.options.usecli:
            # wrong type for conf_target
            assert_raises_rpc_error(-3, "JSON value of type string is not of expected type number", self.nodes[0].estimatesmartfee, 'foo')
            assert_raises_rpc_error(-3, "JSON value of type string is not of expected type number", self.nodes[0].estimaterawfee, 'foo')
            # wrong type for estimatesmartfee(estimate_mode)
            assert_raises_rpc_error(-3, "JSON value of type number is not of expected type string", self.nodes[0].estimatesmartfee, 1, 1)
            # wrong type for estimaterawfee(threshold)
            assert_raises_rpc_error(-3, "JSON value of type string is not of expected type number", self.nodes[0].estimaterawfee, 1, 'foo')

        assert_raises_rpc_error(-8, 'Invalid estimate_mode parameter, must be one of: "unset", "economical", "conservative"', self.nodes[0].estimatesmartfee, 1, 'foo')
        # extra params
        assert_raises_rpc_error(-1, "estimatesmartfee", self.nodes[0].estimatesmartfee, 1, 'ECONOMICAL', 1)
        assert_raises_rpc_error(-1, "estimaterawfee", self.nodes[0].estimaterawfee, 1, 1, 1)

        # max value of 1008 per src/policy/fees/block_policy_estimator.h
        assert_raises_rpc_error(-8, "Invalid conf_target, must be between 1 and 1008", self.nodes[0].estimaterawfee, 1009)

        # valid calls
        self.nodes[0].estimatesmartfee(1)
        # self.nodes[0].estimatesmartfee(1, None)
        self.nodes[0].estimatesmartfee(1, 'ECONOMICAL')
        self.nodes[0].estimatesmartfee(1, 'unset')
        self.nodes[0].estimatesmartfee(1, 'conservative')

        self.nodes[0].estimaterawfee(1)
        self.nodes[0].estimaterawfee(1, None)
        self.nodes[0].estimaterawfee(1, 1)


if __name__ == '__main__':
    EstimateFeeTest(__file__).main()
