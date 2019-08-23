#!/usr/bin/env python3
# Copyright (c) 2017-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that the wallet resends transactions periodically."""
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import wait_until

class ResubmitWalletTransactionsToMempoolTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):

        node = self.nodes[0]

        self.log.info("Create a new wallet transaction")

        relayfee = node.getnetworkinfo()['relayfee']
        node.settxfee(relayfee)
        txhsh = node.sendtoaddress(node.getnewaddress(), 1)

        assert txhsh in node.getrawmempool()

        # bump mocktime so the transaction should expire
        # add an extra hour for good measure
        two_weeks_in_seconds = 60 * 60 * 24 * 14
        mocktime = int(time.time()) + two_weeks_in_seconds + 60 * 60
        node.setmocktime(mocktime)

        # making a new transaction invokes ATMP which expires old txns
        node.sendtoaddress(node.getnewaddress(), 1)

        # confirm txn is no longer in mempool
        self.log.info("Confirm transaction is no longer in mempool")
        assert txhsh not in node.getrawmempool()

        # bumptime so ResubmitWalletTransactionsToMempool triggers
        # we resend once / day, so bump 25 hours just to be sure
        # we don't resubmit the first time, so we bump mocktime
        # twice so the resend occurs the second time around
        one_day_in_seconds = 60 * 60 * 25
        node.setmocktime(mocktime + one_day_in_seconds)

        time.sleep(1.1)

        node.setmocktime(mocktime + 2 * one_day_in_seconds)

        # confirm that its back in the mempool
        self.log.info("Transaction should be resubmitted to mempool")
        wait_until(lambda: txhsh in node.getrawmempool(), timeout=30)


if __name__ == '__main__':
    ResubmitWalletTransactionsToMempoolTest().main()
