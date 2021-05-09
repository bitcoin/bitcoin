#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid network messages."""

import struct
import time

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
    msg_version,
    ser_string,
)
from test_framework.p2p import (
    P2PDataStore,
    P2PInterface,
)
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    hex_str_to_bytes,
    force_finish_mnsync,
)

VALID_DATA_LIMIT = MAX_PROTOCOL_MESSAGE_LENGTH - 5  # Account for the 5-byte length prefix


class msg_unrecognized:
    """Nonsensical message. Modeled after similar types in test_framework.messages."""

    msgtype = b'badmsg\x01'

    def __init__(self, *, str_data):
        self.str_data = str_data.encode() if not isinstance(str_data, bytes) else str_data

    def serialize(self):
        return ser_string(self.str_data)

    def __repr__(self):
        return "{}(data={})".format(self.msgtype, self.str_data)


class SenderOfAddrV2(P2PInterface):
    def wait_for_sendaddrv2(self):
        self.wait_until(lambda: 'sendaddrv2' in self.last_message)


class InvalidMessagesTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        # SYSCOIN
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        self.test_buffer()
        self.test_duplicate_version_msg()
        self.test_magic_bytes()
        self.test_checksum()
        self.test_size()
        self.test_msgtype()
        self.test_addrv2_empty()
        self.test_addrv2_no_addresses()
        self.test_addrv2_too_long_address()
        self.test_addrv2_unrecognized_network()
        self.test_oversized_inv_msg()
        self.test_oversized_getdata_msg()
        self.test_oversized_headers_msg()
        self.test_resource_exhaustion()

    def test_buffer(self):
        self.log.info("Test message with header split across two buffers is received")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        # Create valid message
        msg = conn.build_message(msg_ping(nonce=12345))
        cut_pos = 12  # Chosen at an arbitrary position within the header
        # Send message in two pieces
        before = self.nodes[0].getnettotals()['totalbytesrecv']
        conn.send_raw_message(msg[:cut_pos])
        # Wait until node has processed the first half of the message
        self.wait_until(lambda: self.nodes[0].getnettotals()['totalbytesrecv'] != before)
        middle = self.nodes[0].getnettotals()['totalbytesrecv']
        # If this assert fails, we've hit an unlikely race
        # where the test framework sent a message in between the two halves
        assert_equal(middle, before + cut_pos)
        conn.send_raw_message(msg[cut_pos:])
        conn.sync_with_ping(timeout=1)
        self.nodes[0].disconnect_p2ps()

    def test_duplicate_version_msg(self):
        self.log.info("Test duplicate version message is ignored")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['redundant version message from peer']):
            conn.send_and_ping(msg_version())
        self.nodes[0].disconnect_p2ps()

    def test_magic_bytes(self):
        self.log.info("Test message with invalid magic bytes disconnects peer")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['Header error: Wrong MessageStart ffffffff received']):
            msg = conn.build_message(msg_unrecognized(str_data="d"))
            # modify magic bytes
            msg = b'\xff' * 4 + msg[4:]
            conn.send_raw_message(msg)
            conn.wait_for_disconnect(timeout=1)
        self.nodes[0].disconnect_p2ps()

    def test_checksum(self):
        self.log.info("Test message with invalid checksum logs an error")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['Header error: Wrong checksum (badmsg, 2 bytes), expected 78df0a04 was ffffffff']):
            msg = conn.build_message(msg_unrecognized(str_data="d"))
            # Checksum is after start bytes (4B), message type (12B), len (4B)
            cut_len = 4 + 12 + 4
            # modify checksum
            msg = msg[:cut_len] + b'\xff' * 4 + msg[cut_len + 4:]
            conn.send_raw_message(msg)
            conn.sync_with_ping(timeout=1)
        # Check that traffic is accounted for (24 bytes header + 2 bytes payload)
        assert_equal(self.nodes[0].getpeerinfo()[0]['bytesrecv_per_msg']['*other*'], 26)
        self.nodes[0].disconnect_p2ps()

    def test_size(self):
        self.log.info("Test message with oversized payload disconnects peer")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        # SYSCOIN 32MB
        with self.nodes[0].assert_debug_log(['Header error: Size too large (badmsg, 33554433 bytes)']):
            msg = msg_unrecognized(str_data="d" * (VALID_DATA_LIMIT + 1))
            msg = conn.build_message(msg)
            conn.send_raw_message(msg)
            conn.wait_for_disconnect(timeout=1)
        self.nodes[0].disconnect_p2ps()

    def test_msgtype(self):
        self.log.info("Test message with invalid message type logs an error")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        with self.nodes[0].assert_debug_log(['Header error: Invalid message type']):
            msg = msg_unrecognized(str_data="d")
            msg = conn.build_message(msg)
            # Modify msgtype
            msg = msg[:7] + b'\x00' + msg[7 + 1:]
            conn.send_raw_message(msg)
            conn.sync_with_ping(timeout=1)
        # Check that traffic is accounted for (24 bytes header + 2 bytes payload)
        assert_equal(self.nodes[0].getpeerinfo()[0]['bytesrecv_per_msg']['*other*'], 26)
        self.nodes[0].disconnect_p2ps()

    def test_addrv2(self, label, required_log_messages, raw_addrv2):
        node = self.nodes[0]
        conn = node.add_p2p_connection(SenderOfAddrV2())

        # Make sure syscoind signals support for ADDRv2, otherwise this test
        # will bombard an old node with messages it does not recognize which
        # will produce unexpected results.
        conn.wait_for_sendaddrv2()

        self.log.info('Test addrv2: ' + label)

        msg = msg_unrecognized(str_data=b'')
        msg.msgtype = b'addrv2'
        with node.assert_debug_log(required_log_messages):
            # override serialize() which would include the length of the data
            msg.serialize = lambda: raw_addrv2
            conn.send_raw_message(conn.build_message(msg))
            conn.sync_with_ping()

        node.disconnect_p2ps()

    def test_addrv2_empty(self):
        self.test_addrv2('empty',
            [
                'received: addrv2 (0 bytes)',
                'ProcessMessages(addrv2, 0 bytes): Exception',
                'end of data',
            ],
            b'')

    def test_addrv2_no_addresses(self):
        self.test_addrv2('no addresses',
            [
                'received: addrv2 (1 bytes)',
            ],
            hex_str_to_bytes('00'))

    def test_addrv2_too_long_address(self):
        self.test_addrv2('too long address',
            [
                'received: addrv2 (525 bytes)',
                'ProcessMessages(addrv2, 525 bytes): Exception',
                'Address too long: 513 > 512',
            ],
            hex_str_to_bytes(
                '01' +       # number of entries
                '61bc6649' + # time, Fri Jan  9 02:54:25 UTC 2009
                '00' +       # service flags, COMPACTSIZE(NODE_NONE)
                '01' +       # network type (IPv4)
                'fd0102' +   # address length (COMPACTSIZE(513))
                'ab' * 513 + # address
                '208d'))     # port

    def test_addrv2_unrecognized_network(self):
        now_hex = struct.pack('<I', int(time.time())).hex()
        self.test_addrv2('unrecognized network',
            [
                'received: addrv2 (25 bytes)',
                'IP 9.9.9.9 mapped',
                'Added 1 addresses',
            ],
            hex_str_to_bytes(
                '02' +     # number of entries
                # this should be ignored without impeding acceptance of subsequent ones
                now_hex +  # time
                '01' +     # service flags, COMPACTSIZE(NODE_NETWORK)
                '99' +     # network type (unrecognized)
                '02' +     # address length (COMPACTSIZE(2))
                'ab' * 2 + # address
                '208d' +   # port
                # this should be added:
                now_hex +  # time
                '01' +     # service flags, COMPACTSIZE(NODE_NETWORK)
                '01' +     # network type (IPv4)
                '04' +     # address length (COMPACTSIZE(4))
                '09' * 4 + # address
                '208d'))   # port

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
        # SYSCOIN
        msg_at_size = msg_unrecognized(str_data="b" * ((4 * 1024 * 1024) - 5))
        assert len(msg_at_size.serialize()) == (4 * 1024 * 1024)

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
