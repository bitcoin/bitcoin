#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test block propagation and sync to the new chain tip after invalidateblock.

1) Generate N+1 blocks on node0.
2) Invalidate a block on both nodes using invalidateblock.
3) Generate N blocks on node0.
3) Check if both node0 and non-mining node1 sync to the new, shorter chain tip.
4) Generate 1 more block on node0, verify both nodes sync.
4) Generate N more blocks on node0, verify both nodes sync.

This test is performed twice, order-independently:
N = MAX_BLOCKS_IN_TRANSIT_PER_PEER + 1 -> node0 succeeds, node1 often degraded
N = MAX_BLOCKS_IN_TRANSIT_PER_PEER     -> succeeds for both nodes

For the non-mining peer, sync to the new tip on the first coinbase transaction
appears to be degraded beyond the limit of MAX_BLOCKS_IN_TRANSIT_PER_PEER. The
first 16 blocks succeed because of the call to GETDATA on processing each INV,
which occurs until the number of blocks in flight to the peer is maxed out.
See issue #5806.

"""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than

BLOCKS = 16  # Number of blocks to test, equal to MAX_BLOCKS_IN_TRANSIT_PER_PEER

# Hardcoded addresses from test_node.py::PRIV_KEYS. They are used to generate
# to different addresses at each step, to ensure that the coinbase transaction
# (and therefore the block hash) is different.
ADDRESSES = [
    "mjTkW3DjgyZck4KbiRusZsqTgaYTxdSz6z",
    "msX6jQXvxiNhx3Q62PKeLPrhrqZQdSimTg",
    "mnonCMyH9TmAsSj3M59DsbH8H63U3RKoFP",
    "mqJupas8Dt2uestQDvV2NH3RU8uZh2dqQR",
    "msYac7Rvd5ywm6pEmkjyxhbCDKqWsVeYws",
    "n2rnuUnwLgXqf9kk2kjvVm8R5BZK1yxQBi",
    "myzuPxRwsf3vvGzEuzPfK9Nf2RfwauwYe6",
    "mumwTaMtbxEPUswmLBBN3vM9oGRtGBrys8",
]


class P2PInvalidateBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.addr_index = 0

    def assert_block_counts(self, expected):
        for node in self.nodes:
            assert_equal(expected, node.getblockcount())

    def test_invalidate_regenerate(self, block_count, blocks):
        self.log.info("Test syncing {} blocks to new tip after invalidateblock".format(blocks))
        self.log.info(" Generate {} + 1 blocks on node0".format(blocks))
        block_hashes = self.nodes[0].generatetoaddress(blocks + 1, ADDRESSES[self.addr_index])
        self.addr_index += 1
        self.sync_blocks()
        self.assert_block_counts(expected=block_count + blocks + 1)
        self.log.info(" Invalidate a block")
        for node in self.nodes:
            node.invalidateblock(block_hashes[0])
        self.assert_block_counts(expected=block_count)
        self.log.info(" Generate an alternate chain of {} blocks on node0".format(blocks))
        self.nodes[0].generatetoaddress(blocks, ADDRESSES[self.addr_index])
        self.addr_index += 1

    def test_sync_after_more_blocks(self, block_count, blocks):
        self.log.info(" Generate {} more block(s) on the new chain on node0".format(blocks))
        self.nodes[0].generatetoaddress(blocks, ADDRESSES[self.addr_index])
        self.addr_index += 1
        self.log.info(" Verify both nodes sync to the new tip")
        self.sync_blocks()
        self.assert_block_counts(expected=block_count + blocks)

    def test_sync_MAX_BLOCKS_IN_TRANSIT_PER_PEER_blocks_after_invalidateblock(self):
        block_count = self.nodes[0].getblockcount()
        target_height = block_count + BLOCKS
        self.test_invalidate_regenerate(block_count, blocks=BLOCKS)
        self.log.info(" Verify both nodes sync to the new tip")
        self.sync_blocks()
        self.assert_block_counts(expected=target_height)
        self.test_sync_after_more_blocks(target_height, blocks=1)
        self.test_sync_after_more_blocks(target_height + 1, blocks=BLOCKS)

    def test_sync_n_plus_MAX_BLOCKS_IN_TRANSIT_PER_PEER_blocks_after_invalidateblock(self, n=1):
        block_count = self.nodes[0].getblockcount()
        target_height = block_count + BLOCKS + n
        self.test_invalidate_regenerate(block_count, blocks=BLOCKS + n)
        # Usually node1 doesn't sync past MAX_BLOCKS_IN_TRANSIT_PER_PEER and times out, but it
        # can sometimes succeed. Handle both cases to aid in diagnosing and fixing the issue.
        try:
            self.sync_blocks(timeout=3)
        except AssertionError:
            self.log.info(" Verify node1 fails to sync to the new tip")
            assert_greater_than(target_height, self.nodes[1].getblockcount())
        else:
            self.log.info(" Verify node1 syncs to the new tip")
            assert_equal(target_height, self.nodes[1].getblockcount())
        self.log.info(" Verify node0 syncs to the new tip")
        assert_equal(target_height, self.nodes[0].getblockcount())
        self.test_sync_after_more_blocks(target_height, blocks=1)
        self.test_sync_after_more_blocks(target_height + 1, blocks=BLOCKS + n)

    def run_test(self):
        self.test_sync_n_plus_MAX_BLOCKS_IN_TRANSIT_PER_PEER_blocks_after_invalidateblock()
        self.test_sync_MAX_BLOCKS_IN_TRANSIT_PER_PEER_blocks_after_invalidateblock()


if __name__ == '__main__':
    P2PInvalidateBlockTest().main()
