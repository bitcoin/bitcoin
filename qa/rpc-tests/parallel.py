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
        self.nodes[2].generate(1)
        self.sync_all()

        # Mine the rest on node0 where we will generate the bigger block.
        self.nodes[0].generate(100)
        self.sync_all()

        self.nodes[0].generate(100)
        self.sync_all()

        # Create many utxo's
        print ("Generating txns...")
        send_to = {}
        self.nodes[0].keypoolrefill(100)
        self.nodes[0].keypoolrefill(100)
        for i in range(200):
            send_to[self.nodes[0].getnewaddress()] = Decimal("0.01")
        self.nodes[0].sendmany("", send_to)
        self.nodes[0].keypoolrefill(100)
        self.nodes[0].keypoolrefill(100)
        for i in range(200):
            send_to[self.nodes[0].getnewaddress()] = Decimal("0.01")
        self.nodes[0].sendmany("", send_to)
        self.nodes[0].keypoolrefill(100)
        self.nodes[0].keypoolrefill(100)
        for i in range(200):
            send_to[self.nodes[0].getnewaddress()] = Decimal("0.01")
        self.nodes[0].sendmany("", send_to)
        self.nodes[0].keypoolrefill(100)
        self.nodes[0].keypoolrefill(100)
        for i in range(200):
            send_to[self.nodes[0].getnewaddress()] = Decimal("0.01")
        self.nodes[0].sendmany("", send_to)
        self.nodes[0].keypoolrefill(100)
        self.nodes[0].keypoolrefill(100)
        for i in range(200):
            send_to[self.nodes[0].getnewaddress()] = Decimal("0.01")
        self.nodes[0].sendmany("", send_to)
        self.nodes[0].keypoolrefill(100)
        self.nodes[0].keypoolrefill(100)
        for i in range(200):
            send_to[self.nodes[0].getnewaddress()] = Decimal("0.01")
        self.nodes[0].sendmany("", send_to)
        self.nodes[0].keypoolrefill(100)
        self.nodes[0].keypoolrefill(100)
        for i in range(200):
            send_to[self.nodes[0].getnewaddress()] = Decimal("0.01")
        self.nodes[0].sendmany("", send_to)
        self.nodes[0].keypoolrefill(100)
        self.nodes[0].keypoolrefill(100)
        for i in range(200):
            send_to[self.nodes[0].getnewaddress()] = Decimal("0.01")
        self.nodes[0].sendmany("", send_to)
        self.nodes[0].keypoolrefill(100)
        self.nodes[0].keypoolrefill(100)
        for i in range(200):
            send_to[self.nodes[0].getnewaddress()] = Decimal("0.01")
        self.nodes[0].sendmany("", send_to)

        # Mine a block so that the utxos are now spendable
        print ("mine a block")
        self.nodes[0].generate(1)

        connect_nodes(self.nodes[0],1)
        connect_nodes(self.nodes[0],2)
        connect_nodes(self.nodes[0],3)
        self.is_network_split=False
        self.sync_all()

        # Send more transactions
        print ("Generating more txns...")
        output_total = Decimal(0)
        j = 0
        self.utxo = self.nodes[0].listunspent()
        utxo = self.utxo.pop()
        txns_to_send = []
        num_txns = 700
        while j <= num_txns:
            inputs = []
            outputs = {}
            utxo = self.utxo.pop()
            if utxo["amount"] > Decimal("0.0100000") or utxo["amount"] < Decimal("0.0100000"):
                continue
            if utxo["spendable"] is True:
                j = j + 1
                inputs.append({ "txid" : utxo["txid"], "vout" : utxo["vout"]})
                outputs[self.nodes[0].getnewaddress()] = utxo["amount"] - Decimal("0.000010000")
                raw_tx = self.nodes[0].createrawtransaction(inputs, outputs)
                txns_to_send.append(self.nodes[0].signrawtransaction(raw_tx))

                # send the transaction
        for i in range(num_txns):
            self.nodes[0].sendrawtransaction(txns_to_send[i]["hex"], True)
        while self.nodes[0].getmempoolinfo()['size'] < num_txns:
            time.sleep(1)

        self.nodes[0].generate(1)
        self.sync_all()


        #stop and restart nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-par=1","-rpcservertimeout=0","-debug", "-debug=parallel", "-debug=parallel_2", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-debug=parallel", "-debug=parallel_2", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-debug=parallel", "-debug=parallel_2", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-debug=parallel", "-debug=parallel_2", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))


        # create transactions with many inputs
        print ("Generating even more txns...")
        num_txns2 = 5
        self.utxo = self.nodes[0].listunspent()
        for i in range(num_txns2):
            inputs = []
            outputs = {}
            output_total = Decimal(0)
            j = 1
            print ("utxo length " + str(len(self.utxo)))
            txns_to_send = []
            while j < 600:
                utxo = self.utxo.pop()
                if utxo["amount"] > Decimal("0.0100000") or utxo["amount"] < Decimal("0.0100000"):
                    continue
                j = j + 1
                inputs.append({ "txid" : utxo["txid"], "vout" : utxo["vout"]})
                output_total = output_total + utxo["amount"]
            outputs[self.nodes[0].getnewaddress()] = output_total/2 - Decimal("0.001")
            outputs[self.nodes[0].getnewaddress()] = output_total/2 - Decimal("0.001")
            raw_tx = self.nodes[0].createrawtransaction(inputs, outputs)
            txns_to_send.append(self.nodes[0].signrawtransaction(raw_tx))
    
            # send the transactions
            self.nodes[0].sendrawtransaction(txns_to_send[0]["hex"], True)

        # Send big tx's which will now have many inputs
        num_range = 50
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 1)

        while self.nodes[0].getmempoolinfo()['size'] < (num_txns2 + num_range):
            time.sleep(1)

        # Send a few transactions from node2 that will get mined so that we will have at least
        # a few inputs to check when the two competing blocks enter parallel validation.
        for i in range(10):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), "0.01")

        # Have node0 and node2 mine the same block which will compete to advance the chaintip when
        # The nodes are connected back together.
        print ("Mine two competing blocks...")
        self.nodes[0].generate(1)
        self.nodes[2].generate(1)
        print ("Connect nodes...")
        connect_nodes(self.nodes[0],1)
        #time.sleep(1.2) #add this time in if you have compiled with --enable-debug since you need some extra time for the block to propagate due to the slowness of the build.
        connect_nodes(self.nodes[1],2)
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


        # mine a block on node3 and then connect to the others.  This tests when a third block arrives after
        # the tip has been advanced.
        # this block should propagate to the other nodes but not cause a re-org
        print ("Mine another block...")
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


        self.nodes.append(start_node(0, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0", "-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
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


        self.nodes.append(start_node(0, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(3, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(4, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))
        self.nodes.append(start_node(5, self.options.tmpdir, ["-par=1", "-rpcservertimeout=0","-debug", "-use-thinblocks=0", "-excessiveblocksize=6000000", "-blockprioritysize=6000000", "-blockmaxsize=6000000"]))

        print ("Send more transactions...")
        num_range = 50
        for i in range(num_range):
            self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.1)
        num_range = 10
        for i in range(num_range):
            self.nodes[2].sendtoaddress(self.nodes[2].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[3].sendtoaddress(self.nodes[3].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[4].sendtoaddress(self.nodes[4].getnewaddress(), 0.01)
        for i in range(num_range):
            self.nodes[5].sendtoaddress(self.nodes[5].getnewaddress(), 0.01)

        # Mine 5 competing blocks. This should not cause a crash or failure to sync nodes.
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

        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()


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
        
    p.main ()
    
