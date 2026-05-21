#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet fallbackfee."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

HIGH_TX_FEE_PER_KB = Decimal('0.01')


class WalletFallbackFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def sending_succeeds(self, node):
        # Check that fallback fee is being used as a test-of-the-test.
        assert_equal(
            node.sendtoaddress(node.getnewaddress(), 1, verbose=True)['fee_reason'],
            "Fallback fee"
        )
        node.fundrawtransaction(node.createrawtransaction([], {node.getnewaddress(): 1}))
        assert_equal(
            node.sendmany("", {node.getnewaddress(): 1}, verbose=True)["fee_reason"],
            "Fallback fee"
        )

    def sending_fails(self, node):
        assert_raises_rpc_error(-6, "Fee estimation failed", lambda: node.sendtoaddress(node.getnewaddress(), 1))
        assert_raises_rpc_error(-4, "Fee estimation failed", lambda: node.fundrawtransaction(node.createrawtransaction([], {node.getnewaddress(): 1})))
        assert_raises_rpc_error(-6, "Fee estimation failed", lambda: node.sendmany("", {node.getnewaddress(): 1}))

    def run_test(self):
        node = self.nodes[0]
        self.generate(node, COINBASE_MATURITY + 1)

        # By default, the test framework sets a fallback fee for nodes,
        # in order to test default behavior, comment this line out.
        node.replace_in_config([("fallbackfee=", "#fallbackfee=")])
        self.restart_node(0)

        # Sending a transaction with no -fallbackfee setting fails, since the
        # default value is 0.
        self.sending_fails(node)

        # Sending a tx with explicitly disabled fallback fee fails.
        self.restart_node(0, extra_args=["-fallbackfee=0"])
        self.sending_fails(node)

        # Sending a transaction with a fallback fee set succeeds. Use the
        # largest fallbackfee value that doesn't trigger a warning.
        self.restart_node(0, extra_args=[f"-fallbackfee={HIGH_TX_FEE_PER_KB}"])
        self.sending_succeeds(node)
        self.stop_node(0, expected_stderr='')

        # Starting a node with a large fallback fee set...
        excessive_fallback = HIGH_TX_FEE_PER_KB + Decimal('0.00000001')
        self.start_node(0, extra_args=[f"-fallbackfee={excessive_fallback}"])
        # ...works...
        self.sending_succeeds(node)
        # ...but results in a warning message.
        expected_error = "Warning: -fallbackfee is set very high! This is the transaction fee you may pay when fee estimates are not available."
        self.stop_node(0, expected_stderr=expected_error)


if __name__ == '__main__':
    WalletFallbackFeeTest(__file__).main()
