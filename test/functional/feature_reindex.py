#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test running bitcoind with -reindex and -reindex-chainstate options.

- Start a single node and generate 3 blocks.
- Stop the node and restart it with -reindex. Verify that the node has reindexed up to block 3.
- Stop the node and restart it with -reindex-chainstate. Verify that the node has reindexed up to block 3.
- Verify that out-of-order blocks are correctly processed, see LoadExternalBlockFile()
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import MAGIC_BYTES
from test_framework.util import assert_equal


class ReindexTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def reindex(self, justchainstate=False):
        self.generatetoaddress(self.nodes[0], 3, self.nodes[0].get_deterministic_priv_key().address)
        blockcount = self.nodes[0].getblockcount()
        self.stop_nodes()
        extra_args = [["-reindex-chainstate" if justchainstate else "-reindex"]]
        self.start_nodes(extra_args)
        assert_equal(self.nodes[0].getblockcount(), blockcount)  # start_node is blocking on reindex
        self.log.info("Success")

    # Check that blocks can be processed out of order
    def out_of_order(self):
        # The previous test created 12 blocks
        assert_equal(self.nodes[0].getblockcount(), 12)
        self.stop_nodes()

        # In this test environment, blocks will always be in order (since
        # we're generating them rather than getting them from peers), so to
        # test out-of-order handling, swap blocks 1 and 2 on disk.
        blk0 = self.nodes[0].blocks_path / "blk00000.dat"
        with open(self.nodes[0].blocks_path / "xor.dat", "rb") as xor_f:
            NUM_XOR_BYTES = 8 # From InitBlocksdirXorKey::xor_key.size()
            xor_dat = xor_f.read(NUM_XOR_BYTES)

        def util_xor(data, key):
            data = bytearray(data)
            for i in range(len(data)):
                data[i] ^= key[i % len(key)]
            return bytes(data)

        with open(blk0, 'r+b') as bf:
            # Read at least the first few blocks (including genesis)
            b = util_xor(bf.read(2000), xor_dat)

            # Find the offsets of blocks 2, 3, and 4 (the first 3 blocks beyond genesis)
            # by searching for the regtest marker bytes (see pchMessageStart).
            def find_block(b, start):
                return b.find(MAGIC_BYTES["regtest"], start)+4

            genesis_start = find_block(b, 0)
            assert_equal(genesis_start, 4)
            b2_start = find_block(b, genesis_start)
            b3_start = find_block(b, b2_start)
            b4_start = find_block(b, b3_start)

            # Blocks 2 and 3 should be the same size.
            assert_equal(b3_start - b2_start, b4_start - b3_start)

            # Swap the second and third blocks (don't disturb the genesis block).
            b_new = bytearray(b)
            b_new[b2_start:b3_start] = b[b3_start:b4_start]
            b_new[b3_start:b4_start] = b[b2_start:b3_start]
            b_new = util_xor(b_new, xor_dat)
            bf.seek(b2_start)
            bf.write(b_new[b2_start:b4_start])

        # The reindexing code should detect and accommodate out of order blocks.
        with self.nodes[0].assert_debug_log([
            'LoadExternalBlockFile: Out of order block',
            'LoadExternalBlockFile: Processing out of order child',
        ]):
            extra_args = [["-reindex"]]
            self.start_nodes(extra_args)

        # All blocks should be accepted and processed.
        assert_equal(self.nodes[0].getblockcount(), 12)

    def run_test(self):
        self.reindex(False)
        self.reindex(True)
        self.reindex(False)
        self.reindex(True)

        self.out_of_order()


if __name__ == '__main__':
    ReindexTest().main()
