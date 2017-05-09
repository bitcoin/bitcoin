#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.mininode import CTransaction, NetworkThread
from test_framework.blocktools import create_coinbase, create_block
from test_framework.comptool import TestInstance, TestManager
from test_framework.script import CScript, OP_1NEGATE, OP_CHECKLOCKTIMEVERIFY, OP_DROP
from io import BytesIO
import time

def cltv_invalidate(tx):
    '''Modify the signature in vin 0 of the tx to fail CLTV

    Prepends -1 CLTV DROP in the scriptSig itself.
    '''
    tx.vin[0].scriptSig = CScript([OP_1NEGATE, OP_CHECKLOCKTIMEVERIFY, OP_DROP] +
                                  list(CScript(tx.vin[0].scriptSig)))

'''
This test is meant to exercise BIP65 (CHECKLOCKTIMEVERIFY)
Connect to a single node.
Mine 2 (version 3) blocks (save the coinbases for later).
Generate 98 more version 3 blocks, verify the node accepts.
Mine 749 version 4 blocks, verify the node accepts.
Check that the new CLTV rules are not enforced on the 750th version 4 block.
Check that the new CLTV rules are enforced on the 751st version 4 block.
Mine 199 new version blocks.
Mine 1 old-version block.
Mine 1 new version block.
Mine 1 old version block, see that the node rejects.
'''

class BIP65Test(ComparisonTestFramework):

    def __init__(self):
        super().__init__()
        self.num_nodes = 1

    def setup_network(self):
        # Must set the blockversion for this test
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir,
                                 extra_args=[['-debug', '-whitelist=127.0.0.1', '-blockversion=3']],
                                 binary=[self.options.testbinary])

    def run_test(self):
        test = TestManager(self, self.options.tmpdir)
        test.add_all_connections(self.nodes)
        NetworkThread().start() # Start up network handling in another thread
        test.run()

    def create_transaction(self, node, coinbase, to_address, amount):
        from_txid = node.getblock(coinbase)['tx'][0]
        inputs = [{ "txid" : from_txid, "vout" : 0}]
        outputs = { to_address : amount }
        rawtx = node.createrawtransaction(inputs, outputs)
        signresult = node.signrawtransaction(rawtx)
        tx = CTransaction()
        f = BytesIO(hex_str_to_bytes(signresult['hex']))
        tx.deserialize(f)
        return tx

    def get_tests(self):

        self.coinbase_blocks = self.nodes[0].generate(2)
        height = 3  # height of the next block to build
        self.tip = int("0x" + self.nodes[0].getbestblockhash(), 0)
        self.nodeaddress = self.nodes[0].getnewaddress()
        self.last_block_time = int(time.time())

        ''' 98 more version 3 blocks '''
        test_blocks = []
        for i in range(98):
            block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
            block.nVersion = 3
            block.rehash()
            block.solve()
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            height += 1
        yield TestInstance(test_blocks, sync_every_block=False)

        ''' Mine 749 version 4 blocks '''
        test_blocks = []
        for i in range(749):
            block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
            block.nVersion = 4
            block.rehash()
            block.solve()
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            height += 1
        yield TestInstance(test_blocks, sync_every_block=False)

        '''
        Check that the new CLTV rules are not enforced in the 750th
        version 3 block.
        '''
        spendtx = self.create_transaction(self.nodes[0],
                self.coinbase_blocks[0], self.nodeaddress, 1.0)
        cltv_invalidate(spendtx)
        spendtx.rehash()

        block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
        block.nVersion = 4
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()

        self.last_block_time += 1
        self.tip = block.sha256
        height += 1
        yield TestInstance([[block, True]])

        '''
        Check that the new CLTV rules are enforced in the 751st version 4
        block.
        '''
        spendtx = self.create_transaction(self.nodes[0],
                self.coinbase_blocks[1], self.nodeaddress, 1.0)
        cltv_invalidate(spendtx)
        spendtx.rehash()

        block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
        block.nVersion = 4
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        self.last_block_time += 1
        yield TestInstance([[block, False]])

        ''' Mine 199 new version blocks on last valid tip '''
        test_blocks = []
        for i in range(199):
            block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
            block.nVersion = 4
            block.rehash()
            block.solve()
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            height += 1
        yield TestInstance(test_blocks, sync_every_block=False)

        ''' Mine 1 old version block '''
        block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
        block.nVersion = 3
        block.rehash()
        block.solve()
        self.last_block_time += 1
        self.tip = block.sha256
        height += 1
        yield TestInstance([[block, True]])

        ''' Mine 1 new version block '''
        block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
        block.nVersion = 4
        block.rehash()
        block.solve()
        self.last_block_time += 1
        self.tip = block.sha256
        height += 1
        yield TestInstance([[block, True]])

        ''' Mine 1 old version block, should be invalid '''
        block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
        block.nVersion = 3
        block.rehash()
        block.solve()
        self.last_block_time += 1
        yield TestInstance([[block, False]])

if __name__ == '__main__':
    BIP65Test().main()
