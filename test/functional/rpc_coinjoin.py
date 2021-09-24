#!/usr/bin/env python3
# Copyright (c) 2019-2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

'''
rpc_coinjoin.py

Tests CoinJoin basic RPC
'''

class CoinJoinTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.test_coinjoin_start_stop()
        self.test_coinjoin_setamount()
        self.test_coinjoin_setrounds()

    def test_coinjoin_start_stop(self):
        # Start Mixing
        self.nodes[0].coinjoin("start")
        # Get CoinJoin info
        cj_info = self.nodes[0].getcoinjoininfo()
        # Ensure that it started properly
        assert_equal(cj_info['enabled'], True)
        assert_equal(cj_info['running'], True)

        # Stop mixing
        self.nodes[0].coinjoin("stop")
        # Get CoinJoin info
        cj_info = self.nodes[0].getcoinjoininfo()
        # Ensure that it stopped properly
        assert_equal(cj_info['enabled'], True)
        assert_equal(cj_info['running'], False)

    def test_coinjoin_setamount(self):
        # Try normal values
        self.nodes[0].setcoinjoinamount(50)
        cj_info = self.nodes[0].getcoinjoininfo()
        assert_equal(cj_info['max_amount'], 50)

        # Try large values
        self.nodes[0].setcoinjoinamount(1200000)
        cj_info = self.nodes[0].getcoinjoininfo()
        assert_equal(cj_info['max_amount'], 1200000)

    def test_coinjoin_setrounds(self):
        # Try normal values
        self.nodes[0].setcoinjoinrounds(5)
        cj_info = self.nodes[0].getcoinjoininfo()
        assert_equal(cj_info['max_rounds'], 5)


if __name__ == '__main__':
    CoinJoinTest().main()
