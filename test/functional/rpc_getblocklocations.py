#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getblocklocations rpc call."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (assert_equal, assert_raises_rpc_error)
from test_framework.messages import ser_vector

import pathlib


class GetblocklocationsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        """Test a trivial usage of the getblocklocations RPC command."""
        node = self.nodes[0]
        mocktime = node.getblockheader(node.getblockhash(0))['time'] + 1
        node.setmocktime(mocktime)
        node.generate(7)

        NULL_HASH = '0000000000000000000000000000000000000000000000000000000000000000'
        EXPECTED_LOCATIONS = [
            {'file': 0, 'data': 1861, 'undo': 254, 'prev': '1893864d81638cae904357907559248964050547ddcd0dbe3f72e65b5a31bf1f'},
            {'file': 0, 'data': 1601, 'undo': 213, 'prev': '1b086a63a5d5c320c747fee32cdb819c6f4f9eb78c24b7c5a33ae794218dc639'},
            {'file': 0, 'data': 1341, 'undo': 172, 'prev': '448640031d1418c70a109ea4f88f497804f3336c7309ff7e535894beb80b26e4'},
            {'file': 0, 'data': 1081, 'undo': 131, 'prev': '602403f82060e224423a9f22062fad3f5333beeee3c30313f6ea808e37b7e0b2'},
            {'file': 0, 'data': 821, 'undo': 90, 'prev': '334ab00aba5d213c8f67161ddff346c4643d0709c3bdafa4b4b05fe6f7e4ed48'},
            {'file': 0, 'data': 561, 'undo': 49, 'prev': '43f10598f19eced9c514f5ae40dbce0ab101362a22e18820901c6e03d7babe0b'},
            {'file': 0, 'data': 301, 'undo': 8, 'prev': '0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206'},
            {'file': 0, 'data': 8, 'prev': NULL_HASH},  # genesis block
        ]

        block_hashes = [node.getblockhash(height) for height in range(len(EXPECTED_LOCATIONS))]
        block_hashes.reverse()

        # Get blocks' locations using several batch sizes
        for batch_size in range(1, 10):
            locations = []
            tip = block_hashes[0]
            while tip != NULL_HASH:
                locations.extend(node.getblocklocations(tip, batch_size))
                assert_equal(locations, EXPECTED_LOCATIONS[:len(locations)])
                tip = locations[-1]['prev']
            assert_equal(locations, EXPECTED_LOCATIONS)

        # Read blocks' data from the file system
        blocks_dir = pathlib.Path(node.datadir) / node.chain / 'blocks'
        with (blocks_dir / 'blk00000.dat').open('rb') as blkfile:
            for block_hash, location in zip(block_hashes, EXPECTED_LOCATIONS):
                block_bytes = bytes.fromhex(node.getblock(block_hash, 0))
                assert_file_contains(blkfile, location['data'], block_bytes)


        empty_undo = ser_vector([])  # empty blocks = no transactions to undo
        with (blocks_dir / 'rev00000.dat').open('rb') as revfile:
            for block_hash, location in zip(block_hashes[:-1], EXPECTED_LOCATIONS):  # skip genesis block (has no undo)
                assert_file_contains(revfile, location['undo'], empty_undo)

        # Fail getting unknown block
        unknown_block_hash = '0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef'
        assert_raises_rpc_error(-5, 'Block not found', node.getblocklocations, unknown_block_hash, 3)

        # Fail in pruned mode
        self.restart_node(0, ['-prune=1'])
        tip = block_hashes[0]
        assert_raises_rpc_error(-1, 'Block locations are not available in prune mode', node.getblocklocations, tip, 3)


def assert_file_contains(fileobj, offset, data):
    fileobj.seek(offset)
    assert_equal(fileobj.read(len(data)), data)

if __name__ == '__main__':
    GetblocklocationsTest().main()
