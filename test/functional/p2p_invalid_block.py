#!/usr/bin/env python3
# Copyright (c) 2015-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid blocks.

In this test we connect to one node over p2p, and test block requests:
1) Valid blocks should be requested and become chain tip.
2) Invalid block with duplicated transaction should be re-requested.
3) Invalid block with bad coinbase value should be rejected and not
re-requested.
4) Invalid block due to future timestamp is later accepted when that timestamp
becomes valid.
"""
import copy
import time

from test_framework.blocktools import (
    MAX_FUTURE_BLOCK_TIME,
    create_block,
    create_coinbase,
    create_tx_with_script,
)
from test_framework.messages import COIN
from test_framework.p2p import P2PDataStore
from test_framework.script import OP_TRUE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_not_equal,
)


class InvalidBlockRequestTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True

    def run_test(self):
        # Add p2p connection to node0
        node = self.nodes[0]  # convenience reference to the node
        peer = node.add_p2p_connection(P2PDataStore())

        best_block = node.getblock(node.getbestblockhash())
        tip = int(node.getbestblockhash(), 16)
        height = best_block["height"] + 1
        block_time = best_block["time"] + 1

        self.log.info("Create a new block with an anyone-can-spend coinbase")

        block = create_block(tip, create_coinbase(height), block_time)
        block.solve()
        # Save the coinbase for later
        block1 = block
        peer.send_blocks_and_test([block1], node, success=True)

        self.log.info("Mature the block.")
        self.generatetoaddress(node, 100, node.get_deterministic_priv_key().address)

        best_block = node.getblock(node.getbestblockhash())
        tip = int(node.getbestblockhash(), 16)
        height = best_block["height"] + 1
        block_time = best_block["time"] + 1

        # Use merkle-root malleability to generate an invalid block with
        # same blockheader (CVE-2012-2459).
        # Manufacture a block with 3 transactions (coinbase, spend of prior
        # coinbase, spend of that spend).  Duplicate the 3rd transaction to
        # leave merkle root and blockheader unchanged but invalidate the block.
        # For more information on merkle-root malleability see src/consensus/merkle.cpp.
        self.log.info("Test merkle root malleability.")

        tx1 = create_tx_with_script(block1.vtx[0], 0, script_sig=bytes([OP_TRUE]), amount=50 * COIN)
        tx2 = create_tx_with_script(tx1, 0, script_sig=bytes([OP_TRUE]), amount=50 * COIN)
        block2 = create_block(tip, create_coinbase(height), block_time, txlist=[tx1, tx2])
        block_time += 1
        block2.solve()
        orig_hash = block2.sha256
        block2_orig = copy.deepcopy(block2)

        # Mutate block 2
        block2.vtx.append(tx2)
        assert_equal(block2.hashMerkleRoot, block2.calc_merkle_root())
        assert_equal(orig_hash, block2.rehash())
        assert_not_equal(block2_orig.vtx, block2.vtx)

        peer.send_blocks_and_test([block2], node, success=False, reject_reason='bad-txns-duplicate')

        # Check transactions for duplicate inputs (CVE-2018-17144)
        self.log.info("Test duplicate input block.")

        block2_dup = copy.deepcopy(block2_orig)
        block2_dup.vtx[2].vin.append(block2_dup.vtx[2].vin[0])
        block2_dup.hashMerkleRoot = block2_dup.calc_merkle_root()
        block2_dup.solve()
        peer.send_blocks_and_test([block2_dup], node, success=False, reject_reason='bad-txns-inputs-duplicate')

        self.log.info("Test very broken block.")

        block3 = create_block(tip, create_coinbase(height, nValue=100), block_time)
        block_time += 1
        block3.solve()

        peer.send_blocks_and_test([block3], node, success=False, reject_reason='bad-cb-amount')


        # Complete testing of CVE-2012-2459 by sending the original block.
        # It should be accepted even though it has the same hash as the mutated one.

        self.log.info("Test accepting original block after rejecting its mutated version.")
        peer.send_blocks_and_test([block2_orig], node, success=True, timeout=5)

        # Update tip info
        height += 1
        block_time += 1
        tip = int(block2_orig.hash, 16)

        # Complete testing of CVE-2018-17144, by checking for the inflation bug.
        # Create a block that spends the output of a tx in a previous block.
        tx3 = create_tx_with_script(tx2, 0, script_sig=bytes([OP_TRUE]), amount=50 * COIN)
        tx3.vin.append(tx3.vin[0])  # Duplicates input
        block4 = create_block(tip, create_coinbase(height), block_time, txlist=[tx3])
        block4.solve()
        self.log.info("Test inflation by duplicating input")
        peer.send_blocks_and_test([block4], node, success=False,  reject_reason='bad-txns-inputs-duplicate')

        self.log.info("Test accepting identical block after rejecting it due to a future timestamp.")
        t = int(time.time())
        node.setmocktime(t)
        # Set block time +1 second past max future validity
        block = create_block(tip, create_coinbase(height), t + MAX_FUTURE_BLOCK_TIME + 1)
        block.solve()
        # Need force_send because the block will get rejected without a getdata otherwise
        peer.send_blocks_and_test([block], node, force_send=True, success=False, reject_reason='time-too-new')
        node.setmocktime(t + 1)
        peer.send_blocks_and_test([block], node, success=True)


if __name__ == '__main__':
    InvalidBlockRequestTest(__file__).main()
