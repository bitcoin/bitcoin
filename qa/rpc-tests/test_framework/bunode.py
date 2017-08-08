#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.mininode import *
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
from test_framework.blocktools import create_block, create_coinbase

EXPEDITED_VERSION = 80002

# class InvResp(Flags):
REQ_TX = 1
REQ_THINBLOCK = 2
REQ_XTHINBLOCK = 4
REQ_BLOCK = 8


class BUProtocolHandler(NodeConnCB):
    def __init__(self):
        NodeConnCB.__init__(self)
        self.connection = None
        self.last_inv = []
        self.last_headers = None
        self.last_block = None
        self.ping_counter = 1
        self.last_pong = msg_pong(0)
        self.last_getdata = []
        self.sleep_time = 0.05
        self.block_announced = False
        self.last_getheaders = None
        self.disconnected = False
        self.remoteVersion = 0
        self.buverack_received = False
        self.parent = None
        self.requestOnInv = 0

    def show_debug_msg(self, msg):
        print(msg)

    # Spin until verack message is received from the node.
    # Tests may want to use this as a signal that the test can begin.
    # This can be called from the testing thread, so it needs to acquire the
    # global lock.
    def wait_for_buverack(self):
        while True:
            if self.disconnected:
                raise DisconnectedError()
            with mininode_lock:
                if self.buverack_received:
                    return
            time.sleep(0.05)

    def clear_last_announcement(self):
        with mininode_lock:
            self.block_announced = False
            self.last_inv = []
            self.last_headers = None

    def on_version(self, conn, message):
        if message.nVersion >= 209:
            conn.send_message(msg_verack())
        conn.ver_send = min(MY_VERSION, message.nVersion)
        if message.nVersion < 209:
            conn.ver_recv = conn.ver_send
        self.remoteVersion = message.nVersion

    def on_verack(self, conn, message):
        conn.ver_recv = conn.ver_send
        self.verack_received = True
        if self.remoteVersion >= EXPEDITED_VERSION:
            msg = msg_buversion(conn.socket.getsockname()[1])
            self.connection.send_message(msg)

    def on_buverack(self, conn, message):
        self.show_debug_msg("BU version ACK\n")
        self.buverack_received = True

    def add_connection(self, conn):
        self.connection = conn

    def add_parent(self, p):
        self.parent = p

    # Request data for a list of block hashes
    def get_data(self, block_hashes):
        msg = msg_getdata()
        for x in block_hashes:
            msg.inv.append(CInv(2, x))
        self.connection.send_message(msg)

    def get_headers(self, locator, hashstop):
        msg = msg_getheaders()
        msg.locator.vHave = locator
        msg.hashstop = hashstop
        self.connection.send_message(msg)

    def send_block_inv(self, blockhash):
        msg = msg_inv()
        msg.inv = [CInv(2, blockhash)]
        self.connection.send_message(msg)

    # Wrapper for the NodeConn's send_message function
    def send_message(self, message):
        self.connection.send_message(message)

    def on_inv(self, conn, message):
        self.last_inv.append(message)
        self.block_announced = True
        for inv in message.inv:
            if inv.type == CInv.MSG_BLOCK:
                if self.requestOnInv & REQ_BLOCK:
                    msg = msg_getdata(inv)
                    self.send_message(msg)
                    self.show_debug_msg("requested block")
                if self.requestOnInv & REQ_THINBLOCK:
                    msg = msg_getdata(CInv(CInv.MSG_THINBLOCK, inv.hash))
                    self.send_message(msg)
                    self.show_debug_msg("requested thinblock")
                if self.requestOnInv & REQ_XTHINBLOCK:
                    msg = msg_getdata(CInv(CInv.MSG_XTHINBLOCK, inv.hash))
                    self.send_message(msg)
                    self.show_debug_msg("requested xtinblock")

    def on_headers(self, conn, message):
        self.last_headers = message
        self.block_announced = True

    def on_block(self, conn, message):
        self.last_block = message.block
        self.last_block.calc_sha256()
        if self.parent and hasattr(self.parent, "on_block"):
            self.parent.on_block(self, message)

    def on_thinblock(self, conn, message):
        self.last_block = message.block
        self.last_block.calc_sha256()
        if self.parent and hasattr(self.parent, "on_thinblock"):
            self.parent.on_thinblock(self, message)

    def on_xthinblock(self, conn, message):
        self.last_block = message.block
        self.last_block.calc_sha256()
        if self.parent and hasattr(self.parent, "on_xthinblock"):
            self.parent.on_xthinblock(self, message)

    def on_getdata(self, conn, message):
        self.last_getdata.append(message)

    def on_pong(self, conn, message):
        self.last_pong = message
        if self.parent and hasattr(self.parent, "on_pong"):
            self.parent.on_pong(self,message)

    def on_getheaders(self, conn, message):
        self.last_getheaders = message

    def on_close(self, conn):
        self.disconnected = True
        if self.parent and hasattr(self.parent, "on_close"):
            self.parent.on_close(self)

    # Test whether the last announcement we received had the
    # right header or the right inv
    # inv and headers should be lists of block hashes
    def check_last_announcement(self, headers=None, inv=[]):
        expect_headers = headers if headers != None else []
        expect_inv = inv if inv != [] else []

        def test_function(): return self.block_announced
        self.sync(test_function)
        timeout = 5
        while timeout > 0:
            with mininode_lock:
                self.block_announced = False

                success = True
                compare_inv = []
                if self.last_inv != []:
                    all_inv = [x.inv for x in self.last_inv]
                    for x in all_inv:
                        test_inv = [y.hash for y in x]
                        compare_inv = compare_inv + test_inv

                # Check whether the inventory received is within the list of block hashes that were
                # mined (During a large reorg the inv's will be fewer than the actual hashes mined).
                s = set(compare_inv)
                expect_inv = [x for x in expect_inv if x in s]
                if compare_inv != expect_inv:
                    success = False

                hash_headers = []
                if self.last_headers != None:
                    # treat headers as a list of block hashes
                    hash_headers = [x.sha256 for x in self.last_headers.headers]
                if hash_headers != expect_headers:
                    success = False

                self.last_inv = []
                self.last_headers = None

            if success == True:
                return success

            time.sleep(self.sleep_time)
            timeout -= self.sleep_time

    # Syncing helpers
    def sync(self, test_function, timeout=60):
        while timeout > 0:
            with mininode_lock:
                if test_function():
                    return

            time.sleep(self.sleep_time)
            timeout -= self.sleep_time
        raise AssertionError("Sync failed to complete")

    # The request manager does not deal with vectors of GETDATA requests but rather one GETDATA per
    # hash, therefore we need to be able to sync_getdata one message at a time rather than in batches.
    def sync_getdata(self, hash_list, timeout=60):
        while timeout > 0:
            with mininode_lock:
                # Check whether any getdata responses are in the hash list and
                # if so remove them from both lists.
                for x in self.last_getdata:
                    for y in hash_list:
                        if (str(x.inv).find(hex(y)[2:]) > 0):
                            self.last_getdata.remove(x)
                            hash_list.remove(y)
                if hash_list == []:
                    return

            time.sleep(self.sleep_time)
            timeout -= self.sleep_time
        raise AssertionError("Sync getdata failed to complete")

    def sync_with_ping(self, timeout=60):
        self.send_message(msg_ping(nonce=self.ping_counter))

        def test_function(): return self.last_pong.nonce == self.ping_counter
        self.sync(test_function, timeout)
        self.ping_counter += 1
        return

    def wait_for_block(self, blockhash, timeout=60):
        def test_function(): return self.last_block != None and self.last_block.sha256 == blockhash
        self.sync(test_function, timeout)
        return

    def wait_for_getheaders(self, timeout=60):
        def test_function(): return self.last_getheaders != None
        self.sync(test_function, timeout)
        return

    def wait_for_getdata(self, hash_list, timeout=60):
        if hash_list == []:
            return

        self.sync_getdata(hash_list, timeout)
        return

    def wait_for_disconnect(self, timeout=60):
        def test_function(): return self.disconnected
        self.sync(test_function, timeout)
        return

    def send_header_for_blocks(self, new_blocks):
        headers_message = msg_headers()
        headers_message.headers = [CBlockHeader(b) for b in new_blocks]
        self.send_message(headers_message)

    def send_getblocks(self, locator):
        getblocks_message = msg_getblocks()
        getblocks_message.locator.vHave = locator
        self.send_message(getblocks_message)

class BasicBUCashNode():
    def __init__(self):
        self.cnxns = {}
        self.nblocks = 0
        self.nthin = 0
        self.nxthin = 0

    def connect(self, id, ip, port, rpc=None, protohandler=None):
        if not protohandler:
            protohandler = BUProtocolHandler()
        conn = NodeConn(ip, port, rpc, protohandler, bitcoinCash=True)
        protohandler.add_connection(conn)
        protohandler.add_parent(self)
        self.cnxns[id] = protohandler

    def on_block(self, frm, message):
        print("got block")
        self.nblocks += 1

    def on_thinblock(self, frm, message):
        print("got thinblock")
        self.nthin += 1

    def on_xthinblock(self, frm, message):
        print("got xthinblock")
        self.nxthin += 1

class BasicBUNode:
    def __init__(self):
        self.cnxns = {}
        self.nblocks = 0
        self.nthin = 0
        self.nxthin = 0

    def connect(self, id, ip, port, rpc=None, protohandler=None):
        if not protohandler:
            protohandler = BUProtocolHandler()
        conn = NodeConn(ip, port, rpc, protohandler, bitcoinCash = False)
        protohandler.add_connection(conn)
        protohandler.add_parent(self)
        self.cnxns[id] = protohandler

    def on_block(self, frm, message):
        print("got block")
        self.nblocks += 1

    def on_thinblock(self, frm, message):
        print("got thinblock")
        self.nthin += 1

    def on_xthinblock(self, frm, message):
        print("got xthinblock")
        self.nxthin += 1
