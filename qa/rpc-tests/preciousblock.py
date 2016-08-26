#!/usr/bin/env python3
# Copyright (c) 2015 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test PreciousBlock code
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

def unidirectional_node_sync_via_rpc(node_src, node_dest):
    blocks_to_copy = []
    blockhash = node_src.getbestblockhash()
    while True:
        try:
            assert(len(node_dest.getblock(blockhash, False)) > 0)
            break
        except:
            blocks_to_copy.append(blockhash)
            blockhash = node_src.getblockheader(blockhash, True)['previousblockhash']
    blocks_to_copy.reverse()
    for blockhash in blocks_to_copy:
        blockdata = node_src.getblock(blockhash, False)
        assert(node_dest.submitblock(blockdata) in (None, 'inconclusive'))

def node_sync_via_rpc(nodes):
    for node_src in nodes:
        for node_dest in nodes:
            if node_src is node_dest:
                continue
            unidirectional_node_sync_via_rpc(node_src, node_dest)

class PreciousTest(BitcoinTestFramework):
    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 3)

    def setup_network(self):
        self.nodes = []
        self.is_network_split = False
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug"]))

    def run_test(self):
        print("Ensure submitblock can in principle reorg to a competing chain")
        self.nodes[0].generate(1)
        assert(self.nodes[0].getblockcount() == 1)
        (hashY, hashZ) = self.nodes[1].generate(2)
        assert(self.nodes[1].getblockcount() == 2)
        node_sync_via_rpc(self.nodes[0:3])
        assert(self.nodes[0].getbestblockhash() == hashZ)

        print("Mine blocks A-B-C on Node 0")
        (hashA, hashB, hashC) = self.nodes[0].generate(3)
        assert(self.nodes[0].getblockcount() == 5)
        print("Mine competing blocks E-F-G on Node 1")
        (hashE, hashF, hashG) = self.nodes[1].generate(3)
        assert(self.nodes[1].getblockcount() == 5)
        assert(hashC != hashG)
        print("Connect nodes and check no reorg occurs")
        # Submit competing blocks via RPC so any reorg should occur before we proceed (no way to wait on inaction for p2p sync)
        node_sync_via_rpc(self.nodes[0:2])
        connect_nodes_bi(self.nodes,0,1)
        assert(self.nodes[0].getbestblockhash() == hashC)
        assert(self.nodes[1].getbestblockhash() == hashG)
        print("Make Node0 prefer block G")
        self.nodes[0].preciousblock(hashG)
        assert(self.nodes[0].getbestblockhash() == hashG)
        print("Make Node0 prefer block C again")
        self.nodes[0].preciousblock(hashC)
        assert(self.nodes[0].getbestblockhash() == hashC)
        print("Make Node1 prefer block C")
        self.nodes[1].preciousblock(hashC)
        sync_chain(self.nodes[0:2]) # wait because node 1 may not have downloaded hashC
        assert(self.nodes[1].getbestblockhash() == hashC)
        print("Make Node1 prefer block G again")
        self.nodes[1].preciousblock(hashG)
        assert(self.nodes[1].getbestblockhash() == hashG)
        print("Make Node0 prefer block G again")
        self.nodes[0].preciousblock(hashG)
        assert(self.nodes[0].getbestblockhash() == hashG)
        print("Make Node1 prefer block C again")
        self.nodes[1].preciousblock(hashC)
        assert(self.nodes[1].getbestblockhash() == hashC)
        print("Mine another block (E-F-G-)H on Node 0 and reorg Node 1")
        self.nodes[0].generate(1)
        assert(self.nodes[0].getblockcount() == 6)
        sync_blocks(self.nodes[0:2])
        hashH = self.nodes[0].getbestblockhash()
        assert(self.nodes[1].getbestblockhash() == hashH)
        print("Node1 should not be able to prefer block C anymore")
        self.nodes[1].preciousblock(hashC)
        assert(self.nodes[1].getbestblockhash() == hashH)
        print("Mine competing blocks I-J-K-L on Node 2")
        self.nodes[2].generate(4)
        assert(self.nodes[2].getblockcount() == 6)
        hashL = self.nodes[2].getbestblockhash()
        print("Connect nodes and check no reorg occurs")
        node_sync_via_rpc(self.nodes[0:3])
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        assert(self.nodes[0].getbestblockhash() == hashH)
        assert(self.nodes[1].getbestblockhash() == hashH)
        assert(self.nodes[2].getbestblockhash() == hashL)
        print("Make Node1 prefer block L")
        self.nodes[1].preciousblock(hashL)
        assert(self.nodes[1].getbestblockhash() == hashL)
        print("Make Node2 prefer block H")
        self.nodes[2].preciousblock(hashH)
        assert(self.nodes[2].getbestblockhash() == hashH)

if __name__ == '__main__':
    PreciousTest().main()
