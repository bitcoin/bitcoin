#!/usr/bin/env python3
# Copyright (c) 2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test being able to connect to the same devnet"""

from test_framework.mininode import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, connect_nodes_bi

class ConnectDevnetNodes(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.chain = "devnet"
        self.num_nodes = 2

    def setup_network(self):
        self.add_nodes(self.num_nodes)
        self.start_node(0)
        self.start_node(1)
        connect_nodes_bi(self.nodes, 0, 1)
        self.sync_all()


    def run_test(self):
        self.nodes[0].add_p2p_connection(P2PInterface())
        assert_equal(self.nodes[0].getconnectioncount(), 3)  # 2 in/out dashd + 1 p2p


if __name__ == '__main__':
    ConnectDevnetNodes().main()
