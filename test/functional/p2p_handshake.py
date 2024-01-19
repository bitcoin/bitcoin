#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test P2P behaviour during the handshake phase (VERSION, VERACK messages).
"""
import itertools

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    NODE_NETWORK,
    NODE_NONE,
    NODE_P2P_V2,
    NODE_WITNESS,
)
from test_framework.p2p import P2PInterface


# usual desirable service flags for outbound non-pruned peers
DESIRABLE_SERVICE_FLAGS = NODE_NETWORK | NODE_WITNESS


class P2PHandshakeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def add_outbound_connection(self, node, connection_type, services, wait_for_disconnect):
        peer = node.add_outbound_p2p_connection(
            P2PInterface(), p2p_idx=0, wait_for_disconnect=wait_for_disconnect,
            connection_type=connection_type, services=services,
            supports_v2_p2p=self.options.v2transport, advertise_v2_p2p=self.options.v2transport)
        if not wait_for_disconnect:
            # check that connection is alive past the version handshake and disconnect manually
            peer.sync_with_ping()
            peer.peer_disconnect()
            peer.wait_for_disconnect()

    def test_desirable_service_flags(self, node, service_flag_tests, expect_disconnect):
        """Check that connecting to a peer either fails or succeeds depending on its offered
           service flags in the VERSION message. The test is exercised for all relevant
           outbound connection types where the desirable service flags check is done."""
        CONNECTION_TYPES = ["outbound-full-relay", "block-relay-only", "addr-fetch"]
        for conn_type, services in itertools.product(CONNECTION_TYPES, service_flag_tests):
            if self.options.v2transport:
                services |= NODE_P2P_V2
            expected_result = "disconnect" if expect_disconnect else "connect"
            self.log.info(f'    - services 0x{services:08x}, type "{conn_type}" [{expected_result}]')
            if expect_disconnect:
                expected_debug_log = f'does not offer the expected services ' \
                        f'({services:08x} offered, {DESIRABLE_SERVICE_FLAGS:08x} expected)'
                with node.assert_debug_log([expected_debug_log]):
                    self.add_outbound_connection(node, conn_type, services, wait_for_disconnect=True)
            else:
                self.add_outbound_connection(node, conn_type, services, wait_for_disconnect=False)

    def run_test(self):
        node = self.nodes[0]
        self.log.info("Check that lacking desired service flags leads to disconnect (non-pruned peers)")
        self.test_desirable_service_flags(node, [NODE_NONE, NODE_NETWORK, NODE_WITNESS], expect_disconnect=True)
        self.test_desirable_service_flags(node, [NODE_NETWORK | NODE_WITNESS], expect_disconnect=False)

        self.log.info("Check that feeler connections get disconnected immediately")
        with node.assert_debug_log([f"feeler connection completed"]):
            self.add_outbound_connection(node, "feeler", NODE_NONE, wait_for_disconnect=True)


if __name__ == '__main__':
    P2PHandshakeTest().main()
