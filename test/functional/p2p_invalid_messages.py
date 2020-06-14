#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid network messages."""

from test_framework.messages import (
    CBlockHeader,
    CInv,
    MAX_HEADERS_RESULTS,
    MAX_INV_SIZE,
    MAX_PROTOCOL_MESSAGE_LENGTH,
    msg_getdata,
    msg_headers,
    msg_inv,
    msg_ping,
    MSG_TX,
    ser_string,
)
from test_framework.mininode import (
    P2PDataStore,
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    wait_until,
)

VALID_DATA_LIMIT = MAX_PROTOCOL_MESSAGE_LENGTH - 5  # Account for the 5-byte length prefix

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
        self.test_buffer()
        self.test_magic_bytes()
        self.test_checksum()
        self.test_size()
        self.test_msgtype()
        self.test_oversized_inv_msg()
        self.test_oversized_getdata_msg()
        self.test_oversized_headers_msg()
        self.test_resource_exhaustion()

    def test_buffer(self):
        self.log.info("Test message with header split across two buffers is received")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        # Create valid message
        msg = conn.build_message(msg_ping(nonce=12345))
        cut_pos = 12    # Chosen at an arbitrary position within the header
        # Send message in two pieces
        before = int(self.nodes[0].getnettotals()['totalbytesrecv'])
        conn.send_raw_message(msg[:cut_pos])
        # Wait until node has processed the first half of the message
        wait_until(lambda: int(self.nodes[0].getnettotals()['totalbytesrecv']) != before)
        middle = int(self.nodes[0].getnettotals()['totalbytesrecv'])
        # If this assert fails, we've hit an unlikely race
        # where the test framework sent a message in between the two halves
        assert_equal(middle, before + cut_pos)
        conn.send_raw_message(msg[cut_pos:])
        conn.sync_with_ping(timeout=1)
        self.nodes[0].disconnect_p2ps()

    def test_magic_bytes(self):
        self.log.info("Test message with invalid magic bytes disconnects peer")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['PROCESSMESSAGE: INVALID MESSAGESTART badmsg']):
            msg = conn.build_message(msg_unrecognized(str_data="d"))
            # modify magic bytes
            msg = b'\xff' * 4 + msg[4:]
            conn.send_raw_message(msg)
            conn.wait_for_disconnect(timeout=1)
        self.nodes[0].disconnect_p2ps()

    def test_checksum(self):
        self.log.info("Test message with invalid checksum logs an error")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['CHECKSUM ERROR (badmsg, 2 bytes), expected 78df0a04 was ffffffff']):
            msg = conn.build_message(msg_unrecognized(str_data="d"))
            # Checksum is after start bytes (4B), message type (12B), len (4B)
            cut_len = 4 + 12 + 4
            # modify checksum
            msg = msg[:cut_len] + b'\xff' * 4 + msg[cut_len + 4:]
            conn.send_raw_message(msg)
            conn.sync_with_ping(timeout=1)
        self.nodes[0].disconnect_p2ps()

    def test_size(self):
        self.log.info("Test message with oversized payload disconnects peer")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['']):
            msg = msg_unrecognized(str_data="d" * (VALID_DATA_LIMIT + 1))
            msg = conn.build_message(msg)
            conn.send_raw_message(msg)
            conn.wait_for_disconnect(timeout=1)
        self.nodes[0].disconnect_p2ps()

    def test_msgtype(self):
        self.log.info("Test message with invalid message type logs an error")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['PROCESSMESSAGE: ERRORS IN HEADER']):
            msg = msg_unrecognized(str_data="d")
            msg.msgtype = b'\xff' * 12
            msg = conn.build_message(msg)
            # Modify msgtype
            msg = msg[:7] + b'\x00' + msg[7 + 1:]
            conn.send_raw_message(msg)
            conn.sync_with_ping(timeout=1)
        self.nodes[0].disconnect_p2ps()

    def test_oversized_msg(self, msg, size):
        msg_type = msg.msgtype.decode('ascii')
        self.log.info("Test {} message of size {} is logged as misbehaving".format(msg_type, size))
        with self.nodes[0].assert_debug_log(['Misbehaving', '{} message size = {}'.format(msg_type, size)]):
            self.nodes[0].add_p2p_connection(P2PInterface()).send_and_ping(msg)
        self.nodes[0].disconnect_p2ps()

    def test_oversized_inv_msg(self):
        size = MAX_INV_SIZE + 1
        self.test_oversized_msg(msg_inv([CInv(MSG_TX, 1)] * size), size)

    def test_oversized_getdata_msg(self):
        size = MAX_INV_SIZE + 1
        self.test_oversized_msg(msg_getdata([CInv(MSG_TX, 1)] * size), size)

    def test_oversized_headers_msg(self):
        size = MAX_HEADERS_RESULTS + 1
        self.test_oversized_msg(msg_headers([CBlockHeader()] * size), size)

    def test_resource_exhaustion(self):
        self.log.info("Test node stays up despite many large junk messages")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        conn2 = self.nodes[0].add_p2p_connection(P2PDataStore())
        msg_at_size = msg_unrecognized(str_data="b" * VALID_DATA_LIMIT)
        assert len(msg_at_size.serialize()) == MAX_PROTOCOL_MESSAGE_LENGTH

        self.log.info("(a) Send 80 messages, each of maximum valid data size (4MB)")
        for _ in range(80):
            conn.send_message(msg_at_size)

        # Check that, even though the node is being hammered by nonsense from one
        # connection, it can still service other peers in a timely way.
        self.log.info("(b) Check node still services peers in a timely way")
        for _ in range(20):
            conn2.sync_with_ping(timeout=2)

        self.log.info("(c) Wait for node to drop junk messages, while remaining connected")
        conn.sync_with_ping(timeout=400)

        # Despite being served up a bunch of nonsense, the peers should still be connected.
        assert conn.is_connected
        assert conn2.is_connected
        self.nodes[0].disconnect_p2ps()


if __name__ == '__main__':
    InvalidMessagesTest().main()
