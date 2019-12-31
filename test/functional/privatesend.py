#!/usr/bin/env python3
# Copyright (c) 2019 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

'''
privatesend.py

Tests PrivateSend basic RPC
'''

class PrivateSendTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        self.test_privatesend_start_stop()
        self.test_privatesend_setamount()
        self.test_privatesend_setrounds()

    def test_privatesend_start_stop(self):
        # Start Mixing
        self.nodes[0].privatesend("start")
        # Get PrivateSend info
        ps_info = self.nodes[0].getprivatesendinfo()
        # Ensure that it started properly
        assert_equal(ps_info['enabled'], True)
        assert_equal(ps_info['running'], True)

        # Stop mixing
        self.nodes[0].privatesend("stop")
        # Get PrivateSend info
        ps_info = self.nodes[0].getprivatesendinfo()
        # Ensure that it stopped properly
        assert_equal(ps_info['enabled'], True)
        assert_equal(ps_info['running'], False)

    def test_privatesend_setamount(self):
        # Try normal values
        self.nodes[0].setprivatesendamount(50)
        ps_info = self.nodes[0].getprivatesendinfo()
        assert_equal(ps_info['max_amount'], 50)

        # Try large values
        self.nodes[0].setprivatesendamount(1200000)
        ps_info = self.nodes[0].getprivatesendinfo()
        assert_equal(ps_info['max_amount'], 1200000)

    def test_privatesend_setrounds(self):
        # Try normal values
        self.nodes[0].setprivatesendrounds(5)
        ps_info = self.nodes[0].getprivatesendinfo()
        assert_equal(ps_info['max_rounds'], 5)


if __name__ == '__main__':
    PrivateSendTest().main()
