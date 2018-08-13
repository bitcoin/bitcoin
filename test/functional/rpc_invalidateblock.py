#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the invalidateblock RPC."""

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, connect_nodes, connect_nodes_bi, wait_until

class InvalidateTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        self.log.info("Make sure we repopulate setBlockIndexCandidates after InvalidateBlock:")
        self.log.info("Mine 4 blocks on Node 0")
        self.nodes[0].generate(4)
        assert(self.nodes[0].getblockcount() == 4)
        besthash = self.nodes[0].getbestblockhash()

        self.log.info("Mine competing 6 blocks on Node 1")
        self.nodes[1].generate(6)
        assert(self.nodes[1].getblockcount() == 6)

        self.log.info("Connect nodes to force a reorg")
        connect_nodes_bi(self.nodes,0,1)
        self.sync_blocks(self.nodes[0:2])
        assert(self.nodes[0].getblockcount() == 6)
        badhash = self.nodes[1].getblockhash(2)

        self.log.info("Invalidate block 2 on node 0 and verify we reorg to node 0's original chain")
        self.nodes[0].invalidateblock(badhash)
        newheight = self.nodes[0].getblockcount()
        newhash = self.nodes[0].getbestblockhash()
        if (newheight != 4 or newhash != besthash):
            raise AssertionError("Wrong tip for node0, hash %s, height %d"%(newhash,newheight))

        self.log.info("Make sure we won't reorg to a lower work chain:")
        connect_nodes_bi(self.nodes,1,2)
        self.log.info("Sync node 2 to node 1 so both have 6 blocks")
        self.sync_blocks(self.nodes[1:3])
        assert(self.nodes[2].getblockcount() == 6)
        self.log.info("Invalidate block 5 on node 1 so its tip is now at 4")
        self.nodes[1].invalidateblock(self.nodes[1].getblockhash(5))
        assert(self.nodes[1].getblockcount() == 4)
        self.log.info("Invalidate block 3 on node 2, so its tip is now 2")
        self.nodes[2].invalidateblock(self.nodes[2].getblockhash(3))
        assert(self.nodes[2].getblockcount() == 2)
        self.log.info("..and then mine a block")
        self.nodes[2].generate(1)
        self.log.info("Verify all nodes are at the right height")
        time.sleep(5)
        assert_equal(self.nodes[2].getblockcount(), 3)
        assert_equal(self.nodes[0].getblockcount(), 4)
        node1height = self.nodes[1].getblockcount()
        if node1height < 4:
            raise AssertionError("Node 1 reorged to a lower height: %d"%node1height)

        self.log.info("Make sure ResetBlockFailureFlags does the job correctly")
        self.restart_node(0, extra_args=["-checkblocks=5"])
        self.restart_node(1, extra_args=["-checkblocks=5"])
        connect_nodes_bi(self.nodes, 0, 1)
        self.nodes[0].generate(10)
        self.sync_blocks(self.nodes[0:2])
        newheight = self.nodes[0].getblockcount()
        for j in range(2):
            self.restart_node(0, extra_args=["-checkblocks=5"])
            tip = self.nodes[0].generate(10)[-1]
            self.nodes[1].generate(9)
            connect_nodes(self.nodes[0], 1)
            self.sync_blocks(self.nodes[0:2])
            assert_equal(self.nodes[0].getblockcount(), newheight + 10 * (j + 1))
            assert_equal(self.nodes[1].getblockcount(), newheight + 10 * (j + 1))
            assert_equal(self.nodes[1].getbestblockhash(), tip)

        tip = self.nodes[1].getbestblockhash()
        self.nodes[1].invalidateblock(self.nodes[1].getblockhash(newheight + 1))
        self.nodes[1].invalidateblock(self.nodes[1].getblockhash(newheight + 1))

        assert_equal(self.nodes[1].getblockcount(), newheight)
        self.restart_node(1, extra_args=["-checkblocks=5"])
        wait_until(lambda: self.nodes[1].getblockcount() == newheight + 20)
        assert_equal(tip, self.nodes[1].getbestblockhash())


if __name__ == '__main__':
    InvalidateTest().main()
