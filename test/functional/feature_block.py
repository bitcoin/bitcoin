#!/usr/bin/env python3
# Copyright (c) 2015-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test block processing.

This reimplements tests from the bitcoinj/FullBlockTestGenerator used
by the pull-tester.

We use the testing framework in which we expect a particular answer from
each test.
"""

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.comptool import TestManager, TestInstance, RejectResult
from test_framework.blocktools import *
import time
from test_framework.key import CECKey
from test_framework.script import *
from test_framework.mininode import network_thread_start
import struct

class PreviousSpendableOutput():
    def __init__(self, tx = CTransaction(), n = -1):
        self.tx = tx
        self.n = n  # the output we're spending

#  Use this class for tests that require behavior other than normal "mininode" behavior.
#  For now, it is used to serialize a bloated varint (b64).
class CBrokenBlock(CBlock):
    def __init__(self, header=None):
        super(CBrokenBlock, self).__init__(header)

    def initialize(self, base_block):
        self.vtx = copy.deepcopy(base_block.vtx)
        self.hashMerkleRoot = self.calc_merkle_root()

    def serialize(self, with_witness=False):
        r = b""
        r += super(CBlock, self).serialize()
        r += struct.pack("<BQ", 255, len(self.vtx))
        for tx in self.vtx:
            if with_witness:
                r += tx.serialize_with_witness()
            else:
                r += tx.serialize_without_witness()
        return r

    def normal_serialize(self):
        r = b""
        r += super(CBrokenBlock, self).serialize()
        return r

class FullBlockTest(ComparisonTestFramework):
    # Can either run this test as 1 node with expected answers, or two and compare them.
    # Change the "outcome" variable from each TestInstance object to only do the comparison.
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.block_heights = {}
        self.coinbase_key = CECKey()
        self.coinbase_key.set_secretbytes(b"horsebattery")
        self.coinbase_pubkey = self.coinbase_key.get_pubkey()
        self.tip = None
        self.blocks = {}

    def add_options(self, parser):
        super().add_options(parser)
        parser.add_option("--runbarelyexpensive", dest="runbarelyexpensive", default=True)

    def run_test(self):
        self.test = TestManager(self, self.options.tmpdir)
        self.test.add_all_connections(self.nodes)
        network_thread_start()
        self.test.run()

    def add_transactions_to_block(self, block, tx_list):
        [ tx.rehash() for tx in tx_list ]
        block.vtx.extend(tx_list)

    # this is a little handier to use than the version in blocktools.py
    def create_tx(self, spend_tx, n, value, script=CScript([OP_TRUE])):
        tx = create_transaction(spend_tx, n, b"", value, script)
        return tx

    # sign a transaction, using the key we know about
    # this signs input 0 in tx, which is assumed to be spending output n in spend_tx
    def sign_tx(self, tx, spend_tx, n):
        scriptPubKey = bytearray(spend_tx.vout[n].scriptPubKey)
        if (scriptPubKey[0] == OP_TRUE):  # an anyone-can-spend
            tx.vin[0].scriptSig = CScript()
            return
        (sighash, err) = SignatureHash(spend_tx.vout[n].scriptPubKey, tx, 0, SIGHASH_ALL)
        tx.vin[0].scriptSig = CScript([self.coinbase_key.sign(sighash) + bytes(bytearray([SIGHASH_ALL]))])

    def create_and_sign_transaction(self, spend_tx, n, value, script=CScript([OP_TRUE])):
        tx = self.create_tx(spend_tx, n, value, script)
        self.sign_tx(tx, spend_tx, n)
        tx.rehash()
        return tx

    def next_block(self, number, spend=None, additional_coinbase_value=0, script=CScript([OP_TRUE]), solve=True):
        if self.tip == None:
            base_block_hash = self.genesis_hash
            block_time = int(time.time())+1
        else:
            base_block_hash = self.tip.sha256
            block_time = self.tip.nTime + 1
        # First create the coinbase
        height = self.block_heights[base_block_hash] + 1
        coinbase = create_coinbase(height, self.coinbase_pubkey)
        coinbase.vout[0].nValue += additional_coinbase_value
        coinbase.rehash()
        if spend == None:
            block = create_block(base_block_hash, coinbase, block_time)
        else:
            coinbase.vout[0].nValue += spend.tx.vout[spend.n].nValue - 1 # all but one satoshi to fees
            coinbase.rehash()
            block = create_block(base_block_hash, coinbase, block_time)
            tx = create_transaction(spend.tx, spend.n, b"", 1, script)  # spend 1 satoshi
            self.sign_tx(tx, spend.tx, spend.n)
            self.add_transactions_to_block(block, [tx])
            block.hashMerkleRoot = block.calc_merkle_root()
        if solve:
            block.solve()
        self.tip = block
        self.block_heights[block.sha256] = height
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

        # get an output that we previously marked as spendable
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

        # adds transactions to the block and updates state
        def update_block(block_number, new_transactions):
            block = self.blocks[block_number]
            self.add_transactions_to_block(block, new_transactions)
            old_sha256 = block.sha256
            block.hashMerkleRoot = block.calc_merkle_root()
            block.solve()
            # Update the internal state just like in next_block
            self.tip = block
            if block.sha256 != old_sha256:
                self.block_heights[block.sha256] = self.block_heights[old_sha256]
                del self.block_heights[old_sha256]
            self.blocks[block_number] = block
            return block

        # shorthand for functions
        block = self.next_block
        create_tx = self.create_tx
        create_and_sign_tx = self.create_and_sign_transaction

        # these must be updated if consensus changes
        MAX_BLOCK_SIGOPS = 20000


        # Create a new block
        block(0)
        save_spendable_output()
        yield accepted()


        # Now we need that block to mature so we can spend the coinbase.
        test = TestInstance(sync_every_block=False)
        for i in range(99):
            block(5000 + i)
            test.blocks_and_transactions.append([self.tip, True])
            save_spendable_output()
        yield test

        # collect spendable outputs now to avoid cluttering the code later on
        out = []
        for i in range(33):
            out.append(get_spendable_output())

        # Start by building a couple of blocks on top (which output is spent is
        # in parentheses):
        #     genesis -> b1 (0) -> b2 (1)
        block(1, spend=out[0])
        save_spendable_output()
        yield accepted()

        block(2, spend=out[1])
        yield accepted()
        save_spendable_output()

        # so fork like this:
        #
        #     genesis -> b1 (0) -> b2 (1)
        #                      \-> b3 (1)
        #
        # Nothing should happen at this point. We saw b2 first so it takes priority.
        tip(1)
        b3 = block(3, spend=out[1])
        txout_b3 = PreviousSpendableOutput(b3.vtx[1], 0)
        yield rejected()


        # Now we add another block to make the alternative chain longer.
        #
        #     genesis -> b1 (0) -> b2 (1)
        #                      \-> b3 (1) -> b4 (2)
        block(4, spend=out[2])
        yield accepted()


        # ... and back to the first chain.
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6 (3)
        #                      \-> b3 (1) -> b4 (2)
        tip(2)
        block(5, spend=out[2])
        save_spendable_output()
        yield rejected()

        block(6, spend=out[3])
        yield accepted()

        # Try to create a fork that double-spends
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6 (3)
        #                                          \-> b7 (2) -> b8 (4)
        #                      \-> b3 (1) -> b4 (2)
        tip(5)
        block(7, spend=out[2])
        yield rejected()

        block(8, spend=out[4])
        yield rejected()

        # Try to create a block that has too much fee
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6 (3)
        #                                                    \-> b9 (4)
        #                      \-> b3 (1) -> b4 (2)
        tip(6)
        block(9, spend=out[4], additional_coinbase_value=1)
        yield rejected(RejectResult(16, b'bad-cb-amount'))

        # Create a fork that ends in a block with too much fee (the one that causes the reorg)
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b10 (3) -> b11 (4)
        #                      \-> b3 (1) -> b4 (2)
        tip(5)
        block(10, spend=out[3])
        yield rejected()

        block(11, spend=out[4], additional_coinbase_value=1)
        yield rejected(RejectResult(16, b'bad-cb-amount'))


        # Try again, but with a valid fork first
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b14 (5)
        #                                              (b12 added last)
        #                      \-> b3 (1) -> b4 (2)
        tip(5)
        b12 = block(12, spend=out[3])
        save_spendable_output()
        b13 = block(13, spend=out[4])
        # Deliver the block header for b12, and the block b13.
        # b13 should be accepted but the tip won't advance until b12 is delivered.
        yield TestInstance([[CBlockHeader(b12), None], [b13, False]])

        save_spendable_output()
        # b14 is invalid, but the node won't know that until it tries to connect
        # Tip still can't advance because b12 is missing
        block(14, spend=out[5], additional_coinbase_value=1)
        yield rejected()

        yield TestInstance([[b12, True, b13.sha256]]) # New tip should be b13.

        # Add a block with MAX_BLOCK_SIGOPS and one with one more sigop
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b16 (6)
        #                      \-> b3 (1) -> b4 (2)

        # Test that a block with a lot of checksigs is okay
        lots_of_checksigs = CScript([OP_CHECKSIG] * (MAX_BLOCK_SIGOPS - 1))
        tip(13)
        block(15, spend=out[5], script=lots_of_checksigs)
        yield accepted()
        save_spendable_output()


        # Test that a block with too many checksigs is rejected
        too_many_checksigs = CScript([OP_CHECKSIG] * (MAX_BLOCK_SIGOPS))
        block(16, spend=out[6], script=too_many_checksigs)
        yield rejected(RejectResult(16, b'bad-blk-sigops'))


        # Attempt to spend a transaction created on a different fork
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b17 (b3.vtx[1])
        #                      \-> b3 (1) -> b4 (2)
        tip(15)
        block(17, spend=txout_b3)
        yield rejected(RejectResult(16, b'bad-txns-inputs-missingorspent'))

        # Attempt to spend a transaction created on a different fork (on a fork this time)
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5)
        #                                                                \-> b18 (b3.vtx[1]) -> b19 (6)
        #                      \-> b3 (1) -> b4 (2)
        tip(13)
        block(18, spend=txout_b3)
        yield rejected()

        block(19, spend=out[6])
        yield rejected()

        # Attempt to spend a coinbase at depth too low
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b20 (7)
        #                      \-> b3 (1) -> b4 (2)
        tip(15)
        block(20, spend=out[7])
        yield rejected(RejectResult(16, b'bad-txns-premature-spend-of-coinbase'))

        # Attempt to spend a coinbase at depth too low (on a fork this time)
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5)
        #                                                                \-> b21 (6) -> b22 (5)
        #                      \-> b3 (1) -> b4 (2)
        tip(13)
        block(21, spend=out[6])
        yield rejected()

        block(22, spend=out[5])
        yield rejected()

        # Create a block on either side of MAX_BLOCK_BASE_SIZE and make sure its accepted/rejected
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b23 (6)
        #                                                                           \-> b24 (6) -> b25 (7)
        #                      \-> b3 (1) -> b4 (2)
        tip(15)
        b23 = block(23, spend=out[6])
        tx = CTransaction()
        script_length = MAX_BLOCK_BASE_SIZE - len(b23.serialize()) - 69
        script_output = CScript([b'\x00' * script_length])
        tx.vout.append(CTxOut(0, script_output))
        tx.vin.append(CTxIn(COutPoint(b23.vtx[1].sha256, 0)))
        b23 = update_block(23, [tx])
        # Make sure the math above worked out to produce a max-sized block
        assert_equal(len(b23.serialize()), MAX_BLOCK_BASE_SIZE)
        yield accepted()
        save_spendable_output()

        # Make the next block one byte bigger and check that it fails
        tip(15)
        b24 = block(24, spend=out[6])
        script_length = MAX_BLOCK_BASE_SIZE - len(b24.serialize()) - 69
        script_output = CScript([b'\x00' * (script_length+1)])
        tx.vout = [CTxOut(0, script_output)]
        b24 = update_block(24, [tx])
        assert_equal(len(b24.serialize()), MAX_BLOCK_BASE_SIZE+1)
        yield rejected(RejectResult(16, b'bad-blk-length'))

        block(25, spend=out[7])
        yield rejected()

        # Create blocks with a coinbase input script size out of range
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b23 (6) -> b30 (7)
        #                                                                           \-> ... (6) -> ... (7)
        #                      \-> b3 (1) -> b4 (2)
        tip(15)
        b26 = block(26, spend=out[6])
        b26.vtx[0].vin[0].scriptSig = b'\x00'
        b26.vtx[0].rehash()
        # update_block causes the merkle root to get updated, even with no new
        # transactions, and updates the required state.
        b26 = update_block(26, [])
        yield rejected(RejectResult(16, b'bad-cb-length'))

        # Extend the b26 chain to make sure bitcoind isn't accepting b26
        block(27, spend=out[7])
        yield rejected(False)

        # Now try a too-large-coinbase script
        tip(15)
        b28 = block(28, spend=out[6])
        b28.vtx[0].vin[0].scriptSig = b'\x00' * 101
        b28.vtx[0].rehash()
        b28 = update_block(28, [])
        yield rejected(RejectResult(16, b'bad-cb-length'))

        # Extend the b28 chain to make sure bitcoind isn't accepting b28
        block(29, spend=out[7])
        yield rejected(False)

        # b30 has a max-sized coinbase scriptSig.
        tip(23)
        b30 = block(30)
        b30.vtx[0].vin[0].scriptSig = b'\x00' * 100
        b30.vtx[0].rehash()
        b30 = update_block(30, [])
        yield accepted()
        save_spendable_output()

        # b31 - b35 - check sigops of OP_CHECKMULTISIG / OP_CHECKMULTISIGVERIFY / OP_CHECKSIGVERIFY
        #
        #     genesis -> ... -> b30 (7) -> b31 (8) -> b33 (9) -> b35 (10)
        #                                                                \-> b36 (11)
        #                                                    \-> b34 (10)
        #                                         \-> b32 (9)
        #

        # MULTISIG: each op code counts as 20 sigops.  To create the edge case, pack another 19 sigops at the end.
        lots_of_multisigs = CScript([OP_CHECKMULTISIG] * ((MAX_BLOCK_SIGOPS-1) // 20) + [OP_CHECKSIG] * 19)
        b31 = block(31, spend=out[8], script=lots_of_multisigs)
        assert_equal(get_legacy_sigopcount_block(b31), MAX_BLOCK_SIGOPS)
        yield accepted()
        save_spendable_output()

        # this goes over the limit because the coinbase has one sigop
        too_many_multisigs = CScript([OP_CHECKMULTISIG] * (MAX_BLOCK_SIGOPS // 20))
        b32 = block(32, spend=out[9], script=too_many_multisigs)
        assert_equal(get_legacy_sigopcount_block(b32), MAX_BLOCK_SIGOPS + 1)
        yield rejected(RejectResult(16, b'bad-blk-sigops'))


        # CHECKMULTISIGVERIFY
        tip(31)
        lots_of_multisigs = CScript([OP_CHECKMULTISIGVERIFY] * ((MAX_BLOCK_SIGOPS-1) // 20) + [OP_CHECKSIG] * 19)
        block(33, spend=out[9], script=lots_of_multisigs)
        yield accepted()
        save_spendable_output()

        too_many_multisigs = CScript([OP_CHECKMULTISIGVERIFY] * (MAX_BLOCK_SIGOPS // 20))
        block(34, spend=out[10], script=too_many_multisigs)
        yield rejected(RejectResult(16, b'bad-blk-sigops'))


        # CHECKSIGVERIFY
        tip(33)
        lots_of_checksigs = CScript([OP_CHECKSIGVERIFY] * (MAX_BLOCK_SIGOPS - 1))
        b35 = block(35, spend=out[10], script=lots_of_checksigs)
        yield accepted()
        save_spendable_output()

        too_many_checksigs = CScript([OP_CHECKSIGVERIFY] * (MAX_BLOCK_SIGOPS))
        block(36, spend=out[11], script=too_many_checksigs)
        yield rejected(RejectResult(16, b'bad-blk-sigops'))


        # Check spending of a transaction in a block which failed to connect
        #
        # b6  (3)
        # b12 (3) -> b13 (4) -> b15 (5) -> b23 (6) -> b30 (7) -> b31 (8) -> b33 (9) -> b35 (10)
        #                                                                                     \-> b37 (11)
        #                                                                                     \-> b38 (11/37)
        #

        # save 37's spendable output, but then double-spend out11 to invalidate the block
        tip(35)
        b37 = block(37, spend=out[11])
        txout_b37 = PreviousSpendableOutput(b37.vtx[1], 0)
        tx = create_and_sign_tx(out[11].tx, out[11].n, 0)
        b37 = update_block(37, [tx])
        yield rejected(RejectResult(16, b'bad-txns-inputs-missingorspent'))

        # attempt to spend b37's first non-coinbase tx, at which point b37 was still considered valid
        tip(35)
        block(38, spend=txout_b37)
        yield rejected(RejectResult(16, b'bad-txns-inputs-missingorspent'))

        # Check P2SH SigOp counting
        #
        #
        #   13 (4) -> b15 (5) -> b23 (6) -> b30 (7) -> b31 (8) -> b33 (9) -> b35 (10) -> b39 (11) -> b41 (12)
        #                                                                                        \-> b40 (12)
        #
        # b39 - create some P2SH outputs that will require 6 sigops to spend:
        #
        #           redeem_script = COINBASE_PUBKEY, (OP_2DUP+OP_CHECKSIGVERIFY) * 5, OP_CHECKSIG
        #           p2sh_script = OP_HASH160, ripemd160(sha256(script)), OP_EQUAL
        #
        tip(35)
        b39 = block(39)
        b39_outputs = 0
        b39_sigops_per_output = 6

        # Build the redeem script, hash it, use hash to create the p2sh script
        redeem_script = CScript([self.coinbase_pubkey] + [OP_2DUP, OP_CHECKSIGVERIFY]*5 + [OP_CHECKSIG])
        redeem_script_hash = hash160(redeem_script)
        p2sh_script = CScript([OP_HASH160, redeem_script_hash, OP_EQUAL])

        # Create a transaction that spends one satoshi to the p2sh_script, the rest to OP_TRUE
        # This must be signed because it is spending a coinbase
        spend = out[11]
        tx = create_tx(spend.tx, spend.n, 1, p2sh_script)
        tx.vout.append(CTxOut(spend.tx.vout[spend.n].nValue - 1, CScript([OP_TRUE])))
        self.sign_tx(tx, spend.tx, spend.n)
        tx.rehash()
        b39 = update_block(39, [tx])
        b39_outputs += 1

        # Until block is full, add tx's with 1 satoshi to p2sh_script, the rest to OP_TRUE
        tx_new = None
        tx_last = tx
        total_size=len(b39.serialize())
        while(total_size < MAX_BLOCK_BASE_SIZE):
            tx_new = create_tx(tx_last, 1, 1, p2sh_script)
            tx_new.vout.append(CTxOut(tx_last.vout[1].nValue - 1, CScript([OP_TRUE])))
            tx_new.rehash()
            total_size += len(tx_new.serialize())
            if total_size >= MAX_BLOCK_BASE_SIZE:
                break
            b39.vtx.append(tx_new) # add tx to block
            tx_last = tx_new
            b39_outputs += 1

        b39 = update_block(39, [])
        yield accepted()
        save_spendable_output()


        # Test sigops in P2SH redeem scripts
        #
        # b40 creates 3333 tx's spending the 6-sigop P2SH outputs from b39 for a total of 19998 sigops.
        # The first tx has one sigop and then at the end we add 2 more to put us just over the max.
        #
        # b41 does the same, less one, so it has the maximum sigops permitted.
        #
        tip(39)
        b40 = block(40, spend=out[12])
        sigops = get_legacy_sigopcount_block(b40)
        numTxes = (MAX_BLOCK_SIGOPS - sigops) // b39_sigops_per_output
        assert_equal(numTxes <= b39_outputs, True)

        lastOutpoint = COutPoint(b40.vtx[1].sha256, 0)
        new_txs = []
        for i in range(1, numTxes+1):
            tx = CTransaction()
            tx.vout.append(CTxOut(1, CScript([OP_TRUE])))
            tx.vin.append(CTxIn(lastOutpoint, b''))
            # second input is corresponding P2SH output from b39
            tx.vin.append(CTxIn(COutPoint(b39.vtx[i].sha256, 0), b''))
            # Note: must pass the redeem_script (not p2sh_script) to the signature hash function
            (sighash, err) = SignatureHash(redeem_script, tx, 1, SIGHASH_ALL)
            sig = self.coinbase_key.sign(sighash) + bytes(bytearray([SIGHASH_ALL]))
            scriptSig = CScript([sig, redeem_script])

            tx.vin[1].scriptSig = scriptSig
            tx.rehash()
            new_txs.append(tx)
            lastOutpoint = COutPoint(tx.sha256, 0)

        b40_sigops_to_fill = MAX_BLOCK_SIGOPS - (numTxes * b39_sigops_per_output + sigops) + 1
        tx = CTransaction()
        tx.vin.append(CTxIn(lastOutpoint, b''))
        tx.vout.append(CTxOut(1, CScript([OP_CHECKSIG] * b40_sigops_to_fill)))
        tx.rehash()
        new_txs.append(tx)
        update_block(40, new_txs)
        yield rejected(RejectResult(16, b'bad-blk-sigops'))

        # same as b40, but one less sigop
        tip(39)
        block(41, spend=None)
        update_block(41, b40.vtx[1:-1])
        b41_sigops_to_fill = b40_sigops_to_fill - 1
        tx = CTransaction()
        tx.vin.append(CTxIn(lastOutpoint, b''))
        tx.vout.append(CTxOut(1, CScript([OP_CHECKSIG] * b41_sigops_to_fill)))
        tx.rehash()
        update_block(41, [tx])
        yield accepted()

        # Fork off of b39 to create a constant base again
        #
        # b23 (6) -> b30 (7) -> b31 (8) -> b33 (9) -> b35 (10) -> b39 (11) -> b42 (12) -> b43 (13)
        #                                                                  \-> b41 (12)
        #
        tip(39)
        block(42, spend=out[12])
        yield rejected()
        save_spendable_output()

        block(43, spend=out[13])
        yield accepted()
        save_spendable_output()


        # Test a number of really invalid scenarios
        #
        #  -> b31 (8) -> b33 (9) -> b35 (10) -> b39 (11) -> b42 (12) -> b43 (13) -> b44 (14)
        #                                                                                   \-> ??? (15)

        # The next few blocks are going to be created "by hand" since they'll do funky things, such as having
        # the first transaction be non-coinbase, etc.  The purpose of b44 is to make sure this works.
        height = self.block_heights[self.tip.sha256] + 1
        coinbase = create_coinbase(height, self.coinbase_pubkey)
        b44 = CBlock()
        b44.nTime = self.tip.nTime + 1
        b44.hashPrevBlock = self.tip.sha256
        b44.nBits = 0x207fffff
        b44.vtx.append(coinbase)
        b44.hashMerkleRoot = b44.calc_merkle_root()
        b44.solve()
        self.tip = b44
        self.block_heights[b44.sha256] = height
        self.blocks[44] = b44
        yield accepted()

        # A block with a non-coinbase as the first tx
        non_coinbase = create_tx(out[15].tx, out[15].n, 1)
        b45 = CBlock()
        b45.nTime = self.tip.nTime + 1
        b45.hashPrevBlock = self.tip.sha256
        b45.nBits = 0x207fffff
        b45.vtx.append(non_coinbase)
        b45.hashMerkleRoot = b45.calc_merkle_root()
        b45.calc_sha256()
        b45.solve()
        self.block_heights[b45.sha256] = self.block_heights[self.tip.sha256]+1
        self.tip = b45
        self.blocks[45] = b45
        yield rejected(RejectResult(16, b'bad-cb-missing'))

        # A block with no txns
        tip(44)
        b46 = CBlock()
        b46.nTime = b44.nTime+1
        b46.hashPrevBlock = b44.sha256
        b46.nBits = 0x207fffff
        b46.vtx = []
        b46.hashMerkleRoot = 0
        b46.solve()
        self.block_heights[b46.sha256] = self.block_heights[b44.sha256]+1
        self.tip = b46
        assert 46 not in self.blocks
        self.blocks[46] = b46
        s = ser_uint256(b46.hashMerkleRoot)
        yield rejected(RejectResult(16, b'bad-blk-length'))

        # A block with invalid work
        tip(44)
        b47 = block(47, solve=False)
        target = uint256_from_compact(b47.nBits)
        while b47.sha256 < target: #changed > to <
            b47.nNonce += 1
            b47.rehash()
        yield rejected(RejectResult(16, b'high-hash'))

        # A block with timestamp > 2 hrs in the future
        tip(44)
        b48 = block(48, solve=False)
        b48.nTime = int(time.time()) + 60 * 60 * 3
        b48.solve()
        yield rejected(RejectResult(16, b'time-too-new'))

        # A block with an invalid merkle hash
        tip(44)
        b49 = block(49)
        b49.hashMerkleRoot += 1
        b49.solve()
        yield rejected(RejectResult(16, b'bad-txnmrklroot'))

        # A block with an incorrect POW limit
        tip(44)
        b50 = block(50)
        b50.nBits = b50.nBits - 1
        b50.solve()
        yield rejected(RejectResult(16, b'bad-diffbits'))

        # A block with two coinbase txns
        tip(44)
        b51 = block(51)
        cb2 = create_coinbase(51, self.coinbase_pubkey)
        b51 = update_block(51, [cb2])
        yield rejected(RejectResult(16, b'bad-cb-multiple'))

        # A block w/ duplicate txns
        # Note: txns have to be in the right position in the merkle tree to trigger this error
        tip(44)
        b52 = block(52, spend=out[15])
        tx = create_tx(b52.vtx[1], 0, 1)
        b52 = update_block(52, [tx, tx])
        yield rejected(RejectResult(16, b'bad-txns-duplicate'))

        # Test block timestamps
        #  -> b31 (8) -> b33 (9) -> b35 (10) -> b39 (11) -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15)
        #                                                                                   \-> b54 (15)
        #
        tip(43)
        block(53, spend=out[14])
        yield rejected() # rejected since b44 is at same height
        save_spendable_output()

        # invalid timestamp (b35 is 5 blocks back, so its time is MedianTimePast)
        b54 = block(54, spend=out[15])
        b54.nTime = b35.nTime - 1
        b54.solve()
        yield rejected(RejectResult(16, b'time-too-old'))

        # valid timestamp
        tip(53)
        b55 = block(55, spend=out[15])
        b55.nTime = b35.nTime
        update_block(55, [])
        yield accepted()
        save_spendable_output()


        # Test CVE-2012-2459
        #
        # -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15) -> b57p2 (16)
        #                                                \-> b57   (16)
        #                                                \-> b56p2 (16)
        #                                                \-> b56   (16)
        #
        # Merkle tree malleability (CVE-2012-2459): repeating sequences of transactions in a block without 
        #                           affecting the merkle root of a block, while still invalidating it.
        #                           See:  src/consensus/merkle.h
        #
        #  b57 has three txns:  coinbase, tx, tx1.  The merkle root computation will duplicate tx.
        #  Result:  OK
        #
        #  b56 copies b57 but duplicates tx1 and does not recalculate the block hash.  So it has a valid merkle
        #  root but duplicate transactions.
        #  Result:  Fails
        #
        #  b57p2 has six transactions in its merkle tree:
        #       - coinbase, tx, tx1, tx2, tx3, tx4
        #  Merkle root calculation will duplicate as necessary.
        #  Result:  OK.
        #
        #  b56p2 copies b57p2 but adds both tx3 and tx4.  The purpose of the test is to make sure the code catches
        #  duplicate txns that are not next to one another with the "bad-txns-duplicate" error (which indicates
        #  that the error was caught early, avoiding a DOS vulnerability.)

        # b57 - a good block with 2 txs, don't submit until end
        tip(55)
        b57 = block(57)
        tx = create_and_sign_tx(out[16].tx, out[16].n, 1)
        tx1 = create_tx(tx, 0, 1)
        b57 = update_block(57, [tx, tx1])

        # b56 - copy b57, add a duplicate tx
        tip(55)
        b56 = copy.deepcopy(b57)
        self.blocks[56] = b56
        assert_equal(len(b56.vtx),3)
        b56 = update_block(56, [tx1])
        assert_equal(b56.hash, b57.hash)
        yield rejected(RejectResult(16, b'bad-txns-duplicate'))

        # b57p2 - a good block with 6 tx'es, don't submit until end
        tip(55)
        b57p2 = block("57p2")
        tx = create_and_sign_tx(out[16].tx, out[16].n, 1)
        tx1 = create_tx(tx, 0, 1)
        tx2 = create_tx(tx1, 0, 1)
        tx3 = create_tx(tx2, 0, 1)
        tx4 = create_tx(tx3, 0, 1)
        b57p2 = update_block("57p2", [tx, tx1, tx2, tx3, tx4])

        # b56p2 - copy b57p2, duplicate two non-consecutive tx's
        tip(55)
        b56p2 = copy.deepcopy(b57p2)
        self.blocks["b56p2"] = b56p2
        assert_equal(b56p2.hash, b57p2.hash)
        assert_equal(len(b56p2.vtx),6)
        b56p2 = update_block("b56p2", [tx3, tx4])
        yield rejected(RejectResult(16, b'bad-txns-duplicate'))

        tip("57p2")
        yield accepted()

        tip(57)
        yield rejected()  #rejected because 57p2 seen first
        save_spendable_output()

        # Test a few invalid tx types
        #
        # -> b35 (10) -> b39 (11) -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17)
        #                                                                                    \-> ??? (17)
        #

        # tx with prevout.n out of range
        tip(57)
        b58 = block(58, spend=out[17])
        tx = CTransaction()
        assert(len(out[17].tx.vout) < 42)
        tx.vin.append(CTxIn(COutPoint(out[17].tx.sha256, 42), CScript([OP_TRUE]), 0xffffffff))
        tx.vout.append(CTxOut(0, b""))
        tx.calc_sha256()
        b58 = update_block(58, [tx])
        yield rejected(RejectResult(16, b'bad-txns-inputs-missingorspent'))

        # tx with output value > input value out of range
        tip(57)
        b59 = block(59)
        tx = create_and_sign_tx(out[17].tx, out[17].n, 51*COIN)
        b59 = update_block(59, [tx])
        yield rejected(RejectResult(16, b'bad-txns-in-belowout'))

        # reset to good chain
        tip(57)
        b60 = block(60, spend=out[17])
        yield accepted()
        save_spendable_output()

        # Test BIP30
        #
        # -> b39 (11) -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17)
        #                                                                                    \-> b61 (18)
        #
        # Blocks are not allowed to contain a transaction whose id matches that of an earlier,
        # not-fully-spent transaction in the same chain. To test, make identical coinbases;
        # the second one should be rejected.
        #
        tip(60)
        b61 = block(61, spend=out[18])
        b61.vtx[0].vin[0].scriptSig = b60.vtx[0].vin[0].scriptSig  #equalize the coinbases
        b61.vtx[0].rehash()
        b61 = update_block(61, [])
        assert_equal(b60.vtx[0].serialize(), b61.vtx[0].serialize())
        yield rejected(RejectResult(16, b'bad-txns-BIP30'))


        # Test tx.isFinal is properly rejected (not an exhaustive tx.isFinal test, that should be in data-driven transaction tests)
        #
        #   -> b39 (11) -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17)
        #                                                                                     \-> b62 (18)
        #
        tip(60)
        b62 = block(62)
        tx = CTransaction()
        tx.nLockTime = 0xffffffff  #this locktime is non-final
        assert(out[18].n < len(out[18].tx.vout))
        tx.vin.append(CTxIn(COutPoint(out[18].tx.sha256, out[18].n))) # don't set nSequence
        tx.vout.append(CTxOut(0, CScript([OP_TRUE])))
        assert(tx.vin[0].nSequence < 0xffffffff)
        tx.calc_sha256()
        b62 = update_block(62, [tx])
        yield rejected(RejectResult(16, b'bad-txns-nonfinal'))


        # Test a non-final coinbase is also rejected
        #
        #   -> b39 (11) -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17)
        #                                                                                     \-> b63 (-)
        #
        tip(60)
        b63 = block(63)
        b63.vtx[0].nLockTime = 0xffffffff
        b63.vtx[0].vin[0].nSequence = 0xDEADBEEF
        b63.vtx[0].rehash()
        b63 = update_block(63, [])
        yield rejected(RejectResult(16, b'bad-txns-nonfinal'))


        #  This checks that a block with a bloated VARINT between the block_header and the array of tx such that
        #  the block is > MAX_BLOCK_BASE_SIZE with the bloated varint, but <= MAX_BLOCK_BASE_SIZE without the bloated varint,
        #  does not cause a subsequent, identical block with canonical encoding to be rejected.  The test does not
        #  care whether the bloated block is accepted or rejected; it only cares that the second block is accepted.
        #
        #  What matters is that the receiving node should not reject the bloated block, and then reject the canonical
        #  block on the basis that it's the same as an already-rejected block (which would be a consensus failure.)
        #
        #  -> b39 (11) -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17) -> b64 (18)
        #                                                                                        \
        #                                                                                         b64a (18)
        #  b64a is a bloated block (non-canonical varint)
        #  b64 is a good block (same as b64 but w/ canonical varint)
        #
        tip(60)
        regular_block = block("64a", spend=out[18])

        # make it a "broken_block," with non-canonical serialization
        b64a = CBrokenBlock(regular_block)
        b64a.initialize(regular_block)
        self.blocks["64a"] = b64a
        self.tip = b64a
        tx = CTransaction()

        # use canonical serialization to calculate size
        script_length = MAX_BLOCK_BASE_SIZE - len(b64a.normal_serialize()) - 69
        script_output = CScript([b'\x00' * script_length])
        tx.vout.append(CTxOut(0, script_output))
        tx.vin.append(CTxIn(COutPoint(b64a.vtx[1].sha256, 0)))
        b64a = update_block("64a", [tx])
        assert_equal(len(b64a.serialize()), MAX_BLOCK_BASE_SIZE + 8)
        yield TestInstance([[self.tip, None]])

        # comptool workaround: to make sure b64 is delivered, manually erase b64a from blockstore
        self.test.block_store.erase(b64a.sha256)

        tip(60)
        b64 = CBlock(b64a)
        b64.vtx = copy.deepcopy(b64a.vtx)
        assert_equal(b64.hash, b64a.hash)
        assert_equal(len(b64.serialize()), MAX_BLOCK_BASE_SIZE)
        self.blocks[64] = b64
        update_block(64, [])
        yield accepted()
        save_spendable_output()

        # Spend an output created in the block itself
        #
        # -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17) -> b64 (18) -> b65 (19)
        #
        tip(64)
        block(65)
        tx1 = create_and_sign_tx(out[19].tx, out[19].n, out[19].tx.vout[0].nValue)
        tx2 = create_and_sign_tx(tx1, 0, 0)
        update_block(65, [tx1, tx2])
        yield accepted()
        save_spendable_output()

        # Attempt to spend an output created later in the same block
        #
        # -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17) -> b64 (18) -> b65 (19)
        #                                                                                    \-> b66 (20)
        tip(65)
        block(66)
        tx1 = create_and_sign_tx(out[20].tx, out[20].n, out[20].tx.vout[0].nValue)
        tx2 = create_and_sign_tx(tx1, 0, 1)
        update_block(66, [tx2, tx1])
        yield rejected(RejectResult(16, b'bad-txns-inputs-missingorspent'))

        # Attempt to double-spend a transaction created in a block
        #
        # -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17) -> b64 (18) -> b65 (19)
        #                                                                                    \-> b67 (20)
        #
        #
        tip(65)
        block(67)
        tx1 = create_and_sign_tx(out[20].tx, out[20].n, out[20].tx.vout[0].nValue)
        tx2 = create_and_sign_tx(tx1, 0, 1)
        tx3 = create_and_sign_tx(tx1, 0, 2)
        update_block(67, [tx1, tx2, tx3])
        yield rejected(RejectResult(16, b'bad-txns-inputs-missingorspent'))

        # More tests of block subsidy
        #
        # -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17) -> b64 (18) -> b65 (19) -> b69 (20)
        #                                                                                    \-> b68 (20)
        #
        # b68 - coinbase with an extra 10 satoshis,
        #       creates a tx that has 9 satoshis from out[20] go to fees
        #       this fails because the coinbase is trying to claim 1 satoshi too much in fees
        #
        # b69 - coinbase with extra 10 satoshis, and a tx that gives a 10 satoshi fee
        #       this succeeds
        #
        tip(65)
        block(68, additional_coinbase_value=10)
        tx = create_and_sign_tx(out[20].tx, out[20].n, out[20].tx.vout[0].nValue-9)
        update_block(68, [tx])
        yield rejected(RejectResult(16, b'bad-cb-amount'))

        tip(65)
        b69 = block(69, additional_coinbase_value=10)
        tx = create_and_sign_tx(out[20].tx, out[20].n, out[20].tx.vout[0].nValue-10)
        update_block(69, [tx])
        yield accepted()
        save_spendable_output()

        # Test spending the outpoint of a non-existent transaction
        #
        # -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17) -> b64 (18) -> b65 (19) -> b69 (20)
        #                                                                                    \-> b70 (21)
        #
        tip(69)
        block(70, spend=out[21])
        bogus_tx = CTransaction()
        bogus_tx.sha256 = uint256_from_str(b"23c70ed7c0506e9178fc1a987f40a33946d4ad4c962b5ae3a52546da53af0c5c")
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(bogus_tx.sha256, 0), b"", 0xffffffff))
        tx.vout.append(CTxOut(1, b""))
        update_block(70, [tx])
        yield rejected(RejectResult(16, b'bad-txns-inputs-missingorspent'))


        # Test accepting an invalid block which has the same hash as a valid one (via merkle tree tricks)
        #
        #  -> b53 (14) -> b55 (15) -> b57 (16) -> b60 (17) -> b64 (18) -> b65 (19) -> b69 (20) -> b72 (21)
        #                                                                                      \-> b71 (21)
        #
        # b72 is a good block.
        # b71 is a copy of 72, but re-adds one of its transactions.  However, it has the same hash as b71.
        #
        tip(69)
        b72 = block(72)
        tx1 = create_and_sign_tx(out[21].tx, out[21].n, 2)
        tx2 = create_and_sign_tx(tx1, 0, 1)
        b72 = update_block(72, [tx1, tx2])  # now tip is 72
        b71 = copy.deepcopy(b72)
        b71.vtx.append(tx2)   # add duplicate tx2
        self.block_heights[b71.sha256] = self.block_heights[b69.sha256] + 1  # b71 builds off b69
        self.blocks[71] = b71

        assert_equal(len(b71.vtx), 4)
        assert_equal(len(b72.vtx), 3)
        assert_equal(b72.sha256, b71.sha256)

        tip(71)
        yield rejected(RejectResult(16, b'bad-txns-duplicate'))
        tip(72)
        yield accepted()
        save_spendable_output()


        # Test some invalid scripts and MAX_BLOCK_SIGOPS
        #
        # -> b55 (15) -> b57 (16) -> b60 (17) -> b64 (18) -> b65 (19) -> b69 (20) -> b72 (21)
        #                                                                                    \-> b** (22)
        #

        # b73 - tx with excessive sigops that are placed after an excessively large script element.
        #       The purpose of the test is to make sure those sigops are counted.
        #
        #       script is a bytearray of size 20,526
        #
        #       bytearray[0-19,998]     : OP_CHECKSIG
        #       bytearray[19,999]       : OP_PUSHDATA4
        #       bytearray[20,000-20,003]: 521  (max_script_element_size+1, in little-endian format)
        #       bytearray[20,004-20,525]: unread data (script_element)
        #       bytearray[20,526]       : OP_CHECKSIG (this puts us over the limit)
        #
        tip(72)
        b73 = block(73)
        size = MAX_BLOCK_SIGOPS - 1 + MAX_SCRIPT_ELEMENT_SIZE + 1 + 5 + 1
        a = bytearray([OP_CHECKSIG] * size)
        a[MAX_BLOCK_SIGOPS - 1] = int("4e",16) # OP_PUSHDATA4

        element_size = MAX_SCRIPT_ELEMENT_SIZE + 1
        a[MAX_BLOCK_SIGOPS] = element_size % 256
        a[MAX_BLOCK_SIGOPS+1] = element_size // 256
        a[MAX_BLOCK_SIGOPS+2] = 0
        a[MAX_BLOCK_SIGOPS+3] = 0

        tx = create_and_sign_tx(out[22].tx, 0, 1, CScript(a))
        b73 = update_block(73, [tx])
        assert_equal(get_legacy_sigopcount_block(b73), MAX_BLOCK_SIGOPS+1)
        yield rejected(RejectResult(16, b'bad-blk-sigops'))

        # b74/75 - if we push an invalid script element, all prevous sigops are counted,
        #          but sigops after the element are not counted.
        #
        #       The invalid script element is that the push_data indicates that
        #       there will be a large amount of data (0xffffff bytes), but we only
        #       provide a much smaller number.  These bytes are CHECKSIGS so they would
        #       cause b75 to fail for excessive sigops, if those bytes were counted.
        #
        #       b74 fails because we put MAX_BLOCK_SIGOPS+1 before the element
        #       b75 succeeds because we put MAX_BLOCK_SIGOPS before the element
        #
        #
        tip(72)
        b74 = block(74)
        size = MAX_BLOCK_SIGOPS - 1 + MAX_SCRIPT_ELEMENT_SIZE + 42 # total = 20,561
        a = bytearray([OP_CHECKSIG] * size)
        a[MAX_BLOCK_SIGOPS] = 0x4e
        a[MAX_BLOCK_SIGOPS+1] = 0xfe
        a[MAX_BLOCK_SIGOPS+2] = 0xff
        a[MAX_BLOCK_SIGOPS+3] = 0xff
        a[MAX_BLOCK_SIGOPS+4] = 0xff
        tx = create_and_sign_tx(out[22].tx, 0, 1, CScript(a))
        b74 = update_block(74, [tx])
        yield rejected(RejectResult(16, b'bad-blk-sigops'))

        tip(72)
        b75 = block(75)
        size = MAX_BLOCK_SIGOPS - 1 + MAX_SCRIPT_ELEMENT_SIZE + 42
        a = bytearray([OP_CHECKSIG] * size)
        a[MAX_BLOCK_SIGOPS-1] = 0x4e
        a[MAX_BLOCK_SIGOPS] = 0xff
        a[MAX_BLOCK_SIGOPS+1] = 0xff
        a[MAX_BLOCK_SIGOPS+2] = 0xff
        a[MAX_BLOCK_SIGOPS+3] = 0xff
        tx = create_and_sign_tx(out[22].tx, 0, 1, CScript(a))
        b75 = update_block(75, [tx])
        yield accepted()
        save_spendable_output()

        # Check that if we push an element filled with CHECKSIGs, they are not counted
        tip(75)
        b76 = block(76)
        size = MAX_BLOCK_SIGOPS - 1 + MAX_SCRIPT_ELEMENT_SIZE + 1 + 5
        a = bytearray([OP_CHECKSIG] * size)
        a[MAX_BLOCK_SIGOPS-1] = 0x4e # PUSHDATA4, but leave the following bytes as just checksigs
        tx = create_and_sign_tx(out[23].tx, 0, 1, CScript(a))
        b76 = update_block(76, [tx])
        yield accepted()
        save_spendable_output()

        # Test transaction resurrection
        #
        # -> b77 (24) -> b78 (25) -> b79 (26)
        #            \-> b80 (25) -> b81 (26) -> b82 (27)
        #
        #    b78 creates a tx, which is spent in b79. After b82, both should be in mempool
        #
        #    The tx'es must be unsigned and pass the node's mempool policy.  It is unsigned for the
        #    rather obscure reason that the Python signature code does not distinguish between
        #    Low-S and High-S values (whereas the bitcoin code has custom code which does so);
        #    as a result of which, the odds are 50% that the python code will use the right
        #    value and the transaction will be accepted into the mempool. Until we modify the
        #    test framework to support low-S signing, we are out of luck.
        #
        #    To get around this issue, we construct transactions which are not signed and which
        #    spend to OP_TRUE.  If the standard-ness rules change, this test would need to be
        #    updated.  (Perhaps to spend to a P2SH OP_TRUE script)
        #
        tip(76)
        block(77)
        tx77 = create_and_sign_tx(out[24].tx, out[24].n, 10*COIN)
        update_block(77, [tx77])
        yield accepted()
        save_spendable_output()

        block(78)
        tx78 = create_tx(tx77, 0, 9*COIN)
        update_block(78, [tx78])
        yield accepted()

        block(79)
        tx79 = create_tx(tx78, 0, 8*COIN)
        update_block(79, [tx79])
        yield accepted()

        # mempool should be empty
        assert_equal(len(self.nodes[0].getrawmempool()), 0)

        tip(77)
        block(80, spend=out[25])
        yield rejected()
        save_spendable_output()

        block(81, spend=out[26])
        yield rejected() # other chain is same length
        save_spendable_output()

        block(82, spend=out[27])
        yield accepted()  # now this chain is longer, triggers re-org
        save_spendable_output()

        # now check that tx78 and tx79 have been put back into the peer's mempool
        mempool = self.nodes[0].getrawmempool()
        assert_equal(len(mempool), 2)
        assert(tx78.hash in mempool)
        assert(tx79.hash in mempool)


        # Test invalid opcodes in dead execution paths.
        #
        #  -> b81 (26) -> b82 (27) -> b83 (28)
        #
        block(83)
        op_codes = [OP_IF, OP_INVALIDOPCODE, OP_ELSE, OP_TRUE, OP_ENDIF]
        script = CScript(op_codes)
        tx1 = create_and_sign_tx(out[28].tx, out[28].n, out[28].tx.vout[0].nValue, script)

        tx2 = create_and_sign_tx(tx1, 0, 0, CScript([OP_TRUE]))
        tx2.vin[0].scriptSig = CScript([OP_FALSE])
        tx2.rehash()

        update_block(83, [tx1, tx2])
        yield accepted()
        save_spendable_output()


        # Reorg on/off blocks that have OP_RETURN in them (and try to spend them)
        #
        #  -> b81 (26) -> b82 (27) -> b83 (28) -> b84 (29) -> b87 (30) -> b88 (31)
        #                                    \-> b85 (29) -> b86 (30)            \-> b89a (32)
        #
        #
        block(84)
        tx1 = create_tx(out[29].tx, out[29].n, 0, CScript([OP_RETURN]))
        tx1.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx1.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx1.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx1.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx1.calc_sha256()
        self.sign_tx(tx1, out[29].tx, out[29].n)
        tx1.rehash()
        tx2 = create_tx(tx1, 1, 0, CScript([OP_RETURN]))
        tx2.vout.append(CTxOut(0, CScript([OP_RETURN])))
        tx3 = create_tx(tx1, 2, 0, CScript([OP_RETURN]))
        tx3.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx4 = create_tx(tx1, 3, 0, CScript([OP_TRUE]))
        tx4.vout.append(CTxOut(0, CScript([OP_RETURN])))
        tx5 = create_tx(tx1, 4, 0, CScript([OP_RETURN]))

        update_block(84, [tx1,tx2,tx3,tx4,tx5])
        yield accepted()
        save_spendable_output()

        tip(83)
        block(85, spend=out[29])
        yield rejected()

        block(86, spend=out[30])
        yield accepted()

        tip(84)
        block(87, spend=out[30])
        yield rejected()
        save_spendable_output()

        block(88, spend=out[31])
        yield accepted()
        save_spendable_output()

        # trying to spend the OP_RETURN output is rejected
        block("89a", spend=out[32])
        tx = create_tx(tx1, 0, 0, CScript([OP_TRUE]))
        update_block("89a", [tx])
        yield rejected()


        #  Test re-org of a week's worth of blocks (1088 blocks)
        #  This test takes a minute or two and can be accomplished in memory
        #
        if self.options.runbarelyexpensive:
            tip(88)
            LARGE_REORG_SIZE = 1088
            test1 = TestInstance(sync_every_block=False)
            spend=out[32]
            for i in range(89, LARGE_REORG_SIZE + 89):
                b = block(i, spend)
                tx = CTransaction()
                script_length = MAX_BLOCK_BASE_SIZE - len(b.serialize()) - 69
                script_output = CScript([b'\x00' * script_length])
                tx.vout.append(CTxOut(0, script_output))
                tx.vin.append(CTxIn(COutPoint(b.vtx[1].sha256, 0)))
                b = update_block(i, [tx])
                assert_equal(len(b.serialize()), MAX_BLOCK_BASE_SIZE)
                test1.blocks_and_transactions.append([self.tip, True])
                save_spendable_output()
                spend = get_spendable_output()

            yield test1
            chain1_tip = i

            # now create alt chain of same length
            tip(88)
            test2 = TestInstance(sync_every_block=False)
            for i in range(89, LARGE_REORG_SIZE + 89):
                block("alt"+str(i))
                test2.blocks_and_transactions.append([self.tip, False])
            yield test2

            # extend alt chain to trigger re-org
            block("alt" + str(chain1_tip + 1))
            yield accepted()

            # ... and re-org back to the first chain
            tip(chain1_tip)
            block(chain1_tip + 1)
            yield rejected()
            block(chain1_tip + 2)
            yield accepted()

            chain1_tip += 2



if __name__ == '__main__':
    FullBlockTest().main()
