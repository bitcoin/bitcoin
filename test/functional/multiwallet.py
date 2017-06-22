#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class MultiWalletTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-wallet=w1', '-wallet=w2','-wallet=w3']]

    def setup_network(self):
        self.nodes = self.start_nodes(1, self.options.tmpdir, self.extra_args[:1])

    def run_test(self):
        self.nodes[0].generate(nblocks=1, wallet="w1")

        #check default wallet balance
        assert_raises_jsonrpc(-32601, "Method not found (disabled)", self.nodes[0].getwalletinfo)

        #check w1 wallet balance
        walletinfo = self.nodes[0].getwalletinfo(wallet="w1")
        assert_equal(walletinfo['immature_balance'], 50)

        #check w1 wallet balance
        walletinfo = self.nodes[0].getwalletinfo(wallet="w2")
        assert_equal(walletinfo['immature_balance'], 0)

        self.nodes[0].generate(nblocks=101, wallet="w1")
        assert_equal(self.nodes[0].getbalance(wallet="w1"), 100)
        assert_equal(self.nodes[0].getbalance(wallet="w2"), 0)
        assert_equal(self.nodes[0].getbalance(wallet="w3"), 0)

        huh=self.nodes[0].getnewaddress(wallet="w2")
        self.nodes[0].sendtoaddress(address=huh, amount=1, wallet="w1")
        self.nodes[0].generate(nblocks=1, wallet="w1")
        assert_equal(self.nodes[0].getbalance(wallet="w2"), 1)

if __name__ == '__main__':
    MultiWalletTest().main()
