#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the MultiChainBitcoinTestFramework implementation."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class MultiChainBitcoinTestFrameworkTest (BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 8

    def run_test(self):
        assert_equal(self.is_network_split, False)
        self.nodes[2].generate(101)
        self.sync_all()
        blockcount = self.nodes[0].getblockcount()

        # Split network into two
        self.split_network([4])
        assert_equal(self.is_network_split, True)
        assert_equal(len(self.chains), 2)
        assert_equal(len(self.chains[0]), 4)
        assert_equal(len(self.chains[1]), 4)

        self.chains[0][0].generate(5)
        block = self.chains[1][0].generate(6)[5]
        blockcount += 6
        self.sync_all()
        self.join_network()
        assert_equal(self.nodes[0].getblockcount(), blockcount)
        assert_equal(self.nodes[0].getblockhash(blockcount), block)

        # Split network into three
        self.split_network([3, 6])
        assert_equal(self.is_network_split, True)
        assert_equal(len(self.chains), 3)
        assert_equal(len(self.chains[0]), 3)
        assert_equal(len(self.chains[1]), 3)
        assert_equal(len(self.chains[2]), 2)

        self.chains[0][0].generate(5)
        block = self.chains[1][0].generate(6)[5]
        self.chains[2][0].generate(5)
        blockcount += 6
        self.sync_all()
        self.join_network()
        assert_equal(self.nodes[0].getblockcount(), blockcount)
        assert_equal(self.nodes[0].getblockhash(blockcount), block)

        # Split network into four
        self.split_network([2, 4, 6])
        assert_equal(self.is_network_split, True)
        assert_equal(len(self.chains), 4)
        assert_equal(len(self.chains[0]), 2)
        assert_equal(len(self.chains[1]), 2)
        assert_equal(len(self.chains[2]), 2)
        assert_equal(len(self.chains[3]), 2)

        self.chains[0][0].generate(5)
        block = self.chains[1][0].generate(6)[5]
        self.chains[2][0].generate(5)
        self.chains[3][0].generate(5)
        blockcount += 6
        self.sync_all()
        self.join_network()
        assert_equal(self.nodes[0].getblockcount(), blockcount)
        assert_equal(self.nodes[0].getblockhash(blockcount), block)

if __name__ == '__main__':
    MultiChainBitcoinTestFrameworkTest().main()
