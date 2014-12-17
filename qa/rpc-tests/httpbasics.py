#!/usr/bin/env python2
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test REST interface
#

from test_framework import BitcoinTestFramework
from util import *
import base64

try:
    import http.client as httplib
except ImportError:
    import httplib
try:
    import urllib.parse as urlparse
except ImportError:
    import urlparse

class HTTPBasicsTest (BitcoinTestFramework):        
    def run_test(self):        
        
        #################################################
        # lowlevel check for http persistent connection #
        #################################################
        url = urlparse.urlparse(self.nodes[0].url)
        authpair = url.username + ':' + url.password
        headers = {"Authorization": "Basic " + base64.b64encode(authpair)}
        
        conn = httplib.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('GET', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True)
        assert_equal(conn.sock!=None, True) #according to http/1.1 connection must still be open!
        
        #send 2nd request without closing connection
        conn.request('GET', '/', '{"method": "getchaintips"}', headers)
        out2 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True) #must also response with a correct json-rpc message
        assert_equal(conn.sock!=None, True) #according to http/1.1 connection must still be open!
        conn.close()
        
        #same should be if we add keep-alive because this should be the std. behaviour
        headers = {"Authorization": "Basic " + base64.b64encode(authpair), "Connection": "keep-alive"}
        
        conn = httplib.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('GET', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True)
        assert_equal(conn.sock!=None, True) #according to http/1.1 connection must still be open!
        
        #send 2nd request without closing connection
        conn.request('GET', '/', '{"method": "getchaintips"}', headers)
        out2 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True) #must also response with a correct json-rpc message
        assert_equal(conn.sock!=None, True) #according to http/1.1 connection must still be open!
        conn.close()
        
        #now do the same with "Connection: close"
        headers = {"Authorization": "Basic " + base64.b64encode(authpair), "Connection":"close"}
        
        conn = httplib.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('GET', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read();
        assert_equal('"error":null' in out1, True)
        assert_equal(conn.sock!=None, False) #now the connection must be closed after the response        
        
        
if __name__ == '__main__':
    HTTPBasicsTest ().main ()
