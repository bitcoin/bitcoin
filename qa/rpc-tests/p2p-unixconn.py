#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test P2P connectivity over UNIX socket."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.uhttpconnection import have_af_unix

def unix_connect_nodes(dirname, from_connection, node_num):
    sock_path = ":unix:" + os.path.join(dirname, "node"+str(node_num), 'regtest', 'p2p_sock')
    from_connection.addnode(sock_path, "onetry")
    # poll until version handshake complete to avoid race conditions
    # with transaction relaying
    while any(peer['version'] == 0 for peer in from_connection.getpeerinfo()):
        time.sleep(0.1)

def unix_connect_nodes_bi(dirname, nodes, a, b):
    unix_connect_nodes(dirname, nodes[a], b)
    unix_connect_nodes(dirname, nodes[b], a)

class UnixP2PTest (BitcoinTestFramework):
    '''Test P2P over UNIX socket'''

    def check_fee_amount(self, curr_balance, balance_with_fee, fee_per_byte, tx_size):
        """Return curr_balance after asserting the fee was in range"""
        fee = balance_with_fee - curr_balance
        assert_fee_amount(fee, tx_size, fee_per_byte * 1000)
        return curr_balance

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = False
        self.num_nodes = 3
        # tell nodes to bind P2P on UNIX socket
        self.extra_args = [['-bind=:unix:p2p_sock'] for i in range(3)]

    def setup_network(self, split=False):
        self.nodes = start_nodes(3, self.options.tmpdir, self.extra_args[:3])
        unix_connect_nodes_bi(self.options.tmpdir, self.nodes,0,1)
        unix_connect_nodes_bi(self.options.tmpdir, self.nodes,0,2)
        unix_connect_nodes_bi(self.options.tmpdir, self.nodes,1,2)
        self.is_network_split=False
        self.sync_all()

    def run_test (self):
        assert_equal(self.nodes[0].getbalance(), 1250)
        assert_equal(self.nodes[1].getbalance(), 1250)
        assert_equal(self.nodes[2].getbalance(), 1250)

        # send 42 BTC to node 2 from node 0 and 1
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 11)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 10)
        self.nodes[0].generate(1)
        self.sync_all()

        self.nodes[1].sendtoaddress(self.nodes[2].getnewaddress(), 11)
        self.nodes[1].sendtoaddress(self.nodes[2].getnewaddress(), 10)
        self.nodes[1].generate(1)
        self.sync_all()

        assert_equal(self.nodes[2].getbalance(), 1250 + 42)


if __name__ == '__main__':
    if have_af_unix:
        UnixP2PTest().main()
