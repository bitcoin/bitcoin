#!/usr/bin/env python3
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test framework addition to include VeriBlock PoP functions"""
import struct
import time
from typing import Optional

from .messages import ser_uint256, hash256, uint256_from_str
from .pop_const import KEYSTONE_INTERVAL, NETWORK_ID
from .script import hash160, CScript, OP_DUP, OP_HASH160, OP_EQUALVERIFY, OP_CHECKSIG
from .test_node import TestNode
from .util import hex_str_to_bytes



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


# size = size of chain to be created
def create_endorsed_chain(node, apm, size: int, addr: str) -> None:
    hash = node.getbestblockhash()
    block = node.getblock(hash)
    height = block['height']
    initial_height = height

    for i in range(size):
        atv_id = endorse_block(node, apm, height, addr)
        containinghash = node.generate(nblocks=1)[0]
        # endorsing prev tip
        node.waitforblockheight(height + 1)
        containing = node.getblock(containinghash)
        assert atv_id in containing['pop']['data']['atvs'], \
            "iteration {}: containing block at height {}" \
            "does not contain pop tx {}".format(i, containing['height'], atv_id)

        # we advanced 1 block further
        height += 1

    node.waitforblockheight(initial_height + size)


def endorse_block(node, apm, height: int, addr: str, vtbs: Optional[int] = None) -> str:
    from pypopminer import PublicationData

    # get pubkey for that address
    pubkey = node.getaddressinfo(addr)['pubkey']
    pkh = hash160(hex_str_to_bytes(pubkey))
    script = CScript([OP_DUP, OP_HASH160, pkh, OP_EQUALVERIFY, OP_CHECKSIG])
    payoutInfo = script.hex()

    popdata = node.getpopdata(height)
    last_btc = popdata['last_known_bitcoin_blocks'][0]
    last_vbk = popdata['last_known_veriblock_blocks'][0]
    header = popdata['block_header']
    pub = PublicationData()
    pub.header = header
    pub.payoutInfo = payoutInfo
    pub.identifier = NETWORK_ID

    if vtbs:
        apm.endorseVbkBlock(last_vbk, last_btc, vtbs)

    payloads = apm.endorseAltBlock(pub, last_vbk)
    node.submitpop(*payloads.prepare())
    return payloads.atv.getId()


def mine_vbk_blocks(node, apm, amount: int) -> str:
    vbks = []
    for i in range(amount):
        vbks.append(apm.mineVbkBlocks(1))

    result = node.submitpop([b.toVbkEncodingHex() for b in vbks], [], [])
    return result['vbkblocks']


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


def sync_pop_tips(rpc_connections, *, wait=1, timeout=10, flush_scheduler=True):
    """
    Wait until everybody has the same POP TIPS (BTC tip and VBK tip)
    """

    def test(s):
        return s.count(s[0]) == len(rpc_connections)

    stop_time = time.time() + timeout
    while time.time() <= stop_time:
        btc = [r.getbtcbestblockhash() for r in rpc_connections]
        vbk = [r.getvbkbestblockhash() for r in rpc_connections]

        if test(btc) and test(vbk):
            if flush_scheduler:
                for r in rpc_connections:
                    r.syncwithvalidationinterfacequeue()
            return
        time.sleep(wait)
    raise AssertionError("POP data sync timed out: \nbtc: {}\nvbk: {}\n".format(
        "".join("\n  {!r}".format(m) for m in btc),
        "".join("\n  {!r}".format(m) for m in vbk),
    ))

def assert_pop_state_equal(nodes):
    def is_same(func, msg):
        s = [func(x) for x in nodes]
        if s.count(s[0]) == len(nodes):
            return True

        raise AssertionError("{}: \n    {}\n".format(
            msg,
            "".join("\n  {!r}".format(m) for m in s)
        ))

    is_same(lambda x: x.getbtcblock(x.getbtcbestblockhash()), "BTC tips")
    is_same(lambda x: x.getvbkblock(x.getvbkbestblockhash()), "VBK tips")
    is_same(lambda x: x.getblock(x.getbestblockhash()), "ALT tips")


def sync_pop_mempools(rpc_connections, *, wait=1, timeout=60, flush_scheduler=True):
    """
    Wait until everybody has the same POP data in their POP mempools
    """

    def test(s):
        return s.count(s[0]) == len(rpc_connections)

    stop_time = time.time() + timeout
    while time.time() <= stop_time:
        mpooldata = [r.getrawpopmempool() for r in rpc_connections]
        atvs = [set(data['atvs']) for data in mpooldata]
        vtbs = [set(data['vtbs']) for data in mpooldata]
        vbkblocks = [set(data['vbkblocks']) for data in mpooldata]

        if test(atvs) and test(vtbs) and test(vbkblocks):
            if flush_scheduler:
                for r in rpc_connections:
                    r.syncwithvalidationinterfacequeue()
            return
        time.sleep(wait)
    raise AssertionError("POP mempool sync timed out: \natvs: {}\nvtbs: {}\nvbkblocks:{}".format(
        "".join("\n  {!r}".format(m) for m in atvs),
        "".join("\n  {!r}".format(m) for m in vtbs),
        "".join("\n  {!r}".format(m) for m in vbkblocks)
    ))

def mine_until_pop_enabled(node):
    existing = node.getblockcount()
    activate = node.getblockchaininfo()['softforks']['pop_security']['height']
    assert activate >= 0, "POP security should be able to activate"
    if existing < activate:
        assert activate - existing < 1000, "POP security activates on height {}. Will take too long to activate".format(activate)
        node.generate(nblocks=(activate - existing))
        node.waitforblockheight(activate)
