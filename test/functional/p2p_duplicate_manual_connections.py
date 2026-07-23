#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that -connect and -addnode do not open duplicate manual connections."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    p2p_port,
)

PENDING_MANUAL_CONNECTION_LOG = "connection already in progress"
CONNECTED_MANUAL_CONNECTION_LOG = "Skipping manual connection"


def outbound_manual_peers(node):
    return [
        peer
        for peer in node.getpeerinfo()
        if not peer["inbound"] and peer["connection_type"] == "manual"
    ]


def inbound_peers(node):
    return [peer for peer in node.getpeerinfo() if peer["inbound"]]


def pending_manual_connection_logged(node):
    debug_log = node.debug_log_path.read_text(
        encoding="utf-8",
        errors="replace",
    )
    return PENDING_MANUAL_CONNECTION_LOG in debug_log


def connected_manual_connection_logged(node):
    debug_log = node.debug_log_path.read_text(
        encoding="utf-8",
        errors="replace",
    )
    return CONNECTED_MANUAL_CONNECTION_LOG in debug_log


def duplicate_manual_connection_observed(node):
    return (
        len(inbound_peers(node)) > 1
        or len(outbound_manual_peers(node)) > 1
        or node.getconnectioncount() > 2
    )


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

        self.log.info("Wait for the duplicate attempt to be dropped or observed")
        self.wait_until(
            lambda: any(
                pending_manual_connection_logged(node)
                or connected_manual_connection_logged(node)
                or duplicate_manual_connection_observed(node)
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


if __name__ == "__main__":
    DuplicateManualConnectionsTest(__file__).main()
