#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mempoolchanges RPC call"""

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class MempoolChangesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        # basic start stop
        stream = node.mempoolchanges('start')
        node.mempoolchanges('stop', stream)
        # start gives a different stream id
        stream = node.mempoolchanges('start')
        assert_equal(stream, 2)
        # mempool is empty
        changes = node.mempoolchanges('pull', stream, 10)
        assert_equal(changes, [])
        # mempool with one transaction
        txid = node.sendtoaddress(address=ADDRESS_BCRT1_UNSPENDABLE, amount=1)
        changes = node.mempoolchanges('pull', stream, 10)
        assert_equal(changes, [{'add': txid}])
        # pulling again doesn't give the same data
        changes = node.mempoolchanges('pull', stream, 10)
        assert_equal(changes, [])
        node.mempoolchanges('stop', stream)


if __name__ == '__main__':
    MempoolChangesTest().main()
