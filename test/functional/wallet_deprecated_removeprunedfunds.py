#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
This test exercises deprecatedrpc=removeprunedfunds and should be removed when
the RPC command is deleted.
"""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error


class WalletDeprecatedRemovePrunedFunds(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Setup a wallet with funds
        node = self.nodes[0]
        node.createwallet("super_secure_wallet")
        wallet = node.get_wallet_rpc("super_secure_wallet")
        self.generatetoaddress(node, nblocks=COINBASE_MATURITY + 1, address=wallet.getnewaddress(), sync_fun=self.no_op)
        tx_to_remove = wallet.listunspent()[0]["txid"]

        self.log.info("Test calling removeprunedfunds without -deprecatedrpc=removeprunedfunds")
        assert_raises_rpc_error(
            -32, "Start bitcoind with the `-deprecatedrpc=removeprunedfunds`",
            wallet.removeprunedfunds, tx_to_remove)

        self.log.info("Test calling removeprunedfunds with -deprecatedrpc=removeprunedfunds")
        self.restart_node(0, extra_args=["-deprecatedrpc=removeprunedfunds"])
        node.loadwallet("super_secure_wallet")
        wallet = node.get_wallet_rpc("super_secure_wallet")

        wallet.gettransaction(tx_to_remove)
        wallet.removeprunedfunds(tx_to_remove)
        assert_raises_rpc_error(
            -5, "Invalid or non-wallet transaction id",
            wallet.gettransaction, tx_to_remove)


if __name__ == '__main__':
    WalletDeprecatedRemovePrunedFunds(__file__).main()
