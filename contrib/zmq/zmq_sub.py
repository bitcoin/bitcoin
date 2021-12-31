#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
    ZMQ example using python3's asyncio

    Syscoin should be started with the command line arguments:
        syscoind -testnet -daemon \
                -zmqpubrawtx=tcp://127.0.0.1:28370 \
                -zmqpubrawmempooltx=tcp://127.0.0.1:28370 \
                -zmqpubrawblock=tcp://127.0.0.1:28370 \
                -zmqpubhashtx=tcp://127.0.0.1:28370 \
                -zmqpubhashblock=tcp://127.0.0.1:28370 \
                -zmqpubhashgovernancevote=tcp://127.0.0.1:28370 \
                -zmqpubhashgovernanceobject=tcp://127.0.0.1:28370 \
                -zmqpubrawgovernancevote=tcp://127.0.0.1:28370 \
                -zmqpubrawgovernanceobject=tcp://127.0.0.1:28370 \
                -zmqpubsequence=tcp://127.0.0.1:28332

    We use the asyncio library here.  `self.handle()` installs itself as a
    future at the end of the function.  Since it never returns with the event
    loop having an empty stack of futures, this creates an infinite loop.  An
    alternative is to wrap the contents of `handle` inside `while True`.

    A blocking example using python 2.7 can be obtained from the git history:
    https://github.com/syscoin/syscoin/blob/37a7fe9e440b83e2364d5498931253937abe9294/contrib/zmq/zmq_sub.py
"""

import asyncio
import zmq
import zmq.asyncio
import signal
import struct
import sys

if (sys.version_info.major, sys.version_info.minor) < (3, 5):
    print("This example only works with Python 3.5 and greater")
    sys.exit(1)

port = 28370

class ZMQHandler():
    def __init__(self):
        self.loop = asyncio.get_event_loop()
        self.zmqContext = zmq.asyncio.Context()

        self.zmqSubSocket = self.zmqContext.socket(zmq.SUB)
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashblock")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashtx")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawblock")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawtx")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashgovernancevote")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "hashgovernanceobject")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawgovernancevote")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "rawgovernanceobject")
        self.zmqSubSocket.setsockopt_string(zmq.SUBSCRIBE, "sequence")
        self.zmqSubSocket.connect("tcp://127.0.0.1:%i" % port)

    async def handle(self) :
        topic, body, seq = await self.zmqSubSocket.recv_multipart()
        sequence = "Unknown"
        if len(seq) == 4:
            sequence = str(struct.unpack('<I', seq)[-1])
        if topic == b"hashblock":
            print('- HASH BLOCK ('+sequence+') -')
            print(body.hex())
        elif topic == b"hashtx":
            print('- HASH TX  ('+sequence+') -')
            print(body.hex())
        elif topic == b"rawblock":
            print('- RAW BLOCK HEADER ('+sequence+') -')
            print(body[:80].hex())
        elif topic == b"rawtx":
            print('- RAW TX ('+sequence+') -')
            print(body.hex())
        elif topic == b"hashgovernancevote":
            print('- HASH GOVERNANCE VOTE ('+sequence+') -')
            print(body.hex())
        elif topic == b"hashgovernanceobject":
            print('- HASH GOVERNANCE OBJECT ('+sequence+') -')
            print(body.hex())
        elif topic == b"rawgovernancevote":
            print('- RAW GOVERNANCE VOTE ('+sequence+') -')
            print(body.hex())
        elif topic == b"rawgovernanceobject":
            print('- RAW GOVERNANCE OBJECT ('+sequence+') -')
            print(body.hex())
        elif topic == b"sequence":
            hash = body[:32].hex()
            label = chr(body[32])
            mempool_sequence = None if len(body) != 32+1+8 else struct.unpack("<Q", body[32+1:])[0]
            print('- SEQUENCE ('+sequence+') -')
            print(hash, label, mempool_sequence)
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
