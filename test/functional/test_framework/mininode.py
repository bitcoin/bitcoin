#!/usr/bin/env python3
# Copyright (c) 2010 ArtForz -- public domain half-a-node
# Copyright (c) 2012 Jeff Garzik
# Copyright (c) 2010-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Dash P2P network half-a-node.

This python code was modified from ArtForz' public domain half-a-node, as
found in the mini-node branch of http://github.com/jgarzik/pynode.

P2PConnection: A low-level connection object to a node's P2P interface
P2PInterface: A high-level interface object for communicating to a node over P2P
P2PDataStore: A p2p interface class that keeps a store of transactions and blocks
              and can respond correctly to getdata and getheaders messages
"""
import asyncore
from collections import defaultdict
from io import BytesIO
import logging
import socket
import struct
import sys
import threading

from test_framework.messages import *
from test_framework.util import wait_until

MSG_TX = 1
MSG_BLOCK = 2
MSG_TYPE_MASK = 0xffffffff >> 2

logger = logging.getLogger("TestFramework.mininode")

MESSAGEMAP = {
    b"addr": msg_addr,
    b"addrv2": msg_addrv2,
    b"block": msg_block,
    b"blocktxn": msg_blocktxn,
    b"cmpctblock": msg_cmpctblock,
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
    b"sendaddrv2": msg_sendaddrv2,
    b"sendcmpct": msg_sendcmpct,
    b"sendheaders": msg_sendheaders,
    b"tx": msg_tx,
    b"verack": msg_verack,
    b"version": msg_version,
    # Dash Specific
    b"clsig": msg_clsig,
    b"getmnlistd": msg_getmnlistd,
    b"getsporks": None,
    b"govsync": None,
    b"islock": msg_islock,
    b"mnlistdiff": msg_mnlistdiff,
    b"notfound": None,
    b"qfcommit": None,
    b"qsendrecsigs": None,
    b"qgetdata": msg_qgetdata,
    b"qdata": msg_qdata,
    b"qwatch" : None,
    b"senddsq": None,
    b"spork": None,
}

MAGIC_BYTES = {
    "mainnet": b"\xbf\x0c\x6b\xbd",   # mainnet
    "testnet3": b"\xce\xe2\xca\xff",  # testnet3
    "regtest": b"\xfc\xc1\xb7\xdc",   # regtest
    "devnet": b"\xe2\xca\xff\xce",    # devnet
}

class P2PConnection(asyncore.dispatcher):
    """A low-level connection object to a node's P2P interface.

    This class is responsible for:

    - opening and closing the TCP connection to the node
    - reading bytes from and writing bytes to the socket
    - deserializing and serializing the P2P message header
    - logging messages as they are sent and received

    This class contains no logic for handing the P2P message payloads. It must be
    sub-classed and the on_message() callback overridden."""

    def __init__(self):
        super().__init__(map=mininode_socket_map)

        self._conn_open = False

    @property
    def is_connected(self):
        return self._conn_open

    def peer_connect(self, dstaddr, dstport, *, net, devnet_name=None, uacomment=None):
        self.dstaddr = dstaddr
        self.dstport = dstport
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.sendbuf = b""
        self.recvbuf = b""
        self._asyncore_pre_connection = True
        self.network = net
        self.devnet_name = devnet_name
        self.uacomment = uacomment
        self.disconnect = False

        if self.network == "devnet" and self.devnet_name is not None:
            if self.uacomment is None:
                self.strSubVer = MY_SUBVERSION % ("(devnet=devnet-%s)" % self.devnet_name).encode()
            else:
                self.strSubVer = MY_SUBVERSION % ("(devnet=devnet-%s,%s)" % (self.devnet_name, self.uacomment)).encode()
        elif self.uacomment is not None:
            self.strSubVer = MY_SUBVERSION % ("(%s)" % self.uacomment).encode()
        else:
            self.strSubVer = MY_SUBVERSION % b""

        logger.debug('Connecting to Dash Node: %s:%d' % (self.dstaddr, self.dstport))

        try:
            self.connect((dstaddr, dstport))
        except:
            self.handle_close()

    def peer_disconnect(self):
        # Connection could have already been closed by other end.
        if self.is_connected:
            self.disconnect = True  # Signal asyncore to disconnect

    # Connection and disconnection methods

    def handle_connect(self):
        """asyncore callback when a connection is opened."""
        if not self.is_connected:
            logger.debug("Connected & Listening: %s:%d" % (self.dstaddr, self.dstport))
            self._conn_open = True
            self._asyncore_pre_connection = False
            self.on_open()

    def handle_close(self):
        """asyncore callback when a connection is closed."""
        logger.debug("Closing connection to: %s:%d" % (self.dstaddr, self.dstport))
        self._conn_open = False
        self.recvbuf = b""
        self.sendbuf = b""
        try:
            self.close()
        except:
            pass
        self.on_close()

    # Socket read methods

    def handle_read(self):
        """asyncore callback when data is read from the socket."""
        t = self.recv(8192)
        if len(t) > 0:
            self.recvbuf += t
            self._on_data()

    def _on_data(self):
        """Try to read P2P messages from the recv buffer.

        This method reads data from the buffer in a loop. It deserializes,
        parses and verifies the P2P header, then passes the P2P payload to
        the on_message callback for processing."""
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
                if MESSAGEMAP[command] is None:
                    # Command is known but we don't want/need to handle it
                    continue
                f = BytesIO(msg)
                t = MESSAGEMAP[command]()
                t.deserialize(f)
                self._log_message("receive", t)
                self.on_message(t)
        except Exception as e:
            logger.exception('Error reading message:', repr(e))
            raise

    def on_message(self, message):
        """Callback for processing a P2P payload. Must be overridden by derived class."""
        raise NotImplementedError

    # Socket write methods

    def writable(self):
        """asyncore method to determine whether the handle_write() callback should be called on the next loop."""
        with mininode_lock:
            length = len(self.sendbuf)
        return length > 0 or self._asyncore_pre_connection

    def handle_write(self):
        """asyncore callback when data should be written to the socket."""
        with mininode_lock:
            # asyncore does not expose socket connection, only the first read/write
            # event, thus we must check connection manually here to know when we
            # actually connect
            if self._asyncore_pre_connection:
                self.handle_connect()
            if not self.writable():
                return

            try:
                sent = self.send(self.sendbuf)
            except:
                self.handle_close()
                return
            self.sendbuf = self.sendbuf[sent:]

    def send_message(self, message):
        """Send a P2P message over the socket.

        This method takes a P2P payload, builds the P2P header and adds
        the message to the send buffer to be sent over the socket."""
        if not self.is_connected:
            raise IOError('Not connected')
        self._log_message("send", message)
        tmsg = self._build_message(message)
        with mininode_lock:
            if len(self.sendbuf) == 0:
                try:
                    sent = self.send(tmsg)
                    self.sendbuf = tmsg[sent:]
                except BlockingIOError:
                    self.sendbuf = tmsg
            else:
                self.sendbuf += tmsg

    # Class utility methods

    def _build_message(self, message):
        """Build a serialized P2P message"""
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
    def __init__(self, support_addrv2=False):
        super().__init__()

        # Track number of messages of each type received and the most recent
        # message of each type
        self.message_count = defaultdict(int)
        self.last_message = {}

        # A count of the number of ping messages we've sent to the node
        self.ping_counter = 1

        # The network services received from the peer
        self.nServices = 0

        self.support_addrv2 = support_addrv2

    def peer_connect(self, *args, services=NODE_NETWORK, send_version=True, **kwargs):
        super().peer_connect(*args, **kwargs)

        if send_version:
            # Send a version msg
            vt = msg_version()
            vt.nServices = services
            vt.addrTo.ip = self.dstaddr
            vt.addrTo.port = self.dstport
            vt.addrFrom.ip = "0.0.0.0"
            vt.addrFrom.port = 0
            vt.strSubVer = self.strSubVer
            self.sendbuf = self._build_message(vt)  # Will be sent right after handle_connect

    # Message receiving methods

    def on_message(self, message):
        """Receive message and dispatch message to appropriate callback.

        We keep a count of how many of each message type has been received
        and the most recent message of each type."""
        with mininode_lock:
            try:
                command = message.command.decode('ascii')
                self.message_count[command] += 1
                self.last_message[command] = message
                getattr(self, 'on_' + command)(message)
            except:
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
    def on_cmpctblock(self, message): pass
    def on_feefilter(self, message): pass
    def on_getaddr(self, message): pass
    def on_getblocks(self, message): pass
    def on_getblocktxn(self, message): pass
    def on_getdata(self, message): pass
    def on_getheaders(self, message): pass
    def on_headers(self, message): pass
    def on_mempool(self, message): pass
    def on_pong(self, message): pass
    def on_reject(self, message): pass
    def on_sendaddrv2(self, message): pass
    def on_sendcmpct(self, message): pass
    def on_sendheaders(self, message): pass
    def on_tx(self, message): pass

    def on_inv(self, message):
        want = msg_getdata()
        for i in message.inv:
            if i.type != 0:
                want.inv.append(i)
        if len(want.inv):
            self.send_message(want)

    def on_ping(self, message):
        self.send_message(msg_pong(message.nonce))

    def on_mnlistdiff(self, message): pass
    def on_clsig(self, message): pass
    def on_islock(self, message): pass

    def on_qgetdata(self, message): pass
    def on_qdata(self, message): pass
    def on_qwatch(self, message): pass

    def on_verack(self, message):
        self.verack_received = True

    def on_version(self, message):
        assert message.nVersion >= MIN_VERSION_SUPPORTED, "Version {} received. Test framework only supports versions greater than {}".format(message.nVersion, MIN_VERSION_SUPPORTED)
        self.send_message(msg_verack())
        if self.support_addrv2:
            self.send_message(msg_sendaddrv2())
        self.nServices = message.nServices

    # Connection helper methods

    def wait_for_disconnect(self, timeout=60):
        test_function = lambda: not self.is_connected
        wait_until(test_function, timeout=timeout, lock=mininode_lock)
        # This is a hack. The related issues should be fixed by bitcoin 14119 and 14457.
        time.sleep(1)

    # Message receiving helper methods

    def wait_for_block(self, blockhash, timeout=60):
        test_function = lambda: self.last_message.get("block") and self.last_message["block"].block.rehash() == blockhash
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    def wait_for_getdata(self, timeout=60):
        """Waits for a getdata message.

        Receiving any getdata message will satisfy the predicate. the last_message["getdata"]
        value must be explicitly cleared before calling this method, or this will return
        immediately with success. TODO: change this method to take a hash value and only
        return true if the correct block/tx has been requested."""
        test_function = lambda: self.last_message.get("getdata")
        wait_until(test_function, timeout=timeout, lock=mininode_lock)

    def wait_for_getheaders(self, timeout=60):
        """Waits for a getheaders message.

        Receiving any getheaders message will satisfy the predicate. the last_message["getheaders"]
        value must be explicitly cleared before calling this method, or this will return
        immediately with success. TODO: change this method to take a hash value and only
        return true if the correct block header has been requested."""
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

    def send_and_ping(self, message):
        self.send_message(message)
        self.sync_with_ping()

    # Sync up with the node
    def sync_with_ping(self, timeout=60):
        self.send_message(msg_ping(nonce=self.ping_counter))
        test_function = lambda: self.last_message.get("pong") and self.last_message["pong"].nonce == self.ping_counter
        wait_until(test_function, timeout=timeout, lock=mininode_lock)
        self.ping_counter += 1


# Keep our own socket map for asyncore, so that we can track disconnects
# ourselves (to work around an issue with closing an asyncore socket when
# using select)
mininode_socket_map = dict()

# One lock for synchronizing all data access between the networking thread (see
# NetworkThread below) and the thread running the test logic.  For simplicity,
# P2PConnection acquires this lock whenever delivering a message to a P2PInterface,
# and whenever adding anything to the send buffer (in send_message()).  This
# lock should be acquired in the thread running the test logic to synchronize
# access to any data shared with the P2PInterface or P2PConnection.
mininode_lock = threading.RLock()

class NetworkThread(threading.Thread):
    def __init__(self):
        super().__init__(name="NetworkThread")

    def run(self):
        while mininode_socket_map:
            # We check for whether to disconnect outside of the asyncore
            # loop to work around the behavior of asyncore when using
            # select
            disconnected = []
            for fd, obj in mininode_socket_map.items():
                if obj.disconnect:
                    disconnected.append(obj)
            [obj.handle_close() for obj in disconnected]
            asyncore.loop(0.1, use_poll=True, map=mininode_socket_map, count=1)
        logger.debug("Network thread closing")

def network_thread_start():
    """Start the network thread."""
    # Only one network thread may run at a time
    assert not network_thread_running()

    NetworkThread().start()

def network_thread_running():
    """Return whether the network thread is running."""
    return any([thread.name == "NetworkThread" for thread in threading.enumerate()])

def network_thread_join(timeout=10):
    """Wait timeout seconds for the network thread to terminate.

    Throw if the network thread doesn't terminate in timeout seconds."""
    network_threads = [thread for thread in threading.enumerate() if thread.name == "NetworkThread"]
    assert len(network_threads) <= 1
    for thread in network_threads:
        thread.join(timeout)
        assert not thread.is_alive()

class P2PDataStore(P2PInterface):
    """A P2P data store class.

    Keeps a block and transaction store and responds correctly to getdata and getheaders requests."""

    def __init__(self):
        super().__init__()
        self.reject_code_received = None
        self.reject_reason_received = None
        # store of blocks. key is block hash, value is a CBlock object
        self.block_store = {}
        self.last_block_hash = ''
        # store of txs. key is txid, value is a CTransaction object
        self.tx_store = {}
        self.getdata_requests = []

    def on_getdata(self, message):
        """Check for the tx/block in our stores and if found, reply with an inv message."""
        for inv in message.inv:
            self.getdata_requests.append(inv.hash)
            if (inv.type & MSG_TYPE_MASK) == MSG_TX and inv.hash in self.tx_store.keys():
                self.send_message(msg_tx(self.tx_store[inv.hash]))
            elif (inv.type & MSG_TYPE_MASK) == MSG_BLOCK and inv.hash in self.block_store.keys():
                self.send_message(msg_block(self.block_store[inv.hash]))
            else:
                logger.debug('getdata message type {} received.'.format(hex(inv.type)))

    def on_getheaders(self, message):
        """Search back through our block store for the locator, and reply with a headers message if found."""

        locator, hash_stop = message.locator, message.hashstop

        # Assume that the most recent block added is the tip
        if not self.block_store:
            return

        headers_list = [self.block_store[self.last_block_hash]]
        maxheaders = 2000
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
        headers_list = headers_list[:-maxheaders - 1:-1]
        response = msg_headers(headers_list)

        if response is not None:
            self.send_message(response)

    def on_reject(self, message):
        """Store reject reason and code for testing."""
        self.reject_code_received = message.code
        self.reject_reason_received = message.reason

    def send_blocks_and_test(self, blocks, node, *, success=True, request_block=True, reject_code=None, reject_reason=None, timeout=60):
        """Send blocks to test node and test whether the tip advances.

         - add all blocks to our block_store
         - send a headers message for the final block
         - the on_getheaders handler will ensure that any getheaders are responded to
         - if request_block is True: wait for getdata for each of the blocks. The on_getdata handler will
           ensure that any getdata messages are responded to
         - if success is True: assert that the node's tip advances to the most recent block
         - if success is False: assert that the node's tip doesn't advance
         - if reject_code and reject_reason are set: assert that the correct reject message is received"""

        with mininode_lock:
            self.reject_code_received = None
            self.reject_reason_received = None

            for block in blocks:
                self.block_store[block.sha256] = block
                self.last_block_hash = block.sha256

        self.send_message(msg_headers([CBlockHeader(blocks[-1])]))

        if request_block:
            wait_until(lambda: blocks[-1].sha256 in self.getdata_requests, timeout=timeout, lock=mininode_lock)

        if success:
            wait_until(lambda: node.getbestblockhash() == blocks[-1].hash, timeout=timeout)
        else:
            assert node.getbestblockhash() != blocks[-1].hash

        if reject_code is not None:
            wait_until(lambda: self.reject_code_received == reject_code, lock=mininode_lock)
        if reject_reason is not None:
            wait_until(lambda: self.reject_reason_received == reject_reason, lock=mininode_lock)

    def send_txs_and_test(self, txs, node, *, success=True, expect_disconnect=False, reject_code=None, reject_reason=None):
        """Send txs to test node and test whether they're accepted to the mempool.

         - add all txs to our tx_store
         - send tx messages for all txs
         - if success is True/False: assert that the txs are/are not accepted to the mempool
         - if expect_disconnect is True: Skip the sync with ping
         - if reject_code and reject_reason are set: assert that the correct reject message is received."""

        with mininode_lock:
            self.reject_code_received = None
            self.reject_reason_received = None

            for tx in txs:
                self.tx_store[tx.sha256] = tx

        for tx in txs:
            self.send_message(msg_tx(tx))

        if expect_disconnect:
            self.wait_for_disconnect()
        else:
            self.sync_with_ping()

        raw_mempool = node.getrawmempool()
        if success:
            # Check that all txs are now in the mempool
            for tx in txs:
                assert tx.hash in raw_mempool, "{} not found in mempool".format(tx.hash)
        else:
            # Check that none of the txs are now in the mempool
            for tx in txs:
                assert tx.hash not in raw_mempool, "{} tx found in mempool".format(tx.hash)

        if reject_code is not None:
            wait_until(lambda: self.reject_code_received == reject_code, lock=mininode_lock)
        if reject_reason is not None:
            wait_until(lambda: self.reject_reason_received == reject_reason, lock=mininode_lock)
