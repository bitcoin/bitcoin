#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC calls related to net.

Tests correspond to code in rpc/net.cpp.
"""

from decimal import Decimal
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.authproxy import JSONRPCException
from test_framework.util import (
    assert_equal,
    start_nodes,
    connect_nodes_bi,
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
        assert_equal(self.nodes[0].getnetworkinfo()['connections'], 2) # bilateral connection

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


if __name__ == '__main__':
    NetTest().main()
