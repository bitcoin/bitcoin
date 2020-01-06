#!/usr/bin/env python3
# Copyright (c) 2015-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid network messages."""
import asyncio
import os
import struct
import sys

from test_framework import messages
from test_framework.mininode import P2PDataStore, NetworkThread
from test_framework.test_framework import BitcoinTestFramework


class msg_unrecognized:
    """Nonsensical message. Modeled after similar types in test_framework.messages."""

    command = b'badmsg'

    def __init__(self, *, str_data):
        self.str_data = str_data.encode() if not isinstance(str_data, bytes) else str_data

    def serialize(self):
        return messages.ser_string(self.str_data)

    def __repr__(self):
        return "{}(data={})".format(self.command, self.str_data)


class InvalidMessagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        """
         . Test msg header
        0. Send a bunch of large (4MB) messages of an unrecognized type. Check to see
           that it isn't an effective DoS against the node.

        1. Send an oversized (4MB+) message and check that we're disconnected.

        2. Send a few messages with an incorrect data size in the header, ensure the
           messages are ignored.
        """
        self.test_magic_bytes()
        self.test_checksum()
        self.test_size()
        self.test_command()

        node = self.nodes[0]
        self.node = node
        node.add_p2p_connection(P2PDataStore())
        conn2 = node.add_p2p_connection(P2PDataStore())

        msg_limit = 4 * 1000 * 1000  # 4MB, per MAX_PROTOCOL_MESSAGE_LENGTH
        valid_data_limit = msg_limit - 5  # Account for the 4-byte length prefix

        #
        # 0.
        #
        # Send as large a message as is valid, ensure we aren't disconnected but
        # also can't exhaust resources.
        #
        msg_at_size = msg_unrecognized(str_data="b" * valid_data_limit)
        assert len(msg_at_size.serialize()) == msg_limit

        increase_allowed = 0.5
        if [s for s in os.environ.get("BITCOIN_CONFIG", "").split(" ") if "--with-sanitizers" in s and "address" in s]:
            increase_allowed = 3.5
        with node.assert_memory_usage_stable(increase_allowed=increase_allowed):
            self.log.info(
                "Sending a bunch of large, junk messages to test "
                "memory exhaustion. May take a bit...")

            # Run a bunch of times to test for memory exhaustion.
            for _ in range(80):
                node.p2p.send_message(msg_at_size)

            # Check that, even though the node is being hammered by nonsense from one
            # connection, it can still service other peers in a timely way.
            for _ in range(20):
                conn2.sync_with_ping(timeout=2)

            # Peer 1, despite serving up a bunch of nonsense, should still be connected.
            self.log.info("Waiting for node to drop junk messages.")
            node.p2p.sync_with_ping(timeout=320)
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

            with node.assert_debug_log(["Oversized message from peer=4, disconnecting"]):
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
                node.p2p.send_message(messages.msg_ping(nonce=123123))
            except IOError:
                pass

            node.p2p.wait_for_disconnect(timeout=10)
            node.disconnect_p2ps()
            node.add_p2p_connection(P2PDataStore())

        # Node is still up.
        conn = node.add_p2p_connection(P2PDataStore())
        conn.sync_with_ping()

    def test_magic_bytes(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())

        def swap_magic_bytes():
            conn._on_data = lambda: None  # Need to ignore all incoming messages from now, since they come with "invalid" magic bytes
            conn.magic_bytes = b'\x00\x11\x22\x32'

        # Call .result() to block until the atomic swap is complete, otherwise
        # we might run into races later on
        asyncio.run_coroutine_threadsafe(asyncio.coroutine(swap_magic_bytes)(), NetworkThread.network_event_loop).result()

        with self.nodes[0].assert_debug_log(['PROCESSMESSAGE: INVALID MESSAGESTART ping']):
            conn.send_message(messages.msg_ping(nonce=0xff))
            conn.wait_for_disconnect(timeout=1)
            self.nodes[0].disconnect_p2ps()

    def test_checksum(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['ProcessMessages(badmsg, 2 bytes): CHECKSUM ERROR expected 78df0a04 was ffffffff']):
            msg = conn.build_message(msg_unrecognized(str_data="d"))
            cut_len = (
                4 +  # magic
                12 +  # command
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
            msg = conn.build_message(msg_unrecognized(str_data="d"))
            cut_len = (
                4 +  # magic
                12  # command
            )
            # modify len to MAX_SIZE + 1
            msg = msg[:cut_len] + struct.pack("<I", 0x02000000 + 1) + msg[cut_len + 4:]
            self.nodes[0].p2p.send_raw_message(msg)
            conn.wait_for_disconnect(timeout=1)
            self.nodes[0].disconnect_p2ps()

    def test_command(self):
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['PROCESSMESSAGE: ERRORS IN HEADER']):
            msg = msg_unrecognized(str_data="d")
            msg.command = b'\xff' * 12
            msg = conn.build_message(msg)
            # Modify command
            msg = msg[:7] + b'\x00' + msg[7 + 1:]
            self.nodes[0].p2p.send_raw_message(msg)
            conn.sync_with_ping(timeout=1)
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
