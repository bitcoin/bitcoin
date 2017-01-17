#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test p2p mempool message.

Test that nodes are disconnected if they send mempool messages when bloom
filters are not enabled.
"""

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class TestNode(NodeConnCB):
    def __init__(self):
        NodeConnCB.__init__(self)
        self.connection = None
        self.ping_counter = 1
        self.last_pong = msg_pong()
        self.block_receive_map = {}

    def add_connection(self, conn):
        self.connection = conn
        self.peer_disconnected = False

    def on_inv(self, conn, message):
        pass

    # Track the last getdata message we receive (used in the test)
    def on_getdata(self, conn, message):
        self.last_getdata = message

    def on_block(self, conn, message):
        message.block.calc_sha256()
        try:
            self.block_receive_map[message.block.sha256] += 1
        except KeyError as e:
            self.block_receive_map[message.block.sha256] = 1

    # Spin until verack message is received from the node.
    # We use this to signal that our test can begin. This
    # is called from the testing thread, so it needs to acquire
    # the global lock.
    def wait_for_verack(self):
        def veracked():
            return self.verack_received
        return wait_until(veracked, timeout=10)

    def wait_for_disconnect(self):
        def disconnected():
            return self.peer_disconnected
        return wait_until(disconnected, timeout=10)

    # Wrapper for the NodeConn's send_message function
    def send_message(self, message):
        self.connection.send_message(message)

    def on_pong(self, conn, message):
        self.last_pong = message

    def on_close(self, conn):
        self.peer_disconnected = True

    # Sync up with the node after delivery of a block
    def sync_with_ping(self, timeout=30):
        def received_pong():
            return (self.last_pong.nonce == self.ping_counter)
        self.connection.send_message(msg_ping(nonce=self.ping_counter))
        success = wait_until(received_pong, timeout=timeout)
        self.ping_counter += 1
        return success

    def send_mempool(self):
        self.lastInv = []
        self.send_message(msg_mempool())

class P2PMempoolTests(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 2

    def setup_network(self):
        # Start a node with maxuploadtarget of 200 MB (/24h)
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-peerbloomfilters=0"]))

    def run_test(self):
        #connect a mininode
        aTestNode = TestNode()
        node = NodeConn('127.0.0.1', p2p_port(0), self.nodes[0], aTestNode)
        aTestNode.add_connection(node)
        NetworkThread().start()
        aTestNode.wait_for_verack()

        #request mempool
        aTestNode.send_mempool()
        aTestNode.wait_for_disconnect()

        #mininode must be disconnected at this point
        assert_equal(len(self.nodes[0].getpeerinfo()), 0)
    
if __name__ == '__main__':
    P2PMempoolTests().main()
