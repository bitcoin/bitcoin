#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the HTTP server basics."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, str_to_b64str

import http.client
import time
import urllib.parse

# Configuration option for some test nodes
RPCSERVERTIMEOUT = 2
# Set in httpserver.cpp and passed to libevent evhttp_set_max_headers_size()
MAX_HEADERS_SIZE = 8192
# Set in serialize.h and passed to libevent evhttp_set_max_body_size()
MAX_SIZE = 0x02000000


class BitcoinHTTPConnection:
    def __init__(self, node):
        self.url = urllib.parse.urlparse(node.url)
        self.authpair = f'{self.url.username}:{self.url.password}'
        self.headers = {"Authorization": f"Basic {str_to_b64str(self.authpair)}"}
        self.reset_conn()

    def reset_conn(self):
        self.conn = http.client.HTTPConnection(self.url.hostname, self.url.port)
        self.conn.connect()

    def sock_closed(self):
        if self.conn.sock is None:
            return True
        try:
            self.conn.request('GET', '/')
            self.conn.getresponse().read()
            return False
        #       macos/linux           windows
        except (ConnectionResetError, ConnectionAbortedError):
            return True

    def close_sock(self):
        self.conn.close()

    def set_timeout(self, seconds):
        self.conn.sock.settimeout(seconds)

    def add_header(self, key, value):
        self.headers.update({key: value})

    def _request(self, method, path, data, connection_header, **kwargs):
        headers = self.headers.copy()
        if connection_header is not None:
            headers["Connection"] = connection_header
        self.conn.request(method, path, data, headers, **kwargs)
        return self.conn.getresponse()

    def post(self, path, data, connection_header=None, **kwargs):
        return self._request('POST', path, data, connection_header, **kwargs)

    def get(self, path, connection_header=None):
        return self._request('GET', path, '', connection_header)

    def post_raw(self, path, data):
        data_bytes = data.encode("utf-8")
        req = f"POST {path} HTTP/1.1\r\n"
        req += f'Authorization: Basic {str_to_b64str(self.authpair)}\r\n'
        req += f'Content-Length: {len(data_bytes)}\r\n\r\n'
        self.conn.sock.sendall(req.encode("ascii") + data_bytes)

    def recv_raw(self):
        '''
        Blocking socket will wait until data is received and return up to 1024 bytes
        '''
        return self.conn.sock.recv(1024)

    def expect_timeout(self, seconds):
        # Wait for response, but expect a timeout disconnection
        start = time.time()
        response1 = self.recv_raw()
        stop = time.time()
        # Server disconnected with EOF
        assert_equal(response1, b"")
        # Server disconnected within an acceptable range of time:
        # not immediately, and not too far over the configured duration.
        # This allows for some jitter in the test between client and server.
        duration = stop - start
        assert duration <= seconds + 2, f"Server disconnected too slow: {duration} > {seconds}"
        assert duration >= seconds - 1, f"Server disconnected too fast: {duration} < {seconds}"
        # The connection is definitely closed.
        assert self.sock_closed()

class HTTPBasicsTest (BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.supports_cli = False

    def setup_network(self):
        self.setup_nodes()
        self.node = self.nodes[0]

    def run_test(self):
        # The test framework typically reuses a single persistent HTTP connection
        # for all RPCs to a TestNode. Because we are setting -rpcservertimeout
        # so low on this one node, its connection will quickly timeout and get dropped by
        # the server. Negating this setting will force the AuthServiceProxy
        # for this node to create a fresh new HTTP connection for every command
        # called for the remainder of this test.
        self.node.reuse_http_connections = False

        self.check_default_connection()
        self.check_keepalive_connection()
        self.check_close_connection()
        self.check_excessive_request_size()
        self.check_pipelining()
        self.check_chunked_transfer()
        self.check_idle_timeout()
        self.check_server_busy_idle_timeout()


    def check_default_connection(self):
        self.log.info("Checking default HTTP/1.1 connection persistence")
        conn = BitcoinHTTPConnection(self.node)
        # Make request without explicit "Connection" header
        response1 = conn.post('/', '{"method": "getbestblockhash"}').read()
        assert b'"error":null' in response1
        # Connection still open after request
        assert not conn.sock_closed()
        # Make second request without explicit "Connection" header
        response2 = conn.post('/', '{"method": "getchaintips"}').read()
        assert b'"error":null' in response2
        # Connection still open after second request
        assert not conn.sock_closed()
        # Close
        conn.close_sock()
        assert conn.sock_closed()


    def check_keepalive_connection(self):
        self.log.info("Checking keep-alive connection persistence")
        conn = BitcoinHTTPConnection(self.node)
        # Make request with explicit "Connection: keep-alive" header
        response1 = conn.post('/', '{"method": "getbestblockhash"}', connection_header='keep-alive').read()
        assert b'"error":null' in response1
        # Connection still open after request
        assert not conn.sock_closed()
        # Make second request with explicit "Connection" header
        response2 = conn.post('/', '{"method": "getchaintips"}', connection_header='keep-alive').read()
        assert b'"error":null' in response2
        # Connection still open after second request
        assert not conn.sock_closed()
        # Close
        conn.close_sock()
        assert conn.sock_closed()


    def check_close_connection(self):
        self.log.info("Checking close connection after response")
        conn = BitcoinHTTPConnection(self.node)
        # Make request with explicit "Connection: close" header
        response1 = conn.post('/', '{"method": "getbestblockhash"}', connection_header='close').read()
        assert b'"error":null' in response1
        # Connection closed after response
        assert conn.sock_closed()


    def check_excessive_request_size(self):
        self.log.info("Checking excessive request size")

        # Large URI plus up to 1000 bytes of default headers
        # added by python's http.client still below total limit.
        conn = BitcoinHTTPConnection(self.node)
        response1 = conn.get(f'/{"x" * (MAX_HEADERS_SIZE - 1000)}')
        assert_equal(response1.status, http.client.NOT_FOUND)

        # Excessive URI size plus default headers breaks the limit.
        conn = BitcoinHTTPConnection(self.node)
        response2 = conn.get(f'/{"x" * MAX_HEADERS_SIZE}')
        assert_equal(response2.status, http.client.BAD_REQUEST)

        # Compute how many short header lines need to be added to http.client
        # default headers to make / break the total limit in a single request.
        header_line_length = len("header_0000: foo\r\n")
        headers_below_limit = (MAX_HEADERS_SIZE - 1000) // header_line_length
        headers_above_limit = MAX_HEADERS_SIZE // header_line_length

        # This is a libevent mystery:
        # libevent does not reject the request until it is more than
        # 1,000 bytes above the configured limit.
        headers_above_limit += 1000 // header_line_length

        # Many small header lines is ok
        conn = BitcoinHTTPConnection(self.node)
        for i in range(headers_below_limit):
            conn.add_header(f"header_{i:04}", "foo")
        response3 = conn.get('/x')
        assert_equal(response3.status, http.client.NOT_FOUND)

        # Too many small header lines exceeds total headers size allowed
        conn = BitcoinHTTPConnection(self.node)
        for i in range(headers_above_limit):
            conn.add_header(f"header_{i:04}", "foo")
        response3 = conn.get('/x')
        assert_equal(response3.status, http.client.BAD_REQUEST)

        # Compute how much data we can add to a request message body
        # to make / break the limit.
        base_request_body_size = len('{"jsonrpc": "2.0", "id": "0", "method": "submitblock", "params": [""]}}')
        bytes_below_limit = MAX_SIZE - base_request_body_size
        bytes_above_limit = MAX_SIZE - base_request_body_size + 2

        # Large request body size is ok
        conn = BitcoinHTTPConnection(self.node)
        response4 = conn.post('/', f'{{"jsonrpc": "2.0", "id": "0", "method": "submitblock", "params": ["{"0" * bytes_below_limit}"]}}')
        assert_equal(response4.status, http.client.OK)

        conn = BitcoinHTTPConnection(self.node)
        try:
            # Excessive body size is invalid
            conn.post_raw('/', f'{{"jsonrpc": "2.0", "id": "0", "method": "submitblock", "params": ["{"0" * bytes_above_limit}"]}}')
            self.log.info("Client finished sending request before connection was terminated")
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            self.log.info("Client did not finish sending request before connection was terminated")

        # The server will send a 413 response and disconnect but due to a race
        # condition, the python client may or may not read the response before
        # detecting the broken socket (which it may still be trying to write to).
        try:
            response5 = conn.conn.getresponse()
            assert_equal(response5.status, http.client.REQUEST_ENTITY_TOO_LARGE)
            self.log.info(f"Client got expected response status {response5.status}")
            assert conn.sock_closed()
        #       macos/linux                   windows
        except (http.client.ResponseNotReady, ConnectionAbortedError):
            self.log.info("Client did not read response before disconnecting")


    def check_pipelining(self):
        """
        Requests are responded to in the order in which they were received
        See https://www.rfc-editor.org/rfc/rfc7230#section-6.3.2
        """
        self.log.info("Check pipelining")
        tip_height = self.node.getblockcount()
        conn = BitcoinHTTPConnection(self.node)
        conn.set_timeout(5)

        # Send two requests in a row.
        # The first request will block the second indefinitely
        conn.post_raw('/', f'{{"method": "waitforblockheight", "params": [{tip_height + 1}]}}')
        conn.post_raw('/', '{"method": "getblockcount"}')

        try:
            # The server should not respond to the second request until the first
            # request has been handled. Since the server will not respond at all
            # to the first request until we generate a block we expect a socket timeout.
            conn.recv_raw()
            assert False
        except TimeoutError:
            pass

        # Use a separate http connection to generate a block
        self.generate(self.node, 1, sync_fun=self.no_op)

        # Wait for two responses to be received
        res = b""
        while res.count(b"result") != 2:
            res += conn.recv_raw()

        # waitforblockheight was responded to first, and then getblockcount
        # which includes the block added after the request was made
        chunks = res.split(b'"result":')
        assert chunks[1].startswith(b'{"hash":')
        assert chunks[2].startswith(bytes(f'{tip_height + 1}', 'utf8'))


    def check_chunked_transfer(self):
        self.log.info("Check HTTP request encoded with chunked transfer")
        conn = BitcoinHTTPConnection(self.node)
        headers_chunked = conn.headers.copy()
        headers_chunked.update({"Transfer-encoding": "chunked"})
        body_chunked = [
            b'{"method": "submitblock", "params": ["',
            b'0' * 1000000,
            b'1' * 1000000,
            b'2' * 1000000,
            b'3' * 1000000,
            b'"]}'
        ]
        conn.conn.request(
            method='POST',
            url='/',
            body=iter(body_chunked),
            headers=headers_chunked,
            encode_chunked=True)
        response1 = conn.recv_raw()
        assert b'{"result":"high-hash","error":null}\n' in response1

        self.log.info("Check excessive size HTTP request encoded with chunked transfer")
        conn = BitcoinHTTPConnection(self.node)
        headers_chunked = conn.headers.copy()
        headers_chunked.update({"Transfer-encoding": "chunked"})
        body_chunked = [
            b'{"method": "submitblock", "params": ["',
            b'0' * 10000000,
            b'1' * 10000000,
            b'2' * 10000000,
            b'3' * 10000000,
            b'"]}'
        ]
        try:
            conn.conn.request(
                method='POST',
                url='/',
                body=iter(body_chunked),
                headers=headers_chunked,
                encode_chunked=True)
            self.log.info("Client finished sending request before connection was terminated")
        except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
            self.log.info("Client did not finish sending request before connection was terminated")

        # The server will send a 413 response and disconnect but due to a race
        # condition, the python client may or may not read the response before
        # detecting the broken socket (which it may still be trying to write to).
        try:
            response2 = conn.conn.getresponse()
            assert_equal(response2.status, http.client.REQUEST_ENTITY_TOO_LARGE)
            self.log.info(f"Client got expected response status {response2.status}")
            assert conn.sock_closed()
        #       macos/linux                   windows
        except (http.client.ResponseNotReady, ConnectionAbortedError):
            self.log.info("Client did not read response before disconnecting")


    def check_idle_timeout(self):
        self.log.info("Check -rpcservertimeout")

        # This is the amount of time the server will wait for a client to
        # send a complete request. Test it by sending an incomplete but
        # so-far otherwise well-formed HTTP request, and never finishing it.
        self.restart_node(0, extra_args=[f"-rpcservertimeout={RPCSERVERTIMEOUT}"])

        # Copied from http_incomplete_test_() in regress_http.c in libevent.
        # A complete request would have an additional "\r\n" at the end.
        bad_http_request = "GET /test1 HTTP/1.1\r\nHost: somehost\r\n"
        conn = BitcoinHTTPConnection(self.node)
        conn.conn.sock.sendall(bad_http_request.encode("ascii"))

        conn.expect_timeout(RPCSERVERTIMEOUT)

        # Sanity check -- complete requests don't timeout waiting for completion
        good_http_request = "GET /test2 HTTP/1.1\r\nHost: somehost\r\n\r\n"
        conn.reset_conn()
        conn.conn.sock.sendall(good_http_request.encode("ascii"))
        response = conn.recv_raw()
        assert response.startswith(b"HTTP/1.1 404 Not Found")

        # Still open
        assert not conn.sock_closed()


    def check_server_busy_idle_timeout(self):
        self.log.info("Check that -rpcservertimeout won't close on a delayed response")

        self.restart_node(0, extra_args=[f"-rpcservertimeout={RPCSERVERTIMEOUT}"])

        tip_height = self.node.getblockcount()
        conn = BitcoinHTTPConnection(self.node)
        conn.post_raw('/', f'{{"method": "waitforblockheight", "params": [{tip_height + 1}]}}')

        # Wait until after the timeout, then generate a block with a second HTTP connection
        time.sleep(RPCSERVERTIMEOUT + 1)
        generated_block = self.generate(self.node, 1, sync_fun=self.no_op)[0]

        # The first connection gets the response it is patiently waiting for
        response1 = conn.recv_raw().decode()
        assert generated_block in response1
        # The connection is still open
        assert not conn.sock_closed()

        # Now it will actually close due to idle timeout
        conn.expect_timeout(RPCSERVERTIMEOUT)


if __name__ == '__main__':
    HTTPBasicsTest(__file__).main()
