#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Exercise various compression levels
# Test sending blocks where one node has compression turned on and the other is off
# 
# Do the following:
#   a) creates 2 nodes with different compression levels and connect
#   b) node0 mines a block
#   c) node1 receives block

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class CompressionTest (BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 4)

    def setup_network(self,split=False):
        # Start a node with compressionlevel set
        self.nodes = []
        self.is_network_split=False

    def run_test (self):
        #sync compressionlevel:default to compressionlevel:1
        # Mine a block, sync nodes, send a transaction, sync nodes
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug", "-compressionlevel=1"]))
        #mining 101 blocks will cause blockblocks to be created as well as give us some coin to spend
        self.nodes[0].generate(101)
        connect_nodes(self.nodes[0],1)
        self.sync_all()
        #create many tx's all at once so that some cxblobs will get created
        for count in range(20):
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.05)
        self.nodes[0].generate(1)
        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), 1)
        stop_nodes(self.nodes)
        wait_bitcoinds()

        #sync compressionlevel:1 to compressionlevel:2
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-compressionlevel=1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug", "-compressionlevel=2"]))
        self.nodes[0].generate(1)
        connect_nodes(self.nodes[0],1)
        self.sync_all()
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        #self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), 2)
        stop_nodes(self.nodes)
        wait_bitcoinds()

        #sync compressionlevel:2 to compressionlevel:1
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-compressionlevel=2"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug", "-compressionlevel=1"]))
        connect_nodes(self.nodes[0],1)
        self.nodes[0].generate(1)
        connect_nodes(self.nodes[0],1)
        self.sync_all()
        #self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        #assert_equal(self.nodes[1].getbalance(), 3)
        stop_nodes(self.nodes)
        wait_bitcoinds()

        #sync compressionlevel:0 to compressionlevel:1
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-compressionlevel=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug", "-compressionlevel=1"]))
        connect_nodes(self.nodes[0],1)
        self.nodes[0].generate(1)
        connect_nodes(self.nodes[0],1)
        self.sync_all()
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        #self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), 3)
        #assert_equal(self.nodes[1].getbalance(), 4)
        stop_nodes(self.nodes)
        wait_bitcoinds()

        #sync compressionlevel:2 to compressionlevel:0
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-compressionlevel=2"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug", "-compressionlevel=0"]))
        self.nodes[0].generate(1)
        connect_nodes(self.nodes[0],1)
        self.sync_all()
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        #self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        assert_equal(self.nodes[1].getbalance(), 4)
        #assert_equal(self.nodes[1].getbalance(), 5)
        stop_nodes(self.nodes)
        wait_bitcoinds()

        #sync compressionlevel:0 to compressionlevel:0
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug", "-compressionlevel=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug", "-compressionlevel=0"]))
        self.nodes[0].generate(1)
        connect_nodes(self.nodes[0],1)
        self.sync_all()
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        #self.sync_all()
        self.nodes[0].generate(1)
        self.sync_all()
        #assert_equal(self.nodes[1].getbalance(), 6)
        assert_equal(self.nodes[1].getbalance(), 5)
        stop_nodes(self.nodes)
        wait_bitcoinds()

if __name__ == '__main__':
    CompressionTest ().main ()
