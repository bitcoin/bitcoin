#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid network messages."""

import random
import time

from test_framework.messages import (
    CBlockHeader,
    CInv,
    MAX_HEADERS_RESULTS,
    MAX_INV_SIZE,
    MAX_PROTOCOL_MESSAGE_LENGTH,
    MSG_TX,
    from_hex,
    msg_getdata,
    msg_headers,
    msg_inv,
    msg_ping,
    msg_version,
    ser_string,
)
from test_framework.p2p import (
    P2PDataStore,
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
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


class InvalidMessagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-whitelist=addr@127.0.0.1"]]

    def run_test(self):
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
        self.test_invalid_pow_headers_msg()
        self.test_noncontinuous_headers_msg()
        self.test_resource_exhaustion()

    def test_buffer(self):
        self.log.info("Test message with header split across two buffers is received")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        # After add_p2p_connection both sides have the verack processed.
        # However the pong from conn in reply to the ping from the node has not
        # been processed and recorded in totalbytesrecv.
        # Flush the pong from conn by sending a ping from conn.
        conn.sync_with_ping(timeout=1)
        # Create valid message
        msg = conn.build_message(msg_ping(nonce=12345))
        cut_pos = 12  # Chosen at an arbitrary position within the header
        # Send message in two pieces
        before = self.nodes[0].getnettotals()['totalbytesrecv']
        conn.send_raw_message(msg[:cut_pos])
        # Wait until node has processed the first half of the message
        self.wait_until(lambda: self.nodes[0].getnettotals()['totalbytesrecv'] != before)
        middle = self.nodes[0].getnettotals()['totalbytesrecv']
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
        # Skip with v2, magic bytes are v1-specific
        if self.options.v2transport:
            return
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
        # Skip with v2, the checksum is v1-specific
        if self.options.v2transport:
            return
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
        error_msg = (
            ['V2 transport error: packet too large (4000014 bytes)'] if self.options.v2transport
            else ['Header error: Size too large (badmsg, 4000001 bytes)']
        )
        with self.nodes[0].assert_debug_log(error_msg):
            msg = msg_unrecognized(str_data="d" * (VALID_DATA_LIMIT + 1))
            msg = conn.build_message(msg)
            conn.send_raw_message(msg)
            conn.wait_for_disconnect(timeout=1)
        self.nodes[0].disconnect_p2ps()

    def test_msgtype(self):
        self.log.info("Test message with invalid message type logs an error")
        conn = self.nodes[0].add_p2p_connection(P2PDataStore())
        if self.options.v2transport:
            msgtype = 99 # not defined
            msg = msg_unrecognized(str_data="d")
            contents = msgtype.to_bytes(1, 'big') + msg.serialize()
            tmsg = conn.v2_state.v2_enc_packet(contents, ignore=False)
            with self.nodes[0].assert_debug_log(['V2 transport error: invalid message type']):
                conn.send_raw_message(tmsg)
                conn.sync_with_ping(timeout=1)
            # Check that traffic is accounted for (20 bytes plus 3 bytes contents)
            assert_equal(self.nodes[0].getpeerinfo()[0]['bytesrecv_per_msg']['*other*'], 23)
        else:
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

        # Make sure bitcoind signals support for ADDRv2, otherwise this test
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
            bytes.fromhex('00'))

    def test_addrv2_too_long_address(self):
        self.test_addrv2('too long address',
            [
                'received: addrv2 (525 bytes)',
                'ProcessMessages(addrv2, 525 bytes): Exception',
                'Address too long: 513 > 512',
            ],
            bytes.fromhex(
                '01' +       # number of entries
                '61bc6649' + # time, Fri Jan  9 02:54:25 UTC 2009
                '00' +       # service flags, COMPACTSIZE(NODE_NONE)
                '01' +       # network type (IPv4)
                'fd0102' +   # address length (COMPACTSIZE(513))
                'ab' * 513 + # address
                '208d'))     # port

    def test_addrv2_unrecognized_network(self):
        now_hex = int(time.time()).to_bytes(4, "little").hex()
        self.test_addrv2('unrecognized network',
            [
                'received: addrv2 (25 bytes)',
                '9.9.9.9:8333',
                'Added 1 addresses',
            ],
            bytes.fromhex(
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
            conn = self.nodes[0].add_p2p_connection(P2PInterface())
            conn.send_message(msg)
            conn.wait_for_disconnect()
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

    def test_invalid_pow_headers_msg(self):
        self.log.info("Test headers message with invalid proof-of-work is logged as misbehaving and disconnects peer")
        blockheader_tip_hash = self.nodes[0].getbestblockhash()
        blockheader_tip = from_hex(CBlockHeader(), self.nodes[0].getblockheader(blockheader_tip_hash, False))

        # send valid headers message first
        assert_equal(self.nodes[0].getblockchaininfo()['headers'], 0)
        blockheader = CBlockHeader()
        blockheader.hashPrevBlock = int(blockheader_tip_hash, 16)
        blockheader.nTime = int(time.time())
        blockheader.nBits = blockheader_tip.nBits
        blockheader.rehash()
        while not blockheader.hash.startswith('0'):
            blockheader.nNonce += 1
            blockheader.rehash()
        peer = self.nodes[0].add_p2p_connection(P2PInterface())
        peer.send_and_ping(msg_headers([blockheader]))
        assert_equal(self.nodes[0].getblockchaininfo()['headers'], 1)
        chaintips = self.nodes[0].getchaintips()
        assert_equal(chaintips[0]['status'], 'headers-only')
        assert_equal(chaintips[0]['hash'], blockheader.hash)

        # invalidate PoW
        while not blockheader.hash.startswith('f'):
            blockheader.nNonce += 1
            blockheader.rehash()
        with self.nodes[0].assert_debug_log(['Misbehaving', 'header with invalid proof of work']):
            peer.send_message(msg_headers([blockheader]))
            peer.wait_for_disconnect()

    def test_noncontinuous_headers_msg(self):
        self.log.info("Test headers message with non-continuous headers sequence is logged as misbehaving")
        block_hashes = self.generate(self.nodes[0], 10)
        block_headers = []
        for block_hash in block_hashes:
            block_headers.append(from_hex(CBlockHeader(), self.nodes[0].getblockheader(block_hash, False)))

        # continuous headers sequence should be fine
        MISBEHAVING_NONCONTINUOUS_HEADERS_MSGS = ['Misbehaving', 'non-continuous headers sequence']
        peer = self.nodes[0].add_p2p_connection(P2PInterface())
        with self.nodes[0].assert_debug_log([], unexpected_msgs=MISBEHAVING_NONCONTINUOUS_HEADERS_MSGS):
            peer.send_and_ping(msg_headers(block_headers))

        # delete arbitrary block header somewhere in the middle to break link
        del block_headers[random.randrange(1, len(block_headers)-1)]
        with self.nodes[0].assert_debug_log(expected_msgs=MISBEHAVING_NONCONTINUOUS_HEADERS_MSGS):
            peer.send_message(msg_headers(block_headers))
            peer.wait_for_disconnect()
        self.nodes[0].disconnect_p2ps()

    def test_resource_exhaustion(self):
        self.log.info("Test node stays up despite many large junk messages")
        # Don't use v2 here - the non-optimised encryption would take too long to encrypt
        # the large messages
        conn = self.nodes[0].add_p2p_connection(P2PDataStore(), supports_v2_p2p=False)
        conn2 = self.nodes[0].add_p2p_connection(P2PDataStore(), supports_v2_p2p=False)
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
    InvalidMessagesTest(__file__).main()
