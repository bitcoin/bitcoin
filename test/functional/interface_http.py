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


        self.log.info("Check pipelining")
        # Requests are responded to in order they were received
        # See https://www.rfc-editor.org/rfc/rfc7230#section-6.3.2
        tip_height = self.nodes[2].getblockcount()

        req = "POST / HTTP/1.1\r\n"
        req += f'Authorization: Basic {str_to_b64str(authpair)}\r\n'

        # First request will take a long time to process
        body1 = f'{{"method": "waitforblockheight", "params": [{tip_height + 1}]}}'
        req1 = req
        req1 += f'Content-Length: {len(body1)}\r\n\r\n'
        req1 += body1

        # Second request will process very fast
        body2 = '{"method": "getblockcount"}'
        req2 = req
        req2 += f'Content-Length: {len(body2)}\r\n\r\n'
        req2 += body2
        # Get the underlying socket from HTTP connection so we can send something unusual
        conn = http.client.HTTPConnection(urlNode2.hostname, urlNode2.port)
        conn.connect()
        sock = conn.sock
        sock.settimeout(5)
        # Send two requests in a row. The first will block the second indefinitely
        sock.sendall(req1.encode("utf-8"))
        sock.sendall(req2.encode("utf-8"))
        try:
            # The server should not respond to the fast, second request
            # until the (very) slow first request has been handled:
            res = sock.recv(1024)
            assert False
        except TimeoutError:
            pass

        # Use a separate http connection to generate a block
        self.generate(self.nodes[2], 1, sync_fun=self.no_op)

        # Wait for two responses to be received
        res = b""
        while res.count(b"result") != 2:
            res += sock.recv(1024)

        # waitforblockheight was responded to first, and then getblockcount
        # which includes the block added after the request was made
        chunks = res.split(b'"result":')
        assert chunks[1].startswith(b'{"hash":')
        assert chunks[2].startswith(bytes(f'{tip_height + 1}', 'utf8'))


        self.log.info("Check HTTP request encoded with chunked transfer")
        headers_chunked = headers.copy()
        headers_chunked.update({"Transfer-encoding": "chunked"})
        body_chunked = [
            b'{"method": "submitblock", "params": ["',
            b'0' * 1000000,
            b'1' * 1000000,
            b'2' * 1000000,
            b'3' * 1000000,
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
        assert_equal(out1, b'{"result":"high-hash","error":null}\n')


        self.log.info("Check -rpcservertimeout")
        # The test framework typically reuses a single persistent HTTP connection
        # for all RPCs to a TestNode. Because we are setting -rpcservertimeout
        # so low on this one node, its connection will quickly timeout and get dropped by
        # the server. Negating this setting will force the AuthServiceProxy
        # for this node to create a fresh new HTTP connection for every command
        # called for the remainder of this test.
        self.nodes[2].reuse_http_connections = False

        self.restart_node(2, extra_args=["-rpcservertimeout=2"])
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
        # Server disconnected with EOF
        assert_equal(res, b"")
        # Server disconnected within an acceptable range of time:
        # not immediately, and not too far over the configured duration.
        # This allows for some jitter in the test between client and server.
        duration = stop - start
        assert duration <= 4, f"Server disconnected too slow: {duration} > 4"
        assert duration >= 1, f"Server disconnected too fast: {duration} < 1"

        # The connection is definitely closed.
        got_expected_error = False
        try:
            conn.request('GET', '/')
            conn.getresponse()
        #       macos/linux           windows
        except (ConnectionResetError, ConnectionAbortedError):
            got_expected_error = True
        assert got_expected_error

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

if __name__ == '__main__':
    HTTPBasicsTest(__file__).main()
