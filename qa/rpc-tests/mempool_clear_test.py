#!/usr/bin/env python2
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework import BitcoinTestFramework
from util import *


class ClearMempoolTest(BitcoinTestFramework):

    def setup_network(self):
        args = ['-checkmempool', '-debug=mempool']
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, args))
        self.is_network_split = False

    def run_test(self):
        txids = []

        while self.nodes[0].getbalance() < Decimal('25.0'):
            self.nodes[0].setgenerate(True, 1)

        # Create a few random transactions
        for n in range(0, 10):
            (txid, txhex, fee) = random_transaction(self.nodes, Decimal('1.1'), Decimal('0.0'), Decimal('0.001'), 20)
            txids.append(txid)

        # Confirm transactions are in the mempool
        for txid in txids:
            assert txid in self.nodes[0].getrawmempool()

        # After clearing the mempool ...
        removed = self.nodes[0].clearmempool()

        # The transactions should reported as removed
        for txid in txids:
            assert txid in removed

        # And the mempool should be empty
        assert_equal(set(self.nodes[0].getrawmempool()), set())


if __name__ == '__main__':
    ClearMempoolTest().main()
