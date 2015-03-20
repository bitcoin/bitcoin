#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test InvalidateBlock code
#

from test_framework import BitcoinTestFramework
from bitcoinrpc.authproxy import AuthServiceProxy, JSONRPCException
from util import *

class InvalidateTest(BitcoinTestFramework):
    
        
    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 2)
                 
    def setup_network(self):
        self.nodes = []
        self.is_network_split = False 
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug"]))
        
    def run_test(self):
        print "Mine 4 blocks on Node 0"
        self.nodes[0].setgenerate(True, 4)
        assert(self.nodes[0].getblockcount() == 4)
        besthash = self.nodes[0].getbestblockhash()

        print "Mine competing 6 blocks on Node 1"
        self.nodes[1].setgenerate(True, 6)
        assert(self.nodes[1].getblockcount() == 6)

        print "Connect nodes to force a reorg"
        connect_nodes_bi(self.nodes,0,1)
        sync_blocks(self.nodes)
        assert(self.nodes[0].getblockcount() == 6)
        badhash = self.nodes[1].getblockhash(2)

        print "Invalidate block 2 on node 0 and verify we reorg to node 0's original chain"
        self.nodes[0].invalidateblock(badhash)
        newheight = self.nodes[0].getblockcount()
        newhash = self.nodes[0].getbestblockhash()
        if (newheight != 4 or newhash != besthash):
            raise AssertionError("Wrong tip for node0, hash %s, height %d"%(newhash,newheight))

if __name__ == '__main__':
    InvalidateTest().main()
