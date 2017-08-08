#!/usr/bin/env python3
# Copyright (c) 2010 ArtForz -- public domain half-a-node
# Copyright (c) 2012 Jeff Garzik
# Copyright (c) 2010-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# mininode.py - Bitcoin P2P network half-a-node
#
# This python code was modified from ArtForz' public domain  half-a-node, as
# found in the mini-node branch of http://github.com/jgarzik/pynode.
#
# NodeConn: an object which manages p2p connectivity to a bitcoin node
# NodeConnCB: a base class that describes the interface for receiving
#             callbacks with network messages from a NodeConn
# CBlock, CTransaction, CBlockHeader, CTxIn, CTxOut, etc....:
#     data structures that should map to corresponding structures in
#     bitcoin/primitives
# msg_block, msg_tx, msg_headers, etc.:
#     data structures that represent network messages
# ser_*, deser_*: functions that handle serialization/deserialization

import pdb
import struct
import socket
import asyncore
import time
import sys
import random
from binascii import hexlify, unhexlify
from io import BytesIO
from codecs import encode
import hashlib
from threading import RLock
from threading import Thread
import logging
import copy

from .nodemessages import *
from .bumessages import *

BIP0031_VERSION = 60000

MAX_INV_SZ = 50000
MAX_BLOCK_SIZE = 1000000

class MiniNodeError(Exception):
    pass

class DisconnectedError(MiniNodeError):
    pass

# Keep our own socket map for asyncore, so that we can track disconnects
# ourselves (to workaround an issue with closing an asyncore socket when
# using select)
mininode_socket_map = dict()

# This is what a callback should look like for NodeConn
# Reimplement the on_* functions to provide handling for events
class NodeConnCB(object):
    def __init__(self):
        self.verack_received = False
        # deliver_sleep_time is helpful for debugging race conditions in p2p
        # tests; it causes message delivery to sleep for the specified time
        # before acquiring the global lock and delivering the next message.
        self.deliver_sleep_time = None
        self.disconnected = False

    def set_deliver_sleep_time(self, value):
        with mininode_lock:
            self.deliver_sleep_time = value

    def get_deliver_sleep_time(self):
        with mininode_lock:
            return self.deliver_sleep_time

    # Spin until verack message is received from the node.
    # Tests may want to use this as a signal that the test can begin.
    # This can be called from the testing thread, so it needs to acquire the
    # global lock.
    def wait_for_verack(self):
        while True:
            if self.disconnected:
                raise DisconnectedError()
            with mininode_lock:
                if self.verack_received:
                    return
            time.sleep(0.05)

    def deliver(self, conn, message):
        deliver_sleep = self.get_deliver_sleep_time()
        if deliver_sleep is not None:
            time.sleep(deliver_sleep)
        with mininode_lock:
            fn = 'on_' + message.command.decode('ascii')
            try:
                getattr(self, fn)(conn, message)
            except:
                print("ERROR delivering %s (%s) to %s" % (repr(message), sys.exc_info()[0], fn))

    def on_version(self, conn, message):
        if message.nVersion >= 209:
            conn.send_message(msg_verack())
        conn.ver_send = min(MY_VERSION, message.nVersion)
        if message.nVersion < 209:
            conn.ver_recv = conn.ver_send

    def on_verack(self, conn, message):
        conn.ver_recv = conn.ver_send
        self.verack_received = True

    def on_inv(self, conn, message):
        want = msg_getdata()
        for i in message.inv:
            if i.type != 0:
                want.inv.append(i)
        if len(want.inv):
            conn.send_message(want)

    def on_addr(self, conn, message): pass

    def on_alert(self, conn, message): pass

    def on_getdata(self, conn, message): pass

    def on_getblocks(self, conn, message): pass

    def on_tx(self, conn, message): pass

    def on_block(self, conn, message): pass

    def on_getaddr(self, conn, message): pass

    def on_headers(self, conn, message): pass

    def on_getheaders(self, conn, message): pass

    def on_ping(self, conn, message):
        if conn.ver_send > BIP0031_VERSION:
            conn.send_message(msg_pong(message.nonce))

    def on_reject(self, conn, message): pass

    def on_close(self, conn):
        self.disconnected=True
        pass

    def on_mempool(self, conn): pass

    def on_pong(self, conn, message): pass

# More useful callbacks and functions for NodeConnCB's which have a single NodeConn


class SingleNodeConnCB(NodeConnCB):
    def __init__(self):
        NodeConnCB.__init__(self)
        self.connection = None
        self.ping_counter = 1
        self.last_pong = msg_pong()

    def add_connection(self, conn):
        self.connection = conn

    # Wrapper for the NodeConn's send_message function
    def send_message(self, message):
        self.connection.send_message(message)

    def on_pong(self, conn, message):
        self.last_pong = message

    # Sync up with the node
    def sync_with_ping(self, timeout=30):
        def received_pong():
            return (self.last_pong.nonce == self.ping_counter)
        self.send_message(msg_ping(nonce=self.ping_counter))
        success = wait_until(received_pong, timeout)
        self.ping_counter += 1
        return success


def dupdate(x, y):
    x.update(y)
    return x


class MsgAnnotater:
    def __init__(self):
        self.idx = 0

    def annotate(self, msg, conn):
        msg.idx = self.idx
        msg.offset = conn.curIndex
        self.idx += 1
        return msg

# The actual NodeConn class
# This class provides an interface for a p2p connection to a specified node


class NodeConn(asyncore.dispatcher):
    messagemap = dupdate({
        b"version": msg_version,
        b"verack": msg_verack,
        b"addr": msg_addr,
        b"alert": msg_alert,
        b"inv": msg_inv,
        b"getdata": msg_getdata,
        b"getblocks": msg_getblocks,
        b"tx": msg_tx,
        b"block": msg_block,
        b"getaddr": msg_getaddr,
        b"ping": msg_ping,
        b"pong": msg_pong,
        b"headers": msg_headers,
        b"getheaders": msg_getheaders,
        b"reject": msg_reject,
        b"mempool": msg_mempool,
        b"sendheaders": msg_sendheaders,
    }, bumessagemap)

    BTC_MAGIC_BYTES = {
        "mainnet": b"\xf9\xbe\xb4\xd9",   # mainnet
        "testnet3": b"\x0b\x11\x09\x07",  # testnet3
        "regtest": b"\xfa\xbf\xb5\xda"    # regtest
        }

    CASH_MAGIC_BYTES = {
        "mainnet": b"\xe3\xe1\xf3\xe8",
        "testnet3": b"\xf4\xe5\xf3\xf4",
        "regtest": b"\xda\xb5\xbf\xfa",
    }

    def __init__(self, dstaddr, dstport, rpc, callback, net="regtest", services=1, bitcoinCash=False):
        self.bitcoinCash = bitcoinCash
        if self.bitcoinCash:
            self.MAGIC_BYTES = self.CASH_MAGIC_BYTES
        else:
            self.MAGIC_BYTES = self.BTC_MAGIC_BYTES
        asyncore.dispatcher.__init__(self, map=mininode_socket_map)
        self.log = logging.getLogger("NodeConn(%s:%d)" % (dstaddr, dstport))
        self.dstaddr = dstaddr
        self.dstport = dstport
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sendbuf = b""
        self.recvbuf = b""
        self.ver_send = 209
        self.ver_recv = 209
        self.last_sent = 0
        self.state = "connecting"
        self.network = net
        self.cb = callback
        self.disconnect = False
        self.curIndex = 0
        # stuff version msg into sendbuf
        vt = msg_version()
        vt.nServices = services
        vt.addrTo.ip = self.dstaddr
        vt.addrTo.port = self.dstport
        vt.addrFrom.ip = "0.0.0.0"
        vt.addrFrom.port = 0
        self.send_message(vt, True)
        print('MiniNode: Connecting to Bitcoin Node IP # ' + dstaddr + ':'
              + str(dstport))
        try:
            self.connect((dstaddr, dstport))
        except:
            self.handle_close()
        self.rpc = rpc

    def show_debug_msg(self, msg):
        self.log.debug(msg)

    def handle_connect(self):
        self.show_debug_msg("MiniNode: Connected & Listening: \n")
        self.state = "connected"

    def handle_close(self):
        self.show_debug_msg("MiniNode: Closing Connection to %s:%d... "
                            % (self.dstaddr, self.dstport))
        self.state = "closed"
        self.recvbuf = b""
        self.sendbuf = b""
        try:
            self.close()
        except:
            pass
        self.cb.on_close(self)

    def parse_messages(self, buffer):
        if not type(buffer) == type(b""):  # if not a buffer its a file
            buffer = buffer.read()
        tmp = self.cb
        ret = []
        ann = MsgAnnotater()
        self.cb = type("", (), {"deliver": lambda self, conn, msg: ret.append(ann.annotate(msg, conn))})()
        self.inject_data(buffer)
        self.cp = tmp
        return ret

    def inject_data(self, buffer):
        self.recvbuf += buffer
        self.got_data()

    def handle_read(self):
        try:
            t = self.recv(8192)
            if len(t) > 0:
                self.recvbuf += t
                self.got_data()
        except:
            pass

    def readable(self):
        return True

    def writable(self):
        with mininode_lock:
            length = len(self.sendbuf)
        return (length > 0)

    def handle_write(self):
        with mininode_lock:
            try:
                sent = self.send(self.sendbuf)
            except:
                self.handle_close()
                return
            self.sendbuf = self.sendbuf[sent:]

    def got_data(self):
        self.recvBufLen = len(self.recvbuf)
        try:
            while True:
                nowLen = len(self.recvbuf)
                self.curIndex += (self.recvBufLen - nowLen)
                self.recvBufLen = nowLen
                if nowLen < 4:
                    return
                if (self.recvbuf[:4] != self.MAGIC_BYTES[self.network]) and (self.recvbuf[:4] != self.BTC_MAGIC_BYTES[self.network]):
                    raise ValueError("got garbage %s" % repr(self.recvbuf))
                if self.ver_recv < 209:
                    if len(self.recvbuf) < 4 + 12 + 4:
                        return
                    command = self.recvbuf[4:4 + 12].split(b"\x00", 1)[0]
                    msglen = struct.unpack("<i", self.recvbuf[4 + 12:4 + 12 + 4])[0]
                    checksum = None
                    if len(self.recvbuf) < 4 + 12 + 4 + msglen:
                        return
                    msg = self.recvbuf[4 + 12 + 4:4 + 12 + 4 + msglen]
                    self.recvbuf = self.recvbuf[4 + 12 + 4 + msglen:]
                else:
                    if len(self.recvbuf) < 4 + 12 + 4 + 4:
                        return
                    command = self.recvbuf[4:4 + 12].split(b"\x00", 1)[0]
                    msglen = struct.unpack("<i", self.recvbuf[4 + 12:4 + 12 + 4])[0]
                    checksum = self.recvbuf[4 + 12 + 4:4 + 12 + 4 + 4]
                    if len(self.recvbuf) < 4 + 12 + 4 + 4 + msglen:
                        return
                    msg = self.recvbuf[4 + 12 + 4 + 4:4 + 12 + 4 + 4 + msglen]
                    th = sha256(msg)
                    h = sha256(th)
                    if checksum != h[:4]:
                        raise ValueError("got bad checksum " + repr(self.recvbuf))
                    self.recvbuf = self.recvbuf[4 + 12 + 4 + 4 + msglen:]
                if command in self.messagemap:
                    f = BytesIO(msg)
                    t = self.messagemap[command]()
                    t.deserialize(f)
                    self.got_message(t)
                else:
                    print("Unknown command: '" + str(command) + "' ")
                    self.show_debug_msg("Unknown command: '" + str(command) + "' " +
                                        repr(msg))
                    # pdb.set_trace()
        except Exception as e:
            print('got_data:', repr(e))
            #import traceback
            #traceback.print_tb(sys.exc_info()[2])
            #pdb.post_mortem(e.__traceback__)

    def send_message(self, message, pushbuf=False):
        if self.state != "connected" and not pushbuf:
            return
        self.show_debug_msg("Send %s" % repr(message))
        command = message.command
        data = message.serialize()
        tmsg = self.MAGIC_BYTES[self.network]
        tmsg += command
        tmsg += b"\x00" * (12 - len(command))
        tmsg += struct.pack("<I", len(data))
        if self.ver_send >= 209:
            th = sha256(data)
            h = sha256(th)
            tmsg += h[:4]
        tmsg += data
        with mininode_lock:
            self.sendbuf += tmsg
            self.last_sent = time.time()

    def got_message(self, message):
        if message.command == b"version":
            if message.nVersion <= BIP0031_VERSION:
                self.messagemap[b'ping'] = msg_ping_prebip31
        if self.last_sent + 30 * 60 < time.time():
            self.send_message(self.messagemap[b'ping']())
        self.show_debug_msg("Recv %s" % repr(message))
        self.cb.deliver(self, message)

    def disconnect_node(self):
        self.disconnect = True


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
        logging.info("mininode network processing thread completed")


# An exception we can raise if we detect a potential disconnect
# (p2p or rpc) before the test is complete
class EarlyDisconnectError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)
