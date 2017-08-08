#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mining RPCs

- getblocktemplate proposal mode
- submitblock"""

from binascii import b2a_hex
import copy

from test_framework.blocktools import create_coinbase
from test_framework.test_framework import BitcoinTestFramework
from test_framework.mininode import CBlock
from test_framework.util import *

def b2x(b):
    return b2a_hex(b).decode('ascii')

def assert_template(node, block, expect, rehash=True):
    if rehash:
        block.hashMerkleRoot = block.calc_merkle_root()
    rsp = node.getblocktemplate({'data': b2x(block.serialize()), 'mode': 'proposal'})
    assert_equal(rsp, expect)

class MiningTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = False

    def run_test(self):
        node = self.nodes[0]
        # Mine a block to leave initial block download
        node.generate(1)
        tmpl = node.getblocktemplate()
        self.log.info("getblocktemplate: Test capability advertised")
        assert 'proposal' in tmpl['capabilities']
        assert 'coinbasetxn' not in tmpl

        coinbase_tx = create_coinbase(height=int(tmpl["height"]) + 1)
        # sequence numbers must not be max for nLockTime to have effect
        coinbase_tx.vin[0].nSequence = 2 ** 32 - 2
        coinbase_tx.rehash()

        block = CBlock()
        block.nVersion = tmpl["version"]
        block.hashPrevBlock = int(tmpl["previousblockhash"], 16)
        block.nTime = tmpl["curtime"]
        block.nBits = int(tmpl["bits"], 16)
        block.nNonce = 0
        block.vtx = [coinbase_tx]

        self.log.info("getblocktemplate: Test valid block")
        assert_template(node, block, None)

        self.log.info("submitblock: Test block decode failure")
        assert_raises_jsonrpc(-22, "Block decode failed", node.submitblock, b2x(block.serialize()[:-15]))

        self.log.info("getblocktemplate: Test bad input hash for coinbase transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx[0].vin[0].prevout.hash += 1
        bad_block.vtx[0].rehash()
        assert_template(node, bad_block, 'bad-cb-missing')

        self.log.info("submitblock: Test invalid coinbase transaction")
        assert_raises_jsonrpc(-22, "Block does not start with a coinbase", node.submitblock, b2x(bad_block.serialize()))

        self.log.info("getblocktemplate: Test truncated final transaction")
        assert_raises_jsonrpc(-22, "Block decode failed", node.getblocktemplate, {'data': b2x(block.serialize()[:-1]), 'mode': 'proposal'})

        self.log.info("getblocktemplate: Test duplicate transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx.append(bad_block.vtx[0])
        assert_template(node, bad_block, 'bad-txns-duplicate')

        self.log.info("getblocktemplate: Test invalid transaction")
        bad_block = copy.deepcopy(block)
        bad_tx = copy.deepcopy(bad_block.vtx[0])
        bad_tx.vin[0].prevout.hash = 255
        bad_tx.rehash()
        bad_block.vtx.append(bad_tx)
        assert_template(node, bad_block, 'bad-txns-inputs-missingorspent')

        self.log.info("getblocktemplate: Test nonfinal transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx[0].nLockTime = 2 ** 32 - 1
        bad_block.vtx[0].rehash()
        assert_template(node, bad_block, 'bad-txns-nonfinal')

        self.log.info("getblocktemplate: Test bad tx count")
        # The tx count is immediately after the block header
        TX_COUNT_OFFSET = 80
        bad_block_sn = bytearray(block.serialize())
        assert_equal(bad_block_sn[TX_COUNT_OFFSET], 1)
        bad_block_sn[TX_COUNT_OFFSET] += 1
        assert_raises_jsonrpc(-22, "Block decode failed", node.getblocktemplate, {'data': b2x(bad_block_sn), 'mode': 'proposal'})

        self.log.info("getblocktemplate: Test bad bits")
        bad_block = copy.deepcopy(block)
        bad_block.nBits = 469762303  # impossible in the real world
        assert_template(node, bad_block, 'bad-diffbits')

        self.log.info("getblocktemplate: Test bad merkle root")
        bad_block = copy.deepcopy(block)
        bad_block.hashMerkleRoot += 1
        assert_template(node, bad_block, 'bad-txnmrklroot', False)

        self.log.info("getblocktemplate: Test bad timestamps")
        bad_block = copy.deepcopy(block)
        bad_block.nTime = 2 ** 31 - 1
        assert_template(node, bad_block, 'time-too-new')
        bad_block.nTime = 0
        assert_template(node, bad_block, 'time-too-old')

        self.log.info("getblocktemplate: Test not best block")
        bad_block = copy.deepcopy(block)
        bad_block.hashPrevBlock = 123
        assert_template(node, bad_block, 'inconclusive-not-best-prevblk')

if __name__ == '__main__':
    MiningTest().main()
