#!/usr/bin/env python3
# Copyright (c) 2017-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPC calls related to net.

Tests correspond to code in rpc/net.cpp.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    connect_nodes_bi,
    p2p_port,
    wait_until,
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
        # getnettotals totalbytesrecv and totalbytessent should be
        # consistent with getpeerinfo. Since the RPC calls are not atomic,
        # and messages might have been recvd or sent between RPC calls, call
        # getnettotals before and after and verify that the returned values
        # from getpeerinfo are bounded by those values.
        net_totals_before = self.nodes[0].getnettotals()
        peer_info = self.nodes[0].getpeerinfo()
        net_totals_after = self.nodes[0].getnettotals()
        assert_equal(len(peer_info), 2)
        peers_recv = sum([peer['bytesrecv'] for peer in peer_info])
        peers_sent = sum([peer['bytessent'] for peer in peer_info])

        assert_greater_than_or_equal(peers_recv, net_totals_before['totalbytesrecv'])
        assert_greater_than_or_equal(net_totals_after['totalbytesrecv'], peers_recv)
        assert_greater_than_or_equal(peers_sent, net_totals_before['totalbytessent'])
        assert_greater_than_or_equal(net_totals_after['totalbytessent'], peers_sent)

        # test getnettotals and getpeerinfo by doing a ping
        # the bytes sent/received should change
        # note ping and pong are 32 bytes each
        self.nodes[0].ping()
        wait_until(lambda: (self.nodes[0].getnettotals()['totalbytessent'] >= net_totals_after['totalbytessent'] + 32 * 2), timeout=1)
        wait_until(lambda: (self.nodes[0].getnettotals()['totalbytesrecv'] >= net_totals_after['totalbytesrecv'] + 32 * 2), timeout=1)

        peer_info_after_ping = self.nodes[0].getpeerinfo()
        for before, after in zip(peer_info, peer_info_after_ping):
            assert_greater_than_or_equal(after['bytesrecv_per_msg']['pong'], before['bytesrecv_per_msg']['pong'] + 32)
            assert_greater_than_or_equal(after['bytessent_per_msg']['ping'], before['bytessent_per_msg']['ping'] + 32)

    def _test_getnetworkinginfo(self):
        assert_equal(self.nodes[0].getnetworkinfo()['networkactive'], True)
        assert_equal(self.nodes[0].getnetworkinfo()['connections'], 2)

        self.nodes[0].setnetworkactive(False)
        assert_equal(self.nodes[0].getnetworkinfo()['networkactive'], False)
        # Wait a bit for all sockets to close
        wait_until(lambda: self.nodes[0].getnetworkinfo()['connections'] == 0, timeout=3)

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
        assert_raises_rpc_error(-24, "Node has not been added", self.nodes[0].getaddednodeinfo, '1.1.1.1')

    def _test_getpeerinfo(self):
        peer_info = [x.getpeerinfo() for x in self.nodes]
        # check both sides of bidirectional connection between nodes
        # the address bound to on one side will be the source address for the other node
        assert_equal(peer_info[0][0]['addrbind'], peer_info[1][0]['addr'])
        assert_equal(peer_info[1][0]['addrbind'], peer_info[0][0]['addr'])

if __name__ == '__main__':
    NetTest().main()
