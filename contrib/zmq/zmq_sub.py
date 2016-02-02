#!/usr/bin/env python2

import array
import binascii
import zmq

port = 28332

zmqContext = zmq.Context()
zmqSubSocket = zmqContext.socket(zmq.SUB)
zmqSubSocket.setsockopt(zmq.SUBSCRIBE, "hashblock")
zmqSubSocket.setsockopt(zmq.SUBSCRIBE, "hashtx")
zmqSubSocket.setsockopt(zmq.SUBSCRIBE, "rawblock")
zmqSubSocket.setsockopt(zmq.SUBSCRIBE, "rawtx")
zmqSubSocket.connect("tcp://127.0.0.1:%i" % port)

try:
    while True:
        msg = zmqSubSocket.recv_multipart()
        topic = str(msg[0])
        body = msg[1]

        if topic == "hashblock":
            print "- HASH BLOCK -"
            print binascii.hexlify(body)
        elif topic == "hashtx":
            print '- HASH TX -'
            print binascii.hexlify(body)
        elif topic == "rawblock":
            print "- RAW BLOCK HEADER -"
            print binascii.hexlify(body[:80])
        elif topic == "rawtx":
            print '- RAW TX -'
            print binascii.hexlify(body)

except KeyboardInterrupt:
    zmqContext.destroy()
