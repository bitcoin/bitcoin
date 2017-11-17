#!/usr/bin/env python3
# Copyright (c) 2010 ArtForz -- public domain half-a-node
# Copyright (c) 2012 Jeff Garzik
# Copyright (c) 2010-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Bitcoin P2P network half-a-node.

This python code was modified from ArtForz' public domain  half-a-node, as
found in the mini-node branch of http://github.com/jgarzik/pynode.

NodeConn: an object which manages p2p connectivity to a bitcoin node
NodeConnCB: a base class that describes the interface for receiving
            callbacks with network messages from a NodeConn
"""
import asyncore
from collections import defaultdict
from io import BytesIO
import logging
import socket
import struct
import sys
import time
from threading import RLock, Thread

from test_framework.messages import *

logger = logging.getLogger("TestFramework.mininode")

MESSAGEMAP = {
    b"addr": msg_addr,
    b"block": msg_block,
    b"blocktxn": msg_blocktxn,
    b"cmpctblock": msg_cmpctblock,
    b"feefilter": msg_feefilter,
    b"getaddr": msg_getaddr,
    b"getblocks": msg_getblocks,
    b"getblocktxn": msg_getblocktxn,
    b"getdata": msg_getdata,
    b"getheaders": msg_getheaders,
    b"headers": msg_headers,
    b"inv": msg_inv,
    b"mempool": msg_mempool,
    b"ping": msg_ping,
    b"pong": msg_pong,
    b"reject": msg_reject,
    b"sendcmpct": msg_sendcmpct,
    b"sendheaders": msg_sendheaders,
    b"tx": msg_tx,
    b"verack": msg_verack,
    b"version": msg_version,
}

MAGIC_BYTES = {
    "mainnet": b"\xf9\xbe\xb4\xd9",   # mainnet
    "testnet3": b"\x0b\x11\x09\x07",  # testnet3
    "regtest": b"\xfa\xbf\xb5\xda",   # regtest
}

class NodeConnCB():
    """Callback and helper functions for P2P connection to a bitcoind node.

    Individual testcases should subclass this and override the on_* methods
    if they want to alter message handling behaviour."""
    def __init__(self):
        # Track whether we have a P2P connection open to the node
        self.connected = False
        self.connection = None

        # Track number of messages of each type received and the most recent
        # message of each type
        self.message_count = defaultdict(int)
        self.last_message = {}

        # A count of the number of ping messages we've sent to the node
        self.ping_counter = 1

    # Message receiving methods

    def deliver(self, conn, message):
        """Receive message and dispatch message to appropriate callback.

        We keep a count of how many of each message type has been received
        and the most recent message of each type."""
        with mininode_lock:
            try:
                command = message.command.decode('ascii')
                self.message_count[command] += 1
                self.last_message[command] = message
                getattr(self, 'on_' + command)(conn, message)
            except:
                print("ERROR delivering %s (%s)" % (repr(message),
                                                    sys.exc_info()[0]))
                raise

    # Callback methods. Can be overridden by subclasses in individual test
    # cases to provide custom message handling behaviour.

    def on_open(self, conn):
        self.connected = True

    def on_close(self, conn):
        self.connected = False
        self.connection = None

    def on_addr(self, conn, message): pass
    def on_block(self, conn, message): pass
    def on_blocktxn(self, conn, message): pass
    def on_cmpctblock(self, conn, message): pass
    def on_feefilter(self, conn, message): pass
    def on_getaddr(self, conn, message): pass
    def on_getblocks(self, conn, message): pass
    def on_getblocktxn(self, conn, message): pass
    def on_getdata(self, conn, message): pass
    def on_getheaders(self, conn, message): pass
    def on_headers(self, conn, message): pass
    def on_mempool(self, conn): pass
    def on_pong(self, conn, message): pass
    def on_reject(self, conn, message): pass
    def on_sendcmpct(self, conn, message): pass
    def on_sendheaders(self, conn, message): pass
    def on_tx(self, conn, message): pass

    def on_inv(self, conn, message):
        want = msg_getdata()
        for i in message.inv:
            if i.type != 0:
                want.inv.append(i)
        if len(want.inv):
            conn.send_message(want)

    def on_ping(self, conn, message):
        conn.send_message(msg_pong(message.nonce))

    def on_verack(self, conn, message):
        self.verack_received = True

    def on_version(self, conn, message):
        assert message.nVersion >= MIN_VERSION_SUPPORTED, "Version {} received. Test framework only supports versions greater than {}".format(message.nVersion, MIN_VERSION_SUPPORTED)
        conn.send_message(msg_verack())
        conn.nServices = message.nServices

    # Connection helper methods

    def add_connection(self, conn):
        self.connection = conn

    def wait_for_disconnect(self, timeout=60):
        test_function = lambda: not self.connected
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    # Message receiving helper methods

    def wait_for_block(self, blockhash, timeout=60):
        test_function = lambda: self.last_message.get("block") and self.last_message["block"].block.rehash() == blockhash
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    def wait_for_getdata(self, timeout=60):
        test_function = lambda: self.last_message.get("getdata")
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    def wait_for_getheaders(self, timeout=60):
        test_function = lambda: self.last_message.get("getheaders")
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    def wait_for_inv(self, expected_inv, timeout=60):
        """Waits for an INV message and checks that the first inv object in the message was as expected."""
        if len(expected_inv) > 1:
            raise NotImplementedError("wait_for_inv() will only verify the first inv object")
        test_function = lambda: self.last_message.get("inv") and \
                                self.last_message["inv"].inv[0].type == expected_inv[0].type and \
                                self.last_message["inv"].inv[0].hash == expected_inv[0].hash
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    def wait_for_verack(self, timeout=60):
        test_function = lambda: self.message_count["verack"]
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    # Message sending helper functions

    def send_message(self, message):
        if self.connection:
            self.connection.send_message(message)
        else:
            logger.error("Cannot send message. No connection to node!")

    def send_and_ping(self, message):
        self.send_message(message)
        self.sync_with_ping()

    # Sync up with the node
    def sync_with_ping(self, timeout=60):
        self.send_message(msg_ping(nonce=self.ping_counter))
        test_function = lambda: self.last_message.get("pong") and self.last_message["pong"].nonce == self.ping_counter
        wait_until(test_function, timeout=timeout, lock=mininode_lock)
        self.ping_counter += 1

class NodeConn(asyncore.dispatcher):
    """The actual NodeConn class

    This class provides an interface for a p2p connection to a specified node."""

    def __init__(self, dstaddr, dstport, callback, net="regtest", services=NODE_NETWORK|NODE_WITNESS, send_version=True):
        asyncore.dispatcher.__init__(self, map=mininode_socket_map)
        self.dstaddr = dstaddr
        self.dstport = dstport
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.sendbuf = b""
        self.recvbuf = b""
        self.last_sent = 0
        self.state = "connecting"
        self.network = net
        self.cb = callback
        self.disconnect = False
        self.nServices = 0

        if send_version:
            # stuff version msg into sendbuf
            vt = msg_version()
            vt.nServices = services
            vt.addrTo.ip = self.dstaddr
            vt.addrTo.port = self.dstport
            vt.addrFrom.ip = "0.0.0.0"
            vt.addrFrom.port = 0
            self.send_message(vt, True)

        logger.info('Connecting to Bitcoin Node: %s:%d' % (self.dstaddr, self.dstport))

        try:
            self.connect((dstaddr, dstport))
        except:
            self.handle_close()

    # Connection and disconnection methods

    def handle_connect(self):
        if self.state != "connected":
            logger.debug("Connected & Listening: %s:%d" % (self.dstaddr, self.dstport))
            self.state = "connected"
            self.cb.on_open(self)

    def handle_close(self):
        logger.debug("Closing connection to: %s:%d" % (self.dstaddr, self.dstport))
        self.state = "closed"
        self.recvbuf = b""
        self.sendbuf = b""
        try:
            self.close()
        except:
            pass
        self.cb.on_close(self)

    def disconnect_node(self):
        """ Disconnect the p2p connection.

        Called by the test logic thread. Causes the p2p connection
        to be disconnected on the next iteration of the asyncore loop."""
        self.disconnect = True

    # Socket read methods

    def readable(self):
        return True

    def handle_read(self):
        t = self.recv(8192)
        if len(t) > 0:
            self.recvbuf += t
            self.got_data()

    def got_data(self):
        try:
            while True:
                if len(self.recvbuf) < 4:
                    return
                if self.recvbuf[:4] != MAGIC_BYTES[self.network]:
                    raise ValueError("got garbage %s" % repr(self.recvbuf))
                if len(self.recvbuf) < 4 + 12 + 4 + 4:
                    return
                command = self.recvbuf[4:4+12].split(b"\x00", 1)[0]
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
                if command not in MESSAGEMAP:
                    raise ValueError("Received unknown command from %s:%d: '%s' %s" % (self.dstaddr, self.dstport, command, repr(msg)))
                f = BytesIO(msg)
                t = MESSAGEMAP[command]()
                t.deserialize(f)
                self.got_message(t)
        except Exception as e:
            logger.exception('Error reading message:', repr(e))
            raise

    def got_message(self, message):
        if self.last_sent + 30 * 60 < time.time():
            self.send_message(MESSAGEMAP[b'ping']())
        self._log_message("receive", message)
        self.cb.deliver(self, message)

    # Socket write methods

    def writable(self):
        with mininode_lock:
            pre_connection = self.state == "connecting"
            length = len(self.sendbuf)
        return (length > 0 or pre_connection)

    def handle_write(self):
        with mininode_lock:
            # asyncore does not expose socket connection, only the first read/write
            # event, thus we must check connection manually here to know when we
            # actually connect
            if self.state == "connecting":
                self.handle_connect()
            if not self.writable():
                return

            try:
                sent = self.send(self.sendbuf)
            except:
                self.handle_close()
                return
            self.sendbuf = self.sendbuf[sent:]

    def send_message(self, message, pushbuf=False):
        if self.state != "connected" and not pushbuf:
            raise IOError('Not connected, no pushbuf')
        self._log_message("send", message)
        command = message.command
        data = message.serialize()
        tmsg = MAGIC_BYTES[self.network]
        tmsg += command
        tmsg += b"\x00" * (12 - len(command))
        tmsg += struct.pack("<I", len(data))
        th = sha256(data)
        h = sha256(th)
        tmsg += h[:4]
        tmsg += data
        with mininode_lock:
            if (len(self.sendbuf) == 0 and not pushbuf):
                try:
                    sent = self.send(tmsg)
                    self.sendbuf = tmsg[sent:]
                except BlockingIOError:
                    self.sendbuf = tmsg
            else:
                self.sendbuf += tmsg
            self.last_sent = time.time()

    # Class utility methods

    def _log_message(self, direction, msg):
        if direction == "send":
            log_message = "Send message to "
        elif direction == "receive":
            log_message = "Received message from "
        log_message += "%s:%d: %s" % (self.dstaddr, self.dstport, repr(msg)[:500])
        if len(log_message) > 500:
            log_message += "... (msg truncated)"
        logger.debug(log_message)


# Keep our own socket map for asyncore, so that we can track disconnects
# ourselves (to workaround an issue with closing an asyncore socket when
# using select)
mininode_socket_map = dict()

# One lock for synchronizing all data access between the networking thread (see
# NetworkThread below) and the thread running the test logic.  For simplicity,
# NodeConn acquires this lock whenever delivering a message to a NodeConnCB,
# and whenever adding anything to the send buffer (in send_message()).  This
# lock should be acquired in the thread running the test logic to synchronize
# access to any data shared with the NodeConnCB or NodeConn.
mininode_lock = RLock()

class NetworkThread(Thread):
    def run(self):
        while mininode_socket_map:
            # We check for whether to disconnect outside of the asyncore
            # loop to workaround the behavior of asyncore when using
            # select
            disconnected = []
            for fd, obj in mininode_socket_map.items():
                if obj.disconnect:
                    disconnected.append(obj)
            [obj.handle_close() for obj in disconnected]
            asyncore.loop(0.1, use_poll=True, map=mininode_socket_map, count=1)
        logger.debug("Network thread closing")
