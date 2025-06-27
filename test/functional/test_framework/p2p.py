#!/usr/bin/env python3
# Copyright (c) 2010 ArtForz -- public domain half-a-node
# Copyright (c) 2012 Jeff Garzik
# Copyright (c) 2010-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test objects for interacting with a bitcoind node over the p2p protocol.

The P2PInterface objects interact with the bitcoind nodes under test using the
node's p2p interface. They can be used to send messages to the node, and
callbacks can be registered that execute when messages are received from the
node. Messages are sent to/received from the node on an asyncio event loop.
State held inside the objects must be guarded by the p2p_lock to avoid data
races between the main testing thread and the event loop.

P2PConnection: A low-level connection object to a node's P2P interface
P2PInterface: A high-level interface object for communicating to a node over P2P
P2PDataStore: A p2p interface class that keeps a store of transactions and blocks
              and can respond correctly to getdata and getheaders messages
P2PTxInvStore: A p2p interface class that inherits from P2PDataStore, and keeps
              a count of how many times each txid has been announced."""

import asyncio
from collections import defaultdict
from io import BytesIO
import logging
import platform
import struct
import sys
import threading

from test_framework.messages import (
    CBlockHeader,
    MAX_HEADERS_RESULTS,
    msg_addr,
    msg_addrv2,
    msg_block,
    MSG_BLOCK,
    msg_blocktxn,
    msg_cfcheckpt,
    msg_cfheaders,
    msg_cfilter,
    msg_cmpctblock,
    msg_feefilter,
    msg_filteradd,
    msg_filterclear,
    msg_filterload,
    msg_getaddr,
    msg_getblocks,
    msg_getblocktxn,
    msg_getcfcheckpt,
    msg_getcfheaders,
    msg_getcfilters,
    msg_getdata,
    msg_getheaders,
    msg_headers,
    msg_inv,
    msg_mempool,
    msg_merkleblock,
    msg_notfound,
    msg_ping,
    msg_pong,
    msg_sendaddrv2,
    msg_sendcmpct,
    msg_sendheaders,
    msg_sendtxrcncl,
    msg_tx,
    MSG_TX,
    MSG_TYPE_MASK,
    msg_verack,
    msg_version,
    MSG_WTX,
    msg_wtxidrelay,
    NODE_NETWORK,
    NODE_WITNESS,
    MAGIC_BYTES,
    sha256,
)
from test_framework.util import (
    assert_not_equal,
    MAX_NODES,
    p2p_port,
    wait_until_helper_internal,
)
from test_framework.v2_p2p import (
    EncryptedP2PState,
    MSGTYPE_TO_SHORTID,
    SHORTID,
)

logger = logging.getLogger("TestFramework.p2p")

# The minimum P2P version that this test framework supports
MIN_P2P_VERSION_SUPPORTED = 60001
# The P2P version that this test framework implements and sends in its `version` message
# Version 70016 supports wtxid relay
P2P_VERSION = 70016
# The services that this test framework offers in its `version` message
P2P_SERVICES = NODE_NETWORK | NODE_WITNESS
# The P2P user agent string that this test framework sends in its `version` message
P2P_SUBVERSION = "/python-p2p-tester:0.0.3/"
# Value for relay that this test framework sends in its `version` message
P2P_VERSION_RELAY = 1
# Delay after receiving a tx inv before requesting transactions from non-preferred peers, in seconds
NONPREF_PEER_TX_DELAY = 2
# Delay for requesting transactions via txids if we have wtxid-relaying peers, in seconds
TXID_RELAY_DELAY = 2
# Delay for requesting transactions if the peer has MAX_PEER_TX_REQUEST_IN_FLIGHT or more requests
OVERLOADED_PEER_TX_DELAY = 2
# How long to wait before downloading a transaction from an additional peer
GETDATA_TX_INTERVAL = 60

MESSAGEMAP = {
    b"addr": msg_addr,
    b"addrv2": msg_addrv2,
    b"block": msg_block,
    b"blocktxn": msg_blocktxn,
    b"cfcheckpt": msg_cfcheckpt,
    b"cfheaders": msg_cfheaders,
    b"cfilter": msg_cfilter,
    b"cmpctblock": msg_cmpctblock,
    b"feefilter": msg_feefilter,
    b"filteradd": msg_filteradd,
    b"filterclear": msg_filterclear,
    b"filterload": msg_filterload,
    b"getaddr": msg_getaddr,
    b"getblocks": msg_getblocks,
    b"getblocktxn": msg_getblocktxn,
    b"getcfcheckpt": msg_getcfcheckpt,
    b"getcfheaders": msg_getcfheaders,
    b"getcfilters": msg_getcfilters,
    b"getdata": msg_getdata,
    b"getheaders": msg_getheaders,
    b"headers": msg_headers,
    b"inv": msg_inv,
    b"mempool": msg_mempool,
    b"merkleblock": msg_merkleblock,
    b"notfound": msg_notfound,
    b"ping": msg_ping,
    b"pong": msg_pong,
    b"sendaddrv2": msg_sendaddrv2,
    b"sendcmpct": msg_sendcmpct,
    b"sendheaders": msg_sendheaders,
    b"sendtxrcncl": msg_sendtxrcncl,
    b"tx": msg_tx,
    b"verack": msg_verack,
    b"version": msg_version,
    b"wtxidrelay": msg_wtxidrelay,
}


class P2PConnection(asyncio.Protocol):
    """A low-level connection object to a node's P2P interface.

    This class is responsible for:

    - opening and closing the TCP connection to the node
    - reading bytes from and writing bytes to the socket
    - deserializing and serializing the P2P message header
    - logging messages as they are sent and received

    This class contains no logic for handing the P2P message payloads. It must be
    sub-classed and the on_message() callback overridden."""

    def __init__(self):
        # The underlying transport of the connection.
        # Should only call methods on this from the NetworkThread, c.f. call_soon_threadsafe
        self._transport = None
        # This lock is acquired before sending messages over the socket. There's an implied lock order and
        # p2p_lock must not be acquired after _send_lock as it could result in deadlocks.
        self._send_lock = threading.Lock()
        self.v2_state = None  # EncryptedP2PState object needed for v2 p2p connections
        self.reconnect = False  # set if reconnection needs to happen

    @property
    def is_connected(self):
        return self._transport is not None

    @property
    def supports_v2_p2p(self):
        return self.v2_state is not None

    def peer_connect_helper(self, dstaddr, dstport, net, timeout_factor):
        assert not self.is_connected
        self.timeout_factor = timeout_factor
        self.dstaddr = dstaddr
        self.dstport = dstport
        # The initial message to send after the connection was made:
        self.on_connection_send_msg = None
        self.recvbuf = b""
        self.magic_bytes = MAGIC_BYTES[net]
        self.p2p_connected_to_node = dstport != 0

    def peer_connect(self, dstaddr, dstport, *, net, timeout_factor, supports_v2_p2p):
        self.peer_connect_helper(dstaddr, dstport, net, timeout_factor)
        if supports_v2_p2p:
            self.v2_state = EncryptedP2PState(initiating=True, net=net)

        loop = NetworkThread.network_event_loop
        logger.debug('Connecting to Bitcoin Node: %s:%d' % (self.dstaddr, self.dstport))
        coroutine = loop.create_connection(lambda: self, host=self.dstaddr, port=self.dstport)
        return lambda: loop.call_soon_threadsafe(loop.create_task, coroutine)

    def peer_accept_connection(self, connect_id, connect_cb=lambda: None, *, net, timeout_factor, supports_v2_p2p, reconnect):
        self.peer_connect_helper('0', 0, net, timeout_factor)
        self.reconnect = reconnect
        if supports_v2_p2p:
            self.v2_state = EncryptedP2PState(initiating=False, net=net)

        logger.debug('Listening for Bitcoin Node with id: {}'.format(connect_id))
        return lambda: NetworkThread.listen(self, connect_cb, idx=connect_id)

    def peer_disconnect(self):
        # Connection could have already been closed by other end.
        NetworkThread.network_event_loop.call_soon_threadsafe(lambda: self._transport and self._transport.abort())

    # Connection and disconnection methods

    def connection_made(self, transport):
        """asyncio callback when a connection is opened."""
        assert not self._transport
        info = transport.get_extra_info("socket")
        us = info.getsockname()
        them = info.getpeername()
        logger.debug(f"Connected: us={us[0]}:{us[1]}, them={them[0]}:{them[1]}")
        self.dstaddr = them[0]
        self.dstport = them[1]
        self._transport = transport
        # in an inbound connection to the TestNode with P2PConnection as the initiator, [TestNode <---- P2PConnection]
        # send the initial handshake immediately
        if self.supports_v2_p2p and self.v2_state.initiating and not self.v2_state.tried_v2_handshake:
            send_handshake_bytes = self.v2_state.initiate_v2_handshake()
            logger.debug(f"sending {len(self.v2_state.sent_garbage)} bytes of garbage data")
            self.send_raw_message(send_handshake_bytes)
        # for v1 outbound connections, send version message immediately after opening
        # (for v2 outbound connections, send it after the initial v2 handshake)
        if self.p2p_connected_to_node and not self.supports_v2_p2p:
            self.send_version()
        self.on_open()

    def connection_lost(self, exc):
        """asyncio callback when a connection is closed."""
        # don't display warning if reconnection needs to be attempted using v1 P2P
        if exc and not self.reconnect:
            logger.warning("Connection lost to {}:{} due to {}".format(self.dstaddr, self.dstport, exc))
        else:
            logger.debug("Closed connection to: %s:%d" % (self.dstaddr, self.dstport))
        self._transport = None
        self.recvbuf = b""
        self.on_close()

    # v2 handshake method
    def _on_data_v2_handshake(self):
        """v2 handshake performed before P2P messages are exchanged (see BIP324). P2PConnection is the initiator
        (in inbound connections to TestNode) and the responder (in outbound connections from TestNode).
        Performed by:
            * initiator using `initiate_v2_handshake()`, `complete_handshake()` and `authenticate_handshake()`
            * responder using `respond_v2_handshake()`, `complete_handshake()` and `authenticate_handshake()`

        `initiate_v2_handshake()` is immediately done by the initiator when the connection is established in
        `connection_made()`. The rest of the initial v2 handshake functions are handled here.
        """
        if not self.v2_state.peer:
            if not self.v2_state.initiating and not self.v2_state.sent_garbage:
                # if the responder hasn't sent garbage yet, the responder is still reading ellswift bytes
                # reads ellswift bytes till the first mismatch from 12 bytes V1_PREFIX
                length, send_handshake_bytes = self.v2_state.respond_v2_handshake(BytesIO(self.recvbuf))
                self.recvbuf = self.recvbuf[length:]
                if send_handshake_bytes == -1:
                    self.v2_state = None
                    return
                elif send_handshake_bytes:
                    logger.debug(f"sending {len(self.v2_state.sent_garbage)} bytes of garbage data")
                    self.send_raw_message(send_handshake_bytes)
                elif send_handshake_bytes == b"":
                    return  # only after send_handshake_bytes are sent can `complete_handshake()` be done

            # `complete_handshake()` reads the remaining ellswift bytes from recvbuf
            # and sends response after deriving shared ECDH secret using received ellswift bytes
            length, response = self.v2_state.complete_handshake(BytesIO(self.recvbuf))
            self.recvbuf = self.recvbuf[length:]
            if response:
                self.send_raw_message(response)
            else:
                return  # only after response is sent can `authenticate_handshake()` be done

        # `self.v2_state.peer` is instantiated only after shared ECDH secret/BIP324 derived keys and ciphers
        # is derived in `complete_handshake()`.
        # so `authenticate_handshake()` which uses the BIP324 derived ciphers gets called after `complete_handshake()`.
        assert self.v2_state.peer
        length, is_mac_auth = self.v2_state.authenticate_handshake(self.recvbuf)
        if not is_mac_auth:
            raise ValueError("invalid v2 mac tag in handshake authentication")
        self.recvbuf = self.recvbuf[length:]
        if self.v2_state.tried_v2_handshake:
            # for v2 outbound connections, send version message immediately after v2 handshake
            if self.p2p_connected_to_node:
                self.send_version()
            # process post-v2-handshake data immediately, if available
            if len(self.recvbuf) > 0:
                self._on_data()

    # Socket read methods

    def data_received(self, t):
        """asyncio callback when data is read from the socket."""
        if len(t) > 0:
            self.recvbuf += t
            if self.supports_v2_p2p and not self.v2_state.tried_v2_handshake:
                self._on_data_v2_handshake()
            else:
                self._on_data()

    def _on_data(self):
        """Try to read P2P messages from the recv buffer.

        This method reads data from the buffer in a loop. It deserializes,
        parses and verifies the P2P header, then passes the P2P payload to
        the on_message callback for processing."""
        try:
            while True:
                if self.supports_v2_p2p:
                    # v2 P2P messages are read
                    msglen, msg = self.v2_state.v2_receive_packet(self.recvbuf)
                    if msglen == -1:
                        raise ValueError("invalid v2 mac tag " + repr(self.recvbuf))
                    elif msglen == 0:  # need to receive more bytes in recvbuf
                        return
                    self.recvbuf = self.recvbuf[msglen:]

                    if msg is None:  # ignore decoy messages
                        return
                    assert msg  # application layer messages (which aren't decoy messages) are non-empty
                    shortid = msg[0]  # 1-byte short message type ID
                    if shortid == 0:
                        # next 12 bytes are interpreted as ASCII message type if shortid is b'\x00'
                        if len(msg) < 13:
                            raise IndexError("msg needs minimum required length of 13 bytes")
                        msgtype = msg[1:13].rstrip(b'\x00')
                        msg = msg[13:]  # msg is set to be payload
                    else:
                        # a 1-byte short message type ID
                        msgtype = SHORTID.get(shortid, f"unknown-{shortid}")
                        msg = msg[1:]
                else:
                    # v1 P2P messages are read
                    if len(self.recvbuf) < 4:
                        return
                    if self.recvbuf[:4] != self.magic_bytes:
                        raise ValueError("magic bytes mismatch: {} != {}".format(repr(self.magic_bytes), repr(self.recvbuf)))
                    if len(self.recvbuf) < 4 + 12 + 4 + 4:
                        return
                    msgtype = self.recvbuf[4:4+12].split(b"\x00", 1)[0]
                    msglen = struct.unpack("<i", self.recvbuf[4+12:4+12+4])[0]
                    checksum = self.recvbuf[4+12+4:4+12+4+4]
                    if len(self.recvbuf) < 4 + 12 + 4 + 4 + msglen:
                        return
                    msg = self.recvbuf[4+12+4+4:4+12+4+4+msglen]
                    th = sha256(msg)
                    h = sha256(th)
                    if checksum != h[:4]:
                        raise ValueError("got bad checksum " + repr(self.recvbuf))
                    self.recvbuf = self.recvbuf[4+12+4+4+msglen:]
                if msgtype not in MESSAGEMAP:
                    raise ValueError("Received unknown msgtype from %s:%d: '%s' %s" % (self.dstaddr, self.dstport, msgtype, repr(msg)))
                f = BytesIO(msg)
                t = MESSAGEMAP[msgtype]()
                t.deserialize(f)
                self._log_message("receive", t)
                self.on_message(t)
        except Exception as e:
            if not self.reconnect:
                logger.exception(f"Error reading message: {repr(e)}")
            raise

    def on_message(self, message):
        """Callback for processing a P2P payload. Must be overridden by derived class."""
        raise NotImplementedError

    # Socket write methods

    def send_without_ping(self, message, is_decoy=False):
        """Send a P2P message over the socket.

        This method takes a P2P payload, builds the P2P header and adds
        the message to the send buffer to be sent over the socket.

        When a message does not lead to a disconnect, send_and_ping is usually
        preferred to send a message. This can help to reduce intermittent test
        failures due to a missing sync. Also, it includes a call to
        sync_with_ping, allowing for concise test code.
        """
        with self._send_lock:
            tmsg = self.build_message(message, is_decoy)
            self._log_message("send", message)
            return self.send_raw_message(tmsg)

    def send_raw_message(self, raw_message_bytes):
        if not self.is_connected:
            raise IOError('Not connected')

        def maybe_write():
            if not self._transport:
                return
            if self._transport.is_closing():
                return
            self._transport.write(raw_message_bytes)
        NetworkThread.network_event_loop.call_soon_threadsafe(maybe_write)

    # Class utility methods

    def build_message(self, message, is_decoy=False):
        """Build a serialized P2P message"""
        msgtype = message.msgtype
        data = message.serialize()
        if self.supports_v2_p2p:
            if msgtype in SHORTID.values():
                tmsg = MSGTYPE_TO_SHORTID.get(msgtype).to_bytes(1, 'big')
            else:
                tmsg = b"\x00"
                tmsg += msgtype
                tmsg += b"\x00" * (12 - len(msgtype))
            tmsg += data
            return self.v2_state.v2_enc_packet(tmsg, ignore=is_decoy)
        else:
            tmsg = self.magic_bytes
            tmsg += msgtype
            tmsg += b"\x00" * (12 - len(msgtype))
            tmsg += len(data).to_bytes(4, "little")
            th = sha256(data)
            h = sha256(th)
            tmsg += h[:4]
            tmsg += data
            return tmsg

    def _log_message(self, direction, msg):
        """Logs a message being sent or received over the connection."""
        if direction == "send":
            log_message = "Send message to "
        elif direction == "receive":
            log_message = "Received message from "
        log_message += "%s:%d: %s" % (self.dstaddr, self.dstport, repr(msg)[:500])
        if len(log_message) > 500:
            log_message += "... (msg truncated)"
        logger.debug(log_message)


class P2PInterface(P2PConnection):
    """A high-level P2P interface class for communicating with a Bitcoin node.

    This class provides high-level callbacks for processing P2P message
    payloads, as well as convenience methods for interacting with the
    node over P2P.

    Individual testcases should subclass this and override the on_* methods
    if they want to alter message handling behaviour."""
    def __init__(self, support_addrv2=False, wtxidrelay=True):
        super().__init__()

        # Track number of messages of each type received.
        # Should be read-only in a test.
        self.message_count = defaultdict(int)

        # Track the most recent message of each type.
        # To wait for a message to be received, pop that message from
        # this and use self.wait_until.
        self.last_message = {}

        # A count of the number of ping messages we've sent to the node
        self.ping_counter = 1

        # The network services received from the peer
        self.nServices = 0

        self.support_addrv2 = support_addrv2

        # If the peer supports wtxid-relay
        self.wtxidrelay = wtxidrelay

    def peer_connect_send_version(self, services):
        # Send a version msg
        vt = msg_version()
        vt.nVersion = P2P_VERSION
        vt.strSubVer = P2P_SUBVERSION
        vt.relay = P2P_VERSION_RELAY
        vt.nServices = services
        vt.addrTo.ip = self.dstaddr
        vt.addrTo.port = self.dstport
        vt.addrFrom.ip = "0.0.0.0"
        vt.addrFrom.port = 0
        self.on_connection_send_msg = vt  # Will be sent in connection_made callback

    def peer_connect(self, *, services=P2P_SERVICES, send_version, **kwargs):
        create_conn = super().peer_connect(**kwargs)

        if send_version:
            self.peer_connect_send_version(services)

        return create_conn

    def peer_accept_connection(self, *args, services=P2P_SERVICES, **kwargs):
        create_conn = super().peer_accept_connection(*args, **kwargs)
        self.peer_connect_send_version(services)

        return create_conn

    # Message receiving methods

    def on_message(self, message):
        """Receive message and dispatch message to appropriate callback.

        We keep a count of how many of each message type has been received
        and the most recent message of each type."""
        with p2p_lock:
            try:
                msgtype = message.msgtype.decode('ascii')
                self.message_count[msgtype] += 1
                self.last_message[msgtype] = message
                getattr(self, 'on_' + msgtype)(message)
            except Exception:
                print("ERROR delivering %s (%s)" % (repr(message), sys.exc_info()[0]))
                raise

    # Callback methods. Can be overridden by subclasses in individual test
    # cases to provide custom message handling behaviour.

    def on_open(self):
        pass

    def on_close(self):
        pass

    def on_addr(self, message): pass
    def on_addrv2(self, message): pass
    def on_block(self, message): pass
    def on_blocktxn(self, message): pass
    def on_cfcheckpt(self, message): pass
    def on_cfheaders(self, message): pass
    def on_cfilter(self, message): pass
    def on_cmpctblock(self, message): pass
    def on_feefilter(self, message): pass
    def on_filteradd(self, message): pass
    def on_filterclear(self, message): pass
    def on_filterload(self, message): pass
    def on_getaddr(self, message): pass
    def on_getblocks(self, message): pass
    def on_getblocktxn(self, message): pass
    def on_getdata(self, message): pass
    def on_getheaders(self, message): pass
    def on_headers(self, message): pass
    def on_mempool(self, message): pass
    def on_merkleblock(self, message): pass
    def on_notfound(self, message): pass
    def on_pong(self, message): pass
    def on_sendaddrv2(self, message): pass
    def on_sendcmpct(self, message): pass
    def on_sendheaders(self, message): pass
    def on_sendtxrcncl(self, message): pass
    def on_tx(self, message): pass
    def on_wtxidrelay(self, message): pass

    def on_inv(self, message):
        want = msg_getdata()
        for i in message.inv:
            if i.type != 0:
                want.inv.append(i)
        if len(want.inv):
            self.send_without_ping(want)

    def on_ping(self, message):
        self.send_without_ping(msg_pong(message.nonce))

    def on_verack(self, message):
        pass

    def on_version(self, message):
        assert message.nVersion >= MIN_P2P_VERSION_SUPPORTED, "Version {} received. Test framework only supports versions greater than {}".format(message.nVersion, MIN_P2P_VERSION_SUPPORTED)
        # for inbound connections, reply to version with own version message
        # (could be due to v1 reconnect after a failed v2 handshake)
        if not self.p2p_connected_to_node:
            self.send_version()
            self.reconnect = False
        if message.nVersion >= 70016 and self.wtxidrelay:
            self.send_without_ping(msg_wtxidrelay())
        if self.support_addrv2:
            self.send_without_ping(msg_sendaddrv2())
        self.send_without_ping(msg_verack())
        self.nServices = message.nServices
        self.relay = message.relay
        if self.p2p_connected_to_node:
            self.send_without_ping(msg_getaddr())

    # Connection helper methods

    def wait_until(self, test_function_in, *, timeout=60, check_connected=True, check_interval=0.05):
        def test_function():
            if check_connected:
                assert self.is_connected
            return test_function_in()

        wait_until_helper_internal(test_function, timeout=timeout, lock=p2p_lock, timeout_factor=self.timeout_factor, check_interval=check_interval)

    def wait_for_connect(self, *, timeout=60):
        test_function = lambda: self.is_connected
        self.wait_until(test_function, timeout=timeout, check_connected=False)

    def wait_for_disconnect(self, *, timeout=60):
        test_function = lambda: not self.is_connected
        self.wait_until(test_function, timeout=timeout, check_connected=False)

    def wait_for_reconnect(self, *, timeout=60):
        def test_function():
            return self.is_connected and self.last_message.get('version') and not self.supports_v2_p2p
        self.wait_until(test_function, timeout=timeout, check_connected=False)

    # Message receiving helper methods

    def wait_for_tx(self, txid, *, timeout=60):
        def test_function():
            if not self.last_message.get('tx'):
                return False
            return self.last_message['tx'].tx.txid_hex == txid

        self.wait_until(test_function, timeout=timeout)

    def wait_for_block(self, blockhash, *, timeout=60):
        def test_function():
            return self.last_message.get("block") and self.last_message["block"].block.rehash() == blockhash

        self.wait_until(test_function, timeout=timeout)

    def wait_for_header(self, blockhash, *, timeout=60):
        def test_function():
            last_headers = self.last_message.get('headers')
            if not last_headers:
                return False
            return last_headers.headers[0].rehash() == int(blockhash, 16)

        self.wait_until(test_function, timeout=timeout)

    def wait_for_merkleblock(self, blockhash, *, timeout=60):
        def test_function():
            last_filtered_block = self.last_message.get('merkleblock')
            if not last_filtered_block:
                return False
            return last_filtered_block.merkleblock.header.rehash() == int(blockhash, 16)

        self.wait_until(test_function, timeout=timeout)

    def wait_for_getdata(self, hash_list, *, timeout=60):
        """Waits for a getdata message.

        The object hashes in the inventory vector must match the provided hash_list."""
        def test_function():
            last_data = self.last_message.get("getdata")
            if not last_data:
                return False
            return [x.hash for x in last_data.inv] == hash_list

        self.wait_until(test_function, timeout=timeout)

    def wait_for_getheaders(self, block_hash=None, *, timeout=60):
        """Waits for a getheaders message containing a specific block hash.

        If no block hash is provided, checks whether any getheaders message has been received by the node."""
        def test_function():
            last_getheaders = self.last_message.pop("getheaders", None)
            if block_hash is None:
                return last_getheaders
            if last_getheaders is None:
                return False
            return block_hash == last_getheaders.locator.vHave[0]

        self.wait_until(test_function, timeout=timeout)

    def wait_for_inv(self, expected_inv, *, timeout=60):
        """Waits for an INV message and checks that the first inv object in the message was as expected."""
        if len(expected_inv) > 1:
            raise NotImplementedError("wait_for_inv() will only verify the first inv object")

        def test_function():
            return self.last_message.get("inv") and \
                                self.last_message["inv"].inv[0].type == expected_inv[0].type and \
                                self.last_message["inv"].inv[0].hash == expected_inv[0].hash

        self.wait_until(test_function, timeout=timeout)

    def wait_for_verack(self, *, timeout=60):
        def test_function():
            return "verack" in self.last_message

        self.wait_until(test_function, timeout=timeout)

    # Message sending helper functions

    def send_version(self):
        if self.on_connection_send_msg:
            self.send_without_ping(self.on_connection_send_msg)
            self.on_connection_send_msg = None  # Never used again

    def send_and_ping(self, message, *, timeout=60):
        self.send_without_ping(message)
        self.sync_with_ping(timeout=timeout)

    def sync_with_ping(self, *, timeout=60):
        """Ensure ProcessMessages and SendMessages is called on this connection"""
        # Sending two pings back-to-back, requires that the node calls
        # `ProcessMessage` twice, and thus ensures `SendMessages` must have
        # been called at least once
        self.send_without_ping(msg_ping(nonce=0))
        self.send_without_ping(msg_ping(nonce=self.ping_counter))

        def test_function():
            return self.last_message.get("pong") and self.last_message["pong"].nonce == self.ping_counter

        self.wait_until(test_function, timeout=timeout)
        self.ping_counter += 1


# One lock for synchronizing all data access between the network event loop (see
# NetworkThread below) and the thread running the test logic.  For simplicity,
# P2PConnection acquires this lock whenever delivering a message to a P2PInterface.
# This lock should be acquired in the thread running the test logic to synchronize
# access to any data shared with the P2PInterface or P2PConnection.
p2p_lock = threading.Lock()


class NetworkThread(threading.Thread):
    network_event_loop = None

    def __init__(self):
        super().__init__(name="NetworkThread")
        # There is only one event loop and no more than one thread must be created
        assert not self.network_event_loop

        NetworkThread.listeners = {}
        NetworkThread.protos = {}
        if platform.system() == 'Windows':
            asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
        NetworkThread.network_event_loop = asyncio.new_event_loop()

    def run(self):
        """Start the network thread."""
        self.network_event_loop.run_forever()

    def close(self, *, timeout=10):
        """Close the connections and network event loop."""
        self.network_event_loop.call_soon_threadsafe(self.network_event_loop.stop)
        wait_until_helper_internal(lambda: not self.network_event_loop.is_running(), timeout=timeout)
        self.network_event_loop.close()
        self.join(timeout)
        # Safe to remove event loop.
        NetworkThread.network_event_loop = None

    @classmethod
    def listen(cls, p2p, callback, port=None, addr=None, idx=1):
        """ Ensure a listening server is running on the given port, and run the
        protocol specified by `p2p` on the next connection to it. Once ready
        for connections, call `callback`."""

        if port is None:
            assert 0 < idx <= MAX_NODES
            port = p2p_port(MAX_NODES - idx)
        if addr is None:
            addr = '127.0.0.1'

        def exception_handler(loop, context):
            if not p2p.reconnect:
                loop.default_exception_handler(context)

        cls.network_event_loop.set_exception_handler(exception_handler)
        coroutine = cls.create_listen_server(addr, port, callback, p2p)
        cls.network_event_loop.call_soon_threadsafe(cls.network_event_loop.create_task, coroutine)

    @classmethod
    async def create_listen_server(cls, addr, port, callback, proto):
        def peer_protocol():
            """Returns a function that does the protocol handling for a new
            connection. To allow different connections to have different
            behaviors, the protocol function is first put in the cls.protos
            dict. When the connection is made, the function removes the
            protocol function from that dict, and returns it so the event loop
            can start executing it."""
            response = cls.protos.get((addr, port))
            # remove protocol function from dict only when reconnection doesn't need to happen/already happened
            if not proto.reconnect:
                cls.protos[(addr, port)] = None
            return response

        if (addr, port) not in cls.listeners:
            # When creating a listener on a given (addr, port) we only need to
            # do it once. If we want different behaviors for different
            # connections, we can accomplish this by providing different
            # `proto` functions

            listener = await cls.network_event_loop.create_server(peer_protocol, addr, port)
            logger.debug("Listening server on %s:%d should be started" % (addr, port))
            cls.listeners[(addr, port)] = listener

        cls.protos[(addr, port)] = proto
        callback(addr, port)


class P2PDataStore(P2PInterface):
    """A P2P data store class.

    Keeps a block and transaction store and responds correctly to getdata and getheaders requests."""

    def __init__(self):
        super().__init__()
        # store of blocks. key is block hash, value is a CBlock object
        self.block_store = {}
        self.last_block_hash = ''
        # store of txs. key is txid, value is a CTransaction object
        self.tx_store = {}
        self.getdata_requests = []

    def on_getdata(self, message):
        """Check for the tx/block in our stores and if found, reply with MSG_TX or MSG_BLOCK."""
        for inv in message.inv:
            self.getdata_requests.append(inv.hash)
            invtype = inv.type & MSG_TYPE_MASK
            if (invtype == MSG_TX or invtype == MSG_WTX) and inv.hash in self.tx_store.keys():
                self.send_without_ping(msg_tx(self.tx_store[inv.hash]))
            elif invtype == MSG_BLOCK and inv.hash in self.block_store.keys():
                self.send_without_ping(msg_block(self.block_store[inv.hash]))
            else:
                logger.debug('getdata message type {} received.'.format(hex(inv.type)))

    def on_getheaders(self, message):
        """Search back through our block store for the locator, and reply with a headers message if found."""

        locator, hash_stop = message.locator, message.hashstop

        # Assume that the most recent block added is the tip
        if not self.block_store:
            return

        headers_list = [self.block_store[self.last_block_hash]]
        while headers_list[-1].sha256 not in locator.vHave:
            # Walk back through the block store, adding headers to headers_list
            # as we go.
            prev_block_hash = headers_list[-1].hashPrevBlock
            if prev_block_hash in self.block_store:
                prev_block_header = CBlockHeader(self.block_store[prev_block_hash])
                headers_list.append(prev_block_header)
                if prev_block_header.sha256 == hash_stop:
                    # if this is the hashstop header, stop here
                    break
            else:
                logger.debug('block hash {} not found in block store'.format(hex(prev_block_hash)))
                break

        # Truncate the list if there are too many headers
        headers_list = headers_list[:-MAX_HEADERS_RESULTS - 1:-1]
        response = msg_headers(headers_list)

        if response is not None:
            self.send_without_ping(response)

    def send_blocks_and_test(self, blocks, node, *, success=True, force_send=False, reject_reason=None, expect_disconnect=False, timeout=60, is_decoy=False):
        """Send blocks to test node and test whether the tip advances.

         - add all blocks to our block_store
         - send a headers message for the final block
         - the on_getheaders handler will ensure that any getheaders are responded to
         - if force_send is False: wait for getdata for each of the blocks. The on_getdata handler will
           ensure that any getdata messages are responded to. Otherwise send the full block unsolicited.
         - if success is True: assert that the node's tip advances to the most recent block
         - if success is False: assert that the node's tip doesn't advance
         - if reject_reason is set: assert that the correct reject message is logged"""

        with p2p_lock:
            for block in blocks:
                self.block_store[block.sha256] = block
                self.last_block_hash = block.sha256

        reject_reason = [reject_reason] if reject_reason else []
        with node.assert_debug_log(expected_msgs=reject_reason):
            if is_decoy:  # since decoy messages are ignored by the recipient - no need to wait for response
                force_send = True
            if force_send:
                for b in blocks:
                    self.send_without_ping(msg_block(block=b), is_decoy)
            else:
                self.send_without_ping(msg_headers([CBlockHeader(block) for block in blocks]))
                self.wait_until(
                    lambda: blocks[-1].sha256 in self.getdata_requests,
                    timeout=timeout,
                    check_connected=success,
                )

            if expect_disconnect:
                self.wait_for_disconnect(timeout=timeout)
            else:
                self.sync_with_ping(timeout=timeout)

            if success:
                self.wait_until(lambda: node.getbestblockhash() == blocks[-1].hash, timeout=timeout)
            else:
                assert_not_equal(node.getbestblockhash(), blocks[-1].hash)

    def send_txs_and_test(self, txs, node, *, success=True, expect_disconnect=False, reject_reason=None):
        """Send txs to test node and test whether they're accepted to the mempool.

         - add all txs to our tx_store
         - send tx messages for all txs
         - if success is True/False: assert that the txs are/are not accepted to the mempool
         - if expect_disconnect is True: Skip the sync with ping
         - if reject_reason is set: assert that the correct reject message is logged."""

        with p2p_lock:
            for tx in txs:
                self.tx_store[tx.txid_int] = tx

        reject_reason = [reject_reason] if reject_reason else []
        with node.assert_debug_log(expected_msgs=reject_reason):
            for tx in txs:
                self.send_without_ping(msg_tx(tx))

            if expect_disconnect:
                self.wait_for_disconnect()
            else:
                self.sync_with_ping()

            raw_mempool = node.getrawmempool()
            if success:
                # Check that all txs are now in the mempool
                for tx in txs:
                    assert tx.txid_hex in raw_mempool, "{} not found in mempool".format(tx.txid_hex)
            else:
                # Check that none of the txs are now in the mempool
                for tx in txs:
                    assert tx.txid_hex not in raw_mempool, "{} tx found in mempool".format(tx.txid_hex)

class P2PTxInvStore(P2PInterface):
    """A P2PInterface which stores a count of how many times each txid has been announced."""
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.tx_invs_received = defaultdict(int)

    def on_inv(self, message):
        super().on_inv(message) # Send getdata in response.
        # Store how many times invs have been received for each tx.
        for i in message.inv:
            if (i.type == MSG_TX) or (i.type == MSG_WTX):
                # save txid
                self.tx_invs_received[i.hash] += 1

    def get_invs(self):
        with p2p_lock:
            return list(self.tx_invs_received.keys())

    def wait_for_broadcast(self, txns, *, timeout=60):
        """Waits for the txns (list of txids) to complete initial broadcast.
        The mempool should mark unbroadcast=False for these transactions.
        """
        # Wait until invs have been received (and getdatas sent) for each txid.
        self.wait_until(lambda: set(self.tx_invs_received.keys()) == set([int(tx, 16) for tx in txns]), timeout=timeout)
        # Flush messages and wait for the getdatas to be processed
        self.sync_with_ping()
