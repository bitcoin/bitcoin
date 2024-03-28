#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests some generic aspects of the RPC interface."""

import json
import os
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than_or_equal
from threading import Thread
import subprocess

RPC_INVALID_ADDRESS_OR_KEY = -5
RPC_INVALID_PARAMETER      = -8
RPC_METHOD_NOT_FOUND       = -32601
RPC_INVALID_REQUEST        = -32600
RPC_PARSE_ERROR            = -32700


class RequestBuilder:
    def __init__(self):
        self.id_count = 0

    def rpc_request(self, fields, version=None, notification=False):
        req = dict(**fields)
        if not version:
            req["version"] = "1.1"
        if version == 1:
            req["jsonrpc"] = "1.0"
        if version == 2:
            req["jsonrpc"] = "2.0"
        if not notification:
            req["id"] = self.id_count
            self.id_count += 1
        return req


def send_raw_rpc(node, raw_body: bytes) -> tuple[object, int]:
    return node._request("POST", "/", raw_body)


def send_json_rpc(node, body: object) -> tuple[object, int]:
    raw = json.dumps(body).encode("utf-8")
    return send_raw_rpc(node, raw)


def expect_http_rpc_status(expected_http_status, expected_rpc_error_code, node, method, params, version=1, notification=False):
    req = RequestBuilder().rpc_request({"method": method, "params": params}, version, notification)
    response, status = send_json_rpc(node, req)

    if expected_rpc_error_code is not None:
        assert_equal(response["error"]["code"], expected_rpc_error_code)

    assert_equal(status, expected_http_status)


def test_work_queue_getblock(node, got_exceeded_error):
    while not got_exceeded_error:
        try:
            node.cli("waitfornewblock", "500").send_cli()
        except subprocess.CalledProcessError as e:
            assert_equal(e.output, 'error: Server response: Work queue depth exceeded\n')
            got_exceeded_error.append(True)


class RPCInterfaceTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.supports_cli = False

    def test_getrpcinfo(self):
        self.log.info("Testing getrpcinfo...")

        info = self.nodes[0].getrpcinfo()
        assert_equal(len(info['active_commands']), 1)

        command = info['active_commands'][0]
        assert_equal(command['method'], 'getrpcinfo')
        assert_greater_than_or_equal(command['duration'], 0)
        assert_equal(info['logpath'], os.path.join(self.nodes[0].chain_path, 'debug.log'))

    def test_batch_request(self):
        commands = [
            # A basic request that will work fine.
            {"method": "getblockcount"},
            # Request that will fail.  The whole batch request should still work fine.
            {"method": "invalidmethod"},
            # Another call that should succeed.
            {"method": "getblockhash", "params": [0]},
            # Invalid request format
            {"pizza": "sausage"}
        ]
        builder = RequestBuilder()

        self.log.info("Testing empty batch request...")
        body = []
        response, status = send_json_rpc(self.nodes[0], body)
        assert_equal(status, 200)
        assert_equal(
            response,
            []
        )

        self.log.info("Testing default JSON-RPC 1.1 batch request...")
        body = []
        for cmd in commands:
            body.append(builder.rpc_request(cmd))
        response, status = send_json_rpc(self.nodes[0], body)
        assert_equal(status, 200)
        # JSON 1.1: Every response has a "result" and an "error"
        assert_equal(
            response,
            [
                {"result": 0, "error": None, "id": 0},
                {"result": None, "error": {"code": RPC_METHOD_NOT_FOUND, "message": "Method not found"}, "id": 1},
                {"result": "0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206", "error": None, "id": 2},
                {"result": None, "error": {"code": RPC_INVALID_REQUEST, "message": "Missing method"}, "id": 3}
            ]
        )

        self.log.info("Testing basic JSON-RPC 1.0 batch request...")
        body = []
        for cmd in commands:
            body.append(builder.rpc_request(cmd, version=1))
        response, status = send_json_rpc(self.nodes[0], body)
        assert_equal(status, 200)
        # JSON 1.1: Every response has a "result" and an "error"
        assert_equal(
            response,
            [
                {"result": 0, "error": None, "id": 4},
                {"result": None, "error": {"code": RPC_METHOD_NOT_FOUND, "message": "Method not found"}, "id": 5},
                {"result": "0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206", "error": None, "id": 6},
                {"result": None, "error": {"code": RPC_INVALID_REQUEST, "message": "Missing method"}, "id": 7}
            ]
        )

        self.log.info("Testing basic JSON-RPC 2.0 batch request...")
        body = []
        for cmd in commands:
            body.append(builder.rpc_request(cmd, version=2))
        response, status = send_json_rpc(self.nodes[0], body)
        assert_equal(status, 200)
        # JSON 2.0: Each response has either a "result" or an "error"
        assert_equal(
            response,
            [
                {"jsonrpc": "2.0", "result": 0, "id": 8},
                {"jsonrpc": "2.0", "error": {"code": RPC_METHOD_NOT_FOUND, "message": "Method not found"}, "id": 9},
                {"jsonrpc": "2.0", "result": "0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206", "id": 10},
                {"jsonrpc": "2.0", "error": {"code": RPC_INVALID_REQUEST, "message": "Missing method"}, "id": 11}
            ]
        )

        self.log.info("Testing mixed JSON-RPC 1.1/2.0 batch request...")
        body = []
        version = 1
        for cmd in commands:
            body.append(builder.rpc_request(cmd, version=version))
            version = 2 if version == 1 else 1
        response, status = send_json_rpc(self.nodes[0], body)
        assert_equal(status, 200)
        # Responses respect the version of the individual request in the batch
        assert_equal(
            response,
            [
                {"result": 0, "error": None, "id": 12},
                {"jsonrpc": "2.0", "error": {"code": RPC_METHOD_NOT_FOUND, "message": "Method not found"}, "id": 13},
                {"result": "0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206", "error": None, "id": 14},
                {"jsonrpc": "2.0", "error": {"code": RPC_INVALID_REQUEST, "message": "Missing method"}, "id": 15}
            ]
        )

        self.log.info("Testing JSON-RPC 2.0 batch with notifications...")
        body = []
        notification = False
        for cmd in commands:
            body.append(builder.rpc_request(cmd, version=2, notification=notification))
            notification = True
        response, status = send_json_rpc(self.nodes[0], body)
        assert_equal(status, 200)
        # Notifications are never responded to, even if there is an error
        assert_equal(
            response,
            [
                {"jsonrpc": "2.0", "result": 0, "id": 16}
            ]
        )

        self.log.info("Testing JSON-RPC 2.0 batch of ALL notifications...")
        body = []
        for cmd in commands:
            body.append(builder.rpc_request(cmd, version=2, notification=True))
        response, status = send_json_rpc(self.nodes[0], body)
        assert_equal(status, 204) # HTTP_NO_CONTENT
        assert_equal(response, None) # not even an empty array

    def test_http_status_codes(self):
        self.log.info("Testing HTTP status codes for JSON-RPC 1.1 requests...")
        # OK
        expect_http_rpc_status(200, None,                  self.nodes[0], "getblockhash", [0])
        # Errors
        expect_http_rpc_status(404, RPC_METHOD_NOT_FOUND,  self.nodes[0], "invalidmethod", [])
        expect_http_rpc_status(500, RPC_INVALID_PARAMETER, self.nodes[0], "getblockhash", [42])
        # force-send invalidly formatted request
        response, status = send_raw_rpc(self.nodes[0], b"this is bad")
        assert_equal(response["id"], None)
        assert_equal(response["error"]["code"], RPC_PARSE_ERROR)
        assert_equal(status, 500)

        self.log.info("Testing HTTP status codes for JSON-RPC 2.0 requests...")
        # OK
        expect_http_rpc_status(200, None,                   self.nodes[0], "getblockhash", [0],  2, False)
        # RPC errors but not HTTP errors
        expect_http_rpc_status(200, RPC_METHOD_NOT_FOUND,   self.nodes[0], "invalidmethod", [],  2, False)
        expect_http_rpc_status(200, RPC_INVALID_PARAMETER,  self.nodes[0], "getblockhash", [42], 2, False)
        # force-send invalidly formatted requests
        response, status = send_json_rpc(self.nodes[0], {"jsonrpc": 2, "method": "getblockcount"})
        assert_equal(response["id"], None)
        assert_equal(response["error"]["code"], RPC_INVALID_REQUEST)
        assert_equal(response["error"]["message"], "jsonrpc field must be a string")
        assert_equal(status, 400)
        response, status = send_json_rpc(self.nodes[0], {"jsonrpc": "3.0", "method": "getblockcount"})
        assert_equal(response["id"], None)
        assert_equal(response["error"]["code"], RPC_INVALID_REQUEST)
        assert_equal(response["error"]["message"], "JSON-RPC version not supported")
        assert_equal(status, 400)

        self.log.info("Testing HTTP status codes for JSON-RPC 2.0 notifications...")
        # Not notification: id exists
        response, status = send_json_rpc(self.nodes[0], {"jsonrpc": "2.0", "id": None, "method": "getblockcount"})
        assert_equal(response["result"], 0)
        assert_equal(status, 200)
        # Not notification: JSON 1.1
        expect_http_rpc_status(200, None,                   self.nodes[0], "getblockcount", [],  1)
        # Not notification: has "id" field
        expect_http_rpc_status(200, None,                   self.nodes[0], "getblockcount", [],  2, False)
        block_count = self.nodes[0].getblockcount()
        # Notification response status code: HTTP_NO_CONTENT
        expect_http_rpc_status(204, None,                   self.nodes[0], "generatetoaddress", [1, "bcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqdku202"],  2, True)
        # The command worked even though there was no response
        assert_equal(block_count + 1, self.nodes[0].getblockcount())
        # No error response for notifications even if they are invalid
        expect_http_rpc_status(204, None,                   self.nodes[0], "generatetoaddress", [1, "invalid_address"], 2, True)
        # Sanity check: command was not executed
        assert_equal(block_count + 1, self.nodes[0].getblockcount())

    def test_work_queue_exceeded(self):
        self.log.info("Testing work queue exceeded...")
        self.restart_node(0, ['-rpcworkqueue=1', '-rpcthreads=1'])
        got_exceeded_error = []
        threads = []
        for _ in range(3):
            t = Thread(target=test_work_queue_getblock, args=(self.nodes[0], got_exceeded_error))
            t.start()
            threads.append(t)
        for t in threads:
            t.join()

    def run_test(self):
        self.test_getrpcinfo()
        self.test_batch_request()
        self.test_http_status_codes()
        self.test_work_queue_exceeded()


if __name__ == '__main__':
    RPCInterfaceTest().main()
