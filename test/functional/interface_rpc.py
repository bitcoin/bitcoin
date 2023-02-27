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


RPC_INVALID_PARAMETER = -8
RPC_METHOD_NOT_FOUND  = -32601
RPC_INVALID_REQUEST   = -32600


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
    response = {}
    response.update(id=None if options.notification else idx)
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

    def test_batch_request(self):
        self.log.info("Testing basic JSON-RPC batch request...")

        calls = [
            # A basic request that will work fine.
            {"method": "getblockcount"},
            # Request that will fail.  The whole batch request should still
            # work fine.
            {"method": "invalidmethod"},
            # Another call that should succeed.
            {"method": "getblockhash", "params": [0]},
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
            options = BatchOptions()
            request.append(format_request(options, idx, call))
            response.append(format_response(options, idx, result))

        rpc_response, http_status = send_json_rpc(self.nodes[0], request)
        assert_equal(http_status, 200)
        assert_equal(rpc_response, response)

    def test_http_status_codes(self):
        self.log.info("Testing HTTP status codes for JSON-RPC requests...")

        expect_http_rpc_status(404, RPC_METHOD_NOT_FOUND,  self.nodes[0], "invalidmethod", [])
        expect_http_rpc_status(500, RPC_INVALID_PARAMETER, self.nodes[0], "getblockhash", [42])

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
