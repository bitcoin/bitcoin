#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet fallbackfee."""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error

class WalletFallbackFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def sending_fails(self, node):
        assert_raises_rpc_error(-6, "Fee estimation failed", lambda: node.sendtoaddress(node.getnewaddress(), 1))
        assert_raises_rpc_error(-4, "Fee estimation failed", lambda: node.fundrawtransaction(node.createrawtransaction([], {node.getnewaddress(): 1})))
        assert_raises_rpc_error(-6, "Fee estimation failed", lambda: node.sendmany("", {node.getnewaddress(): 1}))

    def run_test(self):
        node = self.nodes[0]
        self.generate(node, COINBASE_MATURITY + 1)

        # sending a transaction without fee estimations must be possible by default on regtest
        node.sendtoaddress(node.getnewaddress(), 1)

        # Sending a tx with explicitly disabled fallback fee fails.
        self.restart_node(0, extra_args=["-fallbackfee=0"])
        self.sending_fails(node)


if __name__ == '__main__':
    WalletFallbackFeeTest(__file__).main()
