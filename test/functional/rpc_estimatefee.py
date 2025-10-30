#!/usr/bin/env python3
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the estimatefee RPCs.

Test the following RPCs:
   - estimatesmartfee
   - estimaterawfee
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

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
            # wrong type for estimatesmartfee(options.fee_rate_estimator)
            assert_raises_rpc_error(-3, "JSON value of type number for field fee_rate_estimator is not of expected type string", self.nodes[0].estimatesmartfee, 1, 'ECONOMICAL', {'fee_rate_estimator': 1})
            # wrong type for estimatesmartfee(options.verbosity)
            assert_raises_rpc_error(-3, "JSON value of type string for field verbosity is not of expected type number", self.nodes[0].estimatesmartfee, 1, 'ECONOMICAL', {'verbosity': 'foo'})
            # wrong type for estimaterawfee(threshold)
            assert_raises_rpc_error(-3, "JSON value of type string is not of expected type number", self.nodes[0].estimaterawfee, 1, 'foo')

        assert_raises_rpc_error(-8, 'Invalid estimate_mode parameter, must be one of: "unset", "economical", "conservative"', self.nodes[0].estimatesmartfee, 1, 'foo')
        for fee_rate_estimator in ["foo", "none", ""]:
            assert_raises_rpc_error(-8, 'Invalid fee_rate_estimator parameter, must be one of: "auto", "block_policy", "mempool_policy"',
                                    self.nodes[0].estimatesmartfee, 1, 'ECONOMICAL', {'fee_rate_estimator': fee_rate_estimator})
            assert_raises_rpc_error(-8, 'Invalid fee_rate_estimator parameter, must be one of: "auto", "block_policy", "mempool_policy"',
                                    self.nodes[0].estimatesmartfee, 1, fee_rate_estimator=fee_rate_estimator)
        assert_raises_rpc_error(-3, "Unexpected key block_policy_only", self.nodes[0].estimatesmartfee, 1, 'ECONOMICAL', {'block_policy_only': True})
        # extra params
        assert_raises_rpc_error(-1, "estimatesmartfee", self.nodes[0].estimatesmartfee, 1, 'ECONOMICAL', {}, True)
        assert_raises_rpc_error(-1, "estimaterawfee", self.nodes[0].estimaterawfee, 1, 1, 1)

        # max value of 1008 per src/policy/fees/block_policy_estimator.h
        assert_raises_rpc_error(-8, "Invalid conf_target, must be between 1 and 1008", self.nodes[0].estimaterawfee, 1009)

        # valid calls
        self.nodes[0].estimatesmartfee(1)
        # self.nodes[0].estimatesmartfee(1, None)
        self.nodes[0].estimatesmartfee(1, 'ECONOMICAL')
        self.nodes[0].estimatesmartfee(1, 'unset')
        self.nodes[0].estimatesmartfee(1, 'conservative')
        self.nodes[0].estimatesmartfee(1, 'ECONOMICAL', {"fee_rate_estimator": "block_policy"})
        self.nodes[0].estimatesmartfee(1, 'ECONOMICAL', {"fee_rate_estimator": "mempool_policy"})
        self.nodes[0].estimatesmartfee(1, 'ECONOMICAL', {'verbosity': 1, 'fee_rate_estimator': "auto"})

        self.log.info("Test estimatesmartfee options")
        node = self.nodes[0]
        default_estimate = node.estimatesmartfee(1)
        for options in [None, {}, {"sat_vb": None}, {"sat_vb": False}]:
            assert_equal(node.estimatesmartfee(1, "economical", options), default_estimate)
        for sat_vb in [None, False, True]:
            options = {"fee_rate_estimator": "block_policy", "verbosity": 2, "sat_vb": sat_vb}
            assert_equal(
                node.estimatesmartfee(1, "economical", options),
                node.estimatesmartfee(conf_target=1, options=options),
                node.estimatesmartfee(conf_target=1, **options),
                node.estimatesmartfee(1, **options),
            )
        assert_raises_rpc_error(-3, "Wrong type passed", node.estimatesmartfee, 1, "economical", True)
        assert_raises_rpc_error(-3, "JSON value of type number for field sat_vb is not of expected type bool", node.estimatesmartfee, 1, "economical", {"sat_vb": 1})
        assert_raises_rpc_error(-3, "JSON value of type number for field sat_vb is not of expected type bool", node.estimatesmartfee, 1, sat_vb=1)
        assert_raises_rpc_error(-8, "Parameter options conflicts with parameter sat_vb", node.estimatesmartfee, 1, options={"sat_vb": False}, sat_vb=True)

        self.nodes[0].estimaterawfee(1)
        self.nodes[0].estimaterawfee(1, None)
        self.nodes[0].estimaterawfee(1, 1)


if __name__ == '__main__':
    EstimateFeeTest(__file__).main()
