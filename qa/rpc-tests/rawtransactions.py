#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test re-org scenarios with a mempool that contains transactions
# that spend (directly or indirectly) coinbase transactions.
#

from test_framework import BitcoinTestFramework
from util import *
from pprint import pprint
from time import sleep

# Create one-input, one-output, no-fee transaction:
class RawTransactionsTest(BitcoinTestFramework):
    
    def setup_chain(self):
        print("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 3)

    def setup_network(self, split=False):
        self.nodes = start_nodes(3, self.options.tmpdir)

        #connect to a local machine for debugging
        #url = "http://bitcoinrpc:DP6DvqZtqXarpeNWyN3LZTFchCCyCUuHwNF7E8pX99x1@%s:%d" % ('127.0.0.1', 18332)
        #proxy = AuthServiceProxy(url)
        #proxy.url = url # store URL on proxy for info
        #self.nodes.append(proxy)
        
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        
        self.is_network_split=False
        self.sync_all()
    
    def run_test(self):
        
        #prepare some coins for multiple *rawtransaction commands
        self.nodes[2].generate(1)
        self.nodes[0].generate(101)
        self.sync_all()
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),1.5);
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),1.0);
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),5.0);
        self.sync_all()
        self.nodes[0].generate(5)
        self.sync_all()

        #########################################
        # sendrawtransaction with missing input #
        #########################################
        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1}] #won't exists
        outputs = { self.nodes[0].getnewaddress() : 4.998 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        rawtx   = self.nodes[2].signrawtransaction(rawtx)
        
        errorString = ""
        try:
            rawtx   = self.nodes[2].sendrawtransaction(rawtx['hex'])
        except JSONRPCException,e:
            errorString = e.error['message']
        
        assert_equal("Missing inputs" in errorString, True);        

if __name__ == '__main__':
    RawTransactionsTest().main()
