#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import connect_nodes, wait_until

'''
'''

class SporkTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.setup_clean_chain = True
        self.extra_args = [["-sporkkey=cVpF924EspNh8KjYsfhgY96mmxvT6DgdWiTYMtMjuM74hJaU5psW"], [], []]

    def setup_network(self):
        self.setup_nodes()
        # connect only 2 first nodes at start
        connect_nodes(self.nodes[0], 1)

    def get_test_spork_state(self, node):
        info = node.spork('active')
        # use SB spork for tests
        return info['SPORK_9_SUPERBLOCKS_ENABLED']

    def set_test_spork_state(self, node, state):
        if state:
            value = 0
        else:
            value = 4070908800
        # use SB spork for tests
        node.spork('SPORK_9_SUPERBLOCKS_ENABLED', value)

    def run_test(self):
        self.set_test_spork_state(self.nodes[0], False)
        wait_until(lambda: self.get_test_spork_state(self.nodes[1]), sleep=0.1, timeout=10)
        wait_until(lambda: self.get_test_spork_state(self.nodes[2]), sleep=0.1, timeout=10)
        # check test spork default state
        assert(not self.get_test_spork_state(self.nodes[0]))
        assert(not self.get_test_spork_state(self.nodes[1]))
        assert(not self.get_test_spork_state(self.nodes[2]))

        # check spork propagation for connected nodes
        self.set_test_spork_state(self.nodes[0], True)
        wait_until(lambda: self.get_test_spork_state(self.nodes[1]), sleep=0.1, timeout=10)

        # restart nodes to check spork persistence
        self.stop_node(0)
        self.stop_node(1)
        self.start_node(0)
        self.start_node(1)
        assert(self.get_test_spork_state(self.nodes[0]))
        assert(self.get_test_spork_state(self.nodes[1]))

        # Generate one block to kick off masternode sync, which also starts sporks syncing for node2
        self.nodes[1].generate(1)

        # connect new node and check spork propagation after restoring from cache
        connect_nodes(self.nodes[1], 2)
        wait_until(lambda: self.get_test_spork_state(self.nodes[2]), sleep=0.1, timeout=10)

if __name__ == '__main__':
    SporkTest().main()
