#!/usr/bin/env python2
# Copyright (c) 2015-2016 The Bitcoin Unlimited developers
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
from decimal import *

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *


# Create one-input, one-output, no-fee transaction:
class TransactionPerformanceTest(BitcoinTestFramework):

    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 3)

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
        print "tx len, # inputs, # outputs, time"
        print >>fil, "fieldNames = ['tx len', '# inputs', '# outputs', 'time']"
        print >>fil, "data = ["
        for i in range(0,len(inputs),skip):
          for j in range(0,len(outputs),skip):
            try:
              if i==0: i=1
              if j==0: j=1
              (txn,inp,outp,txid) = split_transaction(node, inputs[0:i], outputs[0:j], txfee=DEFAULT_TX_FEE_PER_BYTE*10, sendtx=False)
            except Exception,e:
              print txLen,",",len(inp),",",len(outp),",","split error"
              print >>fil, "[",txLen,",",len(inp),",",len(outp),",","'split error:", str(e),"'],"
              continue
            try:
              s = str(txn)
	      #print "tx len: ", len(s)
              start=time.time()
	      signedtxn = node.signrawtransaction(s)
              end=time.time()
	      txLen = len(binascii.unhexlify(signedtxn["hex"]))  # Get the actual transaction size for better tx fee estimation the next time around
              print txLen,",",len(inp),",",len(outp),",",end-start
              print >>fil, "[",txLen,",",len(inp),",",len(outp),",",end-start,"],"
	    except:
              print txLen,",",len(inp),",",len(outp),",","timeout"
              print >>fil, "[",txLen,",",len(inp),",",len(outp),",","'timeout'],"
            fil.flush()
        print >>fil,"]"
        fil.close()

    def validatePerformance(self,node, inputCount,outputs,skip=100):
        fil = open("validatePerf.csv","w")
        print "tx len, # inputs, # outputs, time"
        print >>fil, "fieldNames = ['tx len', '# inputs', '# outputs', 'time']"
        print >>fil, "data = ["
        for i in range(0,inputCount,skip):
          for j in range(0,len(outputs),skip):
            print "ITER: ", i, " x ", j
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
            except Exception,e:
              print "split error: ", str(e)
              print >>fil, "[ 'sign',",0,",",i,",",j,",","'split error:", str(e),"'],"
              pdb.set_trace()
              continue

            time.sleep(4) # give the transaction time to propagate so we generate tx validation data separately from block validation data
            startTime = time.time()
	    node.generate(1)
            elapsedTime = time.time() - startTime
            print "generate time: ", elapsedTime
            txLen = len(binascii.unhexlify(txn))  # Get the actual transaction size for better tx fee estimation the next time around
	    print >>fil, "[ 'gen',",txLen,",",len(inp),",",len(outp),",",elapsedTime,"],"

            startTime = time.time()
            self.sync_all()
            elapsedTime = time.time() - startTime
            print "Sync time: ", elapsedTime
	    print >>fil, "[ 'sync',",txLen,",",len(inp),",",len(outp),",",elapsedTime,"],"
            
        print >>fil,"]"
        fil.close()


    def run_test(self):
        TEST_SIZE=500
        #prepare some coins for multiple *rawtransaction commands
        self.nodes[2].generate(1)
        self.sync_all()
        self.nodes[0].generate(100)
        self.sync_all()
        self.nodes[2].generate(21)  # So we can access 10 txouts from nodes[0]
        self.sync_all()

	wallet = self.nodes[0].listunspent()
        print wallet
        start = time.time()
        addrs = [ self.nodes[0].getnewaddress() for _ in range(TEST_SIZE+1)]
        print "['Benchmark', 'generate 2001 addresses', %f]" % (time.time()-start)
        for w in wallet[0:2]:
          split_transaction(self.nodes[0], [w], addrs)
          self.nodes[0].generate(1)
          self.sync_all()
          #tips = self.nodes[0].getchaintips()
          #print "TIPS:\n", tips
          #lastBlock = self.nodes[0].getblock(tips[0]["hash"])
          #print "LAST BLOCK:\n", lastBlock
          #txoutsetinfo = self.nodes[0].gettxoutsetinfo()
          #print "UTXOS:\n", txoutsetinfo

	self.nodes[0].generate(1)
        self.sync_all()
     
	wallet = self.nodes[0].listunspent()
        wallet.sort(key=lambda x: x["amount"],reverse=True)
        print "wallet length: %d" % len(wallet)

	# addrs = [ self.nodes[0].getnewaddress() for _ in range(1000)]
        print "addrs length: %d" % len(addrs)

        # self.signingPerformance(self.nodes[0], wallet[0:5000],addrs[0:2000])
        # self.validatePerformance(self.nodes[0], 2000,addrs,100)

        self.validatePerformance(self.nodes[0], TEST_SIZE,addrs,TEST_SIZE/2)


if __name__ == '__main__':
    TransactionPerformanceTest().main()

def Test():
  rtt = TransactionPerformanceTest()
  rtt.args = ["--nocleanup","--tracerpc",'-debug']
# "--noshutdown"
# "--tmpdir"
  rtt.main()
