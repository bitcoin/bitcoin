#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test node handling
#

from test_framework.mininode import wait_until
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (assert_equal,
                                 assert_raises_jsonrpc,
                                 connect_nodes_bi,
                                 start_node,
                                 stop_node,
                                 )

class NodeHandlingTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = False

    def setup_network(self):
        self.nodes = self.setup_nodes()
        connect_nodes_bi(self.nodes, 0, 1)

    def run_test(self):
        ###########################
        # setban/listbanned tests #
        ###########################
        assert_equal(len(self.nodes[1].getpeerinfo()), 2)  # node1 should have 2 connections to node0 at this point
        self.nodes[1].setban("127.0.0.1", "add")
        assert wait_until(lambda: len(self.nodes[1].getpeerinfo()) == 0, timeout=10)
        assert_equal(len(self.nodes[1].getpeerinfo()), 0)  # all nodes must be disconnected at this point
        assert_equal(len(self.nodes[1].listbanned()), 1)
        self.nodes[1].clearbanned()
        assert_equal(len(self.nodes[1].listbanned()), 0)
        self.nodes[1].setban("127.0.0.0/24", "add")
        assert_equal(len(self.nodes[1].listbanned()), 1)
        # This will throw an exception because 127.0.0.1 is within range 127.0.0.0/24
        assert_raises_jsonrpc(-23, "IP/Subnet already banned", self.nodes[1].setban, "127.0.0.1", "add")
        # This will throw an exception because 127.0.0.1/42 is not a real subnet
        assert_raises_jsonrpc(-30, "Error: Invalid IP/Subnet", self.nodes[1].setban, "127.0.0.1/42", "add")
        assert_equal(len(self.nodes[1].listbanned()), 1)  # still only one banned ip because 127.0.0.1 is within the range of 127.0.0.0/24
        # This will throw an exception because 127.0.0.1 was not added above
        assert_raises_jsonrpc(-30, "Error: Unban failed", self.nodes[1].setban, "127.0.0.1", "remove")
        assert_equal(len(self.nodes[1].listbanned()), 1)
        self.nodes[1].setban("127.0.0.0/24", "remove")
        assert_equal(len(self.nodes[1].listbanned()), 0)
        self.nodes[1].clearbanned()
        assert_equal(len(self.nodes[1].listbanned()), 0)

        # test persisted banlist
        self.nodes[1].setban("127.0.0.0/32", "add")
        self.nodes[1].setban("127.0.0.0/24", "add")
        self.nodes[1].setban("192.168.0.1", "add", 1)  # ban for 1 seconds
        self.nodes[1].setban("2001:4d48:ac57:400:cacf:e9ff:fe1d:9c63/19", "add", 1000)  # ban for 1000 seconds
        listBeforeShutdown = self.nodes[1].listbanned()
        assert_equal("192.168.0.1/32", listBeforeShutdown[2]['address'])
        assert wait_until(lambda: len(self.nodes[1].listbanned()) == 3, timeout=10)

        stop_node(self.nodes[1], 1)

        self.nodes[1] = start_node(1, self.options.tmpdir)
        listAfterShutdown = self.nodes[1].listbanned()
        assert_equal("127.0.0.0/24", listAfterShutdown[0]['address'])
        assert_equal("127.0.0.0/32", listAfterShutdown[1]['address'])
        assert_equal("/19" in listAfterShutdown[2]['address'], True)

        # Clear ban lists
        self.nodes[1].clearbanned()
        connect_nodes_bi(self.nodes, 0, 1)

        ###########################
        # RPC disconnectnode test #
        ###########################
        address1 = self.nodes[0].getpeerinfo()[0]['addr']
        self.nodes[0].disconnectnode(address=address1)
        assert wait_until(lambda: len(self.nodes[0].getpeerinfo()) == 1, timeout=10)
        assert not [node for node in self.nodes[0].getpeerinfo() if node['addr'] == address1]

        connect_nodes_bi(self.nodes, 0, 1)  # reconnect the node
        assert [node for node in self.nodes[0].getpeerinfo() if node['addr'] == address1]

if __name__ == '__main__':
    NodeHandlingTest().main()
