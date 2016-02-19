#!/usr/bin/env python2
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.mininode import CTransaction, NetworkThread
from test_framework.blocktools import create_coinbase, create_block
from test_framework.comptool import TestInstance, TestManager
from test_framework.script import CScript, OP_1NEGATE, OP_NOP3, OP_DROP
from binascii import hexlify, unhexlify
import cStringIO
import time

def sequence_lock_invalidate(tx):
    '''Modify the nSequence to make it fails once sequence lock rule is activated (high timespan)
    '''
    tx.vin[0].nSequence = 0x00FFFFFF
    tx.nLockTime = 0

'''
This test is meant to exercise BIP68 (Sequence lock)
Connect to a single node.
Mine 2 (version 4) blocks (save the coinbases for later).
Generate 98 more version 4 blocks, verify the node accepts.
Mine 749 version 5 blocks, verify the node accepts.
Check that the new sequence lock rules are not enforced on the 750th version 5 block.
Check that the new sequence lock rules are enforced on the 751st version 5 block.
Mine 199 new version blocks.
Mine 1 old-version block.
Mine 1 new version block.
Mine 1 old version block, see that the node rejects.
'''

class BIP68Test(ComparisonTestFramework):

    def __init__(self):
        self.num_nodes = 1

    def setup_network(self):
        # Must set the blockversion for this test
        self.nodes = start_nodes(1, self.options.tmpdir,
                                 extra_args=[['-debug', '-whitelist=127.0.0.1', '-blockversion=4']],
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
        tx = CTransaction()
        f = cStringIO.StringIO(unhexlify(rawtx))
        tx.deserialize(f)
        tx.nVersion = 2
        return tx

    def sign_transaction(self, node, tx):
        signresult = node.signrawtransaction(hexlify(tx.serialize()))
        tx = CTransaction()
        f = cStringIO.StringIO(unhexlify(signresult['hex']))
        tx.deserialize(f)
        return tx

    def get_tests(self):

        self.coinbase_blocks = self.nodes[0].generate(2)
        height = 3  # height of the next block to build
        self.tip = int ("0x" + self.nodes[0].getbestblockhash() + "L", 0)
        self.nodeaddress = self.nodes[0].getnewaddress()
        self.last_block_time = time.time()

        ''' 98 more version 4 blocks '''
        test_blocks = []
        for i in xrange(98):
            block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
            block.nVersion = 4
            block.rehash()
            block.solve()
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            height += 1
        yield TestInstance(test_blocks, sync_every_block=False)

        ''' Mine 749 version 4 blocks '''
        test_blocks = []
        for i in xrange(749):
            block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
            block.nVersion = 5
            block.rehash()
            block.solve()
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            height += 1
        yield TestInstance(test_blocks, sync_every_block=False)

        '''
        Check that the new sequence lock rules are not enforced in the 750th
        version 4 block.
        '''
        spendtx = self.create_transaction(self.nodes[0],
                self.coinbase_blocks[0], self.nodeaddress, 1.0)
        sequence_lock_invalidate(spendtx)
        spendtx = self.sign_transaction(self.nodes[0], spendtx)
        spendtx.rehash()

        block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
        block.nVersion = 5
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()

        self.last_block_time += 1
        self.tip = block.sha256
        height += 1
        yield TestInstance([[block, True]])

        '''
        Check that the new sequence lock rules are enforced in the 751st version 5
        block.
        '''
        spendtx = self.create_transaction(self.nodes[0],
                self.coinbase_blocks[1], self.nodeaddress, 1.0)
        sequence_lock_invalidate(spendtx)
        spendtx = self.sign_transaction(self.nodes[0], spendtx)
        spendtx.rehash()

        block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
        block.nVersion = 5
        block.vtx.append(spendtx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        block.solve()
        self.last_block_time += 1
        yield TestInstance([[block, False]])

        ''' Mine 199 new version blocks on last valid tip '''
        test_blocks = []
        for i in xrange(199):
            block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
            block.nVersion = 5
            block.rehash()
            block.solve()
            test_blocks.append([block, True])
            self.last_block_time += 1
            self.tip = block.sha256
            height += 1
        yield TestInstance(test_blocks, sync_every_block=False)

        ''' Mine 1 old version block '''
        block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
        block.nVersion = 4
        block.rehash()
        block.solve()
        self.last_block_time += 1
        self.tip = block.sha256
        height += 1
        yield TestInstance([[block, True]])

        ''' Mine 1 new version block '''
        block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
        block.nVersion = 5
        block.rehash()
        block.solve()
        self.last_block_time += 1
        self.tip = block.sha256
        height += 1
        yield TestInstance([[block, True]])

        ''' Mine 1 old version block, should be invalid '''
        block = create_block(self.tip, create_coinbase(height), self.last_block_time + 1)
        block.nVersion = 4
        block.rehash()
        block.solve()
        self.last_block_time += 1
        yield TestInstance([[block, False]])

if __name__ == '__main__':
    BIP68Test().main()