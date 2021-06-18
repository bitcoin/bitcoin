#!/usr/bin/env python3
# Copyright (c) 2015-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test block processing."""
import copy
import struct
import time

from test_framework.blocktools import (
    create_block,
    create_coinbase,
    create_tx_with_script,
    get_legacy_sigopcount_block,
    MAX_BLOCK_SIGOPS,
)
from test_framework.key import ECKey
from test_framework.messages import (
    CBlock,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    MAX_BLOCK_BASE_SIZE,
    uint256_from_compact,
    uint256_from_str,
)
from test_framework.p2p import P2PDataStore
from test_framework.script import (
    CScript,
    MAX_SCRIPT_ELEMENT_SIZE,
    OP_2DUP,
    OP_CHECKMULTISIG,
    OP_CHECKMULTISIGVERIFY,
    OP_CHECKSIG,
    OP_CHECKSIGVERIFY,
    OP_ELSE,
    OP_ENDIF,
    OP_EQUAL,
    OP_DROP,
    OP_FALSE,
    OP_HASH160,
    OP_IF,
    OP_INVALIDOPCODE,
    OP_RETURN,
    OP_TRUE,
    SIGHASH_ALL,
    LegacySignatureHash,
    hash160,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from data import invalid_txs

#  Use this class for tests that require behavior other than normal p2p behavior.
#  For now, it is used to serialize a bloated varint (b64).
class CBrokenBlock(CBlock):
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
        return super().serialize()


DUPLICATE_COINBASE_SCRIPT_SIG = b'\x01\x78'  # Valid for block at height 120


class FullBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [['-acceptnonstdtxn=1']]  # This is a consensus block test, we don't care about tx policy

    def run_test(self):
        node = self.nodes[0]  # convenience reference to the node

        self.bootstrap_p2p()  # Add one p2p connection to the node

        self.block_heights = {}
        self.coinbase_key = ECKey()
        self.coinbase_key.generate()
        self.coinbase_pubkey = self.coinbase_key.get_pubkey().get_bytes()
        self.tip = None
        self.blocks = {}
        self.genesis_hash = int(self.nodes[0].getbestblockhash(), 16)
        self.block_heights[self.genesis_hash] = 0
        self.spendable_outputs = []

        # Create a new block
        b_dup_cb = self.next_block('dup_cb')
        b_dup_cb.vtx[0].vin[0].scriptSig = DUPLICATE_COINBASE_SCRIPT_SIG
        b_dup_cb.vtx[0].rehash()
        duplicate_tx = b_dup_cb.vtx[0]
        b_dup_cb = self.update_block('dup_cb', [])
        self.send_blocks([b_dup_cb])

        b0 = self.next_block(0)
        self.save_spendable_output()
        self.send_blocks([b0])

        # These constants chosen specifically to trigger an immature coinbase spend
        # at a certain time below.
        NUM_BUFFER_BLOCKS_TO_GENERATE = 99
        NUM_OUTPUTS_TO_COLLECT = 33

        # Allow the block to mature
        blocks = []
        for i in range(NUM_BUFFER_BLOCKS_TO_GENERATE):
            blocks.append(self.next_block(f"maturitybuffer.{i}"))
            self.save_spendable_output()
        self.send_blocks(blocks)

        # collect spendable outputs now to avoid cluttering the code later on
        out = []
        for _ in range(NUM_OUTPUTS_TO_COLLECT):
            out.append(self.get_spendable_output())

        # Start by building a couple of blocks on top (which output is spent is
        # in parentheses):
        #     genesis -> b1 (0) -> b2 (1)
        b1 = self.next_block(1, spend=out[0])
        self.save_spendable_output()

        b2 = self.next_block(2, spend=out[1])
        self.save_spendable_output()

        self.send_blocks([b1, b2], timeout=4)

        # Select a txn with an output eligible for spending. This won't actually be spent,
        # since we're testing submission of a series of blocks with invalid txns.
        attempt_spend_tx = out[2]

        # Submit blocks for rejection, each of which contains a single transaction
        # (aside from coinbase) which should be considered invalid.
        for TxTemplate in invalid_txs.iter_all_templates():
            template = TxTemplate(spend_tx=attempt_spend_tx)

            if template.valid_in_block:
                continue

            self.log.info(f"Reject block with invalid tx: {TxTemplate.__name__}")
            blockname = f"for_invalid.{TxTemplate.__name__}"
            badblock = self.next_block(blockname)
            badtx = template.get_tx()
            if TxTemplate != invalid_txs.InputMissing:
                self.sign_tx(badtx, attempt_spend_tx)
            badtx.rehash()
            badblock = self.update_block(blockname, [badtx])
            self.send_blocks(
                [badblock], success=False,
                reject_reason=(template.block_reject_reason or template.reject_reason),
                reconnect=True, timeout=2)

            self.move_tip(2)

        # Fork like this:
        #
        #     genesis -> b1 (0) -> b2 (1)
        #                      \-> b3 (1)
        #
        # Nothing should happen at this point. We saw b2 first so it takes priority.
        self.log.info("Don't reorg to a chain of the same length")
        self.move_tip(1)
        b3 = self.next_block(3, spend=out[1])
        txout_b3 = b3.vtx[1]
        self.send_blocks([b3], False)

        # Now we add another block to make the alternative chain longer.
        #
        #     genesis -> b1 (0) -> b2 (1)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Reorg to a longer chain")
        b4 = self.next_block(4, spend=out[2])
        self.send_blocks([b4])

        # ... and back to the first chain.
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6 (3)
        #                      \-> b3 (1) -> b4 (2)
        self.move_tip(2)
        b5 = self.next_block(5, spend=out[2])
        self.save_spendable_output()
        self.send_blocks([b5], False)

        self.log.info("Reorg back to the original chain")
        b6 = self.next_block(6, spend=out[3])
        self.send_blocks([b6], True)

        # Try to create a fork that double-spends
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6 (3)
        #                                          \-> b7 (2) -> b8 (4)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Reject a chain with a double spend, even if it is longer")
        self.move_tip(5)
        b7 = self.next_block(7, spend=out[2])
        self.send_blocks([b7], False)

        b8 = self.next_block(8, spend=out[4])
        self.send_blocks([b8], False, reconnect=True)

        # Try to create a block that has too much fee
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6 (3)
        #                                                    \-> b9 (4)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Reject a block where the miner creates too much coinbase reward")
        self.move_tip(6)
        b9 = self.next_block(9, spend=out[4], additional_coinbase_value=1)
        self.send_blocks([b9], success=False, reject_reason='bad-cb-amount', reconnect=True)

        # Create a fork that ends in a block with too much fee (the one that causes the reorg)
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b10 (3) -> b11 (4)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Reject a chain where the miner creates too much coinbase reward, even if the chain is longer")
        self.move_tip(5)
        b10 = self.next_block(10, spend=out[3])
        self.send_blocks([b10], False)

        b11 = self.next_block(11, spend=out[4], additional_coinbase_value=1)
        self.send_blocks([b11], success=False, reject_reason='bad-cb-amount', reconnect=True)

        # Try again, but with a valid fork first
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b14 (5)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Reject a chain where the miner creates too much coinbase reward, even if the chain is longer (on a forked chain)")
        self.move_tip(5)
        b12 = self.next_block(12, spend=out[3])
        self.save_spendable_output()
        b13 = self.next_block(13, spend=out[4])
        self.save_spendable_output()
        b14 = self.next_block(14, spend=out[5], additional_coinbase_value=1)
        self.send_blocks([b12, b13, b14], success=False, reject_reason='bad-cb-amount', reconnect=True)

        # New tip should be b13.
        assert_equal(node.getbestblockhash(), b13.hash)

        # Add a block with MAX_BLOCK_SIGOPS and one with one more sigop
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b16 (6)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Accept a block with lots of checksigs")
        lots_of_checksigs = CScript([OP_CHECKSIG] * (MAX_BLOCK_SIGOPS - 1))
        self.move_tip(13)
        b15 = self.next_block(15, spend=out[5], script=lots_of_checksigs)
        self.save_spendable_output()
        self.send_blocks([b15], True)

        self.log.info("Reject a block with too many checksigs")
        too_many_checksigs = CScript([OP_CHECKSIG] * (MAX_BLOCK_SIGOPS))
        b16 = self.next_block(16, spend=out[6], script=too_many_checksigs)
        self.send_blocks([b16], success=False, reject_reason='bad-blk-sigops', reconnect=True)

        # Attempt to spend a transaction created on a different fork
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b17 (b3.vtx[1])
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Reject a block with a spend from a re-org'ed out tx")
        self.move_tip(15)
        b17 = self.next_block(17, spend=txout_b3)
        self.send_blocks([b17], success=False, reject_reason='bad-txns-inputs-missingorspent', reconnect=True)

        # Attempt to spend a transaction created on a different fork (on a fork this time)
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5)
        #                                                                \-> b18 (b3.vtx[1]) -> b19 (6)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Reject a block with a spend from a re-org'ed out tx (on a forked chain)")
        self.move_tip(13)
        b18 = self.next_block(18, spend=txout_b3)
        self.send_blocks([b18], False)

        b19 = self.next_block(19, spend=out[6])
        self.send_blocks([b19], success=False, reject_reason='bad-txns-inputs-missingorspent', reconnect=True)

        # Attempt to spend a coinbase at depth too low
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b20 (7)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Reject a block spending an immature coinbase.")
        self.move_tip(15)
        b20 = self.next_block(20, spend=out[7])
        self.send_blocks([b20], success=False, reject_reason='bad-txns-premature-spend-of-coinbase', reconnect=True)

        # Attempt to spend a coinbase at depth too low (on a fork this time)
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5)
        #                                                                \-> b21 (6) -> b22 (5)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Reject a block spending an immature coinbase (on a forked chain)")
        self.move_tip(13)
        b21 = self.next_block(21, spend=out[6])
        self.send_blocks([b21], False)

        b22 = self.next_block(22, spend=out[5])
        self.send_blocks([b22], success=False, reject_reason='bad-txns-premature-spend-of-coinbase', reconnect=True)

        # Create a block on either side of MAX_BLOCK_BASE_SIZE and make sure its accepted/rejected
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b23 (6)
        #                                                                           \-> b24 (6) -> b25 (7)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Accept a block of size MAX_BLOCK_BASE_SIZE")
        self.move_tip(15)
        b23 = self.next_block(23, spend=out[6])
        tx = CTransaction()
        script_length = MAX_BLOCK_BASE_SIZE - len(b23.serialize()) - 69
        script_output = CScript([b'\x00' * script_length])
        tx.vout.append(CTxOut(0, script_output))
        tx.vin.append(CTxIn(COutPoint(b23.vtx[1].sha256, 0)))
        b23 = self.update_block(23, [tx])
        # Make sure the math above worked out to produce a max-sized block
        assert_equal(len(b23.serialize()), MAX_BLOCK_BASE_SIZE)
        self.send_blocks([b23], True)
        self.save_spendable_output()

        self.log.info("Reject a block of size MAX_BLOCK_BASE_SIZE + 1")
        self.move_tip(15)
        b24 = self.next_block(24, spend=out[6])
        script_length = MAX_BLOCK_BASE_SIZE - len(b24.serialize()) - 69
        script_output = CScript([b'\x00' * (script_length + 1)])
        tx.vout = [CTxOut(0, script_output)]
        b24 = self.update_block(24, [tx])
        assert_equal(len(b24.serialize()), MAX_BLOCK_BASE_SIZE + 1)
        self.send_blocks([b24], success=False, reject_reason='bad-blk-length', reconnect=True)

        b25 = self.next_block(25, spend=out[7])
        self.send_blocks([b25], False)

        # Create blocks with a coinbase input script size out of range
        #     genesis -> b1 (0) -> b2 (1) -> b5 (2) -> b6  (3)
        #                                          \-> b12 (3) -> b13 (4) -> b15 (5) -> b23 (6) -> b30 (7)
        #                                                                           \-> ... (6) -> ... (7)
        #                      \-> b3 (1) -> b4 (2)
        self.log.info("Reject a block with coinbase input script size out of range")
        self.move_tip(15)
        b26 = self.next_block(26, spend=out[6])
        b26.vtx[0].vin[0].scriptSig = b'\x00'
        b26.vtx[0].rehash()
        # update_block causes the merkle root to get updated, even with no new
        # transactions, and updates the required state.
        b26 = self.update_block(26, [])
        self.send_blocks([b26], success=False, reject_reason='bad-cb-length', reconnect=True)

        # Extend the b26 chain to make sure bitcoind isn't accepting b26
        b27 = self.next_block(27, spend=out[7])
        self.send_blocks([b27], False)

        # Now try a too-large-coinbase script
        self.move_tip(15)
        b28 = self.next_block(28, spend=out[6])
        b28.vtx[0].vin[0].scriptSig = b'\x00' * 101
        b28.vtx[0].rehash()
        b28 = self.update_block(28, [])
        self.send_blocks([b28], success=False, reject_reason='bad-cb-length', reconnect=True)

        # Extend the b28 chain to make sure bitcoind isn't accepting b28
        b29 = self.next_block(29, spend=out[7])
        self.send_blocks([b29], False)

        # b30 has a max-sized coinbase scriptSig.
        self.move_tip(23)
        b30 = self.next_block(30)
        b30.vtx[0].vin[0].scriptSig = b'\x00' * 100
        b30.vtx[0].rehash()
        b30 = self.update_block(30, [])
        self.send_blocks([b30], True)
        self.save_spendable_output()

        # b31 - b35 - check sigops of OP_CHECKMULTISIG / OP_CHECKMULTISIGVERIFY / OP_CHECKSIGVERIFY
        #
        #     genesis -> ... -> b30 (7) -> b31 (8) -> b33 (9) -> b35 (10)
        #                                                                \-> b36 (11)
        #                                                    \-> b34 (10)
        #                                         \-> b32 (9)
        #

        # MULTISIG: each op code counts as 20 sigops.  To create the edge case, pack another 19 sigops at the end.
        self.log.info("Accept a block with the max number of OP_CHECKMULTISIG sigops")
        lots_of_multisigs = CScript([OP_CHECKMULTISIG] * ((MAX_BLOCK_SIGOPS - 1) // 20) + [OP_CHECKSIG] * 19)
        b31 = self.next_block(31, spend=out[8], script=lots_of_multisigs)
        assert_equal(get_legacy_sigopcount_block(b31), MAX_BLOCK_SIGOPS)
        self.send_blocks([b31], True)
        self.save_spendable_output()

        # this goes over the limit because the coinbase has one sigop
        self.log.info("Reject a block with too many OP_CHECKMULTISIG sigops")
        too_many_multisigs = CScript([OP_CHECKMULTISIG] * (MAX_BLOCK_SIGOPS // 20))
        b32 = self.next_block(32, spend=out[9], script=too_many_multisigs)
        assert_equal(get_legacy_sigopcount_block(b32), MAX_BLOCK_SIGOPS + 1)
        self.send_blocks([b32], success=False, reject_reason='bad-blk-sigops', reconnect=True)

        # CHECKMULTISIGVERIFY
        self.log.info("Accept a block with the max number of OP_CHECKMULTISIGVERIFY sigops")
        self.move_tip(31)
        lots_of_multisigs = CScript([OP_CHECKMULTISIGVERIFY] * ((MAX_BLOCK_SIGOPS - 1) // 20) + [OP_CHECKSIG] * 19)
        b33 = self.next_block(33, spend=out[9], script=lots_of_multisigs)
        self.send_blocks([b33], True)
        self.save_spendable_output()

        self.log.info("Reject a block with too many OP_CHECKMULTISIGVERIFY sigops")
        too_many_multisigs = CScript([OP_CHECKMULTISIGVERIFY] * (MAX_BLOCK_SIGOPS // 20))
        b34 = self.next_block(34, spend=out[10], script=too_many_multisigs)
        self.send_blocks([b34], success=False, reject_reason='bad-blk-sigops', reconnect=True)

        # CHECKSIGVERIFY
        self.log.info("Accept a block with the max number of OP_CHECKSIGVERIFY sigops")
        self.move_tip(33)
        lots_of_checksigs = CScript([OP_CHECKSIGVERIFY] * (MAX_BLOCK_SIGOPS - 1))
        b35 = self.next_block(35, spend=out[10], script=lots_of_checksigs)
        self.send_blocks([b35], True)
        self.save_spendable_output()

        self.log.info("Reject a block with too many OP_CHECKSIGVERIFY sigops")
        too_many_checksigs = CScript([OP_CHECKSIGVERIFY] * (MAX_BLOCK_SIGOPS))
        b36 = self.next_block(36, spend=out[11], script=too_many_checksigs)
        self.send_blocks([b36], success=False, reject_reason='bad-blk-sigops', reconnect=True)

        # Check spending of a transaction in a block which failed to connect
        #
        # b6  (3)
        # b12 (3) -> b13 (4) -> b15 (5) -> b23 (6) -> b30 (7) -> b31 (8) -> b33 (9) -> b35 (10)
        #                                                                                     \-> b37 (11)
        #                                                                                     \-> b38 (11/37)
        #

        # save 37's spendable output, but then double-spend out11 to invalidate the block
        self.log.info("Reject a block spending transaction from a block which failed to connect")
        self.move_tip(35)
        b37 = self.next_block(37, spend=out[11])
        txout_b37 = b37.vtx[1]
        tx = self.create_and_sign_transaction(out[11], 0)
        b37 = self.update_block(37, [tx])
        self.send_blocks([b37], success=False, reject_reason='bad-txns-inputs-missingorspent', reconnect=True)

        # attempt to spend b37's first non-coinbase tx, at which point b37 was still considered valid
        self.move_tip(35)
        b38 = self.next_block(38, spend=txout_b37)
        self.send_blocks([b38], success=False, reject_reason='bad-txns-inputs-missingorspent', reconnect=True)

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
        self.log.info("Check P2SH SIGOPS are correctly counted")
        self.move_tip(35)
        b39 = self.next_block(39)
        b39_outputs = 0
        b39_sigops_per_output = 6

        # Build the redeem script, hash it, use hash to create the p2sh script
        redeem_script = CScript([self.coinbase_pubkey] + [OP_2DUP, OP_CHECKSIGVERIFY] * 5 + [OP_CHECKSIG])
        redeem_script_hash = hash160(redeem_script)
        p2sh_script = CScript([OP_HASH160, redeem_script_hash, OP_EQUAL])

        # Create a transaction that spends one satoshi to the p2sh_script, the rest to OP_TRUE
        # This must be signed because it is spending a coinbase
        spend = out[11]
        tx = self.create_tx(spend, 0, 1, p2sh_script)
        tx.vout.append(CTxOut(spend.vout[0].nValue - 1, CScript([OP_TRUE])))
        self.sign_tx(tx, spend)
        tx.rehash()
        b39 = self.update_block(39, [tx])
        b39_outputs += 1

        # Until block is full, add tx's with 1 satoshi to p2sh_script, the rest to OP_TRUE
        tx_new = None
        tx_last = tx
        total_size = len(b39.serialize())
        while(total_size < MAX_BLOCK_BASE_SIZE):
            tx_new = self.create_tx(tx_last, 1, 1, p2sh_script)
            tx_new.vout.append(CTxOut(tx_last.vout[1].nValue - 1, CScript([OP_TRUE])))
            tx_new.rehash()
            total_size += len(tx_new.serialize())
            if total_size >= MAX_BLOCK_BASE_SIZE:
                break
            b39.vtx.append(tx_new)  # add tx to block
            tx_last = tx_new
            b39_outputs += 1

        # The accounting in the loop above can be off, because it misses the
        # compact size encoding of the number of transactions in the block.
        # Make sure we didn't accidentally make too big a block. Note that the
        # size of the block has non-determinism due to the ECDSA signature in
        # the first transaction.
        while (len(b39.serialize()) >= MAX_BLOCK_BASE_SIZE):
            del b39.vtx[-1]

        b39 = self.update_block(39, [])
        self.send_blocks([b39], True)
        self.save_spendable_output()

        # Test sigops in P2SH redeem scripts
        #
        # b40 creates 3333 tx's spending the 6-sigop P2SH outputs from b39 for a total of 19998 sigops.
        # The first tx has one sigop and then at the end we add 2 more to put us just over the max.
        #
        # b41 does the same, less one, so it has the maximum sigops permitted.
        #
        self.log.info("Reject a block with too many P2SH sigops")
        self.move_tip(39)
        b40 = self.next_block(40, spend=out[12])
        sigops = get_legacy_sigopcount_block(b40)
        numTxes = (MAX_BLOCK_SIGOPS - sigops) // b39_sigops_per_output
        assert_equal(numTxes <= b39_outputs, True)

        lastOutpoint = COutPoint(b40.vtx[1].sha256, 0)
        new_txs = []
        for i in range(1, numTxes + 1):
            tx = CTransaction()
            tx.vout.append(CTxOut(1, CScript([OP_TRUE])))
            tx.vin.append(CTxIn(lastOutpoint, b''))
            # second input is corresponding P2SH output from b39
            tx.vin.append(CTxIn(COutPoint(b39.vtx[i].sha256, 0), b''))
            # Note: must pass the redeem_script (not p2sh_script) to the signature hash function
            (sighash, err) = LegacySignatureHash(redeem_script, tx, 1, SIGHASH_ALL)
            sig = self.coinbase_key.sign_ecdsa(sighash) + bytes(bytearray([SIGHASH_ALL]))
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
        self.update_block(40, new_txs)
        self.send_blocks([b40], success=False, reject_reason='bad-blk-sigops', reconnect=True)

        # same as b40, but one less sigop
        self.log.info("Accept a block with the max number of P2SH sigops")
        self.move_tip(39)
        b41 = self.next_block(41, spend=None)
        self.update_block(41, b40.vtx[1:-1])
        b41_sigops_to_fill = b40_sigops_to_fill - 1
        tx = CTransaction()
        tx.vin.append(CTxIn(lastOutpoint, b''))
        tx.vout.append(CTxOut(1, CScript([OP_CHECKSIG] * b41_sigops_to_fill)))
        tx.rehash()
        self.update_block(41, [tx])
        self.send_blocks([b41], True)

        # Fork off of b39 to create a constant base again
        #
        # b23 (6) -> b30 (7) -> b31 (8) -> b33 (9) -> b35 (10) -> b39 (11) -> b42 (12) -> b43 (13)
        #                                                                  \-> b41 (12)
        #
        self.move_tip(39)
        b42 = self.next_block(42, spend=out[12])
        self.save_spendable_output()

        b43 = self.next_block(43, spend=out[13])
        self.save_spendable_output()
        self.send_blocks([b42, b43], True)

        # Test a number of really invalid scenarios
        #
        #  -> b31 (8) -> b33 (9) -> b35 (10) -> b39 (11) -> b42 (12) -> b43 (13) -> b44 (14)
        #                                                                                   \-> ??? (15)

        # The next few blocks are going to be created "by hand" since they'll do funky things, such as having
        # the first transaction be non-coinbase, etc.  The purpose of b44 is to make sure this works.
        self.log.info("Build block 44 manually")
        height = self.block_heights[self.tip.sha256] + 1
        coinbase = create_coinbase(height, self.coinbase_pubkey)
        b44 = CBlock()
        b44.nTime = self.tip.nTime + 1
        b44.hashPrevBlock = self.tip.sha256
        b44.nBits = 0x207fffff
        b44.vtx.append(coinbase)
        tx = self.create_and_sign_transaction(out[14], 1)
        b44.vtx.append(tx)
        b44.hashMerkleRoot = b44.calc_merkle_root()
        b44.solve()
        self.tip = b44
        self.block_heights[b44.sha256] = height
        self.blocks[44] = b44
        self.send_blocks([b44], True)

        self.log.info("Reject a block with a non-coinbase as the first tx")
        non_coinbase = self.create_tx(out[15], 0, 1)
        b45 = CBlock()
        b45.nTime = self.tip.nTime + 1
        b45.hashPrevBlock = self.tip.sha256
        b45.nBits = 0x207fffff
        b45.vtx.append(non_coinbase)
        b45.hashMerkleRoot = b45.calc_merkle_root()
        b45.calc_sha256()
        b45.solve()
        self.block_heights[b45.sha256] = self.block_heights[self.tip.sha256] + 1
        self.tip = b45
        self.blocks[45] = b45
        self.send_blocks([b45], success=False, reject_reason='bad-cb-missing', reconnect=True)

        self.log.info("Reject a block with no transactions")
        self.move_tip(44)
        b46 = CBlock()
        b46.nTime = b44.nTime + 1
        b46.hashPrevBlock = b44.sha256
        b46.nBits = 0x207fffff
        b46.vtx = []
        b46.hashMerkleRoot = 0
        b46.solve()
        self.block_heights[b46.sha256] = self.block_heights[b44.sha256] + 1
        self.tip = b46
        assert 46 not in self.blocks
        self.blocks[46] = b46
        self.send_blocks([b46], success=False, reject_reason='bad-blk-length', reconnect=True)

        self.log.info("Reject a block with invalid work")
        self.move_tip(44)
        b47 = self.next_block(47)
        target = uint256_from_compact(b47.nBits)
        while b47.sha256 <= target:
            # Rehash nonces until an invalid too-high-hash block is found.
            b47.nNonce += 1
            b47.rehash()
        self.send_blocks([b47], False, force_send=True, reject_reason='high-hash', reconnect=True)

        self.log.info("Reject a block with a timestamp >2 hours in the future")
        self.move_tip(44)
        b48 = self.next_block(48)
        b48.nTime = int(time.time()) + 60 * 60 * 3
        # Header timestamp has changed. Re-solve the block.
        b48.solve()
        self.send_blocks([b48], False, force_send=True, reject_reason='time-too-new')

        self.log.info("Reject a block with invalid merkle hash")
        self.move_tip(44)
        b49 = self.next_block(49)
        b49.hashMerkleRoot += 1
        b49.solve()
        self.send_blocks([b49], success=False, reject_reason='bad-txnmrklroot', reconnect=True)

        self.log.info("Reject a block with incorrect POW limit")
        self.move_tip(44)
        b50 = self.next_block(50)
        b50.nBits = b50.nBits - 1
        b50.solve()
        self.send_blocks([b50], False, force_send=True, reject_reason='bad-diffbits', reconnect=True)

        self.log.info("Reject a block with two coinbase transactions")
        self.move_tip(44)
        b51 = self.next_block(51)
        cb2 = create_coinbase(51, self.coinbase_pubkey)
        b51 = self.update_block(51, [cb2])
        self.send_blocks([b51], success=False, reject_reason='bad-cb-multiple', reconnect=True)

        self.log.info("Reject a block with duplicate transactions")
        # Note: txns have to be in the right position in the merkle tree to trigger this error
        self.move_tip(44)
        b52 = self.next_block(52, spend=out[15])
        tx = self.create_tx(b52.vtx[1], 0, 1)
        b52 = self.update_block(52, [tx, tx])
        self.send_blocks([b52], success=False, reject_reason='bad-txns-duplicate', reconnect=True)

        # Test block timestamps
        #  -> b31 (8) -> b33 (9) -> b35 (10) -> b39 (11) -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15)
        #                                                                                   \-> b54 (15)
        #                                                                        -> b44 (14)\-> b48 ()
        self.move_tip(43)
        b53 = self.next_block(53, spend=out[14])
        self.send_blocks([b53], False)
        self.save_spendable_output()

        self.log.info("Reject a block with timestamp before MedianTimePast")
        b54 = self.next_block(54, spend=out[15])
        b54.nTime = b35.nTime - 1
        b54.solve()
        self.send_blocks([b54], False, force_send=True, reject_reason='time-too-old', reconnect=True)

        # valid timestamp
        self.move_tip(53)
        b55 = self.next_block(55, spend=out[15])
        b55.nTime = b35.nTime
        self.update_block(55, [])
        self.send_blocks([b55], True)
        self.save_spendable_output()

        # The block which was previously rejected because of being "too far(3 hours)" must be accepted 2 hours later.
        # The new block is only 1 hour into future now and we must reorg onto to the new longer chain.
        # The new bestblock b48p is invalidated manually.
        #  -> b31 (8) -> b33 (9) -> b35 (10) -> b39 (11) -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15)
        #                                                                                   \-> b54 (15)
        #                                                                        -> b44 (14)\-> b48 () -> b48p ()
        self.log.info("Accept a previously rejected future block at a later time")
        node.setmocktime(int(time.time()) + 2*60*60)
        self.move_tip(48)
        self.block_heights[b48.sha256] = self.block_heights[b44.sha256] + 1 # b48 is a parent of b44
        b48p = self.next_block("48p")
        self.send_blocks([b48, b48p], success=True) # Reorg to the longer chain
        node.invalidateblock(b48p.hash) # mark b48p as invalid
        node.setmocktime(0)

        # Test Merkle tree malleability
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
        self.move_tip(55)
        b57 = self.next_block(57)
        tx = self.create_and_sign_transaction(out[16], 1)
        tx1 = self.create_tx(tx, 0, 1)
        b57 = self.update_block(57, [tx, tx1])

        # b56 - copy b57, add a duplicate tx
        self.log.info("Reject a block with a duplicate transaction in the Merkle Tree (but with a valid Merkle Root)")
        self.move_tip(55)
        b56 = copy.deepcopy(b57)
        self.blocks[56] = b56
        assert_equal(len(b56.vtx), 3)
        b56 = self.update_block(56, [tx1])
        assert_equal(b56.hash, b57.hash)
        self.send_blocks([b56], success=False, reject_reason='bad-txns-duplicate', reconnect=True)

        # b57p2 - a good block with 6 tx'es, don't submit until end
        self.move_tip(55)
        b57p2 = self.next_block("57p2")
        tx = self.create_and_sign_transaction(out[16], 1)
        tx1 = self.create_tx(tx, 0, 1)
        tx2 = self.create_tx(tx1, 0, 1)
        tx3 = self.create_tx(tx2, 0, 1)
        tx4 = self.create_tx(tx3, 0, 1)
        b57p2 = self.update_block("57p2", [tx, tx1, tx2, tx3, tx4])

        # b56p2 - copy b57p2, duplicate two non-consecutive tx's
        self.log.info("Reject a block with two duplicate transactions in the Merkle Tree (but with a valid Merkle Root)")
        self.move_tip(55)
        b56p2 = copy.deepcopy(b57p2)
        self.blocks["b56p2"] = b56p2
        assert_equal(b56p2.hash, b57p2.hash)
        assert_equal(len(b56p2.vtx), 6)
        b56p2 = self.update_block("b56p2", [tx3, tx4])
        self.send_blocks([b56p2], success=False, reject_reason='bad-txns-duplicate', reconnect=True)

        self.move_tip("57p2")
        self.send_blocks([b57p2], True)

        self.move_tip(57)
        self.send_blocks([b57], False)  # The tip is not updated because 57p2 seen first
        self.save_spendable_output()

        # Test a few invalid tx types
        #
        # -> b35 (10) -> b39 (11) -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 ()
        #                                                                                    \-> ??? (17)
        #

        # tx with prevout.n out of range
        self.log.info("Reject a block with a transaction with prevout.n out of range")
        self.move_tip(57)
        b58 = self.next_block(58, spend=out[17])
        tx = CTransaction()
        assert len(out[17].vout) < 42
        tx.vin.append(CTxIn(COutPoint(out[17].sha256, 42), CScript([OP_TRUE]), 0xffffffff))
        tx.vout.append(CTxOut(0, b""))
        tx.calc_sha256()
        b58 = self.update_block(58, [tx])
        self.send_blocks([b58], success=False, reject_reason='bad-txns-inputs-missingorspent', reconnect=True)

        # tx with output value > input value
        self.log.info("Reject a block with a transaction with outputs > inputs")
        self.move_tip(57)
        b59 = self.next_block(59)
        tx = self.create_and_sign_transaction(out[17], 51 * COIN)
        b59 = self.update_block(59, [tx])
        self.send_blocks([b59], success=False, reject_reason='bad-txns-in-belowout', reconnect=True)

        # reset to good chain
        self.move_tip(57)
        b60 = self.next_block(60)
        self.send_blocks([b60], True)
        self.save_spendable_output()

        # Test BIP30 (reject duplicate)
        #
        # -> b39 (11) -> b42 (12) -> b43 (13) -> b53 (14) -> b55 (15) -> b57 (16) -> b60 ()
        #                                                                                  \-> b61 ()
        #
        # Blocks are not allowed to contain a transaction whose id matches that of an earlier,
        # not-fully-spent transaction in the same chain. To test, make identical coinbases;
        # the second one should be rejected. See also CVE-2012-1909.
        #
        self.log.info("Reject a block with a transaction with a duplicate hash of a previous transaction (BIP30)")
        self.move_tip(60)
        b61 = self.next_block(61)
        b61.vtx[0].vin[0].scriptSig = DUPLICATE_COINBASE_SCRIPT_SIG
        b61.vtx[0].rehash()
        b61 = self.update_block(61, [])
        assert_equal(duplicate_tx.serialize(), b61.vtx[0].serialize())
        self.send_blocks([b61], success=False, reject_reason='bad-txns-BIP30', reconnect=True)

        # Test BIP30 (allow duplicate if spent)
        #
        # -> b57 (16) -> b60 ()
        #            \-> b_spend_dup_cb (b_dup_cb) -> b_dup_2 ()
        #
        self.move_tip(57)
        b_spend_dup_cb = self.next_block('spend_dup_cb')
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(duplicate_tx.sha256, 0)))
        tx.vout.append(CTxOut(0, CScript([OP_TRUE])))
        self.sign_tx(tx, duplicate_tx)
        tx.rehash()
        b_spend_dup_cb = self.update_block('spend_dup_cb', [tx])

        b_dup_2 = self.next_block('dup_2')
        b_dup_2.vtx[0].vin[0].scriptSig = DUPLICATE_COINBASE_SCRIPT_SIG
        b_dup_2.vtx[0].rehash()
        b_dup_2 = self.update_block('dup_2', [])
        assert_equal(duplicate_tx.serialize(), b_dup_2.vtx[0].serialize())
        assert_equal(self.nodes[0].gettxout(txid=duplicate_tx.hash, n=0)['confirmations'], 119)
        self.send_blocks([b_spend_dup_cb, b_dup_2], success=True)
        # The duplicate has less confirmations
        assert_equal(self.nodes[0].gettxout(txid=duplicate_tx.hash, n=0)['confirmations'], 1)

        # Test tx.isFinal is properly rejected (not an exhaustive tx.isFinal test, that should be in data-driven transaction tests)
        #
        # -> b_spend_dup_cb (b_dup_cb) -> b_dup_2 ()
        #                                           \-> b62 (18)
        #
        self.log.info("Reject a block with a transaction with a nonfinal locktime")
        self.move_tip('dup_2')
        b62 = self.next_block(62)
        tx = CTransaction()
        tx.nLockTime = 0xffffffff  # this locktime is non-final
        tx.vin.append(CTxIn(COutPoint(out[18].sha256, 0)))  # don't set nSequence
        tx.vout.append(CTxOut(0, CScript([OP_TRUE])))
        assert tx.vin[0].nSequence < 0xffffffff
        tx.calc_sha256()
        b62 = self.update_block(62, [tx])
        self.send_blocks([b62], success=False, reject_reason='bad-txns-nonfinal', reconnect=True)

        # Test a non-final coinbase is also rejected
        #
        # -> b_spend_dup_cb (b_dup_cb) -> b_dup_2 ()
        #                                           \-> b63 (-)
        #
        self.log.info("Reject a block with a coinbase transaction with a nonfinal locktime")
        self.move_tip('dup_2')
        b63 = self.next_block(63)
        b63.vtx[0].nLockTime = 0xffffffff
        b63.vtx[0].vin[0].nSequence = 0xDEADBEEF
        b63.vtx[0].rehash()
        b63 = self.update_block(63, [])
        self.send_blocks([b63], success=False, reject_reason='bad-txns-nonfinal', reconnect=True)

        #  This checks that a block with a bloated VARINT between the block_header and the array of tx such that
        #  the block is > MAX_BLOCK_BASE_SIZE with the bloated varint, but <= MAX_BLOCK_BASE_SIZE without the bloated varint,
        #  does not cause a subsequent, identical block with canonical encoding to be rejected.  The test does not
        #  care whether the bloated block is accepted or rejected; it only cares that the second block is accepted.
        #
        #  What matters is that the receiving node should not reject the bloated block, and then reject the canonical
        #  block on the basis that it's the same as an already-rejected block (which would be a consensus failure.)
        #
        #  -> b_spend_dup_cb (b_dup_cb) -> b_dup_2 () -> b64 (18)
        #                                              \
        #                                               b64a (18)
        #  b64a is a bloated block (non-canonical varint)
        #  b64 is a good block (same as b64 but w/ canonical varint)
        #
        self.log.info("Accept a valid block even if a bloated version of the block has previously been sent")
        self.move_tip('dup_2')
        regular_block = self.next_block("64a", spend=out[18])

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
        b64a = self.update_block("64a", [tx])
        assert_equal(len(b64a.serialize()), MAX_BLOCK_BASE_SIZE + 8)
        self.send_blocks([b64a], success=False, reject_reason='non-canonical ReadCompactSize()')

        # bitcoind doesn't disconnect us for sending a bloated block, but if we subsequently
        # resend the header message, it won't send us the getdata message again. Just
        # disconnect and reconnect and then call sync_blocks.
        # TODO: improve this test to be less dependent on P2P DOS behaviour.
        node.disconnect_p2ps()
        self.reconnect_p2p()

        self.move_tip('dup_2')
        b64 = CBlock(b64a)
        b64.vtx = copy.deepcopy(b64a.vtx)
        assert_equal(b64.hash, b64a.hash)
        assert_equal(len(b64.serialize()), MAX_BLOCK_BASE_SIZE)
        self.blocks[64] = b64
        b64 = self.update_block(64, [])
        self.send_blocks([b64], True)
        self.save_spendable_output()

        # Spend an output created in the block itself
        #
        # -> b_dup_2 () -> b64 (18) -> b65 (19)
        #
        self.log.info("Accept a block with a transaction spending an output created in the same block")
        self.move_tip(64)
        b65 = self.next_block(65)
        tx1 = self.create_and_sign_transaction(out[19], out[19].vout[0].nValue)
        tx2 = self.create_and_sign_transaction(tx1, 0)
        b65 = self.update_block(65, [tx1, tx2])
        self.send_blocks([b65], True)
        self.save_spendable_output()

        # Attempt to spend an output created later in the same block
        #
        # -> b64 (18) -> b65 (19)
        #                        \-> b66 (20)
        self.log.info("Reject a block with a transaction spending an output created later in the same block")
        self.move_tip(65)
        b66 = self.next_block(66)
        tx1 = self.create_and_sign_transaction(out[20], out[20].vout[0].nValue)
        tx2 = self.create_and_sign_transaction(tx1, 1)
        b66 = self.update_block(66, [tx2, tx1])
        self.send_blocks([b66], success=False, reject_reason='bad-txns-inputs-missingorspent', reconnect=True)

        # Attempt to double-spend a transaction created in a block
        #
        # -> b64 (18) -> b65 (19)
        #                        \-> b67 (20)
        #
        #
        self.log.info("Reject a block with a transaction double spending a transaction created in the same block")
        self.move_tip(65)
        b67 = self.next_block(67)
        tx1 = self.create_and_sign_transaction(out[20], out[20].vout[0].nValue)
        tx2 = self.create_and_sign_transaction(tx1, 1)
        tx3 = self.create_and_sign_transaction(tx1, 2)
        b67 = self.update_block(67, [tx1, tx2, tx3])
        self.send_blocks([b67], success=False, reject_reason='bad-txns-inputs-missingorspent', reconnect=True)

        # More tests of block subsidy
        #
        # -> b64 (18) -> b65 (19) -> b69 (20)
        #                        \-> b68 (20)
        #
        # b68 - coinbase with an extra 10 satoshis,
        #       creates a tx that has 9 satoshis from out[20] go to fees
        #       this fails because the coinbase is trying to claim 1 satoshi too much in fees
        #
        # b69 - coinbase with extra 10 satoshis, and a tx that gives a 10 satoshi fee
        #       this succeeds
        #
        self.log.info("Reject a block trying to claim too much subsidy in the coinbase transaction")
        self.move_tip(65)
        b68 = self.next_block(68, additional_coinbase_value=10)
        tx = self.create_and_sign_transaction(out[20], out[20].vout[0].nValue - 9)
        b68 = self.update_block(68, [tx])
        self.send_blocks([b68], success=False, reject_reason='bad-cb-amount', reconnect=True)

        self.log.info("Accept a block claiming the correct subsidy in the coinbase transaction")
        self.move_tip(65)
        b69 = self.next_block(69, additional_coinbase_value=10)
        tx = self.create_and_sign_transaction(out[20], out[20].vout[0].nValue - 10)
        self.update_block(69, [tx])
        self.send_blocks([b69], True)
        self.save_spendable_output()

        # Test spending the outpoint of a non-existent transaction
        #
        # -> b65 (19) -> b69 (20)
        #                        \-> b70 (21)
        #
        self.log.info("Reject a block containing a transaction spending from a non-existent input")
        self.move_tip(69)
        b70 = self.next_block(70, spend=out[21])
        bogus_tx = CTransaction()
        bogus_tx.sha256 = uint256_from_str(b"23c70ed7c0506e9178fc1a987f40a33946d4ad4c962b5ae3a52546da53af0c5c")
        tx = CTransaction()
        tx.vin.append(CTxIn(COutPoint(bogus_tx.sha256, 0), b"", 0xffffffff))
        tx.vout.append(CTxOut(1, b""))
        b70 = self.update_block(70, [tx])
        self.send_blocks([b70], success=False, reject_reason='bad-txns-inputs-missingorspent', reconnect=True)

        # Test accepting an invalid block which has the same hash as a valid one (via merkle tree tricks)
        #
        #  -> b65 (19) -> b69 (20) -> b72 (21)
        #                          \-> b71 (21)
        #
        # b72 is a good block.
        # b71 is a copy of 72, but re-adds one of its transactions.  However, it has the same hash as b72.
        self.log.info("Reject a block containing a duplicate transaction but with the same Merkle root (Merkle tree malleability")
        self.move_tip(69)
        b72 = self.next_block(72)
        tx1 = self.create_and_sign_transaction(out[21], 2)
        tx2 = self.create_and_sign_transaction(tx1, 1)
        b72 = self.update_block(72, [tx1, tx2])  # now tip is 72
        b71 = copy.deepcopy(b72)
        b71.vtx.append(tx2)   # add duplicate tx2
        self.block_heights[b71.sha256] = self.block_heights[b69.sha256] + 1  # b71 builds off b69
        self.blocks[71] = b71

        assert_equal(len(b71.vtx), 4)
        assert_equal(len(b72.vtx), 3)
        assert_equal(b72.sha256, b71.sha256)

        self.move_tip(71)
        self.send_blocks([b71], success=False, reject_reason='bad-txns-duplicate', reconnect=True)

        self.move_tip(72)
        self.send_blocks([b72], True)
        self.save_spendable_output()

        # Test some invalid scripts and MAX_BLOCK_SIGOPS
        #
        # -> b69 (20) -> b72 (21)
        #                        \-> b** (22)
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
        self.log.info("Reject a block containing too many sigops after a large script element")
        self.move_tip(72)
        b73 = self.next_block(73)
        size = MAX_BLOCK_SIGOPS - 1 + MAX_SCRIPT_ELEMENT_SIZE + 1 + 5 + 1
        a = bytearray([OP_CHECKSIG] * size)
        a[MAX_BLOCK_SIGOPS - 1] = int("4e", 16)  # OP_PUSHDATA4

        element_size = MAX_SCRIPT_ELEMENT_SIZE + 1
        a[MAX_BLOCK_SIGOPS] = element_size % 256
        a[MAX_BLOCK_SIGOPS + 1] = element_size // 256
        a[MAX_BLOCK_SIGOPS + 2] = 0
        a[MAX_BLOCK_SIGOPS + 3] = 0

        tx = self.create_and_sign_transaction(out[22], 1, CScript(a))
        b73 = self.update_block(73, [tx])
        assert_equal(get_legacy_sigopcount_block(b73), MAX_BLOCK_SIGOPS + 1)
        self.send_blocks([b73], success=False, reject_reason='bad-blk-sigops', reconnect=True)

        # b74/75 - if we push an invalid script element, all previous sigops are counted,
        #          but sigops after the element are not counted.
        #
        #       The invalid script element is that the push_data indicates that
        #       there will be a large amount of data (0xffffff bytes), but we only
        #       provide a much smaller number.  These bytes are CHECKSIGS so they would
        #       cause b75 to fail for excessive sigops, if those bytes were counted.
        #
        #       b74 fails because we put MAX_BLOCK_SIGOPS+1 before the element
        #       b75 succeeds because we put MAX_BLOCK_SIGOPS before the element
        self.log.info("Check sigops are counted correctly after an invalid script element")
        self.move_tip(72)
        b74 = self.next_block(74)
        size = MAX_BLOCK_SIGOPS - 1 + MAX_SCRIPT_ELEMENT_SIZE + 42  # total = 20,561
        a = bytearray([OP_CHECKSIG] * size)
        a[MAX_BLOCK_SIGOPS] = 0x4e
        a[MAX_BLOCK_SIGOPS + 1] = 0xfe
        a[MAX_BLOCK_SIGOPS + 2] = 0xff
        a[MAX_BLOCK_SIGOPS + 3] = 0xff
        a[MAX_BLOCK_SIGOPS + 4] = 0xff
        tx = self.create_and_sign_transaction(out[22], 1, CScript(a))
        b74 = self.update_block(74, [tx])
        self.send_blocks([b74], success=False, reject_reason='bad-blk-sigops', reconnect=True)

        self.move_tip(72)
        b75 = self.next_block(75)
        size = MAX_BLOCK_SIGOPS - 1 + MAX_SCRIPT_ELEMENT_SIZE + 42
        a = bytearray([OP_CHECKSIG] * size)
        a[MAX_BLOCK_SIGOPS - 1] = 0x4e
        a[MAX_BLOCK_SIGOPS] = 0xff
        a[MAX_BLOCK_SIGOPS + 1] = 0xff
        a[MAX_BLOCK_SIGOPS + 2] = 0xff
        a[MAX_BLOCK_SIGOPS + 3] = 0xff
        tx = self.create_and_sign_transaction(out[22], 1, CScript(a))
        b75 = self.update_block(75, [tx])
        self.send_blocks([b75], True)
        self.save_spendable_output()

        # Check that if we push an element filled with CHECKSIGs, they are not counted
        self.move_tip(75)
        b76 = self.next_block(76)
        size = MAX_BLOCK_SIGOPS - 1 + MAX_SCRIPT_ELEMENT_SIZE + 1 + 5
        a = bytearray([OP_CHECKSIG] * size)
        a[MAX_BLOCK_SIGOPS - 1] = 0x4e  # PUSHDATA4, but leave the following bytes as just checksigs
        tx = self.create_and_sign_transaction(out[23], 1, CScript(a))
        b76 = self.update_block(76, [tx])
        self.send_blocks([b76], True)
        self.save_spendable_output()

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
        self.log.info("Test transaction resurrection during a re-org")
        self.move_tip(76)
        b77 = self.next_block(77)
        tx77 = self.create_and_sign_transaction(out[24], 10 * COIN)
        b77 = self.update_block(77, [tx77])
        self.send_blocks([b77], True)
        self.save_spendable_output()

        b78 = self.next_block(78)
        tx78 = self.create_tx(tx77, 0, 9 * COIN)
        b78 = self.update_block(78, [tx78])
        self.send_blocks([b78], True)

        b79 = self.next_block(79)
        tx79 = self.create_tx(tx78, 0, 8 * COIN)
        b79 = self.update_block(79, [tx79])
        self.send_blocks([b79], True)

        # mempool should be empty
        assert_equal(len(self.nodes[0].getrawmempool()), 0)

        self.move_tip(77)
        b80 = self.next_block(80, spend=out[25])
        self.send_blocks([b80], False, force_send=True)
        self.save_spendable_output()

        b81 = self.next_block(81, spend=out[26])
        self.send_blocks([b81], False, force_send=True)  # other chain is same length
        self.save_spendable_output()

        b82 = self.next_block(82, spend=out[27])
        self.send_blocks([b82], True)  # now this chain is longer, triggers re-org
        self.save_spendable_output()

        # now check that tx78 and tx79 have been put back into the peer's mempool
        mempool = self.nodes[0].getrawmempool()
        assert_equal(len(mempool), 2)
        assert tx78.hash in mempool
        assert tx79.hash in mempool

        # Test invalid opcodes in dead execution paths.
        #
        #  -> b81 (26) -> b82 (27) -> b83 (28)
        #
        self.log.info("Accept a block with invalid opcodes in dead execution paths")
        b83 = self.next_block(83)
        op_codes = [OP_IF, OP_INVALIDOPCODE, OP_ELSE, OP_TRUE, OP_ENDIF]
        script = CScript(op_codes)
        tx1 = self.create_and_sign_transaction(out[28], out[28].vout[0].nValue, script)

        tx2 = self.create_and_sign_transaction(tx1, 0, CScript([OP_TRUE]))
        tx2.vin[0].scriptSig = CScript([OP_FALSE])
        tx2.rehash()

        b83 = self.update_block(83, [tx1, tx2])
        self.send_blocks([b83], True)
        self.save_spendable_output()

        # Reorg on/off blocks that have OP_RETURN in them (and try to spend them)
        #
        #  -> b81 (26) -> b82 (27) -> b83 (28) -> b84 (29) -> b87 (30) -> b88 (31)
        #                                    \-> b85 (29) -> b86 (30)            \-> b89a (32)
        #
        self.log.info("Test re-orging blocks with OP_RETURN in them")
        b84 = self.next_block(84)
        tx1 = self.create_tx(out[29], 0, 0, CScript([OP_RETURN]))
        tx1.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx1.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx1.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx1.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx1.calc_sha256()
        self.sign_tx(tx1, out[29])
        tx1.rehash()
        tx2 = self.create_tx(tx1, 1, 0, CScript([OP_RETURN]))
        tx2.vout.append(CTxOut(0, CScript([OP_RETURN])))
        tx3 = self.create_tx(tx1, 2, 0, CScript([OP_RETURN]))
        tx3.vout.append(CTxOut(0, CScript([OP_TRUE])))
        tx4 = self.create_tx(tx1, 3, 0, CScript([OP_TRUE]))
        tx4.vout.append(CTxOut(0, CScript([OP_RETURN])))
        tx5 = self.create_tx(tx1, 4, 0, CScript([OP_RETURN]))

        b84 = self.update_block(84, [tx1, tx2, tx3, tx4, tx5])
        self.send_blocks([b84], True)
        self.save_spendable_output()

        self.move_tip(83)
        b85 = self.next_block(85, spend=out[29])
        self.send_blocks([b85], False)  # other chain is same length

        b86 = self.next_block(86, spend=out[30])
        self.send_blocks([b86], True)

        self.move_tip(84)
        b87 = self.next_block(87, spend=out[30])
        self.send_blocks([b87], False)  # other chain is same length
        self.save_spendable_output()

        b88 = self.next_block(88, spend=out[31])
        self.send_blocks([b88], True)
        self.save_spendable_output()

        # trying to spend the OP_RETURN output is rejected
        b89a = self.next_block("89a", spend=out[32])
        tx = self.create_tx(tx1, 0, 0, CScript([OP_TRUE]))
        b89a = self.update_block("89a", [tx])
        self.send_blocks([b89a], success=False, reject_reason='bad-txns-inputs-missingorspent', reconnect=True)

        self.log.info("Test a re-org of one week's worth of blocks (1088 blocks)")

        self.move_tip(88)
        LARGE_REORG_SIZE = 1088
        blocks = []
        spend = out[32]
        for i in range(89, LARGE_REORG_SIZE + 89):
            b = self.next_block(i, spend)
            tx = CTransaction()
            script_length = MAX_BLOCK_BASE_SIZE - len(b.serialize()) - 69
            script_output = CScript([b'\x00' * script_length])
            tx.vout.append(CTxOut(0, script_output))
            tx.vin.append(CTxIn(COutPoint(b.vtx[1].sha256, 0)))
            b = self.update_block(i, [tx])
            assert_equal(len(b.serialize()), MAX_BLOCK_BASE_SIZE)
            blocks.append(b)
            self.save_spendable_output()
            spend = self.get_spendable_output()

        self.send_blocks(blocks, True, timeout=2440)
        chain1_tip = i

        # now create alt chain of same length
        self.move_tip(88)
        blocks2 = []
        for i in range(89, LARGE_REORG_SIZE + 89):
            blocks2.append(self.next_block("alt" + str(i)))
        self.send_blocks(blocks2, False, force_send=True)

        # extend alt chain to trigger re-org
        block = self.next_block("alt" + str(chain1_tip + 1))
        self.send_blocks([block], True, timeout=2440)

        # ... and re-org back to the first chain
        self.move_tip(chain1_tip)
        block = self.next_block(chain1_tip + 1)
        self.send_blocks([block], False, force_send=True)
        block = self.next_block(chain1_tip + 2)
        self.send_blocks([block], True, timeout=2440)

        self.log.info("Reject a block with an invalid block header version")
        b_v1 = self.next_block('b_v1', version=1)
        self.send_blocks([b_v1], success=False, force_send=True, reject_reason='bad-version(0x00000001)', reconnect=True)

        self.move_tip(chain1_tip + 2)
        b_cb34 = self.next_block('b_cb34')
        b_cb34.vtx[0].vin[0].scriptSig = b_cb34.vtx[0].vin[0].scriptSig[:-1]
        b_cb34.vtx[0].rehash()
        b_cb34.hashMerkleRoot = b_cb34.calc_merkle_root()
        b_cb34.solve()
        self.send_blocks([b_cb34], success=False, reject_reason='bad-cb-height', reconnect=True)

    # Helper methods
    ################

    def add_transactions_to_block(self, block, tx_list):
        [tx.rehash() for tx in tx_list]
        block.vtx.extend(tx_list)

    # this is a little handier to use than the version in blocktools.py
    def create_tx(self, spend_tx, n, value, script=CScript([OP_TRUE, OP_DROP] * 15 + [OP_TRUE])):
        return create_tx_with_script(spend_tx, n, amount=value, script_pub_key=script)

    # sign a transaction, using the key we know about
    # this signs input 0 in tx, which is assumed to be spending output 0 in spend_tx
    def sign_tx(self, tx, spend_tx):
        scriptPubKey = bytearray(spend_tx.vout[0].scriptPubKey)
        if (scriptPubKey[0] == OP_TRUE):  # an anyone-can-spend
            tx.vin[0].scriptSig = CScript()
            return
        (sighash, err) = LegacySignatureHash(spend_tx.vout[0].scriptPubKey, tx, 0, SIGHASH_ALL)
        tx.vin[0].scriptSig = CScript([self.coinbase_key.sign_ecdsa(sighash) + bytes(bytearray([SIGHASH_ALL]))])

    def create_and_sign_transaction(self, spend_tx, value, script=CScript([OP_TRUE])):
        tx = self.create_tx(spend_tx, 0, value, script)
        self.sign_tx(tx, spend_tx)
        tx.rehash()
        return tx

    def next_block(self, number, spend=None, additional_coinbase_value=0, script=CScript([OP_TRUE]), *, version=4):
        if self.tip is None:
            base_block_hash = self.genesis_hash
            block_time = int(time.time()) + 1
        else:
            base_block_hash = self.tip.sha256
            block_time = self.tip.nTime + 1
        # First create the coinbase
        height = self.block_heights[base_block_hash] + 1
        coinbase = create_coinbase(height, self.coinbase_pubkey)
        coinbase.vout[0].nValue += additional_coinbase_value
        coinbase.rehash()
        if spend is None:
            block = create_block(base_block_hash, coinbase, block_time, version=version)
        else:
            coinbase.vout[0].nValue += spend.vout[0].nValue - 1  # all but one satoshi to fees
            coinbase.rehash()
            block = create_block(base_block_hash, coinbase, block_time, version=version)
            tx = self.create_tx(spend, 0, 1, script)  # spend 1 satoshi
            self.sign_tx(tx, spend)
            self.add_transactions_to_block(block, [tx])
            block.hashMerkleRoot = block.calc_merkle_root()
        # Block is created. Find a valid nonce.
        block.solve()
        self.tip = block
        self.block_heights[block.sha256] = height
        assert number not in self.blocks
        self.blocks[number] = block
        return block

    # save the current tip so it can be spent by a later block
    def save_spendable_output(self):
        self.log.debug(f"saving spendable output {self.tip.vtx[0]}")
        self.spendable_outputs.append(self.tip)

    # get an output that we previously marked as spendable
    def get_spendable_output(self):
        self.log.debug(f"getting spendable output {self.spendable_outputs[0].vtx[0]}")
        return self.spendable_outputs.pop(0).vtx[0]

    # move the tip back to a previous block
    def move_tip(self, number):
        self.tip = self.blocks[number]

    # adds transactions to the block and updates state
    def update_block(self, block_number, new_transactions):
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

    def bootstrap_p2p(self, timeout=10):
        """Add a P2P connection to the node.

        Helper to connect and wait for version handshake."""
        self.helper_peer = self.nodes[0].add_p2p_connection(P2PDataStore())
        # We need to wait for the initial getheaders from the peer before we
        # start populating our blockstore. If we don't, then we may run ahead
        # to the next subtest before we receive the getheaders. We'd then send
        # an INV for the next block and receive two getheaders - one for the
        # IBD and one for the INV. We'd respond to both and could get
        # unexpectedly disconnected if the DoS score for that error is 50.
        self.helper_peer.wait_for_getheaders(timeout=timeout)

    def reconnect_p2p(self, timeout=60):
        """Tear down and bootstrap the P2P connection to the node.

        The node gets disconnected several times in this test. This helper
        method reconnects the p2p and restarts the network thread."""
        self.nodes[0].disconnect_p2ps()
        self.bootstrap_p2p(timeout=timeout)

    def send_blocks(self, blocks, success=True, reject_reason=None, force_send=False, reconnect=False, timeout=960):
        """Sends blocks to test node. Syncs and verifies that tip has advanced to most recent block.

        Call with success = False if the tip shouldn't advance to the most recent block."""
        self.helper_peer.send_blocks_and_test(blocks, self.nodes[0], success=success, reject_reason=reject_reason, force_send=force_send, timeout=timeout, expect_disconnect=reconnect)

        if reconnect:
            self.reconnect_p2p(timeout=timeout)


if __name__ == '__main__':
    FullBlockTest().main()
