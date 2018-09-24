#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Copyright (c) 2017 The Raven Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
    ZMQ example using python3's asyncio

    Raven should be started with the command line arguments:
        ravend -testnet -daemon \
                -zmqpubhashblock=tcp://127.0.0.1:28766 \
                -zmqpubrawtx=tcp://127.0.0.1:28766 \
                -zmqpubhashtx=tcp://127.0.0.1:28766 \
                -zmqpubhashblock=tcp://127.0.0.1:28766
"""

import sys
import zmq
import struct
import binascii
import codecs

#  Socket to talk to server
context = zmq.Context()
socket = context.socket(zmq.SUB)

print("Getting Ravencoin msgs")
socket.connect("tcp://localhost:28766")

socket.setsockopt_string(zmq.SUBSCRIBE, "hashtx")
socket.setsockopt_string(zmq.SUBSCRIBE, "hashblock")
socket.setsockopt_string(zmq.SUBSCRIBE, "rawblock")
socket.setsockopt_string(zmq.SUBSCRIBE, "rawtx")

while True:
	msg = socket.recv_multipart()
	topic = msg[0]
	body = msg[1]
	sequence = "Unknown"
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
		astr = binascii.hexlify(body).decode("utf-8")
		start = 0
		pos = 0
		while(pos != -1):
			pos = astr.find('72766e', start)
			if (pos > -1):
				print("FOUND RVN issuance at " + str(pos))
				print("After RVN: " + astr[pos+6:pos+8])
				sizestr = astr[pos+8:pos+10]
				print("sizestr: " + sizestr)
				#print(str(astr[pos+8:pos+10]))
				size = int(sizestr, 16)
				print("Bytes: " + str(size))
				print("Name: " + bytes.fromhex(astr[pos+10:pos+10+size*2]).decode('utf-8'))
			pos = astr.find('72766e', start)
			if (pos > -1):
				print("FOUND RVN something at " + str(pos))
			start += pos+8
			print(astr)


