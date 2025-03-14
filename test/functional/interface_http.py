#!/usr/bin/env python3
# Copyright (c) 2014-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the RPC HTTP basics."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, str_to_b64str

import http.client
import time
import urllib.parse

class HTTPBasicsTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.supports_cli = False

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):

        #################################################
        # lowlevel check for http persistent connection #
        #################################################
        url = urllib.parse.urlparse(self.nodes[0].url)
        authpair = f'{url.username}:{url.password}'
        headers = {"Authorization": f"Basic {str_to_b64str(authpair)}"}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        assert b'"error":null' in out1
        assert conn.sock is not None  #according to http/1.1 connection must still be open!

        #send 2nd request without closing connection
        conn.request('POST', '/', '{"method": "getchaintips"}', headers)
        out1 = conn.getresponse().read()
        assert b'"error":null' in out1  #must also response with a correct json-rpc message
        assert conn.sock is not None  #according to http/1.1 connection must still be open!
        conn.close()

        #same should be if we add keep-alive because this should be the std. behaviour
        headers = {"Authorization": f"Basic {str_to_b64str(authpair)}", "Connection": "keep-alive"}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        assert b'"error":null' in out1
        assert conn.sock is not None  #according to http/1.1 connection must still be open!

        #send 2nd request without closing connection
        conn.request('POST', '/', '{"method": "getchaintips"}', headers)
        out1 = conn.getresponse().read()
        assert b'"error":null' in out1  #must also response with a correct json-rpc message
        assert conn.sock is not None  #according to http/1.1 connection must still be open!
        conn.close()

        #now do the same with "Connection: close"
        headers = {"Authorization": f"Basic {str_to_b64str(authpair)}", "Connection":"close"}

        conn = http.client.HTTPConnection(url.hostname, url.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        assert b'"error":null' in out1
        assert conn.sock is None  #now the connection must be closed after the response

        #node1 (2nd node) is running with disabled keep-alive option
        urlNode1 = urllib.parse.urlparse(self.nodes[1].url)
        authpair = f'{urlNode1.username}:{urlNode1.password}'
        headers = {"Authorization": f"Basic {str_to_b64str(authpair)}"}

        conn = http.client.HTTPConnection(urlNode1.hostname, urlNode1.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        assert b'"error":null' in out1

        #node2 (third node) is running with standard keep-alive parameters which means keep-alive is on
        urlNode2 = urllib.parse.urlparse(self.nodes[2].url)
        authpair = f'{urlNode2.username}:{urlNode2.password}'
        headers = {"Authorization": f"Basic {str_to_b64str(authpair)}"}

        conn = http.client.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        conn.request('POST', '/', '{"method": "getbestblockhash"}', headers)
        out1 = conn.getresponse().read()
        assert b'"error":null' in out1
        assert conn.sock is not None  #connection must be closed because bitcoind should use keep-alive by default

        # Check excessive request size
        conn = http.client.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        conn.request('GET', f'/{"x"*1000}', '', headers)
        out1 = conn.getresponse()
        assert_equal(out1.status, http.client.NOT_FOUND)

        conn = http.client.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        conn.request('GET', f'/{"x"*10000}', '', headers)
        out1 = conn.getresponse()
        assert_equal(out1.status, http.client.BAD_REQUEST)

        self.log.info("Check HTTP request encoded with chunked transfer")
        headers_chunked = headers.copy()
        headers_chunked.update({"Transfer-encoding": "chunked"})
        body_chunked = [
            b'{"method": "submitblock", "params": ["',
            b'0A' * 1000000,
            b'0B' * 1000000,
            b'0C' * 1000000,
            b'0D' * 1000000,
            b'"]}'
        ]
        conn = http.client.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        conn.request(
            method='POST',
            url='/',
            body=iter(body_chunked),
            headers=headers_chunked,
            encode_chunked=True)
        out1 = conn.getresponse().read()
        assert out1 == b'{"result":"high-hash","error":null}\n'

        self.log.info("Check -rpcservertimeout")
        self.restart_node(2, extra_args=["-rpcservertimeout=1"])
        # This is the amount of time the server will wait for a client to
        # send a complete request. Test it by sending an incomplete but
        # so-far otherwise well-formed HTTP request, and never finishing it.

        # Copied from http_incomplete_test_() in regress_http.c in libevent.
        # A complete request would have an additional "\r\n" at the end.
        http_request = "GET /test1 HTTP/1.1\r\nHost: somehost\r\n"

        # Get the underlying socket from HTTP connection so we can send something unusual
        conn = http.client.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        sock = conn.sock
        sock.sendall(http_request.encode("utf-8"))
        # Wait for response, but expect a timeout disconnection after 1 second
        start = time.time()
        res = sock.recv(1024)
        stop = time.time()
        assert res == b""
        assert stop - start >= 1
        # definitely closed
        try:
            conn.request('GET', '/')
            conn.getresponse()
        except ConnectionResetError:
            pass

        # Sanity check
        http_request = "GET /test2 HTTP/1.1\r\nHost: somehost\r\n\r\n"
        conn = http.client.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        sock = conn.sock
        sock.sendall(http_request.encode("utf-8"))
        res = sock.recv(1024)
        assert res.startswith(b"HTTP/1.1 404 Not Found")
        # still open
        conn.request('GET', '/')
        conn.getresponse()

        # Because we have set -rpcservertimeout so low, the persistent connection
        # created by AuthServiceProxy for this node when the test framework
        # started has likely closed. Force the test framework to use a fresh
        # new connection for the next RPC otherwise the cleanup process
        # calling `stop` will raise a connection error.
        self.nodes[2]._set_conn()

if __name__ == '__main__':
    HTTPBasicsTest(__file__).main()
