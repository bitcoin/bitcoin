#!/usr/bin/env python3
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test framework addition to include VeriBlock PoP functions"""
import struct
import time
from typing import Optional

from .messages import ser_uint256, hash256, uint256_from_str, CBlockHeader, hash256lr
from .pop_const import KEYSTONE_INTERVAL, NETWORK_ID, POP_ACTIVATION_HEIGHT, POW_REWARD_PERCENTAGE, POP_PAYOUT_DELAY, \
    EMPTY_POPDATA_ROOT_V1
from .script import hash160, CScript, OP_DUP, OP_HASH160, OP_EQUALVERIFY, OP_CHECKSIG
from .test_node import TestNode
from .util import hex_str_to_bytes


def getHighestKeystoneAtOrBefore(height: int):
    assert height >= 0
    diff = height % KEYSTONE_INTERVAL
    return height - diff


def getFirstPreviousKeystoneHeight(height: int):
    if height <= 1:
        return 0

    ret = getHighestKeystoneAtOrBefore((height - 2) if ((height - 1) % KEYSTONE_INTERVAL) == 0 else (height - 1))
    return 0 if ret < 0 else ret


def getSecondPreviousKeystoneHeight(height: int):
    if height <= 1 + KEYSTONE_INTERVAL:
        return 0

    ret = getFirstPreviousKeystoneHeight(height) - KEYSTONE_INTERVAL
    return 0 if ret < 0 else ret


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


def endorse_block(node, apm, height: int, addr: str) -> str:
    from pypoptools.pypopminer import PublicationData

    # get pubkey for that address
    payoutInfo = node.validateaddress(addr)['scriptPubKey']

    popdata = node.getpopdatabyheight(height)
    authctx = popdata['authenticated_context']['serialized']
    last_vbk = popdata['last_known_veriblock_blocks'][-1]
    header = popdata['block_header']

    pub = PublicationData()
    pub.header = header
    pub.payoutInfo = payoutInfo
    pub.identifier = NETWORK_ID
    pub.contextInfo = authctx

    payloads = apm.endorseAltBlock(pub, last_vbk)

    for vbk_block in payloads.context:
        node.submitpopvbk(vbk_block.toVbkEncodingHex())
    for vtb in payloads.vtbs:
        node.submitpopvtb(vtb.toVbkEncodingHex())
    for atv in payloads.atvs:
        node.submitpopatv(atv.toVbkEncodingHex())

    return payloads.atvs[0].getId()


def mine_vbk_blocks(node, apm, amount: int):
    vbks = []
    for i in range(amount):
        vbks.append(apm.mineVbkBlocks(1))

    return [node.submitpopvbk(b.toVbkEncodingHex()) for b in vbks]


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


def mine_until_pop_enabled(node, address=None):
    existing = node.getblockcount()
    enabled = node.getpopparams()['bootstrapBlock']['height']
    assert enabled >= 0, "POP security should be able to enable"
    if existing < enabled:
        assert enabled - existing < 2000, "POP security enables on height {}. Will take too long to enable".format(
            enabled)
        if address is None:
            node.generate(nblocks=(enabled - existing))
        else:
            node.generatetoaddress(nblocks=(enabled - existing), address=address)
        node.waitforblockheight(enabled)


def mine_until_pop_active(node, address=None):
    existing = node.getblockcount()
    activate = node.getblockchaininfo()['softforks']['pop_security']['height']
    assert activate >= 0, "POP security should be able to activate"
    if existing < activate:
        assert activate - existing < 2000, "POP security activates on height {}. Will take too long to activate".format(
            activate)
        if address is None:
            node.generate(nblocks=(activate - existing))
        else:
            node.generatetoaddress(nblocks=(activate - existing), address=address)
        node.waitforblockheight(activate)


class BlockIndex:
    __slots__ = ("height", "hash", "prev")

    def __init__(self, height: int, hash: str, prev: str):
        self.height = height
        self.hash = hash
        self.prev = prev

    def __repr__(self):
        return "BlockIndex(height={} hash={} prev={})".format(self.height, self.hash, self.prev)


def write_single_byte_len_value(val):
    d = b''
    l = len(val)
    assert 0 <= l < 256
    # write length
    d += struct.pack(">B", l)
    # write value
    d += val
    return d


class ContextInfoContainer:
    __slots__ = ("height", "keystone1", "keystone2")

    def getHash(self) -> bytes:
        data = b''
        # zero-padded big-endian block height
        data += (b'\x00' * 4 + struct.pack(">I", self.height))[-4:]
        k1 = b'' if len(self.keystone1) == 0 else bytes.fromhex(self.keystone1)
        data += write_single_byte_len_value(k1)
        k2 = b'' if len(self.keystone2) == 0 else bytes.fromhex(self.keystone2)
        data += write_single_byte_len_value(k2)
        return hash256(data)

    def __str__(self):
        return "ContextInfo(height={} ks1={} ks2={})".format(self.height, self.keystone1, self.keystone2)

    def __repr__(self):
        return str(self)


class PopMiningContext:
    def __init__(self, node: TestNode):
        assert isinstance(node, TestNode)

        self.node = node
        self.params = node.getpopparams()

        bootstrap = self.params['bootstrapBlock']
        self.bootstrap = BlockIndex(
            height=bootstrap['height'],
            # TODO: put in normal order ALT-363
            # reverse bootstrap block hash
            hash=bytes.fromhex(bootstrap['hash'])[::-1].hex(),
            prev=""
        )
        self.blocks = {
            self.bootstrap.hash: self.bootstrap
        }

        def test_config(varname, varvalue, getter):
            k = getter(self.params)
            if varvalue != k:
                raise Exception(
                    "Update {} from {} to {} (got value from getpopparams rpc)".format(
                        varname,
                        varvalue,
                        k
                    )
                )

        test_config("KEYSTONE_INTERVAL", KEYSTONE_INTERVAL, lambda x: x["keystoneInterval"])
        test_config("NETWORK_ID", NETWORK_ID, lambda x: x["networkId"])
        test_config("POP_ACTIVATION_HEIGHT", POP_ACTIVATION_HEIGHT, lambda x: x["popActivationHeight"])
        test_config("POW_REWARD_PERCENTAGE", POW_REWARD_PERCENTAGE, lambda x: x["popRewardPercentage"])
        test_config("POP_PAYOUT_DELAY", POP_PAYOUT_DELAY, lambda x: x["payoutParams"]["popPayoutDelay"])

    def get_block(self, hash: str) -> BlockIndex:
        # first, search in local blocks
        if hash in self.blocks:
            return self.blocks[hash]

        # if not found, try to get one from the node
        try:
            block = self.node.getblock(hash)
            b = BlockIndex(
                height=block['height'],
                hash=block['hash'],
                prev=block['previousblockhash']
            )
            self.blocks[b.hash] = b
            return b
        except Exception as e:
            raise Exception("Can not find block={}".format(hash), e)

    def accept_block(self, height: int, hash: str, prevHash: str):
        if prevHash not in self.blocks:
            # fetch prevHash BlockIndex from the node.
            try:
                prev = self.node.getblock(prevHash)
                p = BlockIndex(
                    height=height - 1,
                    hash=prevHash,
                    prev=prev['previousblockhash']
                )
                self.blocks[p.hash] = p
            except Exception as e:
                raise Exception("Unknown prev block={}. \nException={}".format(prevHash, e))

        if prevHash in self.blocks:
            # connect next block
            index = BlockIndex(
                height=height,
                hash=hash,
                prev=prevHash
            )
            self.blocks[hash] = index
        else:
            raise Exception("Unknown prev={}:{}".format(height, prevHash))

    def get_context_info_from_prev(self, prevHash: str) -> ContextInfoContainer:
        c = ContextInfoContainer()
        c.keystone1 = ""
        c.keystone2 = ""
        try:
            prev = self.get_block(prevHash)
            c.height = prev.height + 1
            ks1height = getFirstPreviousKeystoneHeight(c.height)
            ks2height = getSecondPreviousKeystoneHeight(c.height)

            ks1 = self._get_ancestor(prevHash, ks1height)
            if ks1:
                # TODO: remove hash reversal here when ALT-363 is implemented
                c.keystone1 = bytes.fromhex(ks1.hash)[::-1].hex()
                ks2 = self._get_ancestor(ks1.hash, ks2height)
                if ks2:
                    # TODO: remove hash reversal here when ALT-363 is implemented
                    c.keystone2 = bytes.fromhex(ks2.hash)[::-1].hex()
        except:
            c.height = self.bootstrap.height

        return c

    # returns response of 'getblock' of ancestor of 'hash' on height 'height'
    def _get_ancestor(self, hash: str, height: int):
        _block = self.get_block(hash)

        while not _block or _block.height > height:
            _block = self.get_block(_block.prev)

        return _block


def calculateTopLevelMerkleRoot(
        popctx: PopMiningContext,
        txRoot: int,
        prevHash: str,
        popDataRoot: bytes = EMPTY_POPDATA_ROOT_V1
) -> int:
    ctx = popctx.get_context_info_from_prev(prevHash)
    return _calculateTopLevelMerkleRoot(txRoot, popDataRoot, ctx)


def _calculateTopLevelMerkleRoot(
        txRoot: int,
        popDataRoot: bytes,
        ctx: ContextInfoContainer
) -> int:
    if ctx.height < POP_ACTIVATION_HEIGHT:
        # pop is not activated yet so we use old algos
        return txRoot

    # POP is enabled
    ch = ctx.getHash()
    txroot = ser_uint256(txRoot)
    stateRoot = hash256lr(txroot, popDataRoot)
    return uint256_from_str(hash256lr(stateRoot, ch))
