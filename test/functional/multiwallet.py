#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multiwallet."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class MultiWalletTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-wallet=w1', '-wallet=w2', '-wallet=w3']]

    def run_test(self):
        w1 = self.nodes[0] / "wallet/w1"
        w1.generate(1)

        #accessing wallet RPC without using wallet endpoint fails
        assert_raises_jsonrpc(-32601, "Method not found", self.nodes[0].getwalletinfo)

        #check w1 wallet balance
        walletinfo = w1.getwalletinfo()
        assert_equal(walletinfo['immature_balance'], 500)

        #check w1 wallet balance
        w2 = self.nodes[0] / "wallet/w2"
        walletinfo = w2.getwalletinfo()
        assert_equal(walletinfo['immature_balance'], 0)

        w3 = self.nodes[0] / "wallet/w3"
        
        w1.generate(101)
        assert_equal(w1.getbalance(), 1000)
        assert_equal(w2.getbalance(), 0)
        assert_equal(w3.getbalance(), 0)

        w1.sendtoaddress(w2.getnewaddress(), 1)
        w1.sendtoaddress(w3.getnewaddress(), 2)
        w1.generate(1)
        assert_equal(w2.getbalance(), 1)
        assert_equal(w3.getbalance(), 2)

if __name__ == '__main__':
    MultiWalletTest().main()
