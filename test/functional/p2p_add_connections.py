#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test add_outbound_p2p_connection test framework functionality"""

from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


def check_node_connections(*, node, num_in, num_out):
    info = node.getnetworkinfo()
    assert_equal(info["connections_in"], num_in)
    assert_equal(info["connections_out"], num_out)


class P2PAddConnections(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def setup_network(self):
        self.setup_nodes()
        # Don't connect the nodes

    def run_test(self):
        self.log.info("Add 8 outbounds to node 0")
        for i in range(8):
            self.log.info(f"outbound: {i}")
            self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=i, connection_type="outbound-full-relay")

        self.log.info("Add 2 block-relay-only connections to node 0")
        for i in range(2):
            self.log.info(f"block-relay-only: {i}")
            # set p2p_idx based on the outbound connections already open to the
            # node, so add 8 to account for the previous full-relay connections
            self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=i + 8, connection_type="block-relay-only")

        self.log.info("Add 2 block-relay-only connections to node 1")
        for i in range(2):
            self.log.info(f"block-relay-only: {i}")
            self.nodes[1].add_outbound_p2p_connection(P2PInterface(), p2p_idx=i, connection_type="block-relay-only")

        self.log.info("Add 5 inbound connections to node 1")
        for i in range(5):
            self.log.info(f"inbound: {i}")
            self.nodes[1].add_p2p_connection(P2PInterface())

        self.log.info("Add 8 outbounds to node 1")
        for i in range(8):
            self.log.info(f"outbound: {i}")
            # bump p2p_idx to account for the 2 existing outbounds on node 1
            self.nodes[1].add_outbound_p2p_connection(P2PInterface(), p2p_idx=i + 2)

        self.log.info("Check the connections opened as expected")
        check_node_connections(node=self.nodes[0], num_in=0, num_out=10)
        check_node_connections(node=self.nodes[1], num_in=5, num_out=10)

        self.log.info("Disconnect p2p connections & try to re-open")
        self.nodes[0].disconnect_p2ps()
        check_node_connections(node=self.nodes[0], num_in=0, num_out=0)

        self.log.info("Add 8 outbounds to node 0")
        for i in range(8):
            self.log.info(f"outbound: {i}")
            self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=i)
        check_node_connections(node=self.nodes[0], num_in=0, num_out=8)

        self.log.info("Add 2 block-relay-only connections to node 0")
        for i in range(2):
            self.log.info(f"block-relay-only: {i}")
            # bump p2p_idx to account for the 8 existing outbounds on node 0
            self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=i + 8, connection_type="block-relay-only")
        check_node_connections(node=self.nodes[0], num_in=0, num_out=10)

        self.log.info("Restart node 0 and try to reconnect to p2ps")
        self.restart_node(0)

        self.log.info("Add 4 outbounds to node 0")
        for i in range(4):
            self.log.info(f"outbound: {i}")
            self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=i)
        check_node_connections(node=self.nodes[0], num_in=0, num_out=4)

        self.log.info("Add 2 block-relay-only connections to node 0")
        for i in range(2):
            self.log.info(f"block-relay-only: {i}")
            # bump p2p_idx to account for the 4 existing outbounds on node 0
            self.nodes[0].add_outbound_p2p_connection(P2PInterface(), p2p_idx=i + 4, connection_type="block-relay-only")
        check_node_connections(node=self.nodes[0], num_in=0, num_out=6)

        check_node_connections(node=self.nodes[1], num_in=5, num_out=10)


if __name__ == '__main__':
    P2PAddConnections().main()
