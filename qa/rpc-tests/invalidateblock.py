#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2015-2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test InvalidateBlock code
#

import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *


def retryWhile(fn, exc, excStr=None):
    while 1:
        try:
            fn()
            break
        except exc as e:
            if (not excStr is None):
                if not excStr in str(e):
                    raise
        time.sleep(.5)


class InvalidateTest(BitcoinTestFramework):


    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 4)

    def setup_network(self):
        self.nodes = []
        self.is_network_split = False 
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug"]))

    def run_test(self):
        print("Make sure we repopulate setBlockIndexCandidates after InvalidateBlock:")
        print("Mine 4 blocks on Node 0")
        self.nodes[0].generate(4)
        assert(self.nodes[0].getblockcount() == 4)
        besthash = self.nodes[0].getbestblockhash()

        print("Mine competing 6 blocks on Node 1")
        self.nodes[1].generate(6)
        assert(self.nodes[1].getblockcount() == 6)

        print("Connect nodes to force a reorg")
        connect_nodes_bi(self.nodes,0,1)
        sync_blocks(self.nodes[0:2])
        assert(self.nodes[0].getblockcount() == 6)
        badhash = self.nodes[1].getblockhash(2)

        print("Invalidate block 2 on node 0 and verify we reorg to node 0's original chain")
        self.nodes[0].invalidateblock(badhash)
        newheight = self.nodes[0].getblockcount()
        newhash = self.nodes[0].getbestblockhash()
        if (newheight != 4 or newhash != besthash):
            raise AssertionError("Wrong tip for node0, hash %s, height %d"%(newhash,newheight))

        print("\nMake sure we won't reorg to a lower work chain:")
        connect_nodes_bi(self.nodes,1,2)
        print("Sync node 2 to node 1 so both have 6 blocks")
        sync_blocks(self.nodes[1:3])
        assert(self.nodes[2].getblockcount() == 6)
        print("Invalidate block 5 on node 1 so its tip is now at 4")
        self.nodes[1].invalidateblock(self.nodes[1].getblockhash(5))
        assert(self.nodes[1].getblockcount() == 4)
        print("Invalidate block 3 on node 2, so its tip is now 2")
        self.nodes[2].invalidateblock(self.nodes[2].getblockhash(3))
        assert(self.nodes[2].getblockcount() == 2)
        print("..and then mine a block")
        self.nodes[2].generate(1)
        print("Verify all nodes are at the right height")
        time.sleep(5)
        for i in range(3):
            print(i,self.nodes[i].getblockcount())
        assert(self.nodes[2].getblockcount() == 3)
        assert(self.nodes[0].getblockcount() == 4)
        node1height = self.nodes[1].getblockcount()
        if node1height < 4:
            raise AssertionError("Node 1 reorged to a lower height: %d"%node1height)

        self.testChainSyncWithLongerInvalid()


    def testChainSyncWithLongerInvalid(self):
        print("verify that IBD continues on a separate chain after a block is invalidated")

        ret = self.nodes[0].generate(50)
        # after the headers propagate, invalidate the block
        retryWhile(lambda: self.nodes[1].invalidateblock(ret[0]), JSONRPCException, "Block not found")
        # now generate a competing chain
        ret1 = self.nodes[1].generate(25)

        # now start up a new node to sync with one of the chains
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug"]))
        connect_nodes_bi(self.nodes,0,3)
        connect_nodes_bi(self.nodes,1,3)
        # invalidate the longest chain
        self.nodes[3].invalidateblock(ret[0])
        # give it time to sync with the shorter chain on node 1
        print("allowing node 3 to sync")
        time.sleep(5)
        blocks1 = self.nodes[1].getblockcount()
        nblocks = self.nodes[3].getblockcount()
        # test if it is synced
        if nblocks != blocks1:
            print("ERROR: node 3 did not sync with longest valid chain")
            print("chain tips on 0: %s" % str(self.nodes[0].getchaintips()))
            print("chain tips on 1: %s" % str(self.nodes[1].getchaintips()))
            print("chain tips on 3: %s" % str(self.nodes[3].getchaintips()))
            print("longest chain on 3: %s" % str(self.nodes[3].getblockcount()))
        # enable when fixed: assert(nblocks == blocks1);  # since I invalidated a block on 0's chain, I should be caught up with 1

        print("Now make the other chain (with no invalid blocks) longer")
        ret1 = self.nodes[1].generate(50)
        time.sleep(5)
        blocks1 = self.nodes[1].getblockcount()
        nblocks = self.nodes[3].getblockcount()
        # test if it is synced
        if nblocks != blocks1:
            print("node 3 did not sync up")
            print("chain tips on 0: %s" % str(self.nodes[0].getchaintips()))
            print("chain tips on 1: %s" % str(self.nodes[1].getchaintips()))
            print("chain tips on 3: %s" % str(self.nodes[3].getchaintips()))
            print("longest chain on 3: %s" % str(self.nodes[3].getblockcount()))
        else:
            print("node 1 synced with longest chain")


       
if __name__ == '__main__':
    InvalidateTest().main()

def Test():
    t = InvalidateTest()
    bitcoinConf = {
    "debug":["net","blk","thin","mempool","req","bench","evict"], # "lck"
    "blockprioritysize":2000000  # we don't want any transactions rejected due to insufficient fees...
     }
    t.main(["--nocleanup","--noshutdown", "--tmpdir=/ramdisk/test"],bitcoinConf,None)
