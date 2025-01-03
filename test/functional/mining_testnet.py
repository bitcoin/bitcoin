#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mining on testnet

Test mining related RPCs that involve difficulty adjustment, which
regtest doesn't have.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.blocktools import (
    DIFF_1_N_BITS,
    DIFF_1_TARGET,
    DIFF_4_N_BITS,
    DIFF_4_TARGET,
    nbits_str,
    target_str
)

import os

class MiningTestnetTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.chain = "testnet4"

    def add_options(self, parser):
        parser.add_argument(
            '--datafile',
            default='data/testnet4.hex',
            help='Test data file (default: %(default)s)',
        )

    def run_test(self):
        node = self.nodes[0]
        self.log.info("Load testnet4 blocks")
        self.headers_file_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), self.options.datafile)
        with open(self.headers_file_path, encoding='utf-8') as blocks_data:
            for block in blocks_data:
                node.submitblock(block.strip())

        assert_equal(node.getblockcount(), 2015)

        mining_info = node.getmininginfo()
        assert_equal(mining_info['difficulty'], 1)
        assert_equal(mining_info['bits'], nbits_str(DIFF_1_N_BITS))
        assert_equal(mining_info['target'], target_str(DIFF_1_TARGET))

        assert_equal(mining_info['next']['height'], 2016)
        assert_equal(mining_info['next']['difficulty'], 4)
        assert_equal(mining_info['next']['bits'], nbits_str(DIFF_4_N_BITS))
        assert_equal(mining_info['next']['target'], target_str(DIFF_4_TARGET))

        assert_equal(node.getdifficulty(next=True), 4)
        assert_equal(node.gettarget(next=True), target_str(DIFF_4_TARGET))

if __name__ == '__main__':
    MiningTestnetTest(__file__).main()
