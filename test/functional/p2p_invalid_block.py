#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test node responses to invalid blocks.

In this test we connect to one node over p2p, and test block requests:
1) Valid blocks should be requested and become chain tip.
2) Invalid block with duplicated transaction should be re-requested.
3) Invalid block with bad coinbase value should be rejected and not
re-requested.
"""
import copy

from test_framework.blocktools import create_block, create_coinbase, create_tx_with_script
from test_framework.messages import COIN
from test_framework.mininode import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal


class InvalidBlockRequestTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [["-whitelist=127.0.0.1"]]

    def run_test(self):
        # Add p2p connection to node0
        node = self.nodes[0]  # convenience reference to the node
        node.add_p2p_connection(P2PDataStore())

        best_block = node.getblock(node.getbestblockhash())
        tip = int(node.getbestblockhash(), 16)
        height = best_block["height"] + 1
        block_time = best_block["time"] + 1

        self.log.info("Create a new block with an anyone-can-spend coinbase")

        height = 1
        block = create_block(tip, create_coinbase(height), block_time)
        block.solve()
        # Save the coinbase for later
        block1 = block
        tip = block.sha256
        node.p2p.send_blocks_and_test([block1], node, success=True)

        self.log.info("Mature the block.")
        node.generate(100)

        best_block = node.getblock(node.getbestblockhash())
        tip = int(node.getbestblockhash(), 16)
        height = best_block["height"] + 1
        block_time = best_block["time"] + 1

        # Use merkle-root malleability to generate an invalid block with
        # same blockheader.
        # Manufacture a block with 3 transactions (coinbase, spend of prior
        # coinbase, spend of that spend).  Duplicate the 3rd transaction to
        # leave merkle root and blockheader unchanged but invalidate the block.
        self.log.info("Test merkle root malleability.")

        block2 = create_block(tip, create_coinbase(height), block_time)
        block_time += 1

        # b'0x51' is OP_TRUE
        tx1 = create_tx_with_script(block1.vtx[0], 0, script_sig=b'\x51', amount=50 * COIN)
        tx2 = create_tx_with_script(tx1, 0, script_sig=b'\x51', amount=50 * COIN)

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

        node.p2p.send_blocks_and_test([block2], node, success=False, request_block=False, reject_code=16, reject_reason=b'bad-txns-duplicate')

        self.log.info("Test very broken block.")

        block3 = create_block(tip, create_coinbase(height), block_time)
        block_time += 1
        block3.vtx[0].vout[0].nValue = 1000 * COIN  # Too high!
        block3.vtx[0].sha256 = None
        block3.vtx[0].calc_sha256()
        block3.hashMerkleRoot = block3.calc_merkle_root()
        block3.rehash()
        block3.solve()

        node.p2p.send_blocks_and_test([block3], node, success=False, request_block=False, reject_code=16, reject_reason=b'bad-cb-amount')

if __name__ == '__main__':
    InvalidBlockRequestTest().main()
