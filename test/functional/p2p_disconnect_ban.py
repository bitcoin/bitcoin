#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node disconnect and ban behavior"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

class DisconnectBanTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.supports_cli = False

    def setup_network(self):
        self.setup_nodes()
        self.log.info("Connect nodes both way")
        self.connect_nodes(0, 1)
        self.connect_nodes(1, 0)

    def run_test(self):
        self.log.info("Test disconnectnode RPCs")

        self.log.info("disconnectnode: fail to disconnect when calling with address and nodeid")
        address1 = self.nodes[0].getpeerinfo()[0]['addr']
        node1 = self.nodes[0].getpeerinfo()[0]["id"]
        assert_raises_rpc_error(-32602, "Only one of address and nodeid should be provided.", self.nodes[0].disconnectnode, address=address1, nodeid=node1)

        self.log.info("disconnectnode: fail to disconnect when calling with junk address")
        assert_raises_rpc_error(-29, "Node not found in connected nodes", self.nodes[0].disconnectnode, address="221B Baker Street")

        self.log.info("disconnectnode: successfully disconnect node by address")
        address1 = self.nodes[0].getpeerinfo()[0]['addr']
        self.nodes[0].disconnectnode(address=address1)
        self.wait_until(lambda: len(self.nodes[1].getpeerinfo()) == 1, timeout=10)
        assert not [node for node in self.nodes[0].getpeerinfo() if node['addr'] == address1]

        self.log.info("disconnectnode: successfully reconnect node")
        self.connect_nodes(0, 1)  # reconnect the node
        assert_equal(len(self.nodes[0].getpeerinfo()), 2)
        assert [node for node in self.nodes[0].getpeerinfo() if node['addr'] == address1]

        self.log.info("disconnectnode: successfully disconnect node by node id")
        id1 = self.nodes[0].getpeerinfo()[0]['id']
        self.nodes[0].disconnectnode(nodeid=id1)
        self.wait_until(lambda: len(self.nodes[1].getpeerinfo()) == 1, timeout=10)
        assert not [node for node in self.nodes[0].getpeerinfo() if node['id'] == id1]

if __name__ == '__main__':
    DisconnectBanTest().main()
