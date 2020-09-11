#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
    ZMQ example using python3's asyncio

    Dash should be started with the command line arguments:
        dashd -testnet -daemon \
                -zmqpubrawtx=tcp://127.0.0.1:28332 \
                -zmqpubrawblock=tcp://127.0.0.1:28332 \
                -zmqpubhashtx=tcp://127.0.0.1:28332 \
                -zmqpubhashblock=tcp://127.0.0.1:28332

    We use the asyncio library here.  `self.handle()` installs itself as a
    future at the end of the function.  Since it never returns with the event
    loop having an empty stack of futures, this creates an infinite loop.  An
    alternative is to wrap the contents of `handle` inside `while True`.

    A blocking example using python 2.7 can be obtained from the git history:
    https://github.com/bitcoin/bitcoin/blob/37a7fe9e440b83e2364d5498931253937abe9294/contrib/zmq/zmq_sub.py
"""

import binascii
import asyncio
import zmq
import zmq.asyncio
import signal
import struct
import sys

if (sys.version_info.major, sys.version_info.minor) < (3, 5):
    print("This example only works with Python 3.5 and greater")
    sys.exit(1)

port = 28332

class ZMQHandler():
    def __init__(self):
        self.loop = asyncio.get_event_loop()
        self.zmqContext = zmq.asyncio.Context()

        self.zmqSubSocket = self.zmqContext.socket(zmq.SUB)
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashblock")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashchainlock")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashtx")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashtxlock")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashgovernancevote")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashgovernanceobject")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashinstantsenddoublespend")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawblock")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawchainlock")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawchainlocksig")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawtx")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawtxlock")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawtxlocksig")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawgovernancevote")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawgovernanceobject")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawinstantsenddoublespend")
        self.zmqSubSocket.connect("tcp://127.0.0.1:%i" % port)

    async def handle(self) :
        msg = await self.zmqSubSocket.recv_multipart()
        topic = msg[0]
        body = msg[1]
        sequence = "Unknown"

        if len(msg[-1]) == 4:
          msgSequence = struct.unpack('<I', msg[-1])[-1]
          sequence = str(msgSequence)

        if topic == b"hashblock":
            print('- HASH BLOCK ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"hashchainlock":
            print('- HASH CHAINLOCK ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"hashtx":
            print ('- HASH TX ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"hashtxlock":
            print('- HASH TX LOCK ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"hashgovernancevote":
            print('- HASH GOVERNANCE VOTE ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"hashgovernanceobject":
            print('- HASH GOVERNANCE OBJECT ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"hashinstantsenddoublespend":
            print('- HASH IS DOUBLE SPEND ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"rawblock":
            print('- RAW BLOCK HEADER ('+sequence+') -')
            print(binascii.hexlify(body[:80]).decode("utf-8"))
        elif topic == b"rawchainlock":
            print('- RAW CHAINLOCK ('+sequence+') -')
            print(binascii.hexlify(body[:80]).decode("utf-8"))
        elif topic == b"rawchainlocksig":
            print('- RAW CHAINLOCK SIG ('+sequence+') -')
            print(binascii.hexlify(body[:80]).decode("utf-8"))
        elif topic == b"rawtx":
            print('- RAW TX ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"rawtxlock":
            print('- RAW TX LOCK ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"rawtxlocksig":
            print('- RAW TX LOCK SIG ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"rawgovernancevote":
            print('- RAW GOVERNANCE VOTE ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"rawgovernanceobject":
            print('- RAW GOVERNANCE OBJECT ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        elif topic == b"rawinstantsenddoublespend":
            print('- RAW IS DOUBLE SPEND ('+sequence+') -')
            print(binascii.hexlify(body).decode("utf-8"))
        # schedule ourselves to receive the next message
        asyncio.ensure_future(self.handle())

    def start(self):
        self.loop.add_signal_handler(signal.SIGINT, self.stop)
        self.loop.create_task(self.handle())
        self.loop.run_forever()

    def stop(self):
        self.loop.stop()
        self.zmqContext.destroy()

daemon = ZMQHandler()
daemon.start()
