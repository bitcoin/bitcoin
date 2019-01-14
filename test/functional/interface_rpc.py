#!/usr/bin/env python3
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests some generic aspects of the RPC interface."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than_or_equal

class RPCInterfaceTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def test_getrpcinfo(self):
        self.log.info("Testing getrpcinfo...")

        info = self.nodes[0].getrpcinfo()
        assert_equal(len(info['active_commands']), 1)

        command = info['active_commands'][0]
        assert_equal(command['method'], 'getrpcinfo')
        assert_greater_than_or_equal(command['duration'], 0)

    def test_batch_request(self):
        self.log.info("Testing basic JSON-RPC batch request...")

        results = self.nodes[0].batch([
            # A basic request that will work fine.
            {"method": "getblockcount", "id": 1},
            # Request that will fail.  The whole batch request should still
            # work fine.
            {"method": "invalidmethod", "id": 2},
            # Another call that should succeed.
            {"method": "getbestblockhash", "id": 3},
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

    def run_test(self):
        self.test_getrpcinfo()
        self.test_batch_request()


if __name__ == '__main__':
    RPCInterfaceTest().main()
