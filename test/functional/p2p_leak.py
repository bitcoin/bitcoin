#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test message sending before handshake completion.

Before receiving a VERACK, a node should not send anything but VERSION/VERACK
and feature negotiation messages (WTXIDRELAY, SENDADDRV2).

This test connects to a node and sends it a few messages, trying to entice it
into sending us something it shouldn't."""

import time

from test_framework.messages import (
    msg_getaddr,
    msg_ping,
    msg_version,
)
from test_framework.p2p import (
    P2PInterface,
    P2P_SUBVERSION,
    P2P_SERVICES,
    P2P_VERSION_RELAY,
)
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
)

PEER_TIMEOUT = 3


class LazyPeer(P2PInterface):
    def __init__(self):
        super().__init__()
        self.unexpected_msg = False
        self.ever_connected = False
        self.got_wtxidrelay = False
        self.got_sendaddrv2 = False

    def bad_message(self, message):
        self.unexpected_msg = True
        print("should not have received message: %s" % message.msgtype)

    def on_open(self):
        self.ever_connected = True

    # Does not respond to "version" with "verack"
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
    def on_wtxidrelay(self, message): self.got_wtxidrelay = True
    def on_sendaddrv2(self, message): self.got_sendaddrv2 = True


# Peer that sends a version but not a verack.
class NoVerackIdlePeer(LazyPeer):
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
        # Responds with an appropriate verack
        super().on_version(msg)
        self.version_received = msg


class P2PLeakTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f"-peertimeout={PEER_TIMEOUT}"]]

    def create_old_version(self, nversion):
        old_version_msg = msg_version()
        old_version_msg.nVersion = nversion
        old_version_msg.strSubVer = P2P_SUBVERSION
        old_version_msg.nServices = P2P_SERVICES
        old_version_msg.relay = P2P_VERSION_RELAY
        return old_version_msg

    def run_test(self):
        self.log.info('Check that the node doesn\'t send unexpected messages before handshake completion')
        # Peer that never sends a version, nor any other messages. It shouldn't receive anything from the node.
        no_version_idle_peer = self.nodes[0].add_p2p_connection(LazyPeer(), send_version=False, wait_for_verack=False)

        # Peer that sends a version but not a verack.
        no_verack_idle_peer = self.nodes[0].add_p2p_connection(NoVerackIdlePeer(), wait_for_verack=False)

        # Pre-wtxidRelay peer that sends a version but not a verack and does not support feature negotiation
        # messages which start at nVersion == 70016
        pre_wtxidrelay_peer = self.nodes[0].add_p2p_connection(NoVerackIdlePeer(), send_version=False, wait_for_verack=False)
        pre_wtxidrelay_peer.send_message(self.create_old_version(70015))

        # Wait until the peer gets the verack in response to the version. Though, don't wait for the node to receive the
        # verack, since the peer never sent one
        no_verack_idle_peer.wait_for_verack()
        pre_wtxidrelay_peer.wait_for_verack()

        no_version_idle_peer.wait_until(lambda: no_version_idle_peer.ever_connected)
        no_verack_idle_peer.wait_until(lambda: no_verack_idle_peer.version_received)
        pre_wtxidrelay_peer.wait_until(lambda: pre_wtxidrelay_peer.version_received)

        # Mine a block and make sure that it's not sent to the connected peers
        self.generate(self.nodes[0], nblocks=1)

        # Give the node enough time to possibly leak out a message
        time.sleep(PEER_TIMEOUT + 2)

        self.log.info("Connect peer to ensure the net thread runs the disconnect logic at least once")
        self.nodes[0].add_p2p_connection(P2PInterface())

        # Make sure only expected messages came in
        assert not no_version_idle_peer.unexpected_msg
        assert not no_version_idle_peer.got_wtxidrelay
        assert not no_version_idle_peer.got_sendaddrv2

        assert not no_verack_idle_peer.unexpected_msg
        assert no_verack_idle_peer.got_wtxidrelay
        assert no_verack_idle_peer.got_sendaddrv2

        assert not pre_wtxidrelay_peer.unexpected_msg
        assert not pre_wtxidrelay_peer.got_wtxidrelay
        assert not pre_wtxidrelay_peer.got_sendaddrv2

        # Expect peers to be disconnected due to timeout
        assert not no_version_idle_peer.is_connected
        assert not no_verack_idle_peer.is_connected
        assert not pre_wtxidrelay_peer.is_connected

        self.log.info('Check that the version message does not leak the local address of the node')
        p2p_version_store = self.nodes[0].add_p2p_connection(P2PVersionStore())
        ver = p2p_version_store.version_received
        # Check that received time is within one hour of now
        assert_greater_than_or_equal(ver.nTime, time.time() - 3600)
        assert_greater_than_or_equal(time.time() + 3600, ver.nTime)
        assert_equal(ver.addrFrom.port, 0)
        assert_equal(ver.addrFrom.ip, '0.0.0.0')
        assert_equal(ver.nStartingHeight, 201)
        assert_equal(ver.relay, 1)

        self.log.info('Check that old peers are disconnected')
        p2p_old_peer = self.nodes[0].add_p2p_connection(P2PInterface(), send_version=False, wait_for_verack=False)
        with self.nodes[0].assert_debug_log(["using obsolete version 31799; disconnecting"]):
            p2p_old_peer.send_message(self.create_old_version(31799))
            p2p_old_peer.wait_for_disconnect()


if __name__ == '__main__':
    P2PLeakTest().main()
