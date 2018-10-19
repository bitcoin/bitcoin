#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid network messages."""
import struct

from test_framework import messages
from test_framework.mininode import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework


class msg_unrecognized:
    """Nonsensical message. Modeled after similar types in test_framework.messages."""

    command = b'badmsg'

    def __init__(self, str_data):
        self.str_data = str_data.encode() if not isinstance(str_data, bytes) else str_data

    def serialize(self):
        return messages.ser_string(self.str_data)

    def __repr__(self):
        return "{}(data={})".format(self.command, self.str_data)


class msg_nametoolong(msg_unrecognized):

    command = b'thisnameiswayyyyyyyyytoolong'


class InvalidMessagesTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        """
        0. Send a bunch of large (4MB) messages of an unrecognized type. Check to see
           that it isn't an effective DoS against the node.

        1. Send an oversized (4MB+) message and check that we're disconnected.

        2. Send a few messages with an incorrect data size in the header, ensure the
           messages are ignored.

        3. Send an unrecognized message with a command name longer than 12 characters.

        """
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
        msg_at_size = msg_unrecognized("b" * valid_data_limit)
        assert len(msg_at_size.serialize()) == msg_limit

        with node.assert_memory_usage_stable(perc_increase_allowed=0.03):
            self.log.info(
                "Sending a bunch of large, junk messages to test "
                "memory exhaustion. May take a bit...")

            # Run a bunch of times to test for memory exhaustion.
            for _ in range(200):
                node.p2p.send_message(msg_at_size)

            # Check that, even though the node is being hammered by nonsense from one
            # connection, it can still service other peers in a timely way.
            for _ in range(20):
                conn2.sync_with_ping(timeout=2)

            # Peer 1, despite serving up a bunch of nonsense, should still be connected.
            self.log.info("Waiting for node to drop junk messages.")
            node.p2p.sync_with_ping(timeout=8)
            assert node.p2p.is_connected

        #
        # 1.
        #
        # Send an oversized message, ensure we're disconnected.
        #
        msg_over_size = msg_unrecognized("b" * (valid_data_limit + 1))
        assert len(msg_over_size.serialize()) == (msg_limit + 1)

        with node.assert_debug_log(["Oversized message from peer=0, disconnecting"]):
            # An unknown message type (or *any* message type) over
            # MAX_PROTOCOL_MESSAGE_LENGTH should result in a disconnect.
            node.p2p.send_message(msg_over_size)
            node.p2p.wait_for_disconnect(timeout=4)

        node.disconnect_p2ps()
        conn = node.add_p2p_connection(P2PDataStore())
        conn.wait_for_verack()

        #
        # 2.
        #
        # Send messages with an incorrect data size in the header.
        #
        actual_size = 100
        msg = msg_unrecognized("b" * actual_size)

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

        #
        # 3.
        #
        # Send a message with a too-long command name.
        #
        node.p2p.send_message(msg_nametoolong("foobar"))
        node.p2p.wait_for_disconnect(timeout=4)

        # Node is still up.
        conn = node.add_p2p_connection(P2PDataStore())
        conn.sync_with_ping()


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
