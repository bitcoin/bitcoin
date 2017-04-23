#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC calls related to net.

Tests correspond to code in rpc/net.cpp.
"""

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_jsonrpc,
    connect_nodes_bi,
    p2p_port,
    start_nodes,
)


class NetTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self):
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir)
        connect_nodes_bi(self.nodes, 0, 1)
        self.is_network_split = False
        self.sync_all()

    def run_test(self):
        assert_equal(self.nodes[0].getnetworkinfo()['networkactive'], True)
        assert_equal(self.nodes[0].getnetworkinfo()['connections'], 2)

        self.nodes[0].setnetworkactive(False)
        assert_equal(self.nodes[0].getnetworkinfo()['networkactive'], False)
        timeout = 3
        while self.nodes[0].getnetworkinfo()['connections'] != 0:
            # Wait a bit for all sockets to close
            assert timeout > 0, 'not all connections closed in time'
            timeout -= 0.1
            time.sleep(0.1)

        self.nodes[0].setnetworkactive(True)
        connect_nodes_bi(self.nodes, 0, 1)
        assert_equal(self.nodes[0].getnetworkinfo()['networkactive'], True)
        assert_equal(self.nodes[0].getnetworkinfo()['connections'], 2)

        # test getaddednodeinfo
        assert_equal(self.nodes[0].getaddednodeinfo(), [])
        # add a node (node2) to node0
        ip_port = "127.0.0.1:{}".format(p2p_port(2))
        self.nodes[0].addnode(ip_port, 'add')
        # check that the node has indeed been added
        added_nodes = self.nodes[0].getaddednodeinfo(ip_port)
        assert_equal(len(added_nodes), 1)
        assert_equal(added_nodes[0]['addednode'], ip_port)
        # check that a non-existant node returns an error
        assert_raises_jsonrpc(-24, "Node has not been added",
                              self.nodes[0].getaddednodeinfo, '1.1.1.1')


if __name__ == '__main__':
    NetTest().main()
