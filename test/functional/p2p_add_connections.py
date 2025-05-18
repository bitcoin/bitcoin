#!/usr/bin/env python3
# Copyright (c) 2020-present The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test add_outbound_p2p_connection test framework functionality"""

from test_framework.p2p import P2PInterface
from test_framework.test_framework import TortoisecoinTestFramework
from test_framework.util import (
    assert_equal,
    check_node_connections,
)


class VersionSender(P2PInterface):
    def on_open(self):
        assert self.on_connection_send_msg is not None
        self.send_version()
        assert self.on_connection_send_msg is None


class P2PFeelerReceiver(P2PInterface):
    def on_version(self, message):
        # The tortoisecoind node closes feeler connections as soon as a version
        # message is received from the test framework. Don't send any responses
        # to the node's version message since the connection will already be
        # closed.
        self.send_version()

class P2PAddConnections(TortoisecoinTestFramework):
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

        self.log.info("Add 1 feeler connection to node 0")
        feeler_conn = self.nodes[0].add_outbound_p2p_connection(P2PFeelerReceiver(), p2p_idx=6, connection_type="feeler")

        # Feeler connection is closed
        assert not feeler_conn.is_connected

        # Verify version message received
        assert_equal(feeler_conn.message_count["version"], 1)
        # Feeler connections do not request tx relay
        assert_equal(feeler_conn.last_message["version"].relay, 0)

        self.log.info("Send version message early to node")
        # Normally the test framework would be shy and send the version message
        # only after it received one. See the on_version method. Check that
        # tortoisecoind behaves properly when a version is sent unexpectedly (but
        # tolerably) early.
        #
        # This checks that tortoisecoind sends its own version prior to processing
        # the remote version (and replying with a verack). Otherwise it would
        # be violating its own rules, such as "non-version message before
        # version handshake".
        ver_conn = self.nodes[0].add_outbound_p2p_connection(VersionSender(), p2p_idx=6, connection_type="outbound-full-relay", supports_v2_p2p=False, advertise_v2_p2p=False)
        ver_conn.sync_with_ping()


if __name__ == '__main__':
    P2PAddConnections(__file__).main()
