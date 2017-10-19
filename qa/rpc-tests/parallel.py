#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2015-2016 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class ParallelTest (BitcoinTestFramework):
    def __init__(self):
      self.rep = False
      BitcoinTestFramework.__init__(self)

    def setup_chain(self):
        print ("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 6)

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-parallel=0", "-rpcservertimeout=0", "-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-parallel=0", "-rpcservertimeout=0", "-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-parallel=0", "-rpcservertimeout=0", "-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-parallel=0", "-rpcservertimeout=0", "-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        interconnect_nodes(self.nodes)
        self.is_network_split=False
        self.sync_all()

    def cleanup_and_reset(self):

        # Cleanup - start and connect the other nodes so that we have syncd chains before proceeding
        # to other tests.
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug=", "-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug=", "-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug=", "-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug=", "-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug=", "-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug=", "-pvtest=0", "-whitelist=127.0.0.1"]))
        interconnect_nodes(self.nodes)
        sync_blocks(self.nodes)

        print ("Mine more blocks on each node...")
        self.nodes[0].generate(25)
        sync_blocks(self.nodes)
        self.nodes[1].generate(25)
        sync_blocks(self.nodes)
        self.nodes[2].generate(25)
        sync_blocks(self.nodes)
        self.nodes[3].generate(25)
        sync_blocks(self.nodes)
        self.nodes[4].generate(25)
        sync_blocks(self.nodes)
        self.nodes[5].generate(25)
        sync_blocks(self.nodes)

        stop_nodes(self.nodes)
        wait_bitcoinds()

    def repetitiveTest(self):
        # get some coins
        self.nodeLookup = {}
        i = 0
        for n in self.nodes:
          print("Node %d is %s" % (i,n.url))
          print ("generating coins for node")  
          n.generate(200)
          self.sync_all()
          i += 1
          
        for i in range(0,200):
          # Create many utxo's
          print ("round %d: Generating txns..." % i)
          for n in self.nodes:  
            send_to = {}
            n.keypoolrefill(100)
            n.keypoolrefill(100)
            for i in range(200):
              send_to[n.getnewaddress()] = Decimal("0.01")
            n.sendmany("", send_to)

            self.sync_all()
          print ("  generating blocks...")
          i = 0
          for n in self.nodes:
            try:
                n.generate(1)
            except JSONRPCException as e:
                print (e)
                print ("Node ", i, " ", n.url)
                pdb.set_trace()
            i += 1
          print ("  syncing...")
          self.sync_all()
                    
    def run_test (self):
        if self.rep:
            self.repetitiveTest()
            return

        print ("Mining blocks with PV off...")
        # Mine some blocks on node2 which we will need at the end to generate a few transactions from that node
        # in order to create the small block with just a few transactions in it.
        self.nodes[2].generate(2)
        self.sync_all()

        # Mine the rest on node0 where we will generate the bigger block.
        self.nodes[0].generate(100)
        self.sync_all()

        self.nodes[0].generate(1)
        self.sync_all()

        self.nodes[2].generate(100)
        self.sync_all()


        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        #restart nodes with -pvtest off and do not yet connect the nodes
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))

        # Send tx's which do not propagate
        for i in range(50):
            self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), "0.01")

        # Send a few transactions from node2 that will get mined so that we will have at least
        # a few inputs to check when the two competing blocks enter parallel validation.
        for i in range(5):
            self.nodes[2].sendtoaddress(self.nodes[0].getnewaddress(), "0.01")
 

        # Have node0 and node2 mine the same block which will compete to advance the chaintip when
        # The nodes are connected back together.
        print ("Mine two competing blocks...")
        self.nodes[0].generate(1)
        self.nodes[2].generate(1)

        #stop nodes and restart right away
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # Restart nodes with pvtest=1
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=1"]))

        print ("Connect nodes...")
        interconnect_nodes(self.nodes)
        sync_blocks(self.nodes[0:3])

        # Wait here to make sure a re-org does not happen on node0 so we want to give it some time.  If the 
        # memory pool on node 0 does not change within 5 seconds then we assume a reorg is not occurring
        # because a reorg would cause transactions to be placed in the mempool from the old block on node 0.
        old_mempoolbytes = self.nodes[0].getmempoolinfo()["bytes"]
        for i in range(5):
            mempoolbytes = self.nodes[0].getmempoolinfo()["bytes"]
            if old_mempoolbytes != mempoolbytes:
                assert("Reorg happened when it should not - Mempoolbytes has changed")
            old_mempoolbytes = mempoolbytes
            # node0 has the bigger block and was sent and began processing first, however the block from node2
            # should have come in after and beaten node0's block.  Therefore the blockhash from chaintip from 
            # node2 should now match the blockhash from the chaintip on node1; and node0 and node1 should not match.
            print ("check for re-org " + str(i+1))
            assert_equal(self.nodes[1].getbestblockhash(), self.nodes[2].getbestblockhash())
            assert_not_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
            time.sleep(1)

        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # Restart nodes with pvtest off.
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))

        print ("Connect nodes...")
        interconnect_nodes(self.nodes)

        # mine a block on node3 and then connect to the others.  This tests when a third block arrives after
        # the tip has been advanced.
        # this block should propagate to the other nodes but not cause a re-org
        print ("Mine another block...")
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes[3].generate(1)
        connect_nodes(self.nodes[1],3)
        sync_blocks(self.nodes)

        # Wait here to make sure a re-org does not happen on node0 so we want to give it some time.  If the 
        # memory pool on node 0 does not change within 5 seconds then we assume a reorg is not occurring
        # because a reorg would cause transactions to be placed in the mempool from the old block on node 0.
        for i in range(5):
            mempoolbytes = self.nodes[0].getmempoolinfo()["bytes"]
            if old_mempoolbytes != mempoolbytes:
                assert("Reorg happened when it should not - Mempoolbytes has changed")
            old_mempoolbytes = mempoolbytes
            print ("check for re-org " + str(i+1))
            assert_equal(self.nodes[1].getbestblockhash(), self.nodes[2].getbestblockhash())
            assert_not_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
            assert_not_equal(self.nodes[1].getbestblockhash(), self.nodes[3].getbestblockhash())
            time.sleep(1)


        # Send some transactions and Mine a block on node 2.  
        # This should cause node0 and node3 to re-org and all chains should now match.
        for i in range(5):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), .01)
        print ("Mine another block on node2 which causes a reorg on node0 and node3...")
        self.nodes[2].generate(1)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[2].getbestblockhash())
        assert_equal(self.nodes[0].getbestblockhash(), self.nodes[1].getbestblockhash())
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [205,205,205,205])  

        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()


        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=0"]))
        connect_nodes(self.nodes[1],0)
        connect_nodes(self.nodes[1],2)
        connect_nodes(self.nodes[1],3)
        connect_nodes(self.nodes[1],4)
        connect_nodes(self.nodes[1],5)
        sync_blocks(self.nodes)

        # Mine blocks on each node and then mine 100 to age them such that they are spendable.
        print ("Mine more blocks on each node...")
        self.nodes[1].generate(5)
        sync_blocks(self.nodes)
        self.nodes[2].generate(5)
        sync_blocks(self.nodes)
        self.nodes[3].generate(5)
        sync_blocks(self.nodes)
        self.nodes[4].generate(5)
        sync_blocks(self.nodes)
        self.nodes[5].generate(5)
        sync_blocks(self.nodes)
        self.nodes[1].generate(100)
        sync_blocks(self.nodes)

        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()


        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=0"]))

        print ("Send more transactions...")
        num_range = 50
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.01)
        num_range = 10
        for i in range(num_range):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[3].sendtoaddress(self.nodes[3].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[4].sendtoaddress(self.nodes[4].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[5].sendtoaddress(self.nodes[5].getnewaddress(), 0.01)

        # Mine 5 competing blocks.
        print ("Mine 5 competing blocks...")
        self.nodes[0].generate(1)
        self.nodes[2].generate(1)
        self.nodes[3].generate(1)
        self.nodes[4].generate(1)
        self.nodes[5].generate(1)
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [331,330,331,331,331,331])  

        # Connect nodes so that all blocks are sent at same time to node1. Largest block from node0 will be terminated.
        print ("connnect nodes...")
        connect_nodes(self.nodes[1],0)
        connect_nodes(self.nodes[1],2)
        connect_nodes(self.nodes[1],3)
        connect_nodes(self.nodes[1],4)
        connect_nodes(self.nodes[1],5)
        sync_blocks(self.nodes)


        # Mine a block which will cause a reorg back to node0
        print ("Mine another block...")
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # Mine 5 more competing blocks of the same size. The last block to arrive will have its validation terminated.
        print ("Mine 5 more competing blocks...")
        self.nodes[0].generate(1)
        self.nodes[2].generate(1)
        self.nodes[3].generate(1)
        self.nodes[4].generate(1)
        self.nodes[5].generate(1)
        sync_blocks(self.nodes)

        # Mine another block which will cause the nodes to sync to one chain
        print ("Mine another block...")
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()
 
        # Cleanup by mining more blocks if we need to run extended tests
        if self.longTest == True:
            self.cleanup_and_reset()

        ################################################
        # Begin extended tests
        ################################################
        if self.longTest == False:
            return
 
        ###########################################################################################
        # Test reorgs
        ###########################################################################################

        ###########################################################################################
        # Basic reorg - see section below on 4 block attack scenarios.  At the end there is a
        # repeated test that does basic reorgs multiple times.


        ###########################################################################################
        # 1) Start a slow to validate block race then mine another block pulling one chain ahead.
        # - threads on the chain that is now not the most proof of work should be stopped and the
        #   most proof of work block should proceed.
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))

        print ("Send more transactions...")
        num_range = 15
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[1].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), 0.01)

        # Mine a block on each node
        print ("Mine a block on each node..")
        self.nodes[0].generate(1)
        self.nodes[1].generate(1)
        self.nodes[2].generate(1)
        basecount = self.nodes[0].getblockcount()

        # Mine another block on node2 so that it's chain will be the longest when we connect it
        print ("Mine another block on node2..")
        self.nodes[2].generate(1)
        bestblock = self.nodes[2].getbestblockhash()

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # Restart nodes with pvtest=1
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=1"]))

        # Connect node 0 and 1 so that a block validation race begins
        print ("Connect nodes0 and 1...")
        connect_nodes(self.nodes[1],0)

        # Wait for a little while before connecting node 2
        time.sleep(3)
        print ("Connect node2...")
        counts = [ x.getblockcount() for x in self.nodes ]
        print (str(counts))
        assert_equal(counts, [basecount,basecount,basecount+1])  
        interconnect_nodes(self.nodes)

        # All chains will sync to node2
        sync_blocks(self.nodes)
        assert_equal(self.nodes[0].getbestblockhash(), bestblock)
        assert_equal(self.nodes[1].getbestblockhash(), bestblock)
        assert_equal(self.nodes[2].getbestblockhash(), bestblock)

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # cleanup and sync chains for next tests
        self.cleanup_and_reset()


        ###########################################################################################
        # Mine two forks of equal work and start slow to validate block race on fork1. Then another
        # block arrives on fork2
        # - the slow to validate blocks will still continue
        # Mine another block on fork2 two pulling that fork ahead.
        # - threads on the fork1 should be stopped allowing fork2 to connect blocks and pull ahead

        print ("Mine two forks.")
        # fork 1 (both nodes on fork1 should be syncd)
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        interconnect_nodes(self.nodes)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # fork 2
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes[2].generate(1)

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # restart nodes but don't connect them yet
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))

        # Create txns on node0 and 1 to setup for a slow to validate race between those nodes.
        print ("Send more transactions...")
        num_range = 15
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[1].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)

        # Mine a block on each node
        print ("Mine a block on each node..")
        self.nodes[0].generate(1)
        self.nodes[1].generate(1)
        self.nodes[2].generate(1)
        basecount = self.nodes[0].getblockcount()

        # Mine another block on node2 so that it's chain will be the longest when we connect it
        print ("Mine another block on node2..")
        self.nodes[2].generate(1)
        bestblock = self.nodes[2].getbestblockhash()

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # Restart nodes with pvtest=1
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))

        # Connect node 0 and 1 so that a block validation race begins
        print ("Connect nodes0 and 1...")
        connect_nodes(self.nodes[1],0)

        # Wait for a little while before connecting node 2
        time.sleep(3)
        print ("Connect node2...")
        counts = [ x.getblockcount() for x in self.nodes ]
        print (str(counts))
        assert_equal(counts, [basecount,basecount,basecount+1])  
        interconnect_nodes(self.nodes)

        # All chains will sync to node2
        sync_blocks(self.nodes)
        assert_equal(self.nodes[0].getbestblockhash(), bestblock)
        assert_equal(self.nodes[1].getbestblockhash(), bestblock)
        assert_equal(self.nodes[2].getbestblockhash(), bestblock)

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # cleanup and sync chains for next tests
        self.cleanup_and_reset()


        ##############################################################################################
        # Mine two forks of equal work and start slow to validate 4 block race on fork1. Then another
        # block arrives on fork2
        # - the slow to validate blocks will still continue
        # Mine another block on fork2 two pulling that fork ahead.
        # - threads on the fork1 should be stopped allowing fork2 to connect blocks and pull ahead

        print ("Mine two forks.")
        # fork 1 (both nodes on fork1 should be syncd)
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        interconnect_nodes(self.nodes)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # fork 2
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes[4].generate(1)

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # restart nodes but don't connect them yet
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))

        # Create txns on node0 and 1 to setup for a slow to validate race between those nodes.
        print ("Send more transactions...")
        num_range = 15
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[1].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[3].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[4].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)

        # Mine a block on each node
        print ("Mine a block on each node..")
        self.nodes[0].generate(1)
        self.nodes[1].generate(1)
        self.nodes[2].generate(1)
        self.nodes[3].generate(1)
        self.nodes[4].generate(1)
        basecount = self.nodes[0].getblockcount()

        # Mine another block on node4 so that it's chain will be the longest when we connect it
        print ("Mine another block on node4..")
        self.nodes[4].generate(1)
        bestblock = self.nodes[4].getbestblockhash()

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # Restart nodes with pvtest=1
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))

        # Connect node 0 and 1 so that a block validation race begins
        print ("Connect nodes0, 1, 2 and 3...")
        connect_nodes(self.nodes[1],0)

        # Wait for a little while before connecting node 4
        time.sleep(3)
        print ("Connect node4...")
        counts = [ x.getblockcount() for x in self.nodes ]
        print (str(counts))
        assert_equal(counts, [basecount,basecount,basecount, basecount, basecount+1])  
        interconnect_nodes(self.nodes)

        # All chains will sync to node2
        sync_blocks(self.nodes)
        assert_equal(self.nodes[0].getbestblockhash(), bestblock)
        assert_equal(self.nodes[1].getbestblockhash(), bestblock)
        assert_equal(self.nodes[2].getbestblockhash(), bestblock)
        assert_equal(self.nodes[3].getbestblockhash(), bestblock)
        assert_equal(self.nodes[4].getbestblockhash(), bestblock)

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # cleanup and sync chains for next tests
        self.cleanup_and_reset()


        ###########################################################################################
        # 1) Mine two forks of equal work and start slow to validate block race on fork1. Then another
        # block arrives on fork2 pulling that fork ahead.
        # - threads on the fork1 should be stopped allowing fork2 to connect blocks and pull ahead
        # 2) As fork2 is being validated, fork 1 pulls ahead
        # - fork 2 is now stopped and fork 1 begins to validate
        # 3) do step 2 repeatedely, going back and forth between forks

        print ("Mine three forks.")
        # fork 1 (both nodes on fork1 should be syncd)
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        interconnect_nodes(self.nodes)
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # fork 2
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes[2].generate(1)

        # fork 3
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes[3].generate(1)

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # restart nodes but don't connect them yet
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0", "-whitelist=127.0.0.1"]))

        # Create txns on node0 and 1 to setup for a slow to validate race between those nodes.
        print ("Send more transactions...")
        num_range = 15
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[1].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)

        # in this test we also generate txns on node 2 so that all nodes will validate slowly.
        for i in range(num_range):
            self.nodes[2].sendtoaddress(self.nodes[1].getnewaddress(), 0.01)

        # Mine a block on each node
        print ("Mine a block on each node..")
        self.nodes[0].generate(1)
        self.nodes[1].generate(1)
        self.nodes[2].generate(1)
        self.nodes[3].generate(1)
        basecount = self.nodes[0].getblockcount()

        # Mine another block on node2 so that it's chain will be the longest when we connect it
        print ("Mine another block on node2..")
        self.nodes[2].generate(1)

        # Mine two blocks on node3 so that it's chain will be the longest when we connect it
        print ("Mine 2 blocks on node3..")
        self.nodes[3].generate(1)
        self.nodes[3].generate(1)
        bestblock = self.nodes[3].getbestblockhash()

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # Restart nodes with pvtest=1
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=1", "-whitelist=127.0.0.1"]))

        # Connect node 0 and 1 so that a block validation race begins
        print ("Connect nodes 0 and 1...")
        connect_nodes(self.nodes[1],0)

        # Wait for a little while before connecting node 2 (fork2)
        time.sleep(3)
        print ("Connect node2 - fork2...")
        counts = [ x.getblockcount() for x in self.nodes ]
        print (str(counts))
        assert_equal(counts, [basecount,basecount,basecount+1])  
        interconnect_nodes(self.nodes)

        # Wait for a little while before connecting node 3 (fork3)
        time.sleep(3)
        print ("Connect node3 - fork3...")
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug=","-pvtest=1", "-whitelist=127.0.0.1"]))
        counts = [ x.getblockcount() for x in self.nodes ]
        interconnect_nodes(self.nodes)
        print (str(counts))
        assert_equal(counts, [basecount-1,basecount-1,basecount+1, basecount+2])  
        interconnect_nodes(self.nodes)

        sync_blocks(self.nodes)
        assert_equal(self.nodes[0].getbestblockhash(), bestblock)
        assert_equal(self.nodes[1].getbestblockhash(), bestblock)
        assert_equal(self.nodes[2].getbestblockhash(), bestblock)
        assert_equal(self.nodes[3].getbestblockhash(), bestblock)

        stop_nodes(self.nodes)
        wait_bitcoinds()

        # cleanup and sync chains for next tests
        self.cleanup_and_reset()

        
        ###########################################################################################
        # 1) Large reorg - can we do a 144 block reorg?
        print ("Starting repeating many competing blocks test")
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug=","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug=","-pvtest=0"]))

        print ("Mine 144 blocks on each chain...")
        self.nodes[0].generate(144)
        self.nodes[1].generate(144)
 
        print ("Connect nodes for larg reorg...")
        connect_nodes(self.nodes[1],0)
        sync_blocks(self.nodes)

        print ("Mine another block on node5 causing large reorg...")
        self.nodes[1].generate(1)
        sync_blocks(self.nodes)

        # Mine another block which will cause some nodes to reorg and sync to the same chain.
        print ("Mine another block on node0...")
        self.nodes[0].generate(1)
        sync_blocks(self.nodes)

        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # cleanup and sync chains for next tests
        self.cleanup_and_reset()
 
        ###########################################################################################
        # Test the 4 block attack scenarios - use -pvtest=true to slow down the checking of inputs.
        ###########################################################################################

        ####################################################################
        # Mine 4 blocks of all different sizes
        # - the smallest block should win
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))

        print ("Send more transactions...")
        num_range = 15
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.01)
        num_range = 14
        for i in range(num_range):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), 0.01)
        num_range = 13
        for i in range(num_range):
            self.nodes[3].sendtoaddress(self.nodes[3].getnewaddress(), 0.01)
        num_range = 2
        for i in range(num_range):
            self.nodes[4].sendtoaddress(self.nodes[4].getnewaddress(), 0.01)

        # Mine 4 competing blocks.
        print ("Mine 4 competing blocks...")
        self.nodes[0].generate(1)
        self.nodes[2].generate(1)
        self.nodes[3].generate(1)
        self.nodes[4].generate(1)

        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # start nodes with -pvtest set to true.
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=1"]))

        # Connect nodes so that all blocks are sent at same time to node1.
        connect_nodes(self.nodes[1],0)
        connect_nodes(self.nodes[1],2)
        connect_nodes(self.nodes[1],3)
        connect_nodes(self.nodes[1],4)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[4].getbestblockhash())
 
        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=0"]))

        connect_nodes(self.nodes[1],0)
        connect_nodes(self.nodes[1],2)
        connect_nodes(self.nodes[1],3)
        connect_nodes(self.nodes[1],4)
        connect_nodes(self.nodes[1],5)
        sync_blocks(self.nodes)

        # Mine a block which will cause all nodes to update their chains
        print ("Mine another block...")
        self.nodes[1].generate(1)
        time.sleep(2) #wait for blocks to propagate
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[2].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[3].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[4].getbestblockhash())
 
        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # cleanup and sync chains for next tests
        self.cleanup_and_reset()

        ########################################################################################################
        # Mine 4 blocks all the same size and get them to start validating and then send a 5th that is smaller
        # - the last smallest and last block arriving should win.
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=0"]))

        print ("Send more transactions...")
        num_range = 15
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.01)
        num_range = 15
        for i in range(num_range):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), 0.01)
        num_range = 15
        for i in range(num_range):
            self.nodes[3].sendtoaddress(self.nodes[3].getnewaddress(), 0.01)
        num_range = 15
        for i in range(num_range):
            self.nodes[4].sendtoaddress(self.nodes[4].getnewaddress(), 0.01)
        num_range = 2
        for i in range(num_range):
            self.nodes[5].sendtoaddress(self.nodes[5].getnewaddress(), 0.01)

        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # start nodes with -pvtest set to true.
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=1"]))

        # Connect nodes so that first 4 blocks are sent at same time to node1.
        connect_nodes(self.nodes[1],0)
        connect_nodes(self.nodes[1],2)
        connect_nodes(self.nodes[1],3)
        connect_nodes(self.nodes[1],4)
        time.sleep(5) #wait for blocks to start processing
        
        # Connect 5th block and this one should win the race
        connect_nodes(self.nodes[1],5)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[5].getbestblockhash())
 
        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=0"]))

        connect_nodes(self.nodes[1],0)
        connect_nodes(self.nodes[1],2)
        connect_nodes(self.nodes[1],3)
        connect_nodes(self.nodes[1],4)
        connect_nodes(self.nodes[1],5)

        # Mine a block which will cause all nodes to update their chains
        print ("Mine another block...")
        self.nodes[1].generate(1)
        time.sleep(2) #wait for blocks to propagate
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[2].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[3].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[4].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[5].getbestblockhash())
 
        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # cleanup and sync chains for next tests
        self.cleanup_and_reset()

        ############################################################################################################
        # Mine 4 blocks all the same size and get them to start validating and then send a 5th that is the same size
        # - the first block arriving should win
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=0"]))

        print ("Send more transactions...")
        num_range = 10
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.01)
        num_range = 10
        for i in range(num_range):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), 0.01)
        num_range = 10
        for i in range(num_range):
            self.nodes[3].sendtoaddress(self.nodes[3].getnewaddress(), 0.01)
        num_range = 10
        for i in range(num_range):
            self.nodes[4].sendtoaddress(self.nodes[4].getnewaddress(), 0.01)
        num_range = 10
        for i in range(num_range):
            self.nodes[5].sendtoaddress(self.nodes[5].getnewaddress(), 0.01)

        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # start nodes with -pvtest set to true.
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=1"]))

        # Connect nodes so that first 4 blocks are sent 1 second apart to node1.
        connect_nodes(self.nodes[1],0)
        time.sleep(1)
        connect_nodes(self.nodes[1],2)
        time.sleep(1)
        connect_nodes(self.nodes[1],3)
        time.sleep(1)
        connect_nodes(self.nodes[1],4)
        time.sleep(1) #wait for blocks to start processing
        
        # Connect 5th block and this one be terminated and the first block to connect from node0 should win the race
        connect_nodes(self.nodes[1],5)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
 
        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=0"]))

        connect_nodes(self.nodes[1],0)
        connect_nodes(self.nodes[1],2)
        connect_nodes(self.nodes[1],3)
        connect_nodes(self.nodes[1],4)
        connect_nodes(self.nodes[1],5)

        # Mine a block which will cause all nodes to update their chains
        print ("Mine another block...")
        self.nodes[1].generate(1)
        time.sleep(2) #wait for blocks to propagate
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[2].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[3].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[4].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[5].getbestblockhash())
 
        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        #########################################################################################################
        # Mine 4 blocks all the same size and get them to start validating and then send a 5th that is bigger
        # - the first block arriving should win
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=0"]))

        print ("Send more transactions...")
        num_range = 10
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.01)
        num_range = 10
        for i in range(num_range):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), 0.01)
        num_range = 10
        for i in range(num_range):
            self.nodes[3].sendtoaddress(self.nodes[3].getnewaddress(), 0.01)
        num_range = 10
        for i in range(num_range):
            self.nodes[4].sendtoaddress(self.nodes[4].getnewaddress(), 0.01)
        num_range = 20
        for i in range(num_range):
            self.nodes[5].sendtoaddress(self.nodes[5].getnewaddress(), 0.01)

        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # start nodes with -pvtest set to true.
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=1"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=1"]))

        # Connect nodes so that first 4 blocks are sent 1 second apart to node1.
        connect_nodes(self.nodes[1],0)
        time.sleep(1)
        connect_nodes(self.nodes[1],2)
        time.sleep(1)
        connect_nodes(self.nodes[1],3)
        time.sleep(1)
        connect_nodes(self.nodes[1],4)
        time.sleep(1) #wait for blocks to start processing
        
        # Connect 5th block and this one be terminated and the first block to connect from node0 should win the race
        connect_nodes(self.nodes[1],5)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
 
        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=0"]))

        connect_nodes(self.nodes[1],0)
        connect_nodes(self.nodes[1],2)
        connect_nodes(self.nodes[1],3)
        connect_nodes(self.nodes[1],4)
        connect_nodes(self.nodes[1],5)

        # Mine a block which will cause all nodes to update their chains
        print ("Mine another block...")
        self.nodes[1].generate(1)
        time.sleep(2) #wait for blocks to propagate
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[2].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[3].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[4].getbestblockhash())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[5].getbestblockhash())
 
        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # cleanup and sync chains for next tests
        self.cleanup_and_reset()


        #################################################################################
        # Repeated 5 blocks mined with a reorg after
        #################################################################################
        
        # Repeatedly mine 5 blocks at a time on each node to have many blocks both arriving
        # at the same time and racing each other to see which can extend the chain the fastest.
        # This is intented just a stress test of the 4 block scenario but also while blocks
        # are in the process of being both mined and with reorgs sometimes happening at the same time.
        print ("Starting repeating many competing blocks test")
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-debug","-pvtest=0"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-debug","-pvtest=0"]))

        connect_nodes(self.nodes[1],0)
        connect_nodes(self.nodes[1],2)
        connect_nodes(self.nodes[1],3)
        connect_nodes(self.nodes[1],4)
        connect_nodes(self.nodes[1],5)
        sync_blocks(self.nodes)

        for i in range(100):

            print ("Mine many more competing blocks...")
            self.nodes[0].generate(1)
            self.nodes[2].generate(1)
            self.nodes[3].generate(1)
            self.nodes[4].generate(1)
            self.nodes[5].generate(1)
            sync_blocks(self.nodes)

            # Mine another block which will cause some nodes to reorg and sync to the same chain.
            print ("Mine another block...")
            self.nodes[0].generate(1)
            sync_blocks(self.nodes)

        # stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        # cleanup and sync chains for next tests
        self.cleanup_and_reset()

def Test():
    t = ParallelTest()
    t.rep = True
    t.main(["--tmpdir=/ramdisk/test", "--nocleanup","--noshutdown"])

if __name__ == '__main__':  

    p = ParallelTest()  
    if "--rep" in sys.argv:
        print("Repetitive test")
        p.rep = True
        sys.argv.remove("--rep")
    else:
        p.rep = False

    if "--extensive" in sys.argv:
      p.longTest = True
      # we must remove duplicate 'extensive' arg here
      while True:
          try:
              sys.argv.remove('--extensive')
          except:
              break
      print ("Running extensive tests")
    else:
      p.longTest = False


    p.main ()
    
