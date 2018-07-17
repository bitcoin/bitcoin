#!/usr/bin/env python3
# Copyright (c) 2015-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid blocks.

In this test we connect to one node over p2p, and test block requests:
1) Valid blocks should be requested and become chain tip.
2) Invalid block with duplicated transaction should be re-requested.
3) Invalid block with bad coinbase value should be rejected and not
re-requested.
"""

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.comptool import TestManager, TestInstance, RejectResult
from test_framework.blocktools import *
from test_framework.mininode import network_thread_start
import copy
import time

# Use the ComparisonTestFramework with 1 node: only use --testbinary.
class InvalidBlockRequestTest(ComparisonTestFramework):

    ''' Can either run this test as 1 node with expected answers, or two and compare them. 
        Change the "outcome" variable from each TestInstance object to only do the comparison. '''
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        test = TestManager(self, self.options.tmpdir)
        test.add_all_connections(self.nodes)
        self.tip = None
        self.block_time = None
        network_thread_start()
        test.run()

    def get_tests(self):
        if self.tip is None:
            self.tip = int("0x" + self.nodes[0].getbestblockhash(), 0)
        self.block_time = int(time.time())+1

        '''
        Create a new block with an anyone-can-spend coinbase
        '''
        height = 1
        block = create_block(self.tip, create_coinbase(height), self.block_time)
        self.block_time += 1
        block.solve()
        # Save the coinbase for later
        self.block1 = block
        self.tip = block.sha256
        height += 1
        yield TestInstance([[block, True]])

        '''
        Now we need that block to mature so we can spend the coinbase.
        '''
        test = TestInstance(sync_every_block=False)
        for i in range(100):
            block = create_block(self.tip, create_coinbase(height), self.block_time)
            block.solve()
            self.tip = block.sha256
            self.block_time += 1
            test.blocks_and_transactions.append([block, True])
            height += 1
        yield test

        '''
        Now we use merkle-root malleability to generate an invalid block with
        same blockheader.
        Manufacture a block with 3 transactions (coinbase, spend of prior
        coinbase, spend of that spend).  Duplicate the 3rd transaction to 
        leave merkle root and blockheader unchanged but invalidate the block.
        '''
        block2 = create_block(self.tip, create_coinbase(height), self.block_time)
        self.block_time += 1

        # b'0x51' is OP_TRUE
        tx1 = create_transaction(self.block1.vtx[0], 0, b'\x51', 50 * COIN)
        tx2 = create_transaction(tx1, 0, b'\x51', 50 * COIN)

        block2.vtx.extend([tx1, tx2])
        block2.hashMerkleRoot = block2.calc_merkle_root()
        block2.rehash()
        block2.solve()
        orig_hash = block2.sha256
        block2_orig = copy.deepcopy(block2)

        # Mutate block 2
        block2.vtx.append(tx2)
        assert_equal(block2.hashMerkleRoot, block2.calc_merkle_root())
        assert_equal(orig_hash, block2.rehash())
        assert(block2_orig.vtx != block2.vtx)

        self.tip = block2.sha256
        yield TestInstance([[block2, RejectResult(16, b'bad-txns-duplicate')], [block2_orig, True]])
        height += 1

        '''
        Make sure that a totally screwed up block is not valid.
        '''
        block3 = create_block(self.tip, create_coinbase(height), self.block_time)
        self.block_time += 1
        block3.vtx[0].vout[0].nValue = 100 * COIN # Too high!
        block3.vtx[0].sha256=None
        block3.vtx[0].calc_sha256()
        block3.hashMerkleRoot = block3.calc_merkle_root()
        block3.rehash()
        block3.solve()

        yield TestInstance([[block3, RejectResult(16, b'bad-cb-amount')]])


if __name__ == '__main__':
    InvalidBlockRequestTest().main()
