#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test getblocktemplate complies with potential future timewarp-fixing
   softforks (modulo 600 second nTime decrease).

   We first mine a chain up to the difficulty-adjustment block and set
   the last block's timestamp 2 hours in the future, then check the times
   returned by getblocktemplate. Note that we cannot check the actual
   retarget behavior as difficulty adjustments do not occur on regtest
   (though it still technically has a difficulty adjustment interval)."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    connect_nodes_bi,
)


class TimewarpMiningTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]

        self.log.info('Create some old blocks')
        block_1 = node.generate(2014)[0]
        last_block_time = node.getblockheader(block_1)["time"] + 7199

        self.log.info('Create a block 7199 seconds in the future')
        node.setmocktime(last_block_time)
        block_2015 = node.generate(1)[0]
        assert_equal(node.getblockheader(block_2015)["time"], last_block_time)

        mining_info = node.getmininginfo()
        assert_equal(mining_info['blocks'], 2015)
        assert_equal(mining_info['currentblocktx'], 0)
        assert_equal(mining_info['currentblockweight'], 4000)

        self.restart_node(0)
        connect_nodes_bi(self.nodes, 0, 1)
        node = self.nodes[0]

        self.log.info('Check that mintime and curtime are last-block - 600 seconds')
        # Now test that mintime and curtime are last_block_time - 600
        template = node.getblocktemplate({'rules': ['segwit']})
        assert_equal(template["curtime"], last_block_time - 600)
        assert_equal(template["mintime"], last_block_time - 600)

        self.log.info('Generate a 2016th block and check that the next block\'s time goes back to now')
        block_2016 = node.generate(1)[0]
        assert_equal(node.getblockheader(block_2016)["time"], last_block_time - 600)

        # Assume test doesn't take 2 hours and check that we let time jump backwards
        # now that we mined the difficulty-adjustment block
        block_2017 = node.generate(1)[0]
        assert(node.getblockheader(block_2017)["time"] < last_block_time - 600)

if __name__ == '__main__':
    TimewarpMiningTest().main()
