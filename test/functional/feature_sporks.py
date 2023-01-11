#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time
from test_framework.test_framework import DashTestFramework
from test_framework.util import force_finish_mnsync

'''
'''

class SporkTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(3, 0)

    def setup_network(self):
        self.setup_nodes()
        # connect only 2 first nodes at start
        self.connect_nodes(0, 1)

    def get_test_spork_state(self, node):
        info = node.spork('active')
        self.bump_mocktime(5)
        # use SB spork for tests
        return info['SPORK_TEST']

    def set_test_spork_state(self, node, state):
        if state:
            value = 0
        else:
            value = 4070908800
        # use SB spork for tests
        node.spork('SPORK_TEST', value)

    def run_test(self):
        spork_default_state = self.get_test_spork_state(self.nodes[0])
        # check test spork default state matches on all nodes
        assert self.get_test_spork_state(self.nodes[1]) == spork_default_state
        assert self.get_test_spork_state(self.nodes[2]) == spork_default_state

        # check spork propagation for connected nodes
        spork_new_state = not spork_default_state
        self.set_test_spork_state(self.nodes[0], spork_new_state)
        time.sleep(0.1)
        self.wait_until(lambda: self.get_test_spork_state(self.nodes[1]), timeout=10)
        self.wait_until(lambda: self.get_test_spork_state(self.nodes[0]), timeout=10)

        # restart nodes to check spork persistence
        self.stop_node(0)
        self.stop_node(1)
        self.start_node(0)
        self.start_node(1)
        self.connect_nodes(0, 1)
        assert self.get_test_spork_state(self.nodes[0]) == spork_new_state
        assert self.get_test_spork_state(self.nodes[1]) == spork_new_state

        # Generate one block to kick off masternode sync, which also starts sporks syncing for node2
        self.generate(self.nodes[1], 1, sync_fun=self.no_op)

        # connect new node and check spork propagation after restoring from cache
        self.connect_nodes(1, 2)
        time.sleep(0.1)
        self.wait_until(lambda: self.get_test_spork_state(self.nodes[2]), timeout=12)

        # turn off and check
        self.bump_mocktime(1)
        force_finish_mnsync(self.nodes[0])
        force_finish_mnsync(self.nodes[1])
        self.set_test_spork_state(self.nodes[0], False)

        time.sleep(0.1)
        self.wait_until(lambda: not self.get_test_spork_state(self.nodes[1]), timeout=10)
        self.wait_until(lambda: not self.get_test_spork_state(self.nodes[2]), timeout=10)
        self.wait_until(lambda: not self.get_test_spork_state(self.nodes[0]), timeout=10)

if __name__ == '__main__':
    SporkTest().main()
