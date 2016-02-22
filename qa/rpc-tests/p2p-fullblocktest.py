#!/usr/bin/env python2

#
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.comptool import TestManager, TestInstance, RejectResult
from test_framework.blocktools import *
import time
from test_framework.key import CECKey
from test_framework.script import CScript, SignatureHash, SIGHASH_ALL, OP_TRUE, OP_FALSE

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
        def rejected(reject = None):
            if reject is None:
                return TestInstance([[self.tip, False]])
            else:
                return TestInstance([[self.tip, reject]])
       
        # move the tip back to a previous block
        def tip(number):
            self.tip = self.blocks[number]

        # add transactions to a block produced by next_block
        def update_block(block_number, new_transactions):
            block = self.blocks[block_number]
            old_hash = block.sha256
            self.add_transactions_to_block(block, new_transactions)
            block.solve()
            # Update the internal state just like in next_block
            self.tip = block
            self.block_heights[block.sha256] = self.block_heights[old_hash]
            del self.block_heights[old_hash]
            self.blocks[block_number] = block
            return block

        # creates a new block and advances the tip to that block
        block = self.next_block


        # Create a new block
        block(0)
        save_spendable_output()
        yield accepted()


        # Now we need that block to mature so we can spend the coinbase.
        test = TestInstance(sync_every_block=False)
        for i in range(99):
            block(1000 + i)
            test.blocks_and_transactions.append([self.tip, True])
            save_spendable_output()
        yield test


        # Start by building a couple of blocks on top (which output is spent is
        # in parentheses):
        #     genesis -> b1 (0) -> b2 (1)
        out0 = get_spendable_output()
        block(1, spend=out0)
        save_spendable_output()
        yield accepted()

        out1 = get_spendable_output()
        b2 = block(2, spend=out1)
        yield accepted()


        # so fork like this:
        # 
        #     genesis -> b1 (0) -> b2 (1)
        #                      \-> b3 (1)
        # 
        # Nothing should happen at this point. We saw b2 first so it takes priority.
        tip(1)
        b3 = block(3, spend=out1)
        txout_b3 = PreviousSpendableOutput(b3.vtx[1], 1)
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
        yield rejected(RejectResult(16, 'bad-cb-amount'))

        
        # Create a fork that ends in a block with too much fee (the one that causes the reorg)
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b10 (3) -> b11 (4)
        #                      \-> b3 (1) -> b4 (2)
        tip(5)
        block(10, spend=out3)
        yield rejected()

        block(11, spend=out4, additional_coinbase_value=1)
        yield rejected(RejectResult(16, 'bad-cb-amount'))


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

        # Add a block with MAX_BLOCK_SIGOPS and one with one more sigop
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b16 (6)
        #                      \-> b3 (1) -> b4 (2)
        
        # Test that a block with a lot of checksigs is okay
        lots_of_checksigs = CScript([OP_CHECKSIG] * (1000000 / 50 - 1))
        tip(13)
        block(15, spend=out5, script=lots_of_checksigs)
        yield accepted()


        # Test that a block with too many checksigs is rejected
        out6 = get_spendable_output()
        too_many_checksigs = CScript([OP_CHECKSIG] * (1000000 / 50))
        block(16, spend=out6, script=too_many_checksigs)
        yield rejected(RejectResult(16, 'bad-blk-sigops'))


        # Attempt to spend a transaction created on a different fork
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b17 (b3.vtx[1])
        #                      \-> b3 (1) -> b4 (2)
        tip(15)
        block(17, spend=txout_b3)
        yield rejected(RejectResult(16, 'bad-txns-inputs-missingorspent'))

        # Attempt to spend a transaction created on a different fork (on a fork this time)
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5)
        #                                                                \-> b18 (b3.vtx[1]) -> b19 (6)
        #                      \-> b3 (1) -> b4 (2)
        tip(13)
        block(18, spend=txout_b3)
        yield rejected()

        block(19, spend=out6)
        yield rejected()

        # Attempt to spend a coinbase at depth too low
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b20 (7)
        #                      \-> b3 (1) -> b4 (2)
        tip(15)
        out7 = get_spendable_output()
        block(20, spend=out7)
        yield rejected(RejectResult(16, 'bad-txns-premature-spend-of-coinbase'))

        # Attempt to spend a coinbase at depth too low (on a fork this time)
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5)
        #                                                                \-> b21 (6) -> b22 (5)
        #                      \-> b3 (1) -> b4 (2)
        tip(13)
        block(21, spend=out6)
        yield rejected()

        block(22, spend=out5)
        yield rejected()

        # Create a block on either side of MAX_BLOCK_SIZE and make sure its accepted/rejected
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b23 (6)
        #                                                                           \-> b24 (6) -> b25 (7)
        #                      \-> b3 (1) -> b4 (2)
        tip(15)
        b23 = block(23, spend=out6)
        old_hash = b23.sha256
        tx = CTransaction()
        script_length = MAX_BLOCK_SIZE - len(b23.serialize()) - 69
        script_output = CScript([chr(0)*script_length])
        tx.vout.append(CTxOut(0, script_output))
        tx.vin.append(CTxIn(COutPoint(b23.vtx[1].sha256, 1)))
        b23 = update_block(23, [tx])
        # Make sure the math above worked out to produce a max-sized block
        assert_equal(len(b23.serialize()), MAX_BLOCK_SIZE)
        yield accepted()

        # Make the next block one byte bigger and check that it fails
        tip(15)
        b24 = block(24, spend=out6)
        script_length = MAX_BLOCK_SIZE - len(b24.serialize()) - 69
        script_output = CScript([chr(0)*(script_length+1)])
        tx.vout = [CTxOut(0, script_output)]
        b24 = update_block(24, [tx])
        assert_equal(len(b24.serialize()), MAX_BLOCK_SIZE+1)
        yield rejected(RejectResult(16, 'bad-blk-length'))

        b25 = block(25, spend=out7)
        yield rejected()

        # Create blocks with a coinbase input script size out of range
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b23 (6) -> b30 (7)
        #                                                                           \-> ... (6) -> ... (7)
        #                      \-> b3 (1) -> b4 (2)
        tip(15)
        b26 = block(26, spend=out6)
        b26.vtx[0].vin[0].scriptSig = chr(0)
        b26.vtx[0].rehash()
        # update_block causes the merkle root to get updated, even with no new
        # transactions, and updates the required state.
        b26 = update_block(26, [])
        yield rejected(RejectResult(16, 'bad-cb-length'))

        # Extend the b26 chain to make sure bitcoind isn't accepting b26
        b27 = block(27, spend=out7)
        yield rejected()

        # Now try a too-large-coinbase script
        tip(15)
        b28 = block(28, spend=out6)
        b28.vtx[0].vin[0].scriptSig = chr(0)*101
        b28.vtx[0].rehash()
        b28 = update_block(28, [])
        yield rejected(RejectResult(16, 'bad-cb-length'))

        # Extend the b28 chain to make sure bitcoind isn't accepted b28
        b29 = block(29, spend=out7)
        # TODO: Should get a reject message back with "bad-prevblk", except
        # there's a bug that prevents this from being detected.  Just note
        # failure for now, and add the reject result later.
        yield rejected()

        # b30 has a max-sized coinbase scriptSig.
        tip(23)
        b30 = block(30)
        b30.vtx[0].vin[0].scriptSig = chr(0)*100
        b30.vtx[0].rehash()
        b30 = update_block(30, [])
        yield accepted()


if __name__ == '__main__':
    FullBlockTest().main()
