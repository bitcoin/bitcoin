#!/usr/bin/env python2
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test ZMQ interface
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import zmq
import binascii
from test_framework.mininode import hash256

try:
    import http.client as httplib
except ImportError:
    import httplib
try:
    import urllib.parse as urlparse
except ImportError:
    import urlparse

class ZMQTest (BitcoinTestFramework):

    port = 28332

    def setup_nodes(self):
        self.zmqContext = zmq.Context()
        self.zmqSubSocket = self.zmqContext.socket(zmq.SUB)
        self.zmqSubSocket.setsockopt(zmq.SUBSCRIBE, "hashblock")
        self.zmqSubSocket.setsockopt(zmq.SUBSCRIBE, "hashtx")
        self.zmqSubSocket.connect("tcp://127.0.0.1:%i" % self.port)
        return start_nodes(4, self.options.tmpdir, extra_args=[
            ['-zmqpubhashtx=tcp://127.0.0.1:'+str(self.port), '-zmqpubhashblock=tcp://127.0.0.1:'+str(self.port)],
            [],
            [],
            []
            ])

    def run_test(self):
        self.sync_all()

        genhashes = self.nodes[0].generate(1);
        self.sync_all()

        print "listen..."
        msg = self.zmqSubSocket.recv_multipart()
        topic = str(msg[0])
        body = msg[1]

        msg = self.zmqSubSocket.recv_multipart()
        topic = str(msg[0])
        body = msg[1]
        blkhash = binascii.hexlify(body)

        assert_equal(genhashes[0], blkhash) #blockhash from generate must be equal to the hash received over zmq

        n = 10
        genhashes = self.nodes[1].generate(n);
        self.sync_all()

        zmqHashes = []
        for x in range(0,n*2):
            msg = self.zmqSubSocket.recv_multipart()
            topic = str(msg[0])
            body = msg[1]
            if topic == "hashblock":
                zmqHashes.append(binascii.hexlify(body))

        for x in range(0,n):
            assert_equal(genhashes[x], zmqHashes[x]) #blockhash from generate must be equal to the hash received over zmq

        #test tx from a second node
        hashRPC = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1.0)
        self.sync_all()

        #now we should receive a zmq msg because the tx was broadcastet
        msg = self.zmqSubSocket.recv_multipart()
        topic = str(msg[0])
        body = msg[1]
        hashZMQ = ""
        if topic == "hashtx":
            hashZMQ = binascii.hexlify(body)

        assert_equal(hashRPC, hashZMQ) #blockhash from generate must be equal to the hash received over zmq


if __name__ == '__main__':
    ZMQTest ().main ()
