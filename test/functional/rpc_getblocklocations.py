#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getblocklocations rpc call."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    util_xor,
)
from test_framework.messages import ser_vector


class GetblocklocationsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        """Test a trivial usage of the getblocklocations RPC command."""
        node = self.nodes[0]
        test_block_count = 7
        self.generate(node, test_block_count)

        NULL_HASH = '0000000000000000000000000000000000000000000000000000000000000000'

        block_hashes = [node.getblockhash(height) for height in range(test_block_count)]
        block_hashes.reverse()

        block_locations = {}
        def check_consistency(tip, a):
            for o in a:
                if tip in block_locations:
                    assert_equal(block_locations[tip], o)
                else:
                    block_locations[tip] = o
                tip = o['prev']

        # Get blocks' locations using several batch sizes
        last_locations = None
        for batch_size in range(1, 10):
            locations = []
            tip = block_hashes[0]
            while tip != NULL_HASH:
                locations.extend(node.getblocklocations(tip, batch_size))
                check_consistency(block_hashes[0], locations)
                tip = locations[-1]['prev']
            if last_locations: assert_equal(last_locations, locations)
            last_locations = locations

        xor_key = node.read_xor_key()

        # Read blocks' data from the file system
        blocks_dir = node.chain_path / 'blocks'
        with (blocks_dir / 'blk00000.dat').open('rb') as blkfile:
            for block_hash in block_hashes:
                location = block_locations[block_hash]
                block_bytes = bytes.fromhex(node.getblock(block_hash, 0))
                assert_file_contains(blkfile, location['data'], block_bytes, xor_key)


        empty_undo = ser_vector([])  # empty blocks = no transactions to undo
        with (blocks_dir / 'rev00000.dat').open('rb') as revfile:
            for block_hash in block_hashes[:-1]:  # skip genesis block (has no undo)
                location = block_locations[block_hash]
                assert_file_contains(revfile, location['undo'], empty_undo, xor_key)

        # Fail getting unknown block
        unknown_block_hash = '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef'
        assert_raises_rpc_error(-5, 'Block not found', node.getblocklocations, unknown_block_hash, 3)

        # Fail in pruned mode
        self.restart_node(0, ['-prune=1'])
        tip = block_hashes[0]
        assert_raises_rpc_error(-1, 'Block locations are not available in prune mode', node.getblocklocations, tip, 3)


def assert_file_contains(fileobj, offset, data, xor_key):
    fileobj.seek(offset)
    read_data = fileobj.read(len(data))
    read_data = util_xor(read_data, xor_key, offset=offset)
    assert_equal(read_data, data)

if __name__ == '__main__':
    GetblocklocationsTest(__file__).main()
