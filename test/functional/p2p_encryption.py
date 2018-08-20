#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests NODE_ENCRYPTED (v2 protocol, encryption after BIP151)."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, connect_nodes

class EncryptionTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.extra_args = [['-netencryption'], ['-netencryption'], ['-netencryption=0'], ['-netencryption']]

    def setup_network(self):
        self.setup_nodes()
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[2], 1)
        connect_nodes(self.nodes[2], 3)
        connect_nodes(self.nodes[3], 1)
        self.sync_all()

    def getEncryptionSessions(self, node):
        session_ids = []
        for peer in node.getpeerinfo():
            if peer['encrypted']:
                session_ids.append(peer['encryption_session_id'])
        return session_ids

    def run_test(self):
        self.nodes[0].generate(101)
        self.sync_all()
        nodes_session_id = []
        for i in range(0,4):
            nodes_session_id.append(self.getEncryptionSessions(self.nodes[i]))
        print(nodes_session_id)
        self.log.info("Check that node0 has one encrypted connections.")
        assert_equal(len(nodes_session_id[0]), 1)
        self.log.info("Check that node1 has two encrypted connections.")
        assert_equal(len(nodes_session_id[1]), 2)
        self.log.info("Check that node2 has zero encrypted connections.")
        assert_equal(len(nodes_session_id[2]), 0)
        self.log.info("Check that node3 has one encrypted connections.")
        assert_equal(len(nodes_session_id[3]), 1)
        self.log.info("Make sure session id matches")
        assert(nodes_session_id[0][0] in nodes_session_id[1])
        assert(nodes_session_id[3][0] in nodes_session_id[1])

if __name__ == '__main__':
    EncryptionTest().main()
