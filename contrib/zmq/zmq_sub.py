#!/usr/bin/env python2

import zmq
import binascii

port = 28332
topic1 = "BLK"
topic2 = "TXN"
topic_len = len(topic1)

zmqContext = zmq.Context()
zmqSubSocket = zmqContext.socket(zmq.SUB)
zmqSubSocket.setsockopt(zmq.SUBSCRIBE, topic1)
zmqSubSocket.setsockopt(zmq.SUBSCRIBE, topic2)
zmqSubSocket.connect("tcp://127.0.0.1:%i" % port)


def handleBLK(blk):
    print "-BLKHDR-"
    print binascii.hexlify(blk[:80])


def handleTX(tx):
    print "-TX-"
    print binascii.hexlify(tx)


try:
    while True:
        msg = zmqSubSocket.recv()
        msg_topic = msg[:topic_len]
        msg_data  = msg[topic_len:]

        if msg_topic == "TXN":
            handleTX(msg_data)
        elif msg_topic == "BLK":
            handleBLK(msg_data)

except KeyboardInterrupt:
    zmqContext.destroy()
