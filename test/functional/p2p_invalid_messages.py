#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid network messages."""
import struct
import sys


from test_framework.messages import (
    CBlockHeader,
    CInv,
    msg_ping,
    ser_string,
    msg_getdata,
    msg_headers,
    msg_inv,
    MSG_TX,
)
from test_framework.mininode import (
    P2PDataStore, P2PInterface
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    wait_until,
)


class msg_unrecognized:
    """Nonsensical message. Modeled after similar types in test_framework.messages."""

    command = b'badmsg'

    def __init__(self, *, str_data):
        self.str_data = str_data.encode() if not isinstance(str_data, bytes) else str_data

    def serialize(self):
        return ser_string(self.str_data)

    def __repr__(self):
        return "{}(data={})".format(self.command, self.str_data)


class InvalidMessagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        """
         . Test msg header
        0. Send a bunch of large (3MB) messages of an unrecognized type. Check to see
           that it isn't an effective DoS against the node.

        1. Send an oversized (3MB+) message and check that we're disconnected.

        2. Send a few messages with an incorrect data size in the header, ensure the
           messages are ignored.
        """
        self.test_buffer()
        self.test_magic_bytes()
        self.test_checksum()
        self.test_size()
        self.test_command()
        self.test_large_inv()

        node = self.nodes[0]
        self.node = node
        node.add_p2p_connection(P2PDataStore())
        conn2 = node.add_p2p_connection(P2PDataStore())

        msg_limit = 3 * 1024 * 1024  # 3MB, per MAX_PROTOCOL_MESSAGE_LENGTH
        valid_data_limit = msg_limit - 5  # Account for the 4-byte length prefix

        #
        # 0.
        #
        # Send as large a message as is valid, ensure we aren't disconnected but
        # also can't exhaust resources.
        #
        msg_at_size = msg_unrecognized(str_data="b" * valid_data_limit)
        assert len(msg_at_size.serialize()) == msg_limit

        self.log.info("Sending a bunch of large, junk messages to test memory exhaustion. May take a bit...")

        # Run a bunch of times to test for memory exhaustion.
        for _ in range(80):
            node.p2p.send_message(msg_at_size)

        # Check that, even though the node is being hammered by nonsense from one
        # connection, it can still service other peers in a timely way.
        for _ in range(20):
            conn2.sync_with_ping(timeout=2)

        # Peer 1, despite serving up a bunch of nonsense, should still be connected.
        self.log.info("Waiting for node to drop junk messages.")
        node.p2p.sync_with_ping(timeout=400)
        assert node.p2p.is_connected

        #
        # 1.
        #
        # Send an oversized message, ensure we're disconnected.
        #
        # Under macOS this test is skipped due to an unexpected error code
        # returned from the closing socket which python/asyncio does not
        # yet know how to handle.
        #
        if sys.platform != 'darwin':
            msg_over_size = msg_unrecognized(str_data="b" * (valid_data_limit + 1))
            assert len(msg_over_size.serialize()) == (msg_limit + 1)

            # An unknown message type (or *any* message type) over
            # MAX_PROTOCOL_MESSAGE_LENGTH should result in a disconnect.
            node.p2p.send_message(msg_over_size)
            node.p2p.wait_for_disconnect(timeout=4)

            node.disconnect_p2ps()
            conn = node.add_p2p_connection(P2PDataStore())
            conn.wait_for_verack()
        else:
            self.log.info("Skipping test p2p_invalid_messages/1 (oversized message) under macOS")

        #
        # 2.
        #
        # Send messages with an incorrect data size in the header.
        #
        actual_size = 100
        msg = msg_unrecognized(str_data="b" * actual_size)

        # TODO: handle larger-than cases. I haven't been able to pin down what behavior to expect.
        for wrong_size in (2, 77, 78, 79):
            self.log.info("Sending a message with incorrect size of {}".format(wrong_size))

            # Unmodified message should submit okay.
            node.p2p.send_and_ping(msg)

            # A message lying about its data size results in a disconnect when the incorrect
            # data size is less than the actual size.
            #
            # TODO: why does behavior change at 78 bytes?
            #
            node.p2p.send_raw_message(self._tweak_msg_data_size(msg, wrong_size))

            # For some reason unknown to me, we sometimes have to push additional data to the
            # peer in order for it to realize a disconnect.
            try:
                node.p2p.send_message(msg_ping(nonce=123123))
            except IOError:
                pass

            node.p2p.wait_for_disconnect(timeout=10)
            node.disconnect_p2ps()
            node.add_p2p_connection(P2PDataStore())

        # Node is still up.
        conn = node.add_p2p_connection(P2PDataStore())

    def test_buffer(self):
        self.log.info("Test message with header split across two buffers, should be received")
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
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['HEADER ERROR - MESSAGESTART (badmsg, 2 bytes), received ffffffff']):
            msg = conn.build_message(msg_unrecognized(str_data="d"))
            # modify magic bytes
            msg = b'\xff' * 4 + msg[4:]
            conn.send_raw_message(msg)
            conn.wait_for_disconnect(timeout=5)
            self.nodes[0].disconnect_p2ps()

    def test_checksum(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['CHECKSUM ERROR (badmsg, 2 bytes), expected 78df0a04 was ffffffff']):
            msg = conn.build_message(msg_unrecognized(str_data="d"))
            # Checksum is after start bytes (4B), message type (12B), len (4B)
            cut_len = 4 + 12 + 4
            # modify checksum
            msg = msg[:cut_len] + b'\xff' * 4 + msg[cut_len + 4:]
            self.nodes[0].p2p.send_raw_message(msg)
            conn.sync_with_ping(timeout=1)
            self.nodes[0].disconnect_p2ps()

    def test_size(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['HEADER ERROR - SIZE (badmsg, 33554433 bytes)']):
            msg = conn.build_message(msg_unrecognized(str_data="d"))
            cut_len = (
                4 +  # magic
                12  # command
            )
            # modify len to MAX_SIZE + 1
            msg = msg[:cut_len] + struct.pack("<I", 0x02000000 + 1) + msg[cut_len + 4:]
            self.nodes[0].p2p.send_raw_message(msg)
            conn.wait_for_disconnect(timeout=5)
            self.nodes[0].disconnect_p2ps()

    def test_command(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['HEADER ERROR - COMMAND']):
            msg = msg_unrecognized(str_data="d")
            msg = conn.build_message(msg)
            # Modify command
            msg = msg[:7] + b'\x00' + msg[7 + 1:]
            self.nodes[0].p2p.send_raw_message(msg)
            conn.sync_with_ping(timeout=1)
            self.nodes[0].disconnect_p2ps()

    def test_large_inv(self):
        conn = self.nodes[0].add_p2p_connection(P2PInterface())
        with self.nodes[0].assert_debug_log(['Misbehaving', 'peer=5 (0 -> 20): message inv size() = 50001']):
            msg = msg_inv([CInv(MSG_TX, 1)] * 50001)
            conn.send_and_ping(msg)
        with self.nodes[0].assert_debug_log(['Misbehaving', 'peer=5 (20 -> 40): message getdata size() = 50001']):
            msg = msg_getdata([CInv(MSG_TX, 1)] * 50001)
            conn.send_and_ping(msg)
        with self.nodes[0].assert_debug_log(['Misbehaving', 'peer=5 (40 -> 60): headers message size = 2001']):
            msg = msg_headers([CBlockHeader()] * 2001)
            conn.send_and_ping(msg)
        self.nodes[0].disconnect_p2ps()

    def _tweak_msg_data_size(self, message, wrong_size):
        """
        Return a raw message based on another message but with an incorrect data size in
        the message header.
        """
        raw_msg = self.node.p2p.build_message(message)

        bad_size_bytes = struct.pack("<I", wrong_size)
        num_header_bytes_before_size = 4 + 12

        # Replace the correct data size in the message with an incorrect one.
        raw_msg_with_wrong_size = (
            raw_msg[:num_header_bytes_before_size] +
            bad_size_bytes +
            raw_msg[(num_header_bytes_before_size + len(bad_size_bytes)):]
        )
        assert len(raw_msg) == len(raw_msg_with_wrong_size)

        return raw_msg_with_wrong_size


if __name__ == '__main__':
    InvalidMessagesTest().main()
