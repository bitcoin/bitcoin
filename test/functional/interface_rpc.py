#!/usr/bin/env python3
# Copyright (c) 2018-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests some generic aspects of the RPC interface."""

import os
from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than_or_equal

def expect_http_status(expected_http_status, expected_rpc_code,
                       fcn, *args):
    try:
        fcn(*args)
        raise AssertionError("Expected RPC error %d, got none" % expected_rpc_code)
    except JSONRPCException as exc:
        assert_equal(exc.error["code"], expected_rpc_code)
        assert_equal(exc.http_status, expected_http_status)

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
        assert_equal(info['logpath'], os.path.join(self.nodes[0].datadir, self.chain, 'debug.log'))

    def test_batch_request(self):
        self.log.info("Testing basic JSON-RPC batch request...")

        results = self.nodes[0].batch([
            # A basic request that will work fine.
            {"method": "getblockcount", "id": 1},
            # Request that will fail.  The whole batch request should still
            # work fine.
            {"method": "invalidmethod", "id": 2},
            # Another call that should succeed.
            {"method": "getblockhash", "id": 3, "params": [0]},
        ])

        result_by_id = {}
        for res in results:
            result_by_id[res["id"]] = res

        assert_equal(result_by_id[1]['error'], None)
        assert_equal(result_by_id[1]['result'], 0)

        assert_equal(result_by_id[2]['error']['code'], -32601)
        assert_equal(result_by_id[2]['result'], None)

        assert_equal(result_by_id[3]['error'], None)
        assert result_by_id[3]['result'] is not None

    def test_http_status_codes(self):
        self.log.info("Testing HTTP status codes for JSON-RPC requests...")

        expect_http_status(404, -32601, self.nodes[0].invalidmethod)
        expect_http_status(500, -8, self.nodes[0].getblockhash, 42)

    def run_test(self):
        self.test_getrpcinfo()
        self.test_batch_request()
        self.test_http_status_codes()


if __name__ == '__main__':
    RPCInterfaceTest().main()
