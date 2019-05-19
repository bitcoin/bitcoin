#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the RPC HTTP basics."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

import http.client
import urllib.parse

class HTTPBasicsTest (BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 3
        self.setup_clean_chain = False

    def setup_network(self):
        self.nodes = self.setup_nodes()

    def run_test(self):

        #################################################
        # lowlevel check for http persistent connection #
        #################################################
        url = urllib.parse.urlparse(self.nodes[0].url)
        authpair = url.username + ':' + url.password
        headers = {"Authorization": "Basic " + str_to_b64str(authpair)}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        assert(b'"error":null' in out1)
        assert(conn.sock!=None) #according to http/1.1 connection must still be open!

        #send 2nd request without closing connection
        conn.request('POST', '/', '{"method": "getchaintips"}', headers)
        out1 = conn.getresponse().read()
        assert(b'"error":null' in out1) #must also response with a correct json-rpc message
        assert(conn.sock!=None) #according to http/1.1 connection must still be open!
        conn.close()

        #same should be if we add keep-alive because this should be the std. behaviour
        headers = {"Authorization": "Basic " + str_to_b64str(authpair), "Connection": "keep-alive"}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        assert(b'"error":null' in out1)
        assert(conn.sock!=None) #according to http/1.1 connection must still be open!

        #send 2nd request without closing connection
        conn.request('POST', '/', '{"method": "getchaintips"}', headers)
        out1 = conn.getresponse().read()
        assert(b'"error":null' in out1) #must also response with a correct json-rpc message
        assert(conn.sock!=None) #according to http/1.1 connection must still be open!
        conn.close()

        #now do the same with "Connection: close"
        headers = {"Authorization": "Basic " + str_to_b64str(authpair), "Connection":"close"}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        assert(b'"error":null' in out1)
        assert(conn.sock==None) #now the connection must be closed after the response

        #node1 (2nd node) is running with disabled keep-alive option
        urlNode1 = urllib.parse.urlparse(self.nodes[1].url)
        authpair = urlNode1.username + ':' + urlNode1.password
        headers = {"Authorization": "Basic " + str_to_b64str(authpair)}

        conn = http.client.HTTPConnection(urlNode1.hostname, urlNode1.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        assert(b'"error":null' in out1)

        #node2 (third node) is running with standard keep-alive parameters which means keep-alive is on
        urlNode2 = urllib.parse.urlparse(self.nodes[2].url)
        authpair = urlNode2.username + ':' + urlNode2.password
        headers = {"Authorization": "Basic " + str_to_b64str(authpair)}

        conn = http.client.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        assert(b'"error":null' in out1)
        assert(conn.sock!=None) #connection must be closed because bitcoind should use keep-alive by default

        # Check excessive request size
        conn = http.client.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        conn.request('GET', '/' + ('x'*1000), '', headers)
        out1 = conn.getresponse()
        assert_equal(out1.status, http.client.NOT_FOUND)

        conn = http.client.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        conn.request('GET', '/' + ('x'*10000), '', headers)
        out1 = conn.getresponse()
        assert_equal(out1.status, http.client.BAD_REQUEST)


if __name__ == '__main__':
    HTTPBasicsTest ().main ()
