#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test CoinStatsIndex across nodes.

Test that the values returned by gettxoutsetinfo are consistent
between a node running the coinstatsindex and a node without
the index.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    try_rpc,
    wait_until,
)

class CoinStatsIndexTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.supports_cli = False
        self.extra_args = [
            [],
            ["-coinstatsindex"]
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self._test_coin_stats_index()

    def _test_coin_stats_index(self):
        node = self.nodes[0]
        index_node = self.nodes[1]

        # Generate a normal transaction and mine it
        node.generate(101)
        address = self.nodes[0].get_deterministic_priv_key().address
        node.sendtoaddress(address=address, amount=10, subtractfeefromamount=True)
        node.generate(1)

        self.sync_blocks(timeout=120)

        self.log.info("Test that gettxoutsetinfo() output is consistent with or without coinstatsindex option")
        wait_until(lambda: not try_rpc(-32603, "Unable to read UTXO set", node.gettxoutsetinfo))
        res0 = node.gettxoutsetinfo()
        wait_until(lambda: not try_rpc(-32603, "Unable to read UTXO set", index_node.gettxoutsetinfo))
        res1 = index_node.gettxoutsetinfo()

        # The field 'disk_size' is non-deterministic and can thus not be
        # compared across different nodes.
        del res1['disk_size'], res0['disk_size']

        # Everything left should be the same
        assert_equal(res1, res0)


if __name__ == '__main__':
    CoinStatsIndexTest().main()
