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

# Configuration option for some tests
RPCSERVERTIMEOUT = 2
# Set in httpserver.h
MAX_HEADERS_SIZE = 8192
MAX_BODY_SIZE = 32 * 1024 * 1024

# When a test expects a server disconnection, any of these errors are
# acceptable. The specific event is determined by race condition and platform OS.
NETWORK_ERRORS = (
    BrokenPipeError,                 # write to a closed socket/pipe
    ConnectionResetError,            # connection forcibly closed by peer
    ConnectionAbortedError,          # connection aborted locally or by network stack
    http.client.ResponseNotReady,    # server response not ready or connection out of sync
)

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
        except NETWORK_ERRORS:
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

    def send_raw(self, data):
        self.conn.sock.sendall(data)

    def post_raw(self, path, data):
        data_bytes = data.encode("utf-8")
        req = f"POST {path} HTTP/1.1\r\n"
        req += f'Authorization: Basic {str_to_b64str(self.authpair)}\r\n'
        req += f'Content-Length: {len(data_bytes)}\r\n\r\n'
        self.send_raw(req.encode("ascii") + data_bytes)

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
        self.check_auth_required()
        self.check_wrong_credentials()
        self.check_malformed_auth_headers()
        self.check_disallowed_http_methods()
        self.check_path_traversal()
        self.check_request_smuggling_cl_te()
        self.check_duplicate_content_length()
        self.check_null_byte_in_uri()
        self.check_invalid_http_version()
        self.check_whitespace_in_headers()


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
        bytes_below_limit = MAX_BODY_SIZE - base_request_body_size
        bytes_above_limit = MAX_BODY_SIZE - base_request_body_size + 2

        # Large request body size is ok
        conn = BitcoinHTTPConnection(self.node)
        response4 = conn.post('/', f'{{"jsonrpc": "2.0", "id": "0", "method": "submitblock", "params": ["{"0" * bytes_below_limit}"]}}')
        assert_equal(response4.status, http.client.OK)

        conn = BitcoinHTTPConnection(self.node)
        try:
            # Excessive body size is invalid
            conn.post_raw('/', f'{{"jsonrpc": "2.0", "id": "0", "method": "submitblock", "params": ["{"0" * bytes_above_limit}"]}}')
            self.log.info("Client finished sending request before connection was terminated")
        except NETWORK_ERRORS:
            self.log.info("Client did not finish sending request before connection was terminated")

        # The server will send a 413 response and disconnect but due to a race
        # condition, the python client may or may not read the response before
        # detecting the broken socket (which it may still be trying to write to).
        try:
            response5 = conn.conn.getresponse()
            assert_equal(response5.status, http.client.REQUEST_ENTITY_TOO_LARGE)
            self.log.info(f"Client got expected response status {response5.status}")
            assert conn.sock_closed()
        except NETWORK_ERRORS:
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
        except NETWORK_ERRORS:
            self.log.info("Client did not finish sending request before connection was terminated")

        # The server will send a 413 response and disconnect but due to a race
        # condition, the python client may or may not read the response before
        # detecting the broken socket (which it may still be trying to write to).
        try:
            response2 = conn.conn.getresponse()
            assert_equal(response2.status, http.client.REQUEST_ENTITY_TOO_LARGE)
            self.log.info(f"Client got expected response status {response2.status}")
            assert conn.sock_closed()
        except NETWORK_ERRORS:
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
        conn.send_raw(bad_http_request.encode("ascii"))

        conn.expect_timeout(RPCSERVERTIMEOUT)

        # Sanity check -- complete requests don't timeout waiting for completion
        good_http_request = "GET /test2 HTTP/1.1\r\nHost: somehost\r\n\r\n"
        conn.reset_conn()
        conn.send_raw(good_http_request.encode("ascii"))
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


    def check_auth_required(self):
        self.log.info("Check that requests without credentials return 401 Unauthorized with WWW-Authenticate")
        conn = BitcoinHTTPConnection(self.node)
        conn.headers = {}
        response = conn.post('/', '{"method": "getbestblockhash"}')
        assert_equal(response.status, http.client.UNAUTHORIZED)
        assert response.getheader('WWW-Authenticate') is not None


    def check_wrong_credentials(self):
        self.log.info("Check that incorrect credentials return 401 Unauthorized")
        conn = BitcoinHTTPConnection(self.node)
        wrong_pair = f"{conn.url.username}:wrong_password"
        conn.headers = {"Authorization": f"Basic {str_to_b64str(wrong_pair)}"}
        response = conn.post('/', '{"method": "getbestblockhash"}')
        assert_equal(response.status, http.client.UNAUTHORIZED)
        assert response.getheader('WWW-Authenticate') is not None


    def check_malformed_auth_headers(self):
        self.log.info("Check that malformed Authorization headers return 401 Unauthorized")
        cases = [
            "Bearer sometoken123",
            'Digest username="user", realm="test"',
            "Basic !!!notbase64!!!",
            f"Basic {str_to_b64str('nocolon')}",
            "Basic ",
        ]
        for auth_value in cases:
            conn = BitcoinHTTPConnection(self.node)
            conn.headers = {"Authorization": f"{auth_value}"}
            response = conn.post('/', '{"method": "getbestblockhash"}')
            assert_equal(response.status, http.client.UNAUTHORIZED)
            assert response.getheader('WWW-Authenticate') is not None


    def check_disallowed_http_methods(self):
        self.log.info("Check that unsafe or unsupported HTTP methods are rejected")
        for method, err in [
            ['TRACE',   http.client.METHOD_NOT_ALLOWED],
            ['CONNECT', http.client.METHOD_NOT_ALLOWED],
            ['DELETE',  http.client.METHOD_NOT_ALLOWED],
            ['PATCH',   http.client.METHOD_NOT_ALLOWED],
            ['OPTIONS', http.client.METHOD_NOT_ALLOWED],
            ['GET',     http.client.METHOD_NOT_ALLOWED] # RPC endpoint '/' only handles POST
        ]:
            conn = BitcoinHTTPConnection(self.node)
            response = conn._request(method, '/', data=None, connection_header=None)
            assert_equal(response.status, err)


    def check_path_traversal(self):
        self.log.info("Check that path traversal attempts are safely rejected")
        traversal_paths = [
            '/../etc/passwd',
            '/../../etc/shadow',
            '/%2e%2e/%2e%2e/etc/passwd',     # URL-encoded dots
            '/..%2Fetc%2Fpasswd',            # URL-encoded slash
            '/.%2e/.%2e/etc/passwd',         # mixed encoding
            '/valid/../../../etc/passwd',
        ]
        for path in traversal_paths:
            conn = BitcoinHTTPConnection(self.node)
            response = conn.get(path)
            assert_equal(response.status, http.client.NOT_FOUND)


    def check_request_smuggling_cl_te(self):
        self.log.info("Check request smuggling is not possible")
        # https://www.rfc-editor.org/rfc/rfc7230#section-3.3.3
        # Transfer-Encoding takes precedence over Content-Length.
        # Sending both creates a smuggling vector: a front-end proxy that
        # uses Content-Length while the back-end uses Transfer-Encoding lets an
        # attacker prepend arbitrary bytes to the next victim's request.

        # The real JSON-RPC body sent as a single chunk.
        body = b'{"method":"getblockcount"}'
        # Content-Length is set to the length of just the chunk-size line
        # ("1a\r\n" = 4 bytes), not the full chunked body — the ambiguity that
        # smuggling exploits. A server using Transfer-Encoding reads the complete
        # chunk and responds with the block count; a server confused by the
        # mismatch may stall, close the connection, or return an error.
        chunk_size_line = f"{len(body):x}\r\n".encode("ascii")
        # Signals end of body
        empty_chunk = b'\r\n0\r\n\r\n'
        chunk_body = chunk_size_line + body + empty_chunk

        conn = BitcoinHTTPConnection(self.node)
        raw = (
            f"POST / HTTP/1.1\r\n"
            f"Host: {conn.url.hostname}\r\n"
            f"Authorization: Basic {str_to_b64str(conn.authpair)}\r\n"
            f"Content-Length: {len(chunk_size_line)}\r\n"
            f"Transfer-Encoding: chunked\r\n"
            f"\r\n"
        ).encode("ascii") + chunk_body
        conn.send_raw(raw)
        response = conn.recv_raw().decode()
        assert "HTTP/1.1 200 OK" in response
        count = self.node.getblockcount()
        assert f'"result":{count}' in response


    def check_duplicate_content_length(self):
        self.log.info("Check that duplicate Content-Length headers are handled")
        # https://www.rfc-editor.org/rfc/rfc7230#section-3.3.3
        # Multiple Content-Length headers with differing values "MUST"
        # result in an error.
        conn = BitcoinHTTPConnection(self.node)
        body = '{"method":"getblockcount"}'
        raw = (
            f"POST / HTTP/1.1\r\n"
            f"Host: {conn.url.hostname}\r\n"
            f"Authorization: Basic {str_to_b64str(conn.authpair)}\r\n"
            f"Content-Length: {len(body)}\r\n"
            f"Content-Length: 999\r\n"
            f"\r\n"
            f"{body}"
        ).encode("ascii")
        conn.send_raw(raw)
        response = conn.recv_raw().decode()
        assert response.startswith("HTTP/1.1 400")


    def check_null_byte_in_uri(self):
        self.log.info("Check that null bytes in the URI are safely rejected")
        # Null-byte injection can truncate the path string in C environments,
        # bypassing suffix/extension checks and causing unexpected file access.
        conn = BitcoinHTTPConnection(self.node)
        raw = (
            "GET /safe\x00/../etc/passwd HTTP/1.1\r\n"
            f"Host: {conn.url.hostname}\r\n"
            f"Authorization: Basic {str_to_b64str(conn.authpair)}\r\n"
            "\r\n"
        ).encode("ascii")
        conn.send_raw(raw)
        response = conn.recv_raw().decode()
        assert response.startswith("HTTP/1.1 400")


    def check_invalid_http_version(self):
        self.log.info("Check that requests with invalid HTTP versions are safely rejected")
        cases = [
            b"GET / \r\n\r\n",                                    # HTTP/0.9 — no version
            b"GET / HTTP/9.9\r\nHost: localhost\r\n\r\n",         # far-future version
            b"GET / HTTP/INVALID\r\nHost: localhost\r\n\r\n",     # non-numeric version
            b"GET / NOTHTTP/1.1\r\nHost: localhost\r\n\r\n",      # wrong protocol name
        ]
        for raw in cases:
            conn = BitcoinHTTPConnection(self.node)
            conn.send_raw(raw)
            response = conn.recv_raw().decode()
            assert response.startswith("HTTP/1.1 400")


    def check_whitespace_in_headers(self):
        self.log.info("Check that requests with whitespace in headers are rejected")
        # Extra whitespace before colon in header.
        conn = BitcoinHTTPConnection(self.node)
        conn.headers = {"Authorization ": f"Basic {str_to_b64str(conn.authpair)}"}
        response = conn.post('/', '{"method": "getbestblockhash"}')
        assert_equal(response.status, http.client.BAD_REQUEST)

        # Extra whitespace at start of new line.
        # "line folding" as defined in
        # https://www.rfc-editor.org/rfc/rfc2616#section-2.2
        # is considered unsafe and is explicitly deprecated in
        # https://www.rfc-editor.org/rfc/rfc7230#section-3.2.4
        conn = BitcoinHTTPConnection(self.node)
        conn.headers = {"Authorization": f"Basic \n {str_to_b64str(conn.authpair)}"}
        response = conn.post('/', '{"method": "getbestblockhash"}')
        assert_equal(response.status, http.client.BAD_REQUEST)


if __name__ == '__main__':
    HTTPBasicsTest(__file__).main()
