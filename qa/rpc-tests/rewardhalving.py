#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Test for -halvinginterval

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *


class RewardHalvingTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.nodes = list()
        self.setup_clean_chain = True
        self.is_network_split = False

    def setup_network(self):
        self.nodes.append(start_node(0, self.options.tmpdir))
        self.nodes.append(start_node(1, self.options.tmpdir, ['-halvinginterval=1']))
        self.nodes.append(start_node(2, self.options.tmpdir, ['-halvinginterval=50']))
        self.nodes.append(start_node(3, self.options.tmpdir, ['-halvinginterval=100']))
        self.sync_all()

    def _default_halving_interval(self):
        node = self.nodes[0]
        assert_equal(node.getbalance(), 0)
        node.generate(100)
        assert_equal(node.getbalance(), 0)
        node.generate(1)
        assert_equal(node.getbalance(), 50)
        node.generate(1)
        assert_equal(node.getbalance(), 100)
        node.generate(198)
        assert_equal(node.getbalance(), 8725)
        node.generate(1)
        assert_equal(node.getbalance(), 8750)
        node.generate(1)
        assert_equal(node.getbalance(), 8775)

    def _one_halving_interval(self):
        node = self.nodes[1]
        assert_equal(node.getbalance(), 0)
        node.generate(100)
        assert_equal(node.getbalance(), 0)
        node.generate(1)
        assert_equal(node.getbalance(), 25)
        node.generate(1)
        assert_equal(node.getbalance(), 37.5)
        node.generate(1)
        assert_equal(node.getbalance(), 43.75)

    def _fifty_halving_interval(self):
        node = self.nodes[2]
        assert_equal(node.getbalance(), 0)
        node.generate(100)
        assert_equal(node.getbalance(), 0)
        node.generate(1)
        assert_equal(node.getbalance(), 50)
        node.generate(1)
        assert_equal(node.getbalance(), 100)
        node.generate(48)
        assert_equal(node.getbalance(), 2475)
        node.generate(1)
        assert_equal(node.getbalance(), 2500)
        node.generate(1)
        assert_equal(node.getbalance(), 2525)

    def _onehundred_halving_interval(self):
        node = self.nodes[3]
        assert_equal(node.getbalance(), 0)
        node.generate(100)
        assert_equal(node.getbalance(), 0)
        node.generate(1)
        assert_equal(node.getbalance(), 50)
        node.generate(1)
        assert_equal(node.getbalance(), 100)
        node.generate(98)
        assert_equal(node.getbalance(), 4975)
        node.generate(1)
        assert_equal(node.getbalance(), 5000)
        node.generate(1)
        assert_equal(node.getbalance(), 5025)

    def run_test(self):
        self._default_halving_interval()
        self._one_halving_interval()
        self._fifty_halving_interval()
        self._onehundred_halving_interval()
        

if __name__ == '__main__':
    RewardHalvingTest().main()
