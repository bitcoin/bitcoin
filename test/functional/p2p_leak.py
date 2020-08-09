#!/usr/bin/env python3
# Copyright (c) 2017-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test message sending before handshake completion.

A node should never send anything other than VERSION/VERACK until it's
received a VERACK.

This test connects to a node and sends it a few messages, trying to entice it
into sending us something it shouldn't."""

import time

from test_framework.messages import (
    msg_getaddr,
    msg_ping,
    msg_verack,
    msg_version,
)
from test_framework.mininode import mininode_lock, P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    wait_until,
)

DISCOURAGEMENT_THRESHOLD = 100


class CLazyNode(P2PInterface):
    def __init__(self):
        super().__init__()
        self.unexpected_msg = False
        self.ever_connected = False

    def bad_message(self, message):
        self.unexpected_msg = True
        self.log.info("should not have received message: %s" % message.msgtype)

    def on_open(self):
        self.ever_connected = True

    def on_version(self, message): self.bad_message(message)
    def on_verack(self, message): self.bad_message(message)
    def on_inv(self, message): self.bad_message(message)
    def on_addr(self, message): self.bad_message(message)
    def on_getdata(self, message): self.bad_message(message)
    def on_getblocks(self, message): self.bad_message(message)
    def on_tx(self, message): self.bad_message(message)
    def on_block(self, message): self.bad_message(message)
    def on_getaddr(self, message): self.bad_message(message)
    def on_headers(self, message): self.bad_message(message)
    def on_getheaders(self, message): self.bad_message(message)
    def on_ping(self, message): self.bad_message(message)
    def on_mempool(self, message): self.bad_message(message)
    def on_pong(self, message): self.bad_message(message)
    def on_feefilter(self, message): self.bad_message(message)
    def on_sendheaders(self, message): self.bad_message(message)
    def on_sendcmpct(self, message): self.bad_message(message)
    def on_cmpctblock(self, message): self.bad_message(message)
    def on_getblocktxn(self, message): self.bad_message(message)
    def on_blocktxn(self, message): self.bad_message(message)


# Node that never sends a version. We'll use this to send a bunch of messages
# anyway, and eventually get disconnected.
class CNodeNoVersionMisbehavior(CLazyNode):
    pass


# Node that never sends a version. This one just sits idle and hopes to receive
# any message (it shouldn't!)
class CNodeNoVersionIdle(CLazyNode):
    def __init__(self):
        super().__init__()


# Node that sends a version but not a verack.
class CNodeNoVerackIdle(CLazyNode):
    def __init__(self):
        self.version_received = False
        super().__init__()

    def on_verack(self, message): pass
    # When version is received, don't reply with a verack. Instead, see if the
    # node will give us a message that it shouldn't. This is not an exhaustive
    # list!
    def on_version(self, message):
        self.version_received = True
        self.send_message(msg_ping())
        self.send_message(msg_getaddr())


class P2PVersionStore(P2PInterface):
    version_received = None

    def on_version(self, msg):
        super().on_version(msg)
        self.version_received = msg


class P2PLeakTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        no_version_disconnect_node = self.nodes[0].add_p2p_connection(
            CNodeNoVersionMisbehavior(), send_version=False, wait_for_verack=False)
        no_version_idlenode = self.nodes[0].add_p2p_connection(CNodeNoVersionIdle(), send_version=False, wait_for_verack=False)
        no_verack_idlenode = self.nodes[0].add_p2p_connection(CNodeNoVerackIdle(), wait_for_verack=False)

        # Send enough veracks without a message to reach the peer discouragement
        # threshold. This should get us disconnected.
        for _ in range(DISCOURAGEMENT_THRESHOLD):
            no_version_disconnect_node.send_message(msg_verack())

        # Wait until we got the verack in response to the version. Though, don't wait for the other node to receive the
        # verack, since we never sent one
        no_verack_idlenode.wait_for_verack()

        wait_until(lambda: no_version_disconnect_node.ever_connected, timeout=10, lock=mininode_lock)
        wait_until(lambda: no_version_idlenode.ever_connected, timeout=10, lock=mininode_lock)
        wait_until(lambda: no_verack_idlenode.version_received, timeout=10, lock=mininode_lock)

        # Mine a block and make sure that it's not sent to the connected nodes
        self.nodes[0].generatetoaddress(1, self.nodes[0].get_deterministic_priv_key().address)

        #Give the node enough time to possibly leak out a message
        time.sleep(5)

        # Expect this node to be disconnected for misbehavior
        assert not no_version_disconnect_node.is_connected

        self.nodes[0].disconnect_p2ps()

        # Make sure no unexpected messages came in
        assert no_version_disconnect_node.unexpected_msg == False
        assert no_version_idlenode.unexpected_msg == False
        assert no_verack_idlenode.unexpected_msg == False

        self.log.info('Check that the version message does not leak the local address of the node')
        p2p_version_store = self.nodes[0].add_p2p_connection(P2PVersionStore())
        ver = p2p_version_store.version_received
        # Check that received time is within one hour of now
        assert_greater_than_or_equal(ver.nTime, time.time() - 3600)
        assert_greater_than_or_equal(time.time() + 3600, ver.nTime)
        assert_equal(ver.addrFrom.port, 0)
        assert_equal(ver.addrFrom.ip, '0.0.0.0')
        assert_equal(ver.nStartingHeight, 201)
        assert_equal(ver.nRelay, 1)

        self.log.info('Check that old nodes are disconnected')
        p2p_old_node = self.nodes[0].add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False)
        old_version_msg = msg_version()
        old_version_msg.nVersion = 31799
        with self.nodes[0].assert_debug_log(['peer=4 using obsolete version 31799; disconnecting']):
            p2p_old_node.send_message(old_version_msg)
            p2p_old_node.wait_for_disconnect()


if __name__ == '__main__':
    P2PLeakTest().main()
