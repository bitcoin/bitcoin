#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import array
import binascii
import asyncio, zmq, zmq.asyncio
import struct

port = 28332

zmqContext = zmq.asyncio.Context()

async def recv_and_process():
    zmqSubSocket = zmqContext.socket(zmq.SUB)
    zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashblock")
    zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashtx")
    zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawblock")
    zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawtx")
    zmqSubSocket.connect("tcp://127.0.0.1:%i" % port)

    poller = zmq.asyncio.Poller()
    poller.register(zmqSubSocket, zmq.POLLIN)
    while True:
        s = await poller.poll()
        msg = await s[0][0].recv_multipart()
        topic = msg[0]
        body = msg[1]
        sequence = "Unknown";
        if len(msg[-1]) == 4:
          msgSequence = struct.unpack('<I', msg[-1])[-1]
          sequence = str(msgSequence)
        if topic == b"hashblock":
            print('- HASH BLOCK ('+sequence+') -')
            print(binascii.hexlify(body))
        elif topic == b"hashtx":
            print('- HASH TX  ('+sequence+') -')
            print(binascii.hexlify(body))
        elif topic == b"rawblock":
            print('- RAW BLOCK HEADER ('+sequence+') -')
            print(binascii.hexlify(body[:80]))
        elif topic == b"rawtx":
            print('- RAW TX ('+sequence+') -')
            print(binascii.hexlify(body))

try:
    loop = zmq.asyncio.ZMQEventLoop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(setup())
except KeyboardInterrupt:
    zmqContext.destroy()
