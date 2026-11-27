#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test P2P handling of TCP urgent data and resets while sending."""

import math
import socket
import time

from test_framework.messages import (
    CInv,
    MAGIC_BYTES,
    MSG_BLOCK,
    MSG_WITNESS_FLAG,
    msg_getdata,
    msg_ping,
    msg_version,
)
from test_framework.p2p import NetworkThread, P2PInterface, P2P_VERSION
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than, mine_large_block, p2p_port
from test_framework.wallet import MiniWallet

# Amount of block data the peer requests, in bytes. Needs to be more than the
# kernel is willing to buffer for a socket that nobody reads from, so that the
# node cannot send all of it. There is no portable way to obtain that limit,
# so check_reset_while_sending() asserts that the node sent less than this.
REQUESTED_BLOCK_BYTES = 16_000_000
# Size of a v1 P2P message header: magic, message type, length and checksum.
MESSAGE_HEADER_SIZE = 4 + 12 + 4 + 4


class SocketErrorsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-maxreceivebuffer=1"]]

    def run_test(self):
        self.message_builder = P2PInterface()
        self.message_builder.magic_bytes = MAGIC_BYTES[self.chain]
        self.version = msg_version()
        self.version.nVersion = P2P_VERSION
        self.check_urgent_bytes()
        self.check_reset_while_sending()

    def check_probe(self, send_probe, expected_bytes):
        node = self.nodes[0]
        with socket.create_connection(("127.0.0.1", p2p_port(0)), timeout=10) as peer:
            self.wait_until(lambda: len(node.getpeerinfo()) == 1)
            send_probe(peer)
            # The node ignores the urgent byte: the connection stays open,
            # the ordinary version message is processed and the urgent byte
            # is not counted as received.
            def received_version():
                peers = node.getpeerinfo()
                assert_equal(len(peers), 1)
                return peers[0]["version"] == self.version.nVersion

            self.wait_until(received_version)
            assert_equal(node.getpeerinfo()[0]["bytesrecv"], expected_bytes)
        self.wait_until(lambda: len(node.getpeerinfo()) == 0)

    def check_urgent_bytes(self):
        self.log.info("Check handling of individual urgent bytes")
        version = self.message_builder.build_message(self.version)
        # Use a fresh connection for each byte so earlier urgent indications
        # cannot be overwritten before they are observed by the node.
        for byte in b"URG":
            def send_probe(peer):
                peer.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                assert_equal(peer.send(bytes([byte]), socket.MSG_OOB), 1)
                peer.sendall(version)

            self.check_probe(send_probe, len(version))

    def check_reset_while_sending(self):
        self.log.info("Check reset while the node has unsent block data")
        # A peer requests many large blocks from the node, but stalls receiving
        # them. At the same time it sends pings that exceed the node's
        # -maxreceivebuffer (1 kB), causing the node to pause receiving. Then
        # it resets the connection. The node must disconnect without reading
        # any more from the peer.
        node = self.nodes[0]
        wallet = MiniWallet(node)
        block_hash, block_size = mine_large_block(self, wallet, node)
        block_msg_size = block_size + MESSAGE_HEADER_SIZE
        peer = node.add_p2p_connection(P2PInterface(), supports_v2_p2p=False)
        peer.sync_with_ping()
        NetworkThread.network_event_loop.call_soon_threadsafe(peer._transport.pause_reading)
        requests = math.ceil(REQUESTED_BLOCK_BYTES / block_msg_size)
        peer.send_without_ping(msg_getdata([CInv(MSG_BLOCK | MSG_WITNESS_FLAG, int(block_hash, 16))] * requests))
        sent_bytes = self.wait_for_send_stall(node)
        # We expect the kernel to not accept all bytes from the node.
        assert_greater_than(requests * block_msg_size, sent_bytes)
        # Exceed -maxreceivebuffer=1 (1000 bytes) while processing is blocked
        # by the outgoing data.
        ping = msg_ping(nonce=123)
        num_pings = 1000 // len(ping.serialize()) + 1
        pings = peer.build_message(ping) * num_pings
        expected_received = node.getnettotals()["totalbytesrecv"] + len(pings)
        peer.send_raw_message(pings)
        # The node reads up to 64 kB at a time and only then checks the limit,
        # so it reads the whole batch before it pauses receiving.
        self.wait_until(lambda: node.getnettotals()["totalbytesrecv"] == expected_received)
        # Send more pings while receiving is paused. Give them time to reach the
        # node's kernel, as the reset discards anything not yet delivered.
        peer.send_raw_message(pings)
        time.sleep(1 * self.options.timeout_factor)
        # Linux is the only platform where the node uses poll(), unless it is
        # a Windows binary running under Wine.
        uses_poll = self.linux_host and not self.windows_target
        expected_msgs = ["socket error or hangup"] if uses_poll else []
        with node.assert_debug_log(expected_msgs):
            peer.peer_disconnect(reset=True)
            self.wait_until(lambda: len(node.getpeerinfo()) == 0, timeout=10)
        # The node disconnected without receiving the additional pings.
        assert_equal(node.getnettotals()["totalbytesrecv"], expected_received)
        node.disconnect_p2ps()


if __name__ == '__main__':
    SocketErrorsTest(__file__).main()
