#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test resendwallettransactions RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class ResendWalletTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [['--walletbroadcast=false']]

    def run_test(self):
        # Should raise RPC_WALLET_ERROR (-4) if walletbroadcast is disabled.
        assert_raises_rpc_error(-4, "Error: Wallet transaction broadcasting is disabled with -walletbroadcast", self.nodes[0].resendwallettransactions)

        # Should return an empty array if there aren't unconfirmed wallet transactions.
        self.stop_node(0)
        self.start_node(0, extra_args=[])
        assert_equal(self.nodes[0].resendwallettransactions(), [])

        # Should return an array with the unconfirmed wallet transaction.
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        assert_equal(self.nodes[0].resendwallettransactions(), [txid])

if __name__ == '__main__':
    ResendWalletTransactionsTest().main()
