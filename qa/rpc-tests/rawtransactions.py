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

# Create one-input, one-output, no-fee transaction:
class RawTransactionsTest(BitcoinTestFramework):
    
    def run_test(self):
        
        newAddr = self.nodes[2].getnewaddress()
        
        inputs = []
        outputs = { newAddr : 10 }
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        rawtxfund = self.nodes[0].fundrawtransaction(rawtx)
        dec_rawtxfund = self.nodes[0].decoderawtransaction(rawtxfund['hex'])
        
        assert_equal(len(dec_rawtxfund['vin']), 1)
        assert_equal(len(dec_rawtxfund['vout']), 2)
        assert_equal(rawtxfund['fee'] > 0, True)
        assert_equal(dec_rawtxfund['vin'][0]['scriptSig']['hex'], '')
        
        rawtxfundsigned = self.nodes[0].signrawtransaction(rawtxfund['hex'])
        dec_rawtxfundsigned = self.nodes[0].decoderawtransaction(rawtxfundsigned['hex'])
        
        assert_equal(len(dec_rawtxfundsigned['vin'][0]['scriptSig']['hex']) > 20, True)
        
        
        listunspent = self.nodes[2].listunspent()
        inputs = [{'txid' : listunspent[0]['txid'], 'vout' : listunspent[0]['vout']}]
        outputs = { newAddr : 10 }
        rawtx = self.nodes[0].createrawtransaction(inputs, outputs)
        aException = False
        try:
            rawtxfund = self.nodes[0].fundrawtransaction(rawtx)
        except JSONRPCException,e:
            aException = True

        assert_equal(aException, True)
        

if __name__ == '__main__':
    RawTransactionsTest().main()
