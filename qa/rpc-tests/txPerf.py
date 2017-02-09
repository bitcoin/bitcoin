#!/usr/bin/env python3
# Copyright (c) 2015-2017 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test re-org scenarios with a mempool that contains transactions
# that spend (directly or indirectly) coinbase transactions.
#
import pdb
import binascii
import time
import math
import json
import logging
logging.basicConfig(format='%(asctime)s.%(levelname)s: %(message)s', level=logging.INFO)

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *


# Create one-input, one-output, no-fee transaction:
class TransactionPerformanceTest(BitcoinTestFramework):

    def setup_chain(self,bitcoinConfDict=None, wallets=None):
        logging.info("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 3, bitcoinConfDict, wallets)

    def setup_network(self, split=False):
        self.nodes = start_nodes(3, self.options.tmpdir,timewait=60*60)

        #connect to a local machine for debugging
        #url = "http://bitcoinrpc:DP6DvqZtqXarpeNWyN3LZTFchCCyCUuHwNF7E8pX99x1@%s:%d" % ('127.0.0.1', 18332)
        #proxy = AuthServiceProxy(url)
        #proxy.url = url # store URL on proxy for info
        #self.nodes.append(proxy)

        # Connect each node to the other
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)

        self.is_network_split=False
        self.sync_all()

    def generate_utxos(self,node, count,amt=0.1):
      if type(node) == type(0):  # Convert a node index to a node object
        node = self.nodes[node]

      addrs = []
      for i in range(0,count):
        addr = node.getnewaddress()
        addrs.append(addr)
        node.sendtoaddress(addr, amt)
      node.generate(1)
      self.sync_all()

    def signingPerformance(self,node, inputs,outputs,skip=100):
        fil = open("signPerf.csv","w")
        logging.info("tx len, # inputs, # outputs, time")
        print ("fieldNames = ['tx len', '# inputs', '# outputs', 'time']", file=fil)
        print ("data = [", file=fil)
        for i in range(0,len(inputs),skip):
          for j in range(0,len(outputs),skip):
            try:
              if i==0: i=1
              if j==0: j=1
              (txn,inp,outp,txid) = split_transaction(node, inputs[0:i], outputs[0:j], txfee=DEFAULT_TX_FEE_PER_BYTE*10, sendtx=False)
            except e:
              logging.info("%d, %d, %d, split error" % (txLen,len(inp),len(outp)))
              print("[",txLen,",",len(inp),",",len(outp),",",'"split error:"', str(e),'"],', file=fil)
              continue
            try:
              s = str(txn)
	      #print ("tx len: ", len(s))
              start=time.time()
              signedtxn = node.signrawtransaction(s)
              end=time.time()
              txLen = len(binascii.unhexlify(signedtxn["hex"]))  # Get the actual transaction size for better tx fee estimation the next time around
              logging.info("%d, %d, %d, %f" % (txLen,len(inp),len(outp),end-start))
              print("[",txLen,",",len(inp),",",len(outp),",",end-start,"],",file=fil)
            except:
              logging.info("%d, %d, %d, %s" % (txLen,len(inp),len(outp),'"timeout"'))
              print (txLen,",",len(inp),",",len(outp),",","timeout")
              print ("[",txLen,",",len(inp),",",len(outp),",",'"timeout"],',file=fil)
            fil.flush()
        print("]",file=fil)
        fil.close()

    def validatePerformance(self,node, inputCount,outputs,skip=100):
        fil = open("validatePerf.csv","w")
        print("tx len, # inputs, # outputs, time")
        print ("fieldNames = ['tx len', '# inputs', '# outputs', 'time']",file=fil)
        print ("data = [",file=fil)
        for i in range(0,inputCount,skip):
          for j in range(0,len(outputs),skip):
            print("ITER: ", i, " x ", j)
            wallet = node.listunspent()
            wallet.sort(key=lambda x: x["amount"],reverse=True)
            while len(wallet) < i:  # Make a bunch more inputs
              (txn,inp,outp,txid) = split_transaction(node, [wallet[0]], outputs, txfee=DEFAULT_TX_FEE_PER_BYTE*10)
              self.sync_all()
              wallet = node.listunspent()
              wallet.sort(key=lambda x: x["amount"],reverse=True)

            try:
              if i==0: i=1
              if j==0: j=1
              (txn,inp,outp,txid) = split_transaction(node, wallet[0:i], outputs[0:j], txfee=DEFAULT_TX_FEE_PER_BYTE*10, sendtx=True)
            except e:
              logging.info("split error: %s" % str(e))
              print("[ 'sign',",0,",",i,",",j,",","'split error:", str(e),"'],",file=fil)
              pdb.set_trace()
              continue

            time.sleep(4) # give the transaction time to propagate so we generate tx validation data separately from block validation data
            startTime = time.time()
            node.generate(1)
            elapsedTime = time.time() - startTime
            logging.info("generate time: %f" % elapsedTime)
            txLen = len(binascii.unhexlify(txn))  # Get the actual transaction size for better tx fee estimation the next time around
            print("[ 'gen',",txLen,",",len(inp),",",len(outp),",",elapsedTime,"],",file=fil)

            startTime = time.time()
            self.sync_all()
            elapsedTime = time.time() - startTime
            logging.info("Sync time: %f" % elapsedTime)
            print("[ 'sync',",txLen,",",len(inp),",",len(outp),",",elapsedTime,"],",file=fil)
            
        print("]",file=fil)
        fil.close()


    def largeOutput(self):
        """This times the validation of 1 to many and many to 1 transactions.  Its not needed to be run as a daily unit test"""
        print("synchronizing")
        self.sync_all()    
        node = self.nodes[0]        
        start = time.time()
        print("generating addresses")
        if 1:
          addrs = [ node.getnewaddress() for _ in range(20000)]
          f = open("addrs.txt","w")
          f.write(str(addrs))
          f.close()
          print("['Benchmark', 'generate 20000 addresses', %f]" % (time.time()-start))
        else:
          import addrlist
          addrs = addrlist.addrlist

        wallet = node.listunspent()
        wallet.sort(key=lambda x: x["amount"],reverse=True)

        (txn,inp,outp,txid) = split_transaction(node, wallet[0], addrs[0:10000], txfee=DEFAULT_TX_FEE_PER_BYTE, sendtx=True)
        txLen = len(binascii.unhexlify(txn))  # Get the actual transaction size for better tx fee estimation the next time around
        print("[ 'gen',",txLen,",",len(inp),",",len(outp), "],")

        startTime = time.time()          
        node.generate(1)
        elapsedTime = time.time() - startTime
        print ("Generate time: ", elapsedTime)
        startTime = time.time()
        print ("synchronizing")
        self.sync_all()
        elapsedTime = time.time() - startTime
        print("Sync     time: ", elapsedTime)
        
        # Now join with a tx with a huge number of inputs
       	wallet = self.nodes[0].listunspent()
        wallet.sort(key=lambda x: x["amount"])

        (txn,inp,outp,txid) = split_transaction(node, wallet[0:10000], [addrs[0]], txfee=DEFAULT_TX_FEE_PER_BYTE, sendtx=True)
        txLen = len(binascii.unhexlify(txn))  # Get the actual transaction size for better tx fee estimation the next time around
        print("[ 'gen',",txLen,",",len(inp),",",len(outp), "],")
      
            

    def run_test(self):
        TEST_SIZE=200  # To collect a lot of data points, set the TEST_SIZE to 2000

        #prepare some coins for multiple *rawtransaction commands
        self.nodes[2].generate(1)
        self.sync_all()
        self.nodes[0].generate(100)
        self.sync_all()
        self.nodes[2].generate(21)  # So we can access 10 txouts from nodes[0]
        self.sync_all()

        # This times the validation of 1 to many and many to 1 transactions.  Its not needed to be run as a unit test
        # self.largeOutput()
    
        print("Generating new addresses... will take awhile")
        start = time.time()
        addrs = [ self.nodes[0].getnewaddress() for _ in range(TEST_SIZE+1)]
        print("['Benchmark', 'generate 2001 addresses', %f]" % (time.time()-start))

        wallet = self.nodes[0].listunspent()
        wallet.sort(key=lambda x: x["amount"],reverse=True)

        for w in wallet[0:2]:
          split_transaction(self.nodes[0], [w], addrs)
          self.nodes[0].generate(1)
          self.sync_all()
          #tips = self.nodes[0].getchaintips()
          #print ("TIPS:\n", tips)
          #lastBlock = self.nodes[0].getblock(tips[0]["hash"])
          #print ("LAST BLOCK:\n", lastBlock)
          #txoutsetinfo = self.nodes[0].gettxoutsetinfo()
          #print ("UTXOS:\n", txoutsetinfo)

        self.nodes[0].generate(1)
        self.sync_all()
     
        wallet = self.nodes[0].listunspent()
        wallet.sort(key=lambda x: x["amount"],reverse=True)

        logging.info("wallet length: %d" % len(wallet))
        logging.info("addrs length: %d" % len(addrs))
       
        # To collect a lot of data points, set the interval to 100 or even 10 and run overnight
        interval = 100 # TEST_SIZE/2

        # self.signingPerformance(self.nodes[0], wallet[0:TEST_SIZE],addrs[0:TEST_SIZE],interval)
        self.validatePerformance(self.nodes[0], TEST_SIZE,addrs,interval)


if __name__ == '__main__':
    tpt = TransactionPerformanceTest()
    bitcoinConf = {
    "debug":["net","blk","thin","lck","mempool","req","bench","evict"],
    "blockprioritysize":2000000  # we don't want any transactions rejected due to insufficient fees...
    }
    tpt.main(["--nocleanup"],bitcoinConf)

def Test():    
    tpt = TransactionPerformanceTest()
    bitcoinConf = {
    "debug":["bench"],
    "blockprioritysize":2000000  # we don't want any transactions rejected due to insufficient fees...
    }
    tpt.main(["--nocleanup","--tmpdir=/ramdisk/test"],bitcoinConf)
       
    
