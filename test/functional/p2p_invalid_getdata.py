#!/usr/bin/env python3
# Copyright (c) 2016-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Request non-existent block.
"""

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

# TestP2PConn: A peer we use to send messages to bitcoind, and store responses.
class TestP2PConn(P2PInterface):
    def __init__(self):
        super().__init__()

class InvalidGetDataTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        # Setup the p2p connections and start up the network thread.
        self.test_node = self.nodes[0].add_p2p_connection(TestP2PConn())

        network_thread_start()

        self.test_node.wait_for_verack()

        self.test_node.send_message(msg_getdata(inv=[CInv(2, 0)]))

if __name__ == '__main__':
    InvalidGetDataTest().main()

