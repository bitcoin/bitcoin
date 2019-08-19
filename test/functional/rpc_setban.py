#!/usr/bin/env python3
# Copyright (c) 2015-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the setban rpc call."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
    p2p_port
)

class SetBanTests(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[],[]]

    def run_test(self):
        # Node 0 connects to Node 1, check that the noban permission is not granted
        connect_nodes(self.nodes[0], 1)
        peerinfo = self.nodes[1].getpeerinfo()[0]
        assert(not 'noban' in peerinfo['permissions'])

        # Node 0 get banned by Node 1
        self.nodes[1].setban("127.0.0.1", "add")

        # Node 0 should not be able to reconnect
        with self.nodes[1].assert_debug_log(expected_msgs=['dropped (banned)\n']):
            self.restart_node(1, [])
            self.nodes[0].addnode("127.0.0.1:" + str(p2p_port(1)), "onetry")

        # However, node 0 should be able to reconnect if it has noban permission
        self.restart_node(1, ['-whitelist=127.0.0.1'])
        connect_nodes(self.nodes[0], 1)
        peerinfo = self.nodes[1].getpeerinfo()[0]
        assert('noban' in peerinfo['permissions'])

        # If we remove the ban, Node 0 should be able to reconnect even without noban permission
        self.nodes[1].setban("127.0.0.1", "remove")
        self.restart_node(1, [])
        connect_nodes(self.nodes[0], 1)
        peerinfo = self.nodes[1].getpeerinfo()[0]
        assert(not 'noban' in peerinfo['permissions'])

if __name__ == '__main__':
    SetBanTests().main()
