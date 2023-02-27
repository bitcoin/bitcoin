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

RPC_INVALID_PARAMETER = -8
RPC_METHOD_NOT_FOUND  = -32601
RPC_INVALID_REQUEST   = -32600
RPC_PARSE_ERROR       = -32700


class Format:
    ID_COUNT = 0

    @staticmethod
    def rpc_request(body):
        req = body
        req["version"] = "1.1"
        req["id"] = Format.ID_COUNT
        Format.ID_COUNT += 1
        return req


def send_raw_rpc(node, raw_body: bytes) -> tuple[object, int]:
    return node._request("POST", "/", raw_body)


def send_json_rpc(node, body: object) -> tuple[object, int]:
    raw = json.dumps(body).encode("utf-8")
    return send_raw_rpc(node, raw)


def expect_http_rpc_status(expected_http_status, expected_rpc_error_code, node, method, params):
    req = Format.rpc_request({"method": method, "params": params})
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

        body = []
        for cmd in commands:
            body.append(Format.rpc_request(cmd))

        response, status = send_json_rpc(self.nodes[0], body)
        assert_equal(status, 200)
        assert_equal(
            response,
            [
                {"result": 0, "error": None, "id": 0},
                {"result": None, "error": {"code": RPC_METHOD_NOT_FOUND, "message": "Method not found"}, "id": 1},
                {"result": "0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206", "error": None, "id": 2},
                {"result": None, "error": {"code": RPC_INVALID_REQUEST, "message": "Missing method"}, "id": 3}
            ]
        )

    def test_http_status_codes(self):
        self.log.info("Testing HTTP status codes for JSON-RPC requests...")
        expect_http_rpc_status(404, RPC_METHOD_NOT_FOUND,  self.nodes[0], "invalidmethod", [])
        expect_http_rpc_status(500, RPC_INVALID_PARAMETER, self.nodes[0], "getblockhash", [42])
        expect_http_rpc_status(200, None,                  self.nodes[0], "getblockhash", [0])
        # force-send invalidly formatted request
        response, status = send_raw_rpc(self.nodes[0], b"this is bad")
        assert_equal(response["error"]["code"], RPC_PARSE_ERROR)
        assert_equal(status, 500)

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
