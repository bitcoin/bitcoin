#!/usr/bin/env python3
# Copyright (c) 2021 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test being able to connect to the same devnet"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import connect_nodes_bi

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
        """
        There isn't any test logic needed, if the nodes can't connect that means it is broken,
        so the test is seeing if a devnet can be started.
        """

if __name__ == '__main__':
    ConnectDevnetNodes().main()
