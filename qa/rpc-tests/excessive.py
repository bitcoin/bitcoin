#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2015-2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Exercise the getchaintips API.  We introduce a network split, work
# on chains of different lengths, and join the network together again.
# This gives us two tips, verify that it works.
#from signal import signal, SIGPIPE, SIG_DFL
#signal(SIGPIPE,SIG_DFL)

import time
import random
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.util import *
from test_framework.blocktools import *
import test_framework.script as script
import pdb
import sys
if sys.version_info[0] < 3: raise "Use Python 3"
import logging
logging.basicConfig(format='%(asctime)s.%(levelname)s: %(message)s', level=logging.INFO)

class ExcessiveBlockTest (BitcoinTestFramework):
    def __init__(self,extended=False):
      self.extended = extended
      BitcoinTestFramework.__init__(self)

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes.append(start_node(0,self.options.tmpdir, ["-rpcservertimeout=0"], timewait=60*10))
        self.nodes.append(start_node(1,self.options.tmpdir, ["-rpcservertimeout=0"], timewait=60*10))
        self.nodes.append(start_node(2,self.options.tmpdir, ["-rpcservertimeout=0"], timewait=60*10))
        self.nodes.append(start_node(3,self.options.tmpdir, ["-rpcservertimeout=0"], timewait=60*10))
        interconnect_nodes(self.nodes)
        self.is_network_split=False
        self.sync_all()

        if 0:  # getnewaddress can be painfully slow.  This bit of code can be used to during development to
               # create a wallet with lots of addresses, which then can be used in subsequent runs of the test.
               # It is left here for developers to manually enable.
          TEST_SIZE=100 # TMP 00
          print("Creating addresses...")
          self.nodes[0].keypoolrefill(TEST_SIZE+1)
          addrs = [ self.nodes[0].getnewaddress() for _ in range(TEST_SIZE+1)]
          with open("walletAddrs.json","w") as f: 
            f.write(str(addrs))
            pdb.set_trace()

    def run_test(self):
        BitcoinTestFramework.run_test(self)
        self.testCli()
        self.testExcessiveSigops()
        self.testExcessiveBlockSize()
        self.testExcessiveTx()

    def testCli(self):

        # Assumes the default excessive at 16MB and mining at 1MB
        try:
            self.nodes[0].setminingmaxblock(32000000)
        except JSONRPCException as e:
            pass
        else:
            assert(0) # was able to set the mining size > the excessive size

        try:
            self.nodes[0].setminingmaxblock(99)
        except JSONRPCException as e:
            pass
        else:
            assert(0) # was able to set the mining size below our arbitrary minimum

        try:
            self.nodes[0].setexcessiveblock(1000,10)
        except JSONRPCException as e:
            pass
        else:
            assert(0) # was able to set the excessive size < the mining size
        
        

    def createUtxos(self,node,addrs,amt):
          wallet = node.listunspent()
          wallet.sort(key=lambda x: x["amount"],reverse=True)

          # Create a LOT of UTXOs
          logging.info("Create lots of UTXOs...")
          n=0
          group = min(100, amt)
          count = 0
          for w in wallet:
            count += group
            split_transaction(node, [w], addrs[n:group+n])
            n+=group
            if n >= len(addrs): n=0
            if count > amt:
                break
          self.sync_all()
          logging.info("mine blocks")
          node.generate(1)  # mine all the created transactions
          logging.info("sync all blocks and mempools")
          self.sync_all()


    def expectHeights(self, blockHeights, waittime=10):
        loop = 0
        count = []
        while loop < waittime:
            counts = [ x.getblockcount() for x in self.nodes ]
            if counts == blockHeights: return True # success!
            time.sleep(1)
            loop += 1
            if ((loop%30)==0):
                logging.info("...waiting %s" % loop)
        return False                 
          
    def testExcessiveSigops(self):
      """This test checks the behavior of the nodes in the presence of transactions that take a long time to validate.
      """
      NUM_ADDRS=100
      logging.info("Cleaning up node state")

      # We are not testing excessively sized blocks so make these large
      self.nodes[0].set("net.excessiveBlock=5000000")
      self.nodes[1].set("net.excessiveBlock=5000000")
      self.nodes[2].set("net.excessiveBlock=5000000")
      self.nodes[3].set("net.excessiveBlock=5000000")
      self.nodes[0].setminingmaxblock(5000000)
      self.nodes[1].setminingmaxblock(5000000)
      self.nodes[2].setminingmaxblock(5000000)
      self.nodes[3].setminingmaxblock(5000000)
      # Stagger the accept depths so we can see the block accepted stepwise
      self.nodes[0].set("net.excessiveAcceptDepth=0")
      self.nodes[1].set("net.excessiveAcceptDepth=1")
      self.nodes[2].set("net.excessiveAcceptDepth=2")
      self.nodes[3].set("net.excessiveAcceptDepth=3")

      for n in self.nodes:
          n.generate(10)
          self.sync_blocks()

      self.nodes[0].generate(100)  # create a lot of BTC for spending
          
      self.sync_all()

      self.nodes[0].set("net.excessiveSigopsPerMb=100")  # Set low so txns will fail if its used
      self.nodes[1].set("net.excessiveSigopsPerMb=5000")  
      self.nodes[2].set("net.excessiveSigopsPerMb=1000")  
      self.nodes[3].set("net.excessiveSigopsPerMb=100")

      logging.info("Creating addresses...")
      self.nodes[0].keypoolrefill(NUM_ADDRS)
      addrs = [ self.nodes[0].getnewaddress() for _ in range(NUM_ADDRS)]

      # test that a < 1MB block ignores the sigops parameter
      self.nodes[0].setminingmaxblock(1000000)
      self.createUtxos(self.nodes[0], addrs, NUM_ADDRS) # if excessive Sigops was heeded, this txn would not make it into the block
      mpool = self.nodes[0].getmempoolinfo()
      assert_equal(mpool["size"],0) 
      
      # test that a < 1MB block ignores the sigops parameter, even if the max block size is less
      self.nodes[0].setminingmaxblock(5000000)
      self.createUtxos(self.nodes[0], addrs, NUM_ADDRS) # if excessive Sigops was heeded, this txn would not make it into the block
      mpool = self.nodes[0].getmempoolinfo()
      assert_equal(mpool["size"],0) 


      if self.extended:  # creating 1MB+ blocks is too slow for travis due to the signing cost
        self.createUtxos(self.nodes[0], addrs, 10000) # we need a lot to generate 1MB+ blocks
      
        wallet = self.nodes[0].listunspent()
        wallet.sort(key=lambda x: x["amount"],reverse=True)
        self.nodes[0].set("net.excessiveSigopsPerMb=100000")  # Set this huge so all txns are accepted by this node

        logging.info("Generate > 1MB block with excessive sigops")
        # create a MB worth of tx
        blkLen = 0
        i = 0
        lastPrint=0
        while blkLen <= 1100000:
          if len(wallet) == 0:
              pdb.set_trace()
          (tx, vin, vout, txid) = split_transaction(self.nodes[0],wallet[0],addrs[i:i+3],txfeePer=60)
          del wallet[0]
          i = i + 1
          if i>=len(addrs)-2: i = 0
          blkLen += len(binascii.unhexlify(tx))
          if blkLen-lastPrint > 50000:
              logging.info("...working %d" % blkLen)
              lastPrint=blkLen

        counts = [ x.getblockcount() for x in self.nodes ]
        base = counts[0]

        self.nodes[0].generate(1)
        assert_equal(True, self.expectHeights([base+1,base,base,base],30))

        logging.info("Test excessive block propagation to nodes with different AD")
        self.nodes[0].generate(1)
        assert_equal(True, self.expectHeights([base+2,base+2,base,base],500))  # it takes a while to sync all the txns

        self.nodes[0].generate(1)
        assert_equal(True, self.expectHeights([base+3,base+3,base+3,base],90))

        self.nodes[0].generate(1)
        assert_equal(True, self.expectHeights([base+4,base+4,base+4,base+4],90))

      logging.info("Excessive sigops test completed")

      # set it all back to defaults
      
      for n in self.nodes:
          n.generate(150)
          self.sync_blocks()
      
      self.nodes[0].set("net.excessiveSigopsPerMb=20000")  # Set low so txns will fail if its used
      self.nodes[1].set("net.excessiveSigopsPerMb=20000")  
      self.nodes[2].set("net.excessiveSigopsPerMb=20000")  
      self.nodes[3].set("net.excessiveSigopsPerMb=20000")  

      self.nodes[0].setminingmaxblock(1000000)
      self.nodes[1].setminingmaxblock(1000000)
      self.nodes[2].setminingmaxblock(1000000)
      self.nodes[3].setminingmaxblock(1000000)
      self.nodes[0].set("net.excessiveBlock=1000000")
      self.nodes[1].set("net.excessiveBlock=1000000")
      self.nodes[2].set("net.excessiveBlock=1000000")
      self.nodes[3].set("net.excessiveBlock=1000000")
 
    def testExcessiveTx(self):
      """This test checks the behavior of the nodes in the presence of excessively large transactions.
         It will set the accept depth to different values on each node, and then verify that each node
         Does not follow the excessive tip until the accept depth is exceeded.
 
         The test also validates the rejection of a > 100kb transaction in a > 1MB block, and again
         watches as each node eventually accepts the large tx based on accept depth.
      """
      TEST_SIZE=20
      logging.info("Test excessive transactions")
      if 1:
        tips = self.nodes[0].getchaintips ()

        self.nodes[0].set("net.excessiveAcceptDepth=0")
        self.nodes[1].set("net.excessiveAcceptDepth=1")
        self.nodes[2].set("net.excessiveAcceptDepth=2")
        self.nodes[3].set("net.excessiveAcceptDepth=3")

        self.nodes[0].set("net.excessiveBlock=2000000")
        self.nodes[1].set("net.excessiveBlock=2000000")
        self.nodes[2].set("net.excessiveBlock=2000000")
        self.nodes[3].set("net.excessiveBlock=2000000")

        logging.info("Cleaning up node state")
        for n in self.nodes:
          n.generate(10)
          self.sync_blocks()
        
        self.sync_all()
        # verify mempool is cleaned up on all nodes
        mbefore = [ (lambda y: (y["size"],y["bytes"]))(x.getmempoolinfo()) for x in self.nodes]
        assert_equal(mbefore,[(0, 0)]*4)


        if 1:
          logging.info("Creating addresses...")
          self.nodes[0].keypoolrefill(TEST_SIZE+1)
          addrs = [ self.nodes[0].getnewaddress() for _ in range(TEST_SIZE+1)]
        else:  # enable if you are using a pre-created wallet, as described above
          logging.info("Loading addresses...")
          with open("wallet10kAddrs.json") as f: addrs = json.load(f)


        if 1:  # Test not relaying a large transaction

          # Make the excessive transaction size smaller so its quicker to produce a excessive one  
          self.nodes[0].set("net.excessiveTx=100000")
          self.nodes[1].set("net.excessiveTx=100000")
          self.nodes[2].set("net.excessiveTx=100000")
          self.nodes[3].set("net.excessiveTx=100000")
          self.nodes[0].setminingmaxblock(1000000)
          self.nodes[1].setminingmaxblock(1000000)
          self.nodes[2].setminingmaxblock(1000000)
          self.nodes[3].setminingmaxblock(1000000)
             
          wallet = self.nodes[0].listunspent()
          wallet.sort(key=lambda x: x["amount"],reverse=True)
          while len(wallet) < 3000:
            # Create a LOT of UTXOs
            logging.info("Create lots of UTXOs...")
            n=0
            group = min(100, TEST_SIZE)
            count = 0
            for w in wallet:
              count += 1
              # print count, " ",
              split_transaction(self.nodes[0], [w], addrs[n:group+n])
              n+=group
              if n >= len(addrs): n=0
              if count > 50:  # We don't need any more
                break
            self.sync_all()
            logging.info("mine blocks")
            self.nodes[0].generate(5)  # mine all the created transactions
            logging.info("sync all blocks and mempools")
            self.sync_all()

            wallet = self.nodes[0].listunspent()
            wallet.sort(key=lambda x: x["amount"],reverse=True)

          logging.info("clean out the mempool")
          mbefore = [ (lambda y: (y["size"],y["bytes"]))(x.getmempoolinfo()) for x in self.nodes]
          while mbefore != [(0, 0)]*4:
            time.sleep(1)
            self.nodes[0].generate(1)
            time.sleep(10)
            mbefore = [ (lambda y: (y["size"],y["bytes"]))(x.getmempoolinfo()) for x in self.nodes]
          
          assert_equal(mbefore,[(0, 0)]*4)  # we need the mempool to be empty to track that this one tx doesn't prop

          logging.info("Test not relaying a large transaction")

          (tx, vin, vout, txid) = split_transaction(self.nodes[0],wallet[0:3000],[addrs[0]],txfeePer=60)
          logging.debug("Transaction Length is: ", len(binascii.unhexlify(tx)))
          assert(len(binascii.unhexlify(tx)) > 100000) # txn has to be big for the test to work
        
          mbefore = [ (lambda y: (y["size"],y["bytes"]))(x.getmempoolinfo()) for x in self.nodes]
          assert_equal(mbefore[1:],[(0, 0), (0, 0), (0, 0)])  # verify that the transaction did not propagate
          assert(mbefore[0][0] > 0) # verify that the transaction is in my node

          print ("allowing tx a chance to propagate - sleeping...")
          while len(self.nodes[0].getmempoolinfo()) < 1 :
              print ("sleeping 1")
              time.sleep(1)
          time.sleep(5)

          logging.info("Test a large transaction in block < 1MB")
          largeBlock = self.nodes[0].generate(1)
          self.sync_blocks()
          counts = [ x.getblockcount() for x in self.nodes ]
          latest = counts[0]
          assert_equal(counts, [latest,latest,latest,latest]) # Verify that all nodes accepted the block, even if some of them didn't have the transaction.  They should all accept a <= 1MB block with a tx <= 1MB

        if self.extended:  # this test checks the behavior of > 1MB blocks with excessive transactions.  it takes a LONG time to generate and propagate 1MB+ txs.

          logging.info("Creating addresses...")
          self.nodes[0].keypoolrefill(2000)
          addrs = [ self.nodes[0].getnewaddress() for _ in range(2000)]
          # Create a LOT of UTXOs for the next test
          wallet = self.nodes[0].listunspent()
          wallet.sort(key=lambda x: x["amount"],reverse=True)
          wlen = len(wallet)
          while wlen < 8000:
            logging.info("Create lots of UTXOs by 100...")
            n=0  
            for w in wallet:
                split_transaction(self.nodes[0], [w], addrs[n:100+n])
                logging.info(str(wlen))
                n+=100
                if n >= len(addrs): n=0
                wlen += 99
                if wlen > 8000: break

            blk = self.nodes[0].generate(1)
            blkinfo = self.nodes[0].getblock(blk[0])                        
            logging.info("Generated block %d size: %d, num tx: %d" % (blkinfo["height"], blkinfo["size"], len(blkinfo["tx"])))
            wallet = self.nodes[0].listunspent()
            wallet.sort(key=lambda x: x["amount"],reverse=True)
            wlen = len(wallet)
            

          self.nodes[0].generate(1)
          self.sync_all()

          logging.info("Building > 1MB block...")
          self.nodes[0].set("net.excessiveTx=1000000")  # Set the excessive transaction size larger for this node so we can generate an "excessive" block for the other nodes

          wallet = self.nodes[0].listunspent()
          wallet.sort(key=lambda x: x["amount"],reverse=False)
          logging.info("Wallet length is %d" % len(wallet))

          # Generate 1 MB worth of transactions
          size = 0
          count = 0
          while size < 1000000:
            count+=1
            utxo = wallet.pop()
            outp = {}
            outp[addrs[count%len(addrs)]] = utxo["amount"]
            txn = self.nodes[0].createrawtransaction([utxo], outp)
            signedtxn = self.nodes[0].signrawtransaction(txn)
            size += len(binascii.unhexlify(signedtxn["hex"]))
            self.nodes[0].sendrawtransaction(signedtxn["hex"])

          # Now generate a > 100kb transaction & mine it into a > 1MB block

          self.nodes[0].setminingmaxblock(2000000)
          self.nodes[0].set("net.excessiveBlock=2000000")

          wallet.sort(key=lambda x: x["amount"],reverse=True)
          (tx, vin, vout, txid) = split_transaction(self.nodes[0],wallet[0:2500],[addrs[0]],txfeePer=60)
          logging.debug("Transaction Length is: ", len(binascii.unhexlify(tx)))
          assert(len(binascii.unhexlify(tx)) > 100000) # txn has to be big for the test to work

          origCounts = [ x.getblockcount() for x in self.nodes ]
          base = origCounts[0]
          mpool = [ (lambda y: (y["size"],y["bytes"]))(x.getmempoolinfo()) for x in self.nodes]
          logging.debug(str(mpool))
          largeBlock = self.nodes[0].generate(1)
          mpool = [ (lambda y: (y["size"],y["bytes"]))(x.getmempoolinfo()) for x in self.nodes]
          logging.debug(str(mpool))

          logging.info("Syncing node1")
          largeBlock2 = self.nodes[0].generate(1)
          sync_blocks(self.nodes[0:2])
          mpool = [ (lambda y: (y["size"],y["bytes"]))(x.getmempoolinfo()) for x in self.nodes]
          logging.debug(str(mpool))
          self.expectHeights([base+2,base+2, base, base],30)

          logging.info("Syncing node2")
          largeBlock3 = self.nodes[0].generate(1)
          sync_blocks(self.nodes[0:3])
          self.expectHeights([base+3,base+3, base+3, base],30)
          
          logging.info("Syncing node3")          
          largeBlock4 = self.nodes[0].generate(1)
          sync_blocks(self.nodes)
          self.expectHeights([base+4,base+4, base+4, base+4],30)

        # Put it back to the default
        self.nodes[0].set("net.excessiveTx=1000000")
        self.nodes[1].set("net.excessiveTx=1000000")
        self.nodes[2].set("net.excessiveTx=1000000")
        self.nodes[3].set("net.excessiveTx=1000000")

          
    def repeatTx(self,count,node,addr,amt=1.0):
        for i in range(0,count):
          node.sendtoaddress(addr, amt)

    def testExcessiveBlockSize(self):

        # get spendable coins
        if 0:
          for n in self.nodes:
            n.generate(1)
            self.sync_all()
          self.nodes[0].generate(100)
          self.sync_all()
        
        # Set the accept depth at 1, 2, and 3 and watch each nodes resist the chain for that long
        self.nodes[1].setminingmaxblock(1000)
        self.nodes[2].setminingmaxblock(1000)
        self.nodes[3].setminingmaxblock(1000)

        self.nodes[1].setexcessiveblock(1000, 1)
        self.nodes[2].setexcessiveblock(1000, 2)
        self.nodes[3].setexcessiveblock(1000, 3)

        logging.info("Test excessively sized block, not propagating until accept depth is exceeded")
        addr = self.nodes[3].getnewaddress()
        self.repeatTx(20,self.nodes[0],addr)
        counts = [ x.getblockcount() for x in self.nodes ]
        base = counts[0]
        logging.info("node0")
        self.nodes[0].generate(1)
        time.sleep(2) #give blocks a chance to fully propagate
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [base+1,base,base,base])  

        logging.info("node1")
        self.nodes[0].generate(1)
        sync_blocks(self.nodes[0:2])
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [base+2,base+2,base,base])  

        logging.info("node2")
        self.nodes[0].generate(1)
        sync_blocks(self.nodes[0:3])
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [base+3,base+3,base+3,base])  

        logging.info("node3")
        self.nodes[0].generate(1)
        self.sync_all()
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [base+4]*4)  

        # Now generate another excessive block, but all nodes should snap right to it because they have an older excessive block
        logging.info("Test immediate propagation of additional excessively sized block, due to prior excessive")
        self.repeatTx(20,self.nodes[0],addr)
        self.nodes[0].generate(1)
        self.sync_all()
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [base+5]*4)  
      
        logging.info("Test daily excessive reset")
        self.nodes[0].generate(6*24)  # Now generate a day's worth of small blocks which should re-enable the node's reluctance to accept a large block
        self.sync_all()
        self.nodes[0].generate(5) # plus the accept depths
        self.sync_all()
        self.repeatTx(20,self.nodes[0],addr)

        base = self.nodes[0].getblockcount()
        self.nodes[0].generate(1)
        time.sleep(2) #give blocks a chance to fully propagate
        counts = [ x.getblockcount() for x in self.nodes ]
        #assert_equal(counts, [base+1,349,349,349])  
        assert_equal(counts, [base+1,base,base,base])  

        self.repeatTx(20,self.nodes[0],addr)
        self.nodes[0].generate(1)
        time.sleep(2) #give blocks a chance to fully propagate
        sync_blocks(self.nodes[0:2])
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [base+2,base+2,base,base])  

        self.repeatTx(20,self.nodes[0],addr)
        self.nodes[0].generate(1)
        time.sleep(2) #give blocks a chance to fully propagate
        sync_blocks(self.nodes[0:3])
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [base+3,base+3,base+3,base])  

        self.repeatTx(20,self.nodes[0],addr)
        self.nodes[0].generate(1)
        self.sync_all()
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [base+4]*4)  

        self.repeatTx(20,self.nodes[0],addr)
        self.nodes[0].generate(1)
        self.sync_all()
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [base+5]*4)  

        logging.info("Test daily excessive reset #2")
        self.nodes[0].generate(6*24 + 10)  # Now generate a day's worth of small blocks which should re-enable the node's reluctance to accept a large block + 10 because we have to get beyond all the node's accept depths
        self.sync_all()

        # counts = [ x.getblockcount() for x in self.nodes ]
        self.nodes[1].setexcessiveblock(100000, 1)  # not sure how big the txns will be but smaller than this 
        self.nodes[1].setminingmaxblock(100000)  # not sure how big the txns will be but smaller than this 
        self.repeatTx(40,self.nodes[0],addr)
        self.sync_all()
        base = self.nodes[0].getblockcount()
        self.nodes[0].generate(1)
        time.sleep(2) #give blocks a chance to fully propagate
        sync_blocks(self.nodes[0:2])
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [base+1,base+1,base,base])
        
        logging.info("Random test")
        if self.extended: randomRange = 20
        else: randomRange = 2

        for i in range(0,randomRange):
          logging.info("round %d" % i)
          for n in self.nodes:
            size = random.randint(1,1000)*1000
            try:  # since miningmaxblock must be <= excessiveblock, raising/lowering may need to run these in different order
              n.setminingmaxblock(size)
              n.setexcessiveblock(size, random.randint(0,10))
            except JSONRPCException:
              n.setexcessiveblock(size, random.randint(0,10))
              n.setminingmaxblock(size)
               
          addrs = [x.getnewaddress() for x in self.nodes]
          ntxs=0
          for i in range(0,random.randint(1,200)):
            try:
              self.nodes[random.randint(0,3)].sendtoaddress(addrs[random.randint(0,3)], .1)
              ntxs += 1
            except JSONRPCException: # could be spent all the txouts
              pass
          logging.info("%d transactions" % ntxs)
          time.sleep(1) #allow txns a chance to propagate
          self.nodes[random.randint(0,3)].generate(1)
          logging.info("mined a block")
          #TODO:  rather than sleeping we should really be putting a check in here 
          #       based on what the random excessive seletions were from above
          time.sleep(5) #allow block a chance to propagate


if __name__ == '__main__':
    
    
    if "--extensive" in sys.argv:
      longTest=True
      # we must remove duplicate 'extensive' arg here
      while True:
          try:
              sys.argv.remove('--extensive')
          except:
              break
      logging.info("Running extensive tests")
    else:
      longTest=False

    ExcessiveBlockTest(longTest).main ()

def info(type, value, tb):
   if hasattr(sys, 'ps1') or not sys.stderr.isatty():
      # we are in interactive mode or we don't have a tty-like
      # device, so we call the default hook
      sys.__excepthook__(type, value, tb)
   else:
      import traceback, pdb
      # we are NOT in interactive mode, print the exception...
      traceback.print_exception(type, value, tb)
      print
      # ...then start the debugger in post-mortem mode.
      pdb.pm()

sys.excepthook = info

def Test():
  t = ExcessiveBlockTest(True)
  bitcoinConf = {
    "debug":["net","blk","thin","mempool","req","bench","evict"], # "lck"
    "blockprioritysize":2000000  # we don't want any transactions rejected due to insufficient fees...
  }
# "--tmpdir=/ramdisk/test"
  t.main(["--nocleanup","--noshutdown"],bitcoinConf,None) # , "--tracerpc"])
