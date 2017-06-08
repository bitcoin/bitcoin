#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the ZMQ API."""
import configparser
import os
import struct

from test_framework.test_framework import BitcoinTestFramework, SkipTest
from test_framework.util import (assert_equal,
                                 bytes_to_hex_str,
                                 )

class ZMQTest (BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 2

    def setup_nodes(self):
        # Try to import python3-zmq. Skip this test if the import fails.
        try:
            import zmq
        except ImportError:
            raise SkipTest("python3-zmq module not available.")

        # Check that bitcoin has been built with ZMQ enabled
        config = configparser.ConfigParser()
        if not self.options.configfile:
            self.options.configfile = os.path.dirname(__file__) + "/../config.ini"
        config.read_file(open(self.options.configfile))

        if not config["components"].getboolean("ENABLE_ZMQ"):
            raise SkipTest("bitcoind has not been built with zmq enabled.")

        self.zmqContext = zmq.Context()
        self.zmqSubSocket = self.zmqContext.socket(zmq.SUB)
        self.zmqSubSocket.set(zmq.RCVTIMEO, 60000)
        self.zmqSubSocket.setsockopt(zmq.SUBSCRIBE, b"hashblock")
        self.zmqSubSocket.setsockopt(zmq.SUBSCRIBE, b"hashtx")
        ip_address = "tcp://127.0.0.1:28332"
        self.zmqSubSocket.connect(ip_address)
        extra_args = [['-zmqpubhashtx=%s' % ip_address, '-zmqpubhashblock=%s' % ip_address], []]
        self.nodes = self.start_nodes(self.num_nodes, self.options.tmpdir, extra_args)

    def run_test(self):
        try:
            self._zmq_test()
        finally:
            # Destroy the zmq context
            self.log.debug("Destroying zmq context")
            self.zmqContext.destroy(linger=None)

    def _zmq_test(self):
        genhashes = self.nodes[0].generate(1)
        self.sync_all()

        self.log.info("Wait for tx")
        msg = self.zmqSubSocket.recv_multipart()
        topic = msg[0]
        assert_equal(topic, b"hashtx")
        body = msg[1]
        msgSequence = struct.unpack('<I', msg[-1])[-1]
        assert_equal(msgSequence, 0)  # must be sequence 0 on hashtx

        self.log.info("Wait for block")
        msg = self.zmqSubSocket.recv_multipart()
        topic = msg[0]
        body = msg[1]
        msgSequence = struct.unpack('<I', msg[-1])[-1]
        assert_equal(msgSequence, 0)  # must be sequence 0 on hashblock
        blkhash = bytes_to_hex_str(body)

        assert_equal(genhashes[0], blkhash)  # blockhash from generate must be equal to the hash received over zmq

        self.log.info("Generate 10 blocks (and 10 coinbase txes)")
        n = 10
        genhashes = self.nodes[1].generate(n)
        self.sync_all()

        zmqHashes = []
        blockcount = 0
        for x in range(n * 2):
            msg = self.zmqSubSocket.recv_multipart()
            topic = msg[0]
            body = msg[1]
            if topic == b"hashblock":
                zmqHashes.append(bytes_to_hex_str(body))
                msgSequence = struct.unpack('<I', msg[-1])[-1]
                assert_equal(msgSequence, blockcount + 1)
                blockcount += 1

        for x in range(n):
            assert_equal(genhashes[x], zmqHashes[x])  # blockhash from generate must be equal to the hash received over zmq

        self.log.info("Wait for tx from second node")
        # test tx from a second node
        hashRPC = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1.0)
        self.sync_all()

        # now we should receive a zmq msg because the tx was broadcast
        msg = self.zmqSubSocket.recv_multipart()
        topic = msg[0]
        body = msg[1]
        assert_equal(topic, b"hashtx")
        hashZMQ = bytes_to_hex_str(body)
        msgSequence = struct.unpack('<I', msg[-1])[-1]
        assert_equal(msgSequence, blockcount + 1)

        assert_equal(hashRPC, hashZMQ)  # txid from sendtoaddress must be equal to the hash received over zmq

if __name__ == '__main__':
    ZMQTest().main()
