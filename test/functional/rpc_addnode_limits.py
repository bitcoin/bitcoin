#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that persistent addnode entries obey the manual connection limit."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    ensure_for,
    p2p_port,
)


MAX_ADDNODE_CONNECTIONS = 8
ADDED_NODE_COUNT = MAX_ADDNODE_CONNECTIONS + 1


class AddnodeLimitsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = ADDED_NODE_COUNT + 1
        self.setup_clean_chain = True
        self.supports_cli = False

    def setup_network(self):
        self.setup_nodes()

    def manual_connection_count(self):
        return sum(
            peer["connection_type"] == "manual"
            for peer in self.nodes[0].getpeerinfo()
        )

    def connected_added_node_count(self):
        return sum(
            added_node["connected"]
            for added_node in self.nodes[0].getaddednodeinfo()
        )

    def run_test(self):
        node = self.nodes[0]
        added_node_addresses = [
            f"127.0.0.1:{p2p_port(i)}"
            for i in range(1, self.num_nodes)
        ]

        self.log.info("Add one more node than the addnode connection limit")
        for address in added_node_addresses:
            node.addnode(node=address, command="add")

        self.log.info("Check all addnode entries are tracked")
        added_nodes = node.getaddednodeinfo()
        assert_equal(len(added_nodes), ADDED_NODE_COUNT)
        assert_equal(
            sorted(added_node["addednode"] for added_node in added_nodes),
            sorted(added_node_addresses),
        )

        self.log.info("Check addnode connections are capped")
        self.wait_until(
            lambda: self.manual_connection_count() == MAX_ADDNODE_CONNECTIONS
        )
        assert_equal(
            node.getnetworkinfo()["connections_out"],
            MAX_ADDNODE_CONNECTIONS,
        )
        assert_equal(self.connected_added_node_count(), MAX_ADDNODE_CONNECTIONS)

        ensure_for(
            duration=2,
            f=lambda: self.manual_connection_count() <= MAX_ADDNODE_CONNECTIONS,
        )


if __name__ == "__main__":
    AddnodeLimitsTest(__file__).main()
