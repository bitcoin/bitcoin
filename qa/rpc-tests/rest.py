#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test REST interface
#

from test_framework import BitcoinTestFramework
from util import *
import json

try:
    import http.client as httplib
except ImportError:
    import httplib
try:
    import urllib.parse as urlparse
except ImportError:
    import urlparse

def http_get_call(host, port, path, response_object = 0):
    conn = httplib.HTTPConnection(host, port)
    conn.request('GET', path)
    
    if response_object:
        return conn.getresponse()
        
    return conn.getresponse().read()


class RESTTest (BitcoinTestFramework):
    FORMAT_SEPARATOR = "."
    
    def run_test(self):
        url = urlparse.urlparse(self.nodes[0].url)
        bb_hash = self.nodes[0].getbestblockhash()
        
        # check binary format
        response = http_get_call(url.hostname, url.port, '/rest/block/'+bb_hash+self.FORMAT_SEPARATOR+"bin", True)
        assert_equal(response.status, 200)
        assert_greater_than(int(response.getheader('content-length')), 10)
        
        # check json format
        json_string = http_get_call(url.hostname, url.port, '/rest/block/'+bb_hash+self.FORMAT_SEPARATOR+'json')
        json_obj = json.loads(json_string)
        assert_equal(json_obj['hash'], bb_hash)
        
        # do tx test
        tx_hash = json_obj['tx'][0]['txid'];
        json_string = http_get_call(url.hostname, url.port, '/rest/tx/'+tx_hash+self.FORMAT_SEPARATOR+"json")
        json_obj = json.loads(json_string)
        assert_equal(json_obj['txid'], tx_hash)
        
        # check hex format response
        hex_string = http_get_call(url.hostname, url.port, '/rest/tx/'+tx_hash+self.FORMAT_SEPARATOR+"hex", True)
        assert_equal(response.status, 200)
        assert_greater_than(int(response.getheader('content-length')), 10)
        
        # check block tx details
        # let's make 3 tx and mine them on node 1
        txs = []
        txs.append(self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 11))
        txs.append(self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 11))
        txs.append(self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 11))
        self.sync_all()
        
        # now mine the transactions
        newblockhash = self.nodes[1].setgenerate(True, 1)
        self.sync_all()
        
        #check if the 3 tx show up in the new block
        json_string = http_get_call(url.hostname, url.port, '/rest/block/'+newblockhash[0]+self.FORMAT_SEPARATOR+'json')
        json_obj = json.loads(json_string)
        for tx in json_obj['tx']:
            if not 'coinbase' in tx['vin'][0]: #exclude coinbase
                assert_equal(tx['txid'] in txs, True)
        
        #check the same but without tx details
        json_string = http_get_call(url.hostname, url.port, '/rest/block/notxdetails/'+newblockhash[0]+self.FORMAT_SEPARATOR+'json')
        json_obj = json.loads(json_string)
        for tx in txs:
            assert_equal(tx in json_obj['tx'], True)
                
        

if __name__ == '__main__':
    RESTTest ().main ()
