#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test ZMQ interface
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import zmq
import struct

class ZMQTest (BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 4

    port = 28332

    def setup_nodes(self):
        self.zmqContext = zmq.Context()
        self.zmqSubSocket = self.zmqContext.socket(zmq.SUB)
        self.zmqSubSocket.setsockopt(zmq.SUBSCRIBE, b"hashblock")
        self.zmqSubSocket.setsockopt(zmq.SUBSCRIBE, b"hashtx")
        self.zmqSubSocket.connect("tcp://127.0.0.1:%i" % self.port)
        return start_nodes(self.num_nodes, self.options.tmpdir, extra_args=[
            ['-zmqpubhashtx=tcp://127.0.0.1:'+str(self.port), '-zmqpubhashblock=tcp://127.0.0.1:'+str(self.port)],
            [],
            [],
            []
            ])

    def run_test(self):
        self.sync_all()

        genhashes = self.nodes[0].generate(1)
        self.sync_all()

        print("listen...")
        msg = self.zmqSubSocket.recv_multipart()
        topic = msg[0]
        assert_equal(topic, b"hashtx")
        body = msg[1]
        nseq = msg[2]
        msgSequence = struct.unpack('<I', msg[-1])[-1]
        assert_equal(msgSequence, 0) #must be sequence 0 on hashtx

        msg = self.zmqSubSocket.recv_multipart()
        topic = msg[0]
        body = msg[1]
        msgSequence = struct.unpack('<I', msg[-1])[-1]
        assert_equal(msgSequence, 0) #must be sequence 0 on hashblock
        blkhash = bytes_to_hex_str(body)

        assert_equal(genhashes[0], blkhash) #blockhash from generate must be equal to the hash received over zmq

        n = 10
        genhashes = self.nodes[1].generate(n)
        self.sync_all()

        zmqHashes = []
        blockcount = 0
        for x in range(0,n*2):
            msg = self.zmqSubSocket.recv_multipart()
            topic = msg[0]
            body = msg[1]
            if topic == b"hashblock":
                zmqHashes.append(bytes_to_hex_str(body))
                msgSequence = struct.unpack('<I', msg[-1])[-1]
                assert_equal(msgSequence, blockcount+1)
                blockcount += 1

        for x in range(0,n):
            assert_equal(genhashes[x], zmqHashes[x]) #blockhash from generate must be equal to the hash received over zmq

        #test tx from a second node
        hashRPC = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1.0)
        self.sync_all()

        # now we should receive a zmq msg because the tx was broadcast
        msg = self.zmqSubSocket.recv_multipart()
        topic = msg[0]
        body = msg[1]
        hashZMQ = ""
        if topic == b"hashtx":
            hashZMQ = bytes_to_hex_str(body)
            msgSequence = struct.unpack('<I', msg[-1])[-1]
            assert_equal(msgSequence, blockcount+1)

        assert_equal(hashRPC, hashZMQ) #blockhash from generate must be equal to the hash received over zmq


if __name__ == '__main__':
    ZMQTest ().main ()
