#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test coinstatsindex across nodes.

Test that the values returned by gettxoutsetinfo are consistent
between a node running the coinstatsindex and a node without
the index.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    try_rpc,
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
        # Both none and muhash options allow the usage of the index
        index_hash_options = ['none', 'muhash']

        # Generate a normal transaction and mine it
        node.generate(101)
        address = self.nodes[0].get_deterministic_priv_key().address
        node.sendtoaddress(address=address, amount=10, subtractfeefromamount=True)
        node.generate(1)

        self.sync_blocks(timeout=120)

        self.log.info("Test that gettxoutsetinfo() output is consistent with or without coinstatsindex option")
        self.wait_until(lambda: not try_rpc(-32603, "Unable to read UTXO set", node.gettxoutsetinfo))
        res0 = node.gettxoutsetinfo('none')

        # The fields 'disk_size' and 'transactions' do not exist on the index
        del res0['disk_size'], res0['transactions']

        self.wait_until(lambda: not try_rpc(-32603, "Unable to read UTXO set", index_node.gettxoutsetinfo, 'muhash'))
        for hash_option in index_hash_options:
            res1 = index_node.gettxoutsetinfo(hash_option)
            res1.pop('muhash', None)

            # Everything left should be the same
            assert_equal(res1, res0)

        self.log.info("Test that gettxoutsetinfo() can get fetch data on specific heights with index")

        # Generate a new tip
        node.generate(5)

        self.wait_until(lambda: not try_rpc(-32603, "Unable to read UTXO set", index_node.gettxoutsetinfo, 'muhash'))
        for hash_option in index_hash_options:
            # Fetch old stats by height
            res2 = index_node.gettxoutsetinfo(hash_option, 102)
            res2.pop('muhash', None)
            assert_equal(res0, res2)

            # Fetch old stats by hash
            res3 = index_node.gettxoutsetinfo(hash_option, res0['bestblock'])
            res3.pop('muhash', None)
            assert_equal(res0, res3)

            # It does not work without coinstatsindex
            assert_raises_rpc_error(-8, "Querying specific block heights requires coinstatsindex", node.gettxoutsetinfo, hash_option, 102)

if __name__ == '__main__':
    CoinStatsIndexTest().main()
