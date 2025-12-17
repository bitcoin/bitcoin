#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the invalidateblock RPC."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR
from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class InvalidateTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        self.log.info("Make sure we repopulate setBlockIndexCandidates after InvalidateBlock:")
        self.log.info("Mine 4 blocks on Node 0")
        self.generate(self.nodes[0], 4, sync_fun=self.no_op)
        assert_equal(self.nodes[0].getblockcount(), 4)
        besthash_n0 = self.nodes[0].getbestblockhash()

        self.log.info("Mine competing 6 blocks on Node 1")
        self.generate(self.nodes[1], 6, sync_fun=self.no_op)
        assert_equal(self.nodes[1].getblockcount(), 6)

        self.log.info("Connect nodes to force a reorg")
        self.connect_nodes(0, 1)
        self.sync_blocks(self.nodes[0:2])
        assert_equal(self.nodes[0].getblockcount(), 6)

        # Add a header to the tip of node 0 without submitting the block. This shouldn't
        # affect results since this chain will be invalidated next.
        tip = self.nodes[0].getbestblockhash()
        block_time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time'] + 1
        block = create_block(int(tip, 16), create_coinbase(self.nodes[0].getblockcount()), block_time, version=4)
        block.solve()
        self.nodes[0].submitheader(block.serialize().hex())
        assert_equal(self.nodes[0].getblockchaininfo()["headers"], self.nodes[0].getblockchaininfo()["blocks"] + 1)

        self.log.info("Invalidate block 2 on node 0 and verify we reorg to node 0's original chain")
        badhash = self.nodes[1].getblockhash(2)
        self.nodes[0].invalidateblock(badhash)
        assert_equal(self.nodes[0].getblockcount(), 4)
        assert_equal(self.nodes[0].getbestblockhash(), besthash_n0)
        # Should report consistent blockchain info
        assert_equal(self.nodes[0].getblockchaininfo()["headers"], self.nodes[0].getblockchaininfo()["blocks"])

        self.log.info("Reconsider block 6 on node 0 again and verify that the best header is set correctly")
        self.nodes[0].reconsiderblock(tip)
        assert_equal(self.nodes[0].getblockchaininfo()["headers"], self.nodes[0].getblockchaininfo()["blocks"] + 1)

        self.log.info("Invalidate block 2 on node 0 and verify we reorg to node 0's original chain again")
        self.nodes[0].invalidateblock(badhash)
        assert_equal(self.nodes[0].getblockcount(), 4)
        assert_equal(self.nodes[0].getbestblockhash(), besthash_n0)
        assert_equal(self.nodes[0].getblockchaininfo()["headers"], self.nodes[0].getblockchaininfo()["blocks"])

        self.log.info("Make sure we won't reorg to a lower work chain:")
        self.connect_nodes(1, 2)
        self.log.info("Sync node 2 to node 1 so both have 6 blocks")
        self.sync_blocks(self.nodes[1:3])
        assert_equal(self.nodes[2].getblockcount(), 6)
        self.log.info("Invalidate block 5 on node 1 so its tip is now at 4")
        self.nodes[1].invalidateblock(self.nodes[1].getblockhash(5))
        assert_equal(self.nodes[1].getblockcount(), 4)
        self.log.info("Invalidate block 3 on node 2, so its tip is now 2")
        self.nodes[2].invalidateblock(self.nodes[2].getblockhash(3))
        assert_equal(self.nodes[2].getblockcount(), 2)
        self.log.info("..and then mine a block")
        self.generate(self.nodes[2], 1, sync_fun=self.no_op)
        self.log.info("Verify all nodes are at the right height")
        self.wait_until(lambda: self.nodes[2].getblockcount() == 3, timeout=5)
        self.wait_until(lambda: self.nodes[0].getblockcount() == 4, timeout=5)
        self.wait_until(lambda: self.nodes[1].getblockcount() == 4, timeout=5)

        self.log.info("Verify that ancestors can become chain tip candidates when we reconsider blocks")
        # Invalidate node0's current chain (1' -> 2' -> 3' -> 4') so that we don't reorg back to it in this test
        badhash = self.nodes[0].getblockhash(1)
        self.nodes[0].invalidateblock(badhash)
        # Reconsider the tip so that node0's chain becomes this chain again : 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> header 7
        self.nodes[0].reconsiderblock(tip)
        blockhash_3 = self.nodes[0].getblockhash(3)
        blockhash_4 = self.nodes[0].getblockhash(4)
        blockhash_6 = self.nodes[0].getblockhash(6)
        assert_equal(self.nodes[0].getbestblockhash(), blockhash_6)

        # Invalidate block 4 so that chain becomes : 1 -> 2 -> 3
        self.nodes[0].invalidateblock(blockhash_4)
        assert_equal(self.nodes[0].getbestblockhash(), blockhash_3)
        assert_equal(self.nodes[0].getblockchaininfo()['blocks'], 3)
        assert_equal(self.nodes[0].getblockchaininfo()['headers'], 3)

        # Reconsider the header
        self.nodes[0].reconsiderblock(block.hash_hex)
        # Since header doesn't have block data, it can't be chain tip
        # Check if it's possible for an ancestor (with block data) to be the chain tip
        assert_equal(self.nodes[0].getbestblockhash(), blockhash_6)
        assert_equal(self.nodes[0].getblockchaininfo()['blocks'], 6)
        assert_equal(self.nodes[0].getblockchaininfo()['headers'], 7)

        self.log.info("Verify that we reconsider all ancestors as well")
        blocks = self.generatetodescriptor(self.nodes[1], 10, ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR, sync_fun=self.no_op)
        assert_equal(self.nodes[1].getbestblockhash(), blocks[-1])
        # Invalidate the two blocks at the tip
        self.nodes[1].invalidateblock(blocks[-1])
        self.nodes[1].invalidateblock(blocks[-2])
        assert_equal(self.nodes[1].getbestblockhash(), blocks[-3])
        # Reconsider only the previous tip
        self.nodes[1].reconsiderblock(blocks[-1])
        # Should be back at the tip by now
        assert_equal(self.nodes[1].getbestblockhash(), blocks[-1])

        self.log.info("Verify that we reconsider all descendants")
        blocks = self.generatetodescriptor(self.nodes[1], 10, ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR, sync_fun=self.no_op)
        assert_equal(self.nodes[1].getbestblockhash(), blocks[-1])
        # Invalidate the two blocks at the tip
        self.nodes[1].invalidateblock(blocks[-2])
        self.nodes[1].invalidateblock(blocks[-4])
        assert_equal(self.nodes[1].getbestblockhash(), blocks[-5])
        # Reconsider only the previous tip
        self.nodes[1].reconsiderblock(blocks[-4])
        # Should be back at the tip by now
        assert_equal(self.nodes[1].getbestblockhash(), blocks[-1])
        # Should report consistent blockchain info
        assert_equal(self.nodes[1].getblockchaininfo()["headers"], self.nodes[1].getblockchaininfo()["blocks"])

        self.log.info("Verify that invalidating an unknown block throws an error")
        assert_raises_rpc_error(-5, "Block not found", self.nodes[1].invalidateblock, "00" * 32)
        assert_equal(self.nodes[1].getbestblockhash(), blocks[-1])


if __name__ == '__main__':
    InvalidateTest(__file__).main()
