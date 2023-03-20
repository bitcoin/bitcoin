#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getchaintips RPC when a block is invalid.

- pregenerate blocks
- start fresh node, send headers
- malleate one block to invalidate it
- getchaintips should have labeled the invalid chain
"""

from test_framework.blocktools import (
    create_block,
    create_coinbase
)
from test_framework.messages import (
    CTransaction,
    COutPoint,
    CTxIn,
    CTxOut
)
from test_framework.p2p import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class GetChainTipsInvalidTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        peer = P2PDataStore()

        # Generate 20 blocks from genesis
        best_hash = node.getbestblockhash()
        best_block = node.getblock(best_hash)
        height = best_block["height"] + 1
        block_time = best_block["time"] + 1
        tip = int(best_hash, 16)
        block_hashes = []

        # Create tx that is invalid only "in context"
        # The node will download the block and won't mark it invalid until ConnectBlock()
        invalid_tx = CTransaction()
        invalid_tx.vin.append(CTxIn(COutPoint(0xdeadbeef, 0), b"", 0xffffffff))
        invalid_tx.vout.append(CTxOut(0, b""))

        msgs = []
        for _ in range(20):
            block = create_block(tip, create_coinbase(height), block_time)
            # Make this block invalid by inserting 0-input tx
            if height == 11:
                block.vtx.append(invalid_tx)
                block.hashMerkleRoot = block.calc_merkle_root()
                block.calc_sha256()
            block.solve()
            tip = block.sha256
            if height == 11:
                msgs.append(f"Found invalid chain branch ancestor hash={hex(tip)[2:]}")
            if height > 11:
                msgs.append(f"Invalidating child hash={hex(tip)[2:]}")
            height += 1
            block_time += 1
            # python node blockstore
            peer.block_store[tip] = block
            peer.last_block_hash = tip
            # ordered list of block hashes
            block_hashes.append(tip)

        # Child headers not marked as invalid yet
        with node.assert_debug_log(expected_msgs=[], unexpected_msgs=msgs):
            # Connect node to peer
            node.add_p2p_connection(peer, wait_for_verack=False)
            # bad block, bye bye
            peer.wait_for_disconnect()
            assert_equal(node.getblockcount(), 10)

        # This RPC marks the invalid children
        with node.assert_debug_log(expected_msgs=msgs):
            tips = node.getchaintips()
            assert_equal(len(tips), 2)
            a = next(tip for tip in tips if tip["height"] == 10)
            b = next(tip for tip in tips if tip["height"] == 20)
            assert a and b
            assert_equal(a["status"], "active")
            assert_equal(int(a["hash"], 16), block_hashes[9]) # last valid block
            assert_equal(b["status"], "invalid")  # not "headers-only"
            assert_equal(int(b["hash"], 16), tip) # last received block

        # Restart node and check again
        self.restart_node(0)
        tips = node.getchaintips()
        assert_equal(len(tips), 2)

        c = next(tip for tip in tips if tip["height"] == 10)
        d = next(tip for tip in tips if tip["height"] == 20)
        assert c and d
        assert_equal(c["status"], "active")
        assert_equal(int(c["hash"], 16), block_hashes[9]) # last valid block
        assert_equal(d["status"], "invalid")
        assert_equal(int(d["hash"], 16), tip) # last received block

if __name__ == '__main__':
    GetChainTipsInvalidTest().main()
