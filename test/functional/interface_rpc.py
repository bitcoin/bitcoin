#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests some generic aspects of the RPC interface."""

import json
import os
from dataclasses import dataclass
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than_or_equal
from threading import Thread
from typing import Optional
import subprocess


RPC_INVALID_ADDRESS_OR_KEY = -5
RPC_INVALID_PARAMETER      = -8
RPC_METHOD_NOT_FOUND       = -32601
RPC_INVALID_REQUEST        = -32600
RPC_PARSE_ERROR            = -32700


@dataclass
class BatchOptions:
    version: Optional[int] = None
    notification: bool = False
    request_fields: Optional[dict] = None
    response_fields: Optional[dict] = None


def format_request(options, idx, fields):
    request = {}
    if options.version == 1:
        request.update(version="1.1")
    elif options.version == 2:
        request.update(jsonrpc="2.0")
    elif options.version is not None:
        raise NotImplementedError(f"Unknown JSONRPC version {options.version}")
    if not options.notification:
        request.update(id=idx)
    request.update(fields)
    if options.request_fields:
        request.update(options.request_fields)
    return request


def format_response(options, idx, fields):
    if options.version == 2 and options.notification:
        return None
    response = {}
    if not options.notification:
        response.update(id=idx)
    if options.version == 2:
        response.update(jsonrpc="2.0")
    else:
        response.update(result=None, error=None)
    response.update(fields)
    if options.response_fields:
        response.update(options.response_fields)
    return response


def send_raw_rpc(node, raw_body: bytes) -> tuple[object, int]:
    return node._request("POST", "/", raw_body)


def send_json_rpc(node, body: object) -> tuple[object, int]:
    raw = json.dumps(body).encode("utf-8")
    return send_raw_rpc(node, raw)


def expect_http_rpc_status(expected_http_status, expected_rpc_error_code, node, method, params, version=1, notification=False):
    req = format_request(BatchOptions(version, notification), 0, {"method": method, "params": params})
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

    def test_batch_request(self, call_options):
        calls = [
            # A basic request that will work fine.
            {"method": "getblockcount"},
            # Request that will fail.  The whole batch request should still
            # work fine.
            {"method": "invalidmethod"},
            # Another call that should succeed.
            {"method": "getblockhash", "params": [0]},
            # Invalid request format
            {"pizza": "sausage"}
        ]
        results = [
            {"result": 0},
            {"error": {"code": RPC_METHOD_NOT_FOUND, "message": "Method not found"}},
            {"result": "0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"},
            {"error": {"code": RPC_INVALID_REQUEST, "message": "Missing method"}},
        ]

        request = []
        response = []
        for idx, (call, result) in enumerate(zip(calls, results), 1):
            options = call_options(idx)
            if options is None:
                continue
            request.append(format_request(options, idx, call))
            r = format_response(options, idx, result)
            if r is not None:
                response.append(r)

        rpc_response, http_status = send_json_rpc(self.nodes[0], request)
        if len(response) == 0 and len(request) > 0:
            assert_equal(http_status, 204)
            assert_equal(rpc_response, None)
        else:
            assert_equal(http_status, 200)
            assert_equal(rpc_response, response)

    def test_batch_requests(self):
        self.log.info("Testing empty batch request...")
        self.test_batch_request(lambda idx: None)

        self.log.info("Testing basic JSON-RPC 2.0 batch request...")
        self.test_batch_request(lambda idx: BatchOptions(version=2))

        self.log.info("Testing JSON-RPC 2.0 batch with notifications...")
        self.test_batch_request(lambda idx: BatchOptions(version=2, notification=idx < 2))

        self.log.info("Testing JSON-RPC 2.0 batch of ALL notifications...")
        self.test_batch_request(lambda idx: BatchOptions(version=2, notification=True))

        # JSONRPC 1.1 does not support batch requests, but test them for backwards compatibility.
        self.log.info("Testing nonstandard JSON-RPC 1.1 batch request...")
        self.test_batch_request(lambda idx: BatchOptions(version=1))

        self.log.info("Testing nonstandard mixed JSON-RPC 1.1/2.0 batch request...")
        self.test_batch_request(lambda idx: BatchOptions(version=2 if idx % 2 else 1))

        self.log.info("Testing nonstandard batch request without version numbers...")
        self.test_batch_request(lambda idx: BatchOptions())

        self.log.info("Testing nonstandard batch request without version numbers or ids...")
        self.test_batch_request(lambda idx: BatchOptions(notification=True))

        self.log.info("Testing nonstandard jsonrpc 1.0 version number is accepted...")
        self.test_batch_request(lambda idx: BatchOptions(request_fields={"jsonrpc": "1.0"}))

        self.log.info("Testing unrecognized jsonrpc version number is rejected...")
        self.test_batch_request(lambda idx: BatchOptions(
            request_fields={"jsonrpc": "2.1"},
            response_fields={"result": None, "error": {"code": RPC_INVALID_REQUEST, "message": "JSON-RPC version not supported"}}))

    def test_http_status_codes(self):
        self.log.info("Testing HTTP status codes for JSON-RPC 1.1 requests...")
        # OK
        expect_http_rpc_status(200, None,                  self.nodes[0], "getblockhash", [0])
        # Errors
        expect_http_rpc_status(404, RPC_METHOD_NOT_FOUND,  self.nodes[0], "invalidmethod", [])
        expect_http_rpc_status(500, RPC_INVALID_PARAMETER, self.nodes[0], "getblockhash", [42])
        # force-send empty request
        response, status = send_raw_rpc(self.nodes[0], b"")
        assert_equal(response, {"id": None, "result": None, "error": {"code": RPC_PARSE_ERROR, "message": "Parse error"}})
        assert_equal(status, 500)
        # force-send invalidly formatted request
        response, status = send_raw_rpc(self.nodes[0], b"this is bad")
        assert_equal(response, {"id": None, "result": None, "error": {"code": RPC_PARSE_ERROR, "message": "Parse error"}})
        assert_equal(status, 500)

        self.log.info("Testing HTTP status codes for JSON-RPC 2.0 requests...")
        # OK
        expect_http_rpc_status(200, None,                   self.nodes[0], "getblockhash", [0],  2, False)
        # RPC errors but not HTTP errors
        expect_http_rpc_status(200, RPC_METHOD_NOT_FOUND,   self.nodes[0], "invalidmethod", [],  2, False)
        expect_http_rpc_status(200, RPC_INVALID_PARAMETER,  self.nodes[0], "getblockhash", [42], 2, False)
        # force-send invalidly formatted requests
        response, status = send_json_rpc(self.nodes[0], {"jsonrpc": 2, "method": "getblockcount"})
        assert_equal(response, {"result": None, "error": {"code": RPC_INVALID_REQUEST, "message": "jsonrpc field must be a string"}})
        assert_equal(status, 400)
        response, status = send_json_rpc(self.nodes[0], {"jsonrpc": "3.0", "method": "getblockcount"})
        assert_equal(response, {"result": None, "error": {"code": RPC_INVALID_REQUEST, "message": "JSON-RPC version not supported"}})
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
        expect_http_rpc_status(204, None, self.nodes[0], "generatetoaddress", [1, "invalid_address"], 2, True)
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
        self.test_batch_requests()
        self.test_http_status_codes()
        self.test_work_queue_exceeded()


if __name__ == '__main__':
    RPCInterfaceTest().main()
