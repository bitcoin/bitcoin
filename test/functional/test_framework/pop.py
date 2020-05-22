#!/usr/bin/env python3
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test framework addition to include VeriBlock PoP functions"""
import struct
from .messages import ser_uint256, hash256, uint256_from_str
from .test_node import TestNode

KEYSTONE_INTERVAL = 5
POP_PAYOUT_DELAY = 50


def isKeystone(height):
    return height % KEYSTONE_INTERVAL == 0


def getPreviousKeystoneHeight(height):
    diff = height % KEYSTONE_INTERVAL
    if diff == 0:
        diff = KEYSTONE_INTERVAL
    prevks = height - diff
    return max(0, prevks)


def getKeystones(node, height):
    zero = '00' * 32
    if height == 0:
        return [zero, zero]

    prev1 = getPreviousKeystoneHeight(height)
    a = node.getblockhash(prev1)

    assert prev1 >= 0
    if prev1 == 0:
        return [a, zero]

    prev2 = getPreviousKeystoneHeight(prev1)
    b = node.getblockhash(prev2)

    return [a, b]


class ContextInfoContainer:
    __slots__ = ("height", "keystone1", "keystone2", "txRoot")

    @staticmethod
    def create(node, prev=None, prevheight=None):
        assert isinstance(node, TestNode)
        if prevheight:
            return ContextInfoContainer.createFromHeight(node, prevheight + 1)

        if isinstance(prev, int):
            prev = ser_uint256(prev)[::-1].hex()

        assert (isinstance(prev, str))
        best = node.getblock(prev)
        return ContextInfoContainer.createFromHeight(node, best['height'] + 1)

    @staticmethod
    def createFromHeight(node, height):
        assert isinstance(node, TestNode)
        assert (isinstance(height, int))
        p1, p2 = getKeystones(node, height)
        return ContextInfoContainer(height, p1, p2)

    def __init__(self, height=None, keystone1=None, keystone2=None):
        self.height: int = height
        self.keystone1: str = keystone1
        self.keystone2: str = keystone2
        self.txRoot: int = 0

    def getUnauthenticated(self):
        data = b''
        data += struct.pack(">I", self.height)
        data += bytes.fromhex(self.keystone1)[::-1]
        data += bytes.fromhex(self.keystone2)[::-1]
        return data

    def getUnauthenticatedHash(self):
        # a double sha of unauthenticated context
        return hash256(self.getUnauthenticated())

    def getTopLevelMerkleRoot(self):
        data = b''
        data += ser_uint256(self.txRoot)
        data += self.getUnauthenticatedHash()
        return uint256_from_str(hash256(data))

    def setTxRootInt(self, txRoot: int):
        assert isinstance(txRoot, int)
        self.txRoot = txRoot

    def setTxRootHex(self, txRoot: str):
        assert isinstance(txRoot, str)
        assert len(txRoot) == 64
        self.txRoot = int(txRoot, 16)

    def __repr__(self):
        return "ContextInfo(height={}, ks1={}, ks2={}, mroot={})".format(self.height, self.keystone1,
                                                                         self.keystone2,
                                                                         self.txRoot)
