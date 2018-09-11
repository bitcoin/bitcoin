#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test explicit clearing of the mempool."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

from decimal import Decimal


class MempoolClearTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        address = self.nodes[0].getnewaddress()
        txid = self.nodes[0].sendtoaddress(address, Decimal('1'))
        assert_equal(self.nodes[0].getrawmempool(), [txid])

        # Add a fee delta to make the mempool more complicated before we
        # clear it.
        self.nodes[0].prioritisetransaction(txid, None, 1000)

        assert_equal(self.nodes[0].clearmempool(), {"transactions_removed": 1})
        assert_equal(self.nodes[0].getrawmempool(), [])
        assert_equal(self.nodes[0].clearmempool(), {"transactions_removed": 0})
        assert_equal(self.nodes[0].getrawmempool(), [])

        # Mine a block, it should not confirm the wallet transaction.
        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].gettransaction(txid)['confirmations'], 0)


if __name__ == '__main__':
    MempoolClearTest().main()
