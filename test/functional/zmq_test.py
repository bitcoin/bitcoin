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
                                 hash256,
                                )

class Subscriber():
    def __init__(self, socket, address):
        self.address = address
        self.sequence = 0
        self.socket = socket

class ZMQTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.subscribers = {}

    def subscribe(self, type):
        import zmq
        address = "tcp://127.0.0.1:%d" % (28332 + len(self.subscribers))
        socket = self.zmqContext.socket(zmq.SUB)
        socket.set(zmq.RCVTIMEO, 60000)
        socket.setsockopt(zmq.SUBSCRIBE, type.encode('latin-1'))
        socket.connect(address)
        self.subscribers[type] = Subscriber(socket, address)

    def receive(self, type):
        subscriber = self.subscribers[type]
        topic, body, seq = subscriber.socket.recv_multipart()
        assert_equal(topic, type.encode('latin-1'))
        assert_equal(struct.unpack('<I', seq)[-1], subscriber.sequence)
        subscriber.sequence += 1
        return body

    def setup_nodes(self):
        # Try to import python3-zmq. Skip this test if the import fails.
        try:
            import zmq
            self.zmqContext = zmq.Context()
        except ImportError:
            raise SkipTest("python3-zmq module not available.")

        # Check that bitcoin has been built with ZMQ enabled
        config = configparser.ConfigParser()
        if not self.options.configfile:
            self.options.configfile = os.path.dirname(__file__) + "/../config.ini"
        config.read_file(open(self.options.configfile))

        if not config["components"].getboolean("ENABLE_ZMQ"):
            raise SkipTest("bitcoind has not been built with zmq enabled.")

        self.subscribe("hashblock")
        self.subscribe("hashtx")
        self.subscribe("rawblock")
        self.subscribe("rawtx")

        self.extra_args = [["-zmqpub%s=%s" % (type, sub.address) for type, sub in self.subscribers.items()], []]
        self.add_nodes(self.num_nodes, self.extra_args)
        self.start_nodes()

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
        txid = self.receive("hashtx")
        hex = self.receive("rawtx")
        # Check that the rawtx hashes to the hashtx
        assert_equal(hash256(hex), txid)

        self.log.info("Wait for block")
        # block hash from generate must be equal to the hash received over zmq
        hash = bytes_to_hex_str(self.receive("hashblock"))
        assert_equal(genhashes[0], hash)

        block = self.receive("rawblock")
        # Check the hash of the rawblock's header matches generate
        assert_equal(genhashes[0], bytes_to_hex_str(hash256(block[:80])))

        n = 5
        self.log.info("Generate %(n)d blocks (and %(n)d coinbase txes)" % {"n": n})
        genhashes = self.nodes[1].generate(n)
        self.sync_all()

        for x in range(n):
            hash = bytes_to_hex_str(self.receive("hashblock"))
            assert_equal(genhashes[x], hash)

            block = self.receive("rawblock")
            assert_equal(genhashes[x], bytes_to_hex_str(hash256(block[:80])))

            txid = self.receive("hashtx")
            assert_equal(bytes_to_hex_str(txid), self.nodes[1].getblock(hash)["tx"][0])

            self.receive("rawtx")

        self.log.info("Wait for tx from second node")
        # test tx from a second node
        hashRPC = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1.0)
        self.sync_all()

        # now we should receive a zmq msg because the tx was broadcast
        body = self.receive("hashtx")
        hashZMQ = bytes_to_hex_str(body)

        body = self.receive("rawtx")
        hashedZMQ = bytes_to_hex_str(hash256(body))
        assert_equal(hashRPC, hashZMQ)  # txid from sendtoaddress must be equal to the hash received over zmq
        assert_equal(hashRPC, hashedZMQ)

if __name__ == '__main__':
    ZMQTest().main()
