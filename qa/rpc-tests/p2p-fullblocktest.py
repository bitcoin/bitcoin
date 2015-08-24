#!/usr/bin/env python2

#
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.comptool import TestManager, TestInstance
from test_framework.mininode import *
from test_framework.blocktools import *
import logging
import copy
import time
import numbers
from test_framework.key import CECKey
from test_framework.script import CScript, CScriptOp, SignatureHash, SIGHASH_ALL, OP_TRUE

class PreviousSpendableOutput(object):
    def __init__(self, tx = CTransaction(), n = -1):
        self.tx = tx
        self.n = n  # the output we're spending

'''
This reimplements tests from the bitcoinj/FullBlockTestGenerator used
by the pull-tester.

We use the testing framework in which we expect a particular answer from
each test.
'''

class FullBlockTest(ComparisonTestFramework):

    ''' Can either run this test as 1 node with expected answers, or two and compare them. 
        Change the "outcome" variable from each TestInstance object to only do the comparison. '''
    def __init__(self):
        self.num_nodes = 1
        self.block_heights = {}
        self.coinbase_key = CECKey()
        self.coinbase_key.set_secretbytes(bytes("horsebattery"))
        self.coinbase_pubkey = self.coinbase_key.get_pubkey()
        self.block_time = int(time.time())+1
        self.tip = None
        self.blocks = {}

    def run_test(self):
        test = TestManager(self, self.options.tmpdir)
        test.add_all_connections(self.nodes)
        NetworkThread().start() # Start up network handling in another thread
        test.run()

    def add_transactions_to_block(self, block, tx_list):
        [ tx.rehash() for tx in tx_list ]
        block.vtx.extend(tx_list)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.rehash()
        return block
    
    # Create a block on top of self.tip, and advance self.tip to point to the new block
    # if spend is specified, then 1 satoshi will be spent from that to an anyone-can-spend output,
    # and rest will go to fees.
    def next_block(self, number, spend=None, additional_coinbase_value=0, script=None):
        if self.tip == None:
            base_block_hash = self.genesis_hash
        else:
            base_block_hash = self.tip.sha256
        # First create the coinbase
        height = self.block_heights[base_block_hash] + 1
        coinbase = create_coinbase(height, self.coinbase_pubkey)
        coinbase.vout[0].nValue += additional_coinbase_value
        if (spend != None):
            coinbase.vout[0].nValue += spend.tx.vout[spend.n].nValue - 1 # all but one satoshi to fees
        coinbase.rehash()
        block = create_block(base_block_hash, coinbase, self.block_time)
        if (spend != None):
            tx = CTransaction()
            tx.vin.append(CTxIn(COutPoint(spend.tx.sha256, spend.n), "", 0xffffffff))  # no signature yet
            # This copies the java comparison tool testing behavior: the first
            # txout has a garbage scriptPubKey, "to make sure we're not
            # pre-verifying too much" (?)
            tx.vout.append(CTxOut(0, CScript([random.randint(0,255), height & 255])))
            if script == None:
                tx.vout.append(CTxOut(1, CScript([OP_TRUE])))
            else:
                tx.vout.append(CTxOut(1, script))
            # Now sign it if necessary
            scriptSig = ""
            scriptPubKey = bytearray(spend.tx.vout[spend.n].scriptPubKey)
            if (scriptPubKey[0] == OP_TRUE):  # looks like an anyone-can-spend
                scriptSig = CScript([OP_TRUE])
            else:
                # We have to actually sign it
                (sighash, err) = SignatureHash(spend.tx.vout[spend.n].scriptPubKey, tx, 0, SIGHASH_ALL)
                scriptSig = CScript([self.coinbase_key.sign(sighash) + bytes(bytearray([SIGHASH_ALL]))])
            tx.vin[0].scriptSig = scriptSig
            # Now add the transaction to the block
            block = self.add_transactions_to_block(block, [tx])
        block.solve()
        self.tip = block
        self.block_heights[block.sha256] = height
        self.block_time += 1
        assert number not in self.blocks
        self.blocks[number] = block
        return block

    def get_tests(self):
        self.genesis_hash = int(self.nodes[0].getbestblockhash(), 16)
        self.block_heights[self.genesis_hash] = 0
        spendable_outputs = []

        # save the current tip so it can be spent by a later block
        def save_spendable_output():
            spendable_outputs.append(self.tip)

        # get an output that we previous marked as spendable
        def get_spendable_output():
            return PreviousSpendableOutput(spendable_outputs.pop(0).vtx[0], 0)

        # returns a test case that asserts that the current tip was accepted
        def accepted():
            return TestInstance([[self.tip, True]])

        # returns a test case that asserts that the current tip was rejected
        def rejected():
            return TestInstance([[self.tip, False]])
       
        # move the tip back to a previous block
        def tip(number):
            self.tip = self.blocks[number]

        # creates a new block and advances the tip to that block
        block = self.next_block


        # Create a new block
        block(0)
        save_spendable_output()
        yield accepted()


        # Now we need that block to mature so we can spend the coinbase.
        test = TestInstance(sync_every_block=False)
        for i in range(100):
            block(1000 + i)
            test.blocks_and_transactions.append([self.tip, True])
            save_spendable_output()
        yield test


        # Start by bulding a couple of blocks on top (which output is spent is in parentheses):
        #     genesis -> b1 (0) -> b2 (1)
        out0 = get_spendable_output()
        block(1, spend=out0)
        save_spendable_output()
        yield accepted()

        out1 = get_spendable_output()
        block(2, spend=out1)
        # Inv again, then deliver twice (shouldn't break anything).
        yield accepted()


        # so fork like this:
        # 
        #     genesis -> b1 (0) -> b2 (1)
        #                      \-> b3 (1)
        # 
        # Nothing should happen at this point. We saw b2 first so it takes priority.
        tip(1)
        block(3, spend=out1)
        # Deliver twice (should still not break anything)
        yield rejected()


        # Now we add another block to make the alternative chain longer.
        # 
        #     genesis -> b1 (0) -> b2 (1)
        #                      \-> b3 (1) -> b4 (2)
        out2 = get_spendable_output()
        block(4, spend=out2)
        yield accepted()


        # ... and back to the first chain.
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6 (3)
        #                      \-> b3 (1) -> b4 (2)
        tip(2)
        block(5, spend=out2)
        save_spendable_output()
        yield rejected()

        out3 = get_spendable_output()
        block(6, spend=out3)
        yield accepted()


        # Try to create a fork that double-spends
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6 (3)
        #                                          \-> b7 (2) -> b8 (4)
        #                      \-> b3 (1) -> b4 (2)
        tip(5)
        block(7, spend=out2)
        yield rejected()

        out4 = get_spendable_output()
        block(8, spend=out4)
        yield rejected()


        # Try to create a block that has too much fee
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6 (3)
        #                                                    \-> b9 (4)
        #                      \-> b3 (1) -> b4 (2)
        tip(6)
        block(9, spend=out4, additional_coinbase_value=1)
        yield rejected()

        
        # Create a fork that ends in a block with too much fee (the one that causes the reorg)
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b10 (3) -> b11 (4)
        #                      \-> b3 (1) -> b4 (2)
        tip(5)
        block(10, spend=out3)
        yield rejected()

        block(11, spend=out4, additional_coinbase_value=1)
        yield rejected()


        # Try again, but with a valid fork first
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b14 (5)
        #                                              (b12 added last)
        #                      \-> b3 (1) -> b4 (2)
        tip(5)
        b12 = block(12, spend=out3)
        save_spendable_output()
        #yield TestInstance([[b12, False]])
        b13 = block(13, spend=out4)
        # Deliver the block header for b12, and the block b13.
        # b13 should be accepted but the tip won't advance until b12 is delivered.
        yield TestInstance([[CBlockHeader(b12), None], [b13, False]])

        save_spendable_output()
        out5 = get_spendable_output()
        # b14 is invalid, but the node won't know that until it tries to connect
        # Tip still can't advance because b12 is missing
        block(14, spend=out5, additional_coinbase_value=1)
        yield rejected()

        yield TestInstance([[b12, True, b13.sha256]]) # New tip should be b13.

        
        # Test that a block with a lot of checksigs is okay
        lots_of_checksigs = CScript([OP_CHECKSIG] * (1000000 / 50 - 1))
        tip(13)
        block(15, spend=out5, script=lots_of_checksigs)
        yield accepted()


        # Test that a block with too many checksigs is rejected
        out6 = get_spendable_output()
        too_many_checksigs = CScript([OP_CHECKSIG] * (1000000 / 50))
        block(16, spend=out6, script=too_many_checksigs)
        yield rejected()



if __name__ == '__main__':
    FullBlockTest().main()
