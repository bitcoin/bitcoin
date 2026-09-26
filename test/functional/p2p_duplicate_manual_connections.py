#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that -connect and -addnode do not open duplicate manual connections."""

from concurrent.futures import ThreadPoolExecutor
import socket

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    p2p_port,
)

PENDING_MANUAL_CONNECTION_LOG = "connection already in progress"


def outbound_manual_peers(node):
    return [
        peer
        for peer in node.getpeerinfo()
        if not peer["inbound"] and peer["connection_type"] == "manual"
    ]


def inbound_peers(node):
    return [peer for peer in node.getpeerinfo() if peer["inbound"]]


class DuplicateManualConnectionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.disable_autoconnect = False
        self.extra_args = [
            [
                "-listen=1",
                f"-connect=127.0.0.1:{p2p_port(1)}",
                f"-addnode=127.0.0.1:{p2p_port(1)}",
            ],
            [
                "-listen=1",
                f"-connect=127.0.0.1:{p2p_port(0)}",
                f"-addnode=127.0.0.1:{p2p_port(0)}",
            ],
        ]

    def setup_network(self):
        self.setup_nodes()

    def check_pending_attempt(self, node, proxy, first_destination, duplicate_destination):
        """Hold one RPC in the proxy handshake while trying the same destination."""
        with ThreadPoolExecutor(max_workers=1) as executor:
            first = executor.submit(
                node.create_new_rpc_connection().addnode,
                first_destination,
                "onetry",
            )
            connection, _ = proxy.accept()
            try:
                with node.assert_debug_log([PENDING_MANUAL_CONNECTION_LOG]):
                    node.addnode(duplicate_destination, "onetry")
                assert_equal(first.done(), False)
                assert_equal(node.getconnectioncount(), 0)
            finally:
                connection.close()
            first.result(timeout=10)

            # The failed attempt must release the reservation.
            retry = executor.submit(
                node.create_new_rpc_connection().addnode,
                duplicate_destination,
                "onetry",
            )
            connection, _ = proxy.accept()
            connection.close()
            retry.result(timeout=10)

    def run_test(self):
        self.log.info(
            "Wait for each node to have one inbound and one outbound manual peer"
        )
        self.wait_until(
            lambda: all(
                len(inbound_peers(node)) >= 1 and len(outbound_manual_peers(node)) >= 1
                for node in self.nodes
            )
        )

        for i, node in enumerate(self.nodes):
            self.log.info(
                f"Check node {i} has one inbound and one outbound manual peer"
            )
            assert_equal(len(inbound_peers(node)), 1)
            assert_equal(len(outbound_manual_peers(node)), 1)
            assert_equal(node.getconnectioncount(), 2)

        self.log.info("Hold a manual connection in a proxy handshake")
        self.stop_node(1)
        with socket.socket() as proxy:
            proxy.bind(("127.0.0.1", 0))
            proxy.listen()
            proxy.settimeout(10)
            self.restart_node(0, extra_args=[
                "-connect=0",
                f"-proxy=127.0.0.1:{proxy.getsockname()[1]}",
            ])
            node = self.nodes[0]
            self.check_pending_attempt(node, proxy, "example.invalid:18444", "example.invalid:18444")
            # A routable address uses the configured IPv4 proxy; documentation
            # addresses are classified as unroutable and would bypass it.
            self.check_pending_attempt(node, proxy, "8.8.8.8", "8.8.8.8:18444")


if __name__ == "__main__":
    DuplicateManualConnectionsTest(__file__).main()
