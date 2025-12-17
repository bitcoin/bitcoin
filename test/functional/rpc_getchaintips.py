#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the getchaintips RPC.

- introduce a network split
- work on chains of different lengths
- join the network together again
- verify that getchaintips now returns two chain tips.
"""

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class GetChainTipsTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4

    def run_test(self):
        self.log.info("Test getchaintips behavior with two chains of different length")
        tips = self.nodes[0].getchaintips()
        assert_equal(len(tips), 1)
        assert_equal(tips[0]['branchlen'], 0)
        assert_equal(tips[0]['height'], 200)
        assert_equal(tips[0]['status'], 'active')

        self.log.info("Split the network and build two chains of different lengths.")
        self.split_network()
        self.generate(self.nodes[0], 10, sync_fun=lambda: self.sync_all(self.nodes[:2]))
        self.generate(self.nodes[2], 20, sync_fun=lambda: self.sync_all(self.nodes[2:]))

        tips = self.nodes[1].getchaintips()
        assert_equal(len(tips), 1)
        shortTip = tips[0]
        assert_equal(shortTip['branchlen'], 0)
        assert_equal(shortTip['height'], 210)
        assert_equal(tips[0]['status'], 'active')

        tips = self.nodes[3].getchaintips()
        assert_equal(len(tips), 1)
        longTip = tips[0]
        assert_equal(longTip['branchlen'], 0)
        assert_equal(longTip['height'], 220)
        assert_equal(tips[0]['status'], 'active')

        self.log.info("Join the network halves and check that we now have two tips")
        # (at least at the nodes that previously had the short chain).
        self.join_network()

        tips = self.nodes[0].getchaintips()
        assert_equal(len(tips), 2)
        assert_equal(tips[0], longTip)

        assert_equal(tips[1]['branchlen'], 10)
        assert_equal(tips[1]['status'], 'valid-fork')
        tips[1]['branchlen'] = 0
        tips[1]['status'] = 'active'
        assert_equal(tips[1], shortTip)

        self.log.info("Test getchaintips behavior with invalid blocks")
        self.disconnect_nodes(0, 1)
        n0 = self.nodes[0]
        tip = int(n0.getbestblockhash(), 16)
        start_height = self.nodes[0].getblockcount()
        # Create invalid block (too high coinbase)
        block_time = n0.getblock(n0.getbestblockhash())['time'] + 1
        invalid_block = create_block(tip, create_coinbase(start_height+1, nValue=100), block_time)
        invalid_block.solve()

        block_time += 1
        block2 = create_block(invalid_block.hash_int, create_coinbase(2), block_time, version=4)
        block2.solve()

        self.log.info("Submit headers-only chain")
        n0.submitheader(invalid_block.serialize().hex())
        n0.submitheader(block2.serialize().hex())
        tips = n0.getchaintips()
        assert_equal(len(tips), 3)
        assert_equal(tips[0]['status'], 'headers-only')

        self.log.info("Submit invalid block that invalidates the headers-only chain")
        n0.submitblock(invalid_block.serialize().hex())
        tips = n0.getchaintips()
        assert_equal(len(tips), 3)
        assert_equal(tips[0]['status'], 'invalid')


if __name__ == '__main__':
    GetChainTipsTest(__file__).main()
