#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC generate output."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_process_error

class GenerateRpcTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        starting_blockcount = self.nodes[0].getblockcount()
        """check that "generate 1" returns a single block hash"""
        assert_equal(len(self.nodes[0].cli.send_cli("-generate")["blocks"]), 1)
        """check that "generate 1" also moved the chain for one block"""
        assert_equal(self.nodes[0].getblockcount(), starting_blockcount + 1)
        """check that "generate 3" returns three blocks"""
        assert_equal(len(self.nodes[0].cli.send_cli("-generate", 3)['blocks']), 3)
        """check that "generate 3" moved the chain for three more blocks"""
        assert_equal(self.nodes[0].getblockcount(), starting_blockcount + 4)
        """check that "generate 1 100000" generates one block with 10,000 max. tries"""
        assert_equal(len(self.nodes[0].cli.send_cli("-generate", 5, 100000)['blocks']), 5)
        """check that "generate 5 100000" moved the chain for five more blocks"""
        assert_equal(self.nodes[0].getblockcount(), starting_blockcount + 9)
        """check for error response when param is a non-numerical value"""
        wrong_nblocks = "foo"
        assert_raises_process_error(1, "error: Error parsing JSON:{}".format(wrong_nblocks),
                                  self.nodes[0].cli("-generate", wrong_nblocks).echo)

if __name__ == '__main__':
    GenerateRpcTest().main()
