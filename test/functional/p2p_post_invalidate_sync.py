#!/usr/bin/env python3
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test invalidateblock
#

from test_framework.test_framework import BitcoinTestFramework

class InvalidateBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[],[]]

    def run_test(self):
        self.nodes[0].generate(1) # Leave IBD
        self.sync_all()

        cnt = self.nodes[0].getblockcount()

        node1blocks = self.nodes[1].generate(18)

        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 18):
            raise AssertionError("Failed to sync initial blocks")

        self.nodes[0].invalidateblock(node1blocks[0])
        self.nodes[1].invalidateblock(node1blocks[0])

        if (self.nodes[0].getblockcount() != cnt):
            raise AssertionError("Failed to invalidate initial blocks")

        # The test framework uses a static per-node address which will generate
        # a deterministic block if we have no wallet.
        # Instead, mine on nodes[0], which will use a different hardcoded address
        # than the one we previously used, making this block unique.
        self.nodes[0].generate(17)

        print("All blocks generated, trying to sync")
        self.sync_all()
        if (self.nodes[0].getblockcount() != cnt + 17):
            raise AssertionError("Failed to sync shorter but valid chain")

if __name__ == '__main__':
    InvalidateBlockTest().main()
