#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC calls related to net.

Tests correspond to code in rpc/net.cpp.
"""

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    connect_nodes_bi,
    p2p_port,
)

class NetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def run_test(self):
        self._test_connection_count()
        self._test_getnettotals()
        self._test_getnetworkinginfo()
        self._test_getaddednodeinfo()
        self._test_getpeerinfo()

    def _test_connection_count(self):
        # connect_nodes_bi connects each node to the other
        assert_equal(self.nodes[0].getconnectioncount(), 2)

    def _test_getnettotals(self):
        # check that getnettotals totalbytesrecv and totalbytessent
        # are consistent with getpeerinfo
        peer_info = self.nodes[0].getpeerinfo()
        assert_equal(len(peer_info), 2)
        net_totals = self.nodes[0].getnettotals()
        assert_equal(sum([peer['bytesrecv'] for peer in peer_info]),
                     net_totals['totalbytesrecv'])
        assert_equal(sum([peer['bytessent'] for peer in peer_info]),
                     net_totals['totalbytessent'])
        # test getnettotals and getpeerinfo by doing a ping
        # the bytes sent/received should change
        # note ping and pong are 32 bytes each
        self.nodes[0].ping()
        time.sleep(0.1)
        peer_info_after_ping = self.nodes[0].getpeerinfo()
        net_totals_after_ping = self.nodes[0].getnettotals()
        for before, after in zip(peer_info, peer_info_after_ping):
            assert_equal(before['bytesrecv_per_msg']['pong'] + 32, after['bytesrecv_per_msg']['pong'])
            assert_equal(before['bytessent_per_msg']['ping'] + 32, after['bytessent_per_msg']['ping'])
        assert_equal(net_totals['totalbytesrecv'] + 32*2, net_totals_after_ping['totalbytesrecv'])
        assert_equal(net_totals['totalbytessent'] + 32*2, net_totals_after_ping['totalbytessent'])

    def _test_getnetworkinginfo(self):
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

    def _test_getaddednodeinfo(self):
        assert_equal(self.nodes[0].getaddednodeinfo(), [])
        # add a node (node2) to node0
        ip_port = "127.0.0.1:{}".format(p2p_port(2))
        self.nodes[0].addnode(ip_port, 'add')
        # check that the node has indeed been added
        added_nodes = self.nodes[0].getaddednodeinfo(ip_port)
        assert_equal(len(added_nodes), 1)
        assert_equal(added_nodes[0]['addednode'], ip_port)
        # check that a non-existent node returns an error
        assert_raises_rpc_error(-24, "Node has not been added",
                              self.nodes[0].getaddednodeinfo, '1.1.1.1')

    def _test_getpeerinfo(self):
        peer_info = [x.getpeerinfo() for x in self.nodes]
        # check both sides of bidirectional connection between nodes
        # the address bound to on one side will be the source address for the other node
        assert_equal(peer_info[0][0]['addrbind'], peer_info[1][0]['addr'])
        assert_equal(peer_info[1][0]['addrbind'], peer_info[0][0]['addr'])

if __name__ == '__main__':
    NetTest().main()
