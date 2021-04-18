#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Widecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test coinbase transactions return the correct categories.

Tests listtransactions, listsinceblock, and gettransaction.
"""

from test_framework.test_framework import WidecoinTestFramework
from test_framework.util import (
    assert_array_result
)

class CoinbaseCategoryTest(WidecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def assert_category(self, category, address, txid, skip):
        assert_array_result(self.nodes[0].listtransactions(skip=skip),
                            {"address": address},
                            {"category": category})
        assert_array_result(self.nodes[0].listsinceblock()["transactions"],
                            {"address": address},
                            {"category": category})
        assert_array_result(self.nodes[0].gettransaction(txid)["details"],
                            {"address": address},
                            {"category": category})

    def run_test(self):
        # Generate one block to an address
        address = self.nodes[0].getnewaddress()
        self.nodes[0].generatetoaddress(1, address)
        hash = self.nodes[0].getbestblockhash()
        txid = self.nodes[0].getblock(hash)["tx"][0]

        # Coinbase transaction is immature after 1 confirmation
        self.assert_category("immature", address, txid, 0)

        # Mine another 99 blocks on top
        self.nodes[0].generate(99)
        # Coinbase transaction is still immature after 100 confirmations
        self.assert_category("immature", address, txid, 99)

        # Mine one more block
        self.nodes[0].generate(1)
        # Coinbase transaction is now matured, so category is "generate"
        self.assert_category("generate", address, txid, 100)

        # Orphan block that paid to address
        self.nodes[0].invalidateblock(hash)
        # Coinbase transaction is now orphaned
        self.assert_category("orphan", address, txid, 100)

if __name__ == '__main__':
    CoinbaseCategoryTest().main()
