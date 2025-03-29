#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test P2P behaviour during the handshake phase (VERSION, VERACK messages).
"""
import itertools
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_not_equal
from test_framework.messages import (
    NODE_NETWORK,
    NODE_NETWORK_LIMITED,
    NODE_NONE,
    NODE_P2P_V2,
    NODE_WITNESS,
)
from test_framework.p2p import P2PInterface
from test_framework.util import p2p_port


# Desirable service flags for outbound non-pruned and pruned peers. Note that
# the desirable service flags for pruned peers are dynamic and only apply if
#  1. the peer's service flag NODE_NETWORK_LIMITED is set *and*
#  2. the local chain is close to the tip (<24h)
DESIRABLE_SERVICE_FLAGS_FULL = NODE_NETWORK | NODE_WITNESS
DESIRABLE_SERVICE_FLAGS_PRUNED = NODE_NETWORK_LIMITED | NODE_WITNESS


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
        self.wait_until(lambda: len(node.getpeerinfo()) == 0)

    def test_desirable_service_flags(self, node, service_flag_tests, desirable_service_flags, expect_disconnect):
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
                assert_not_equal((services & desirable_service_flags), desirable_service_flags)
                expected_debug_log = f'does not offer the expected services ' \
                        f'({services:08x} offered, {desirable_service_flags:08x} expected)'
                with node.assert_debug_log([expected_debug_log]):
                    self.add_outbound_connection(node, conn_type, services, wait_for_disconnect=True)
            else:
                assert (services & desirable_service_flags) == desirable_service_flags
                self.add_outbound_connection(node, conn_type, services, wait_for_disconnect=False)

    def generate_at_mocktime(self, time):
        self.nodes[0].setmocktime(time)
        self.generate(self.nodes[0], 1)
        self.nodes[0].setmocktime(0)

    def run_test(self):
        node = self.nodes[0]
        self.log.info("Check that lacking desired service flags leads to disconnect (non-pruned peers)")
        self.test_desirable_service_flags(node, [NODE_NONE, NODE_NETWORK, NODE_WITNESS],
                                          DESIRABLE_SERVICE_FLAGS_FULL, expect_disconnect=True)
        self.test_desirable_service_flags(node, [NODE_NETWORK | NODE_WITNESS],
                                          DESIRABLE_SERVICE_FLAGS_FULL, expect_disconnect=False)

        self.log.info("Check that limited peers are only desired if the local chain is close to the tip (<24h)")
        self.generate_at_mocktime(int(time.time()) - 25 * 3600)  # tip outside the 24h window, should fail
        self.test_desirable_service_flags(node, [NODE_NETWORK_LIMITED | NODE_WITNESS],
                                          DESIRABLE_SERVICE_FLAGS_FULL, expect_disconnect=True)
        self.generate_at_mocktime(int(time.time()) - 23 * 3600)  # tip inside the 24h window, should succeed
        self.test_desirable_service_flags(node, [NODE_NETWORK_LIMITED | NODE_WITNESS],
                                          DESIRABLE_SERVICE_FLAGS_PRUNED, expect_disconnect=False)

        self.log.info("Check that feeler connections get disconnected immediately")
        with node.assert_debug_log(["feeler connection completed"]):
            self.add_outbound_connection(node, "feeler", NODE_NONE, wait_for_disconnect=True)

        self.log.info("Check that connecting to ourself leads to immediate disconnect")
        with node.assert_debug_log(["connected to self", "disconnecting"]):
            node_listen_addr = f"127.0.0.1:{p2p_port(0)}"
            node.addconnection(node_listen_addr, "outbound-full-relay", self.options.v2transport)
            self.wait_until(lambda: len(node.getpeerinfo()) == 0)


if __name__ == '__main__':
    P2PHandshakeTest(__file__).main()
