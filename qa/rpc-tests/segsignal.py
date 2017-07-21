#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import ComparisonTestFramework
from test_framework.util import *
from test_framework.blocktools import create_coinbase, create_block

class SegsignalUASFTest(ComparisonTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        self.tip = int("0x" + self.nodes[0].getbestblockhash(), 0)
        self.height = 1  # height of the next block to build
        self.last_block_time = int(time.time())

        # Mine 1439 non segwit blocks
        self.generate_blocks(1439, 0x20000000)

        # Check that segwit is still started
        assert_equal(self.get_bip9_status('segwit')['status'], 'started')

        # Mine a non-segwit block. this should be rejected
        self.generate_blocks(1, 0x20000000, 'bad-no-segwit')

        # Now mine a segwit block. this should not be rejected
        self.generate_blocks(1, 0x20000002)

        # Mine another non-segwit block, this should be rejected
        self.generate_blocks(1, 0x20000000, 'bad-no-segwit')

        # Mine 143 more blocks. should still be started
        print(self.nodes[0].getblockchaininfo())
        self.generate_blocks(142, 0x20000002)
        assert_equal(self.get_bip9_status('segwit')['status'], 'started')

        # Mine 1 more block, should now be locked in
        self.generate_blocks(1, 0x20000002)
        print(self.nodes[0].getblockchaininfo())
        assert_equal(self.get_bip9_status('segwit')['status'], 'locked_in')

        # Mine a non-segwit block, should be accepted
        self.generate_blocks(1, 0x20000000)

    def generate_blocks(self, number, version, error = None):
        for i in range(number):
            block = create_block(self.tip, create_coinbase(self.height), self.last_block_time + 1)
            block.nVersion = version
            block.rehash()
            block.solve()
            assert_equal(self.nodes[0].submitblock(bytes_to_hex_str(block.serialize())), error)
            if (error == None):
                self.last_block_time += 1
                self.tip = block.sha256
                self.height += 1

    def get_bip9_status(self, key):
        info = self.nodes[0].getblockchaininfo()
        return info['bip9_softforks'][key]


if __name__ == '__main__':
    SegsignalUASFTest().main()
