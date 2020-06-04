#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid network messages."""
import asyncio

from test_framework.messages import (
    CBlockHeader,
    CInv,
    msg_getdata,
    msg_headers,
    msg_inv,
    msg_ping,
    MSG_TX,
    ser_string,
)
from test_framework.mininode import (
    NetworkThread,
    P2PDataStore,
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework

MSG_LIMIT = 4 * 1000 * 1000  # 4MB, per MAX_PROTOCOL_MESSAGE_LENGTH
VALID_DATA_LIMIT = MSG_LIMIT - 5  # Account for the 5-byte length prefix

class msg_unrecognized:
    """Nonsensical message. Modeled after similar types in test_framework.messages."""

    msgtype = b'badmsg'

    def __init__(self, *, str_data):
        self.str_data = str_data.encode() if not isinstance(str_data, bytes) else str_data

    def serialize(self):
        return ser_string(self.str_data)

    def __repr__(self):
        return "{}(data={})".format(self.msgtype, self.str_data)


class InvalidMessagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.test_magic_bytes()
        self.test_checksum()
        self.test_size()
        self.test_msgtype()
        self.test_large_inv()
        self.test_resource_exhaustion()

    def test_magic_bytes(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())

        async def swap_magic_bytes():
            conn._on_data = lambda: None  # Need to ignore all incoming messages from now, since they come with "invalid" magic bytes
            conn.magic_bytes = b'\x00\x11\x22\x32'

        # Call .result() to block until the atomic swap is complete, otherwise
        # we might run into races later on
        asyncio.run_coroutine_threadsafe(swap_magic_bytes(), NetworkThread.network_event_loop).result()

        with self.nodes[0].assert_debug_log(['PROCESSMESSAGE: INVALID MESSAGESTART ping']):
            conn.send_message(msg_ping(nonce=0xff))
            conn.wait_for_disconnect(timeout=1)
            self.nodes[0].disconnect_p2ps()

    def test_checksum(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['CHECKSUM ERROR (badmsg, 2 bytes), expected 78df0a04 was ffffffff']):
            msg = conn.build_message(msg_unrecognized(str_data="d"))
            cut_len = (
                4 +  # magic
                12 +  # msgtype
                4  #len
            )
            # modify checksum
            msg = msg[:cut_len] + b'\xff' * 4 + msg[cut_len + 4:]
            self.nodes[0].p2p.send_raw_message(msg)
            conn.sync_with_ping(timeout=1)
            self.nodes[0].disconnect_p2ps()

    def test_size(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['']):
            # Create a message with oversized payload
            msg = msg_unrecognized(str_data="d"*(VALID_DATA_LIMIT + 1))
            msg = conn.build_message(msg)
            self.nodes[0].p2p.send_raw_message(msg)
            conn.wait_for_disconnect(timeout=1)
            self.nodes[0].disconnect_p2ps()

    def test_msgtype(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['PROCESSMESSAGE: ERRORS IN HEADER']):
            msg = msg_unrecognized(str_data="d")
            msg.msgtype = b'\xff' * 12
            msg = conn.build_message(msg)
            # Modify msgtype
            msg = msg[:7] + b'\x00' + msg[7 + 1:]
            self.nodes[0].p2p.send_raw_message(msg)
            conn.sync_with_ping(timeout=1)
            self.nodes[0].disconnect_p2ps()

    def test_large_inv(self):
        conn = self.nodes[0].add_p2p_connection(P2PInterface())
        with self.nodes[0].assert_debug_log(['Misbehaving', 'peer=4 (0 -> 20): message inv size() = 50001']):
            msg = msg_inv([CInv(MSG_TX, 1)] * 50001)
            conn.send_and_ping(msg)
        with self.nodes[0].assert_debug_log(['Misbehaving', 'peer=4 (20 -> 40): message getdata size() = 50001']):
            msg = msg_getdata([CInv(MSG_TX, 1)] * 50001)
            conn.send_and_ping(msg)
        with self.nodes[0].assert_debug_log(['Misbehaving', 'peer=4 (40 -> 60): headers message size = 2001']):
            msg = msg_headers([CBlockHeader()] * 2001)
            conn.send_and_ping(msg)
        self.nodes[0].disconnect_p2ps()

    def test_resource_exhaustion(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        conn2 = self.nodes[0].add_p2p_connection(P2PDataStore())
        msg_at_size = msg_unrecognized(str_data="b" * VALID_DATA_LIMIT)
        assert len(msg_at_size.serialize()) == MSG_LIMIT

        self.log.info("Sending a bunch of large, junk messages to test memory exhaustion. May take a bit...")

        # Run a bunch of times to test for memory exhaustion.
        for _ in range(80):
            conn.send_message(msg_at_size)

        # Check that, even though the node is being hammered by nonsense from one
        # connection, it can still service other peers in a timely way.
        for _ in range(20):
            conn2.sync_with_ping(timeout=2)

        # Peer 1, despite being served up a bunch of nonsense, should still be connected.
        self.log.info("Waiting for node to drop junk messages.")
        conn.sync_with_ping(timeout=400)
        assert conn.is_connected
        self.nodes[0].disconnect_p2ps()


if __name__ == '__main__':
    InvalidMessagesTest().main()
