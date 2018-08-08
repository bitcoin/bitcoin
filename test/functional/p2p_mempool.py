#!/usr/bin/env python3
# Copyright (c) 2015-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test p2p mempool message.

Test that nodes are disconnected if they send mempool messages when bloom
filters are not enabled.
"""

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class P2PMempoolTests(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-peerbloomfilters=0"]]

    def run_test(self):
        # Add a p2p connection
        self.nodes[0].add_p2p_connection(P2PInterface())

        #request mempool
        self.nodes[0].p2p.send_message(msg_mempool())
        self.nodes[0].p2p.wait_for_disconnect()

        #mininode must be disconnected at this point
        assert_equal(len(self.nodes[0].getpeerinfo()), 0)

if __name__ == '__main__':
    P2PMempoolTests().main()
