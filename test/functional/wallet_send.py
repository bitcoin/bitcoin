#!/usr/bin/env python3
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet send RPC methods."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class WalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Check that nodes don't own any UTXOs
        assert_equal(len(self.nodes[0].listunspent()), 0)
        assert_equal(len(self.nodes[1].listunspent()), 0)

        self.log.info("Mine blocks on node 1 and give two coins to wallet 0")

        self.nodes[1].generate(101)
        self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        self.nodes[1].generate(1)
        self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1)
        self.nodes[1].generate(1)
        self.sync_all()

        self.log.info("Check that we have two coins at different confirmation levels")
        assert_equal(self.nodes[0].getbalance(), 2)
        assert_equal({u['confirmations'] for u in self.nodes[0].listunspent()}, {1, 2})

        self.log.info("Check that we only spend the older coin")
        self.nodes[0].sendmany(amounts={self.nodes[1].getnewaddress(): 0.1}, minconf=2)
        assert_equal({u['confirmations'] for u in self.nodes[0].listunspent()}, {1})

        self.log.info("Check that sending more than we have fails")
        assert_raises_rpc_error(-6, 'Insufficient funds', lambda: self.nodes[0].sendmany(minconf=0, amounts={self.nodes[1].getnewaddress(): 3}))


if __name__ == '__main__':
    WalletTest().main()
