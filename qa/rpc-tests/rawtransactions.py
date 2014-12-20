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

        # connect to a local machine for debugging
        # url = "http://bitcoinrpc:DP6DvqZtqXarpeNWyN3LZTFchCCyCUuHwNF7E8pX99x1@%s:%d" % ('127.0.0.1', 18332)
        # proxy = AuthServiceProxy(url)
        # proxy.url = url # store URL on proxy for info
        # self.nodes.append(proxy)
        
        connect_nodes_bi(self.nodes,0,1)
        connect_nodes_bi(self.nodes,1,2)
        connect_nodes_bi(self.nodes,0,2)
        
        self.is_network_split=False
        self.sync_all()
    
    def run_test(self):
        
        self.nodes[2].setgenerate(True, 1)
        self.nodes[0].setgenerate(True, 101)
        self.sync_all()
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),1.5);
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),1.0);
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),5.0);
        self.sync_all()
        self.nodes[0].setgenerate(True, 1)
        self.sync_all()

        ###############
        # simple test #
        ###############
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']        
        
        assert_equal(len(dec_tx['vin']), 1) #one vin coin
        assert_equal(fee*0.00000001+float(totalOut), 1.5) #the 1.5BTC coin must be taken
        
        ##############################
        # simple test with two coins #
        ##############################
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 2.2 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']        
        
        assert_equal(len(dec_tx['vin']), 2) #one vin coin
        assert_equal(fee*0.00000001+float(totalOut), 2.5) #the 1.5BTC+1.0BTC coins must have be taken
        
        ##############################
        # simple test with two coins #
        ##############################
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 2.6 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']        
        
        assert_equal(len(dec_tx['vin']), 1) #one vin coin
        assert_equal(fee*0.00000001+float(totalOut), 5.0) #the 5.0BTC coin must have be taken
        
        
        ################################
        # simple test with two outputs #
        ################################
        inputs  = [ ]
        outputs = { self.nodes[0].getnewaddress() : 2.6, self.nodes[1].getnewaddress() : 2.5 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']        
        
        assert_equal(len(dec_tx['vin']), 2) #one vin coin
        assert_equal(fee*0.00000001+float(totalOut), 6.0) #the 5.0BTC + 1.0BTC coins must have be taken
        

        
        #########################################################################
        # test a fundrawtransaction with a VIN greater than the required amount #
        #########################################################################
        utx = False
        listunspent = self.nodes[2].listunspent()
        for aUtx in listunspent:
            if aUtx['amount'] == 5.0:
                utx = aUtx
                break;

        assert_equal(utx!=False, True)
        
        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']}]
        outputs = { self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        for out in dec_tx['vout']:
            totalOut += out['value']        
            
        assert_equal(fee*0.00000001+float(totalOut), utx['amount']) #compare vin total and totalout+fee
        
        
        #########################################################################
        # test a fundrawtransaction with a VIN smaller than the required amount #
        #########################################################################
        utx = False
        listunspent = self.nodes[2].listunspent()
        for aUtx in listunspent:
            if aUtx['amount'] == 1.0:
                utx = aUtx
                break;

        assert_equal(utx!=False, True)
        
        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']}]
        outputs = { self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for out in dec_tx['vout']:
            totalOut += out['value']
            if outputs.has_key(out['scriptPubKey']['addresses'][0]):
                matchingOuts+=1      
        
        assert_equal(matchingOuts, 1)
        assert_equal(len(dec_tx['vout']), 2)
            
        assert_equal(fee*0.00000001+float(totalOut), 2.5) #this tx must use the 1.0BTC and the 1.5BTC coin
        
        
        ###########################################
        # test a fundrawtransaction with two VINs #
        ###########################################
        utx  = False
        utx2 = False 
        listunspent = self.nodes[2].listunspent()
        for aUtx in listunspent:
            if aUtx['amount'] == 1.0:
                utx = aUtx
            if aUtx['amount'] == 5.0:
                utx2 = aUtx


        assert_equal(utx!=False, True)
        
        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']},{'txid' : utx2['txid'], 'vout' : utx2['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : 6.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for out in dec_tx['vout']:
            totalOut += out['value']
            if outputs.has_key(out['scriptPubKey']['addresses'][0]):
                matchingOuts+=1      
        
        assert_equal(matchingOuts, 1)
        assert_equal(len(dec_tx['vout']), 2)
        
        matchingIns = 0
        for vinOut in dec_tx['vin']:
            for vinIn in inputs:
                if vinIn['txid'] == vinOut['txid']:
                    matchingIns+=1
        
        assert_equal(matchingIns, 2) #we now must see two vins identical to vins given as params
        assert_equal(fee*0.00000001+float(totalOut), 7.5) #this tx must use the 1.0BTC and the 1.5BTC coin
        
        
        #########################################################
        # test a fundrawtransaction with two VINs and two vOUTs #
        #########################################################
        utx  = False
        utx2 = False 
        listunspent = self.nodes[2].listunspent()
        for aUtx in listunspent:
            if aUtx['amount'] == 1.0:
                utx = aUtx
            if aUtx['amount'] == 5.0:
                utx2 = aUtx


        assert_equal(utx!=False, True)
        
        inputs  = [ {'txid' : utx['txid'], 'vout' : utx['vout']},{'txid' : utx2['txid'], 'vout' : utx2['vout']} ]
        outputs = { self.nodes[0].getnewaddress() : 6.0, self.nodes[0].getnewaddress() : 1.0 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        assert_equal(utx['txid'], dec_tx['vin'][0]['txid'])

        rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        fee = rawtxfund['fee']
        dec_tx  = self.nodes[2].decoderawtransaction(rawtxfund['hex'])
        totalOut = 0
        matchingOuts = 0
        for out in dec_tx['vout']:
            totalOut += out['value']
            if outputs.has_key(out['scriptPubKey']['addresses'][0]):
                matchingOuts+=1
        
        assert_equal(matchingOuts, 2)
        assert_equal(len(dec_tx['vout']), 3)
        assert_equal(fee*0.00000001+float(totalOut), 7.5) #this tx must use the 1.0BTC and the 1.5BTC coin
        
        
        ##############################################
        # test a fundrawtransaction with invalid vin #
        ##############################################
        listunspent = self.nodes[2].listunspent()
        inputs  = [ {'txid' : "1c7f966dab21119bac53213a2bc7532bff1fa844c124fd750a7d0b1332440bd1", 'vout' : 0} ] #invalid vin!
        outputs = { self.nodes[0].getnewaddress() : 1.0}
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        dec_tx  = self.nodes[2].decoderawtransaction(rawtx)
        
        errorString = ""
        try:
            rawtxfund = self.nodes[2].fundrawtransaction(rawtx)
        except JSONRPCException,e:
            errorString = e.error['message']
        
        assert_equal("Insufficient" in errorString, True);
        

if __name__ == '__main__':
    RawTransactionsTest().main()
