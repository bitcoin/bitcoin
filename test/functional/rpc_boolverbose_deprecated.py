#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that deprecated boolean verbosity works with -deprecatedrpc=boolverbose flag."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class BoolVerboseDeprecatedTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        # Enable the deprecated boolean verbosity support
        self.extra_args = [["-deprecatedrpc=boolverbose"]]

    def run_test(self):
        self.log.info(
            "Test that boolean verbosity works with -deprecatedrpc=boolverbose"
        )

        # Generate a block to have transactions to test with
        self.generate(self.nodes[0], 1)
        blockhash = self.nodes[0].getbestblockhash()

        # Test getblock with boolean verbosity - should work with deprecation flag
        block_true = self.nodes[0].getblock(blockhash, True)
        block_false = self.nodes[0].getblock(blockhash, False)
        block_int_1 = self.nodes[0].getblock(blockhash, 1)
        block_int_0 = self.nodes[0].getblock(blockhash, 0)

        # Boolean True should be equivalent to integer 1
        assert_equal(block_true, block_int_1)
        # Boolean False should be equivalent to integer 0
        assert_equal(block_false, block_int_0)

        # Test getrawtransaction with boolean verbosity - should work with deprecation flag
        block = self.nodes[0].getblock(blockhash, 1)
        txid = block["tx"][0]

        tx_true = self.nodes[0].getrawtransaction(txid, True, blockhash)
        tx_false = self.nodes[0].getrawtransaction(txid, False, blockhash)
        tx_int_1 = self.nodes[0].getrawtransaction(txid, 1, blockhash)
        tx_int_0 = self.nodes[0].getrawtransaction(txid, 0, blockhash)

        # Boolean True should be equivalent to integer 1
        assert_equal(tx_true, tx_int_1)
        # Boolean False should be equivalent to integer 0
        assert_equal(tx_false, tx_int_0)


if __name__ == "__main__":
    BoolVerboseDeprecatedTest(__file__).main()
