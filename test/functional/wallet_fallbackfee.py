#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet replace-by-fee capabilities in conjunction with the fallbackfee."""

from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

HIGH_TX_FEE_PER_KB = Decimal('0.01')


class WalletRBFTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.generate(self.nodes[0], COINBASE_MATURITY + 1)

        # By default, the test framework sets a fallback fee for nodes,
        # in order to test default behavior, comment this line out.
        self.nodes[0].replace_in_config([("fallbackfee=", "#fallbackfee=")])
        self.restart_node(0)

        # Sending a transaction with no -fallbackfee setting fails, since the default value is 0.
        assert_raises_rpc_error(-6, "Fee estimation failed", lambda: self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1))
        assert_raises_rpc_error(-4, "Fee estimation failed", lambda: self.nodes[0].fundrawtransaction(self.nodes[0].createrawtransaction([], {self.nodes[0].getnewaddress(): 1})))
        assert_raises_rpc_error(-6, "Fee estimation failed", lambda: self.nodes[0].sendmany("", {self.nodes[0].getnewaddress(): 1}))

        # Sending a tx with explicitly disabled fallback fee fails.
        self.restart_node(0, extra_args=["-fallbackfee=0"])
        assert_raises_rpc_error(-6, "Fee estimation failed", lambda: self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1))
        assert_raises_rpc_error(-4, "Fee estimation failed", lambda: self.nodes[0].fundrawtransaction(self.nodes[0].createrawtransaction([], {self.nodes[0].getnewaddress(): 1})))
        assert_raises_rpc_error(-6, "Fee estimation failed", lambda: self.nodes[0].sendmany("", {self.nodes[0].getnewaddress(): 1}))

        # Sending a transaction with a fallback fee set succeeds. Use the
        # largest fallbackfee value that doesn't trigger a warning.
        self.restart_node(0, extra_args=[f"-fallbackfee={HIGH_TX_FEE_PER_KB}"])
        self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        self.nodes[0].fundrawtransaction(self.nodes[0].createrawtransaction([], {self.nodes[0].getnewaddress(): 1}))
        self.nodes[0].sendmany("", {self.nodes[0].getnewaddress(): 1})

if __name__ == '__main__':
    WalletRBFTest(__file__).main()
